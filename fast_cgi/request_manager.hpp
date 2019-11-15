#pragma once

#include "detail/record.hpp"
#include "output_manager.hpp"
#include "params.hpp"
#include "reader.hpp"
#include "request.hpp"
#include "role.hpp"

#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <streambuf>
#include <type_traits>

namespace fast_cgi {

class request_manager
{
public:
	typedef unsigned short id_type;

	void handle_request(reader& reader, output_manager& output_manager, detail::record record)
	{
		switch (record.type) {
		case detail::TYPE::FCGI_BEGIN_REQUEST: {
			handle_begin_request(reader, output_manager, record);

			break;
		}
		case detail::TYPE::FCGI_ABORT_REQUEST: {
			handle_abort_request(reader, output_manager, record);

			break;
		}
		case detail::TYPE::FCGI_PARAMS:
		case detail::TYPE::FCGI_STDIN:
		case detail::TYPE::FCGI_DATA: {
			// get buffer
			buffer* ptr;

			auto token = ptr->begin_writing();

			for (double_type sent = 0; sent < record.content_length;) {
				auto buffer = token.request_buffer(record.content_length - sent);

				reader.read(buffer.first, buffer.second);

				sent += buffer.second;
			}

			break;
		}
		default: {
			// ignore body
			record.skip(reader);

			// tell the server that the record was ignored
			detail::record::write(output_manager, detail::unknown_type{ record.type });

			break;
		}
		}
	}

	void handle_begin_request(reader& reader, output_manager& output_manager, detail::record record)
	{
		auto body = detail::begin_request::read(reader);

		{
			std::lock_guard<std::mutex> lock(mutex);

			// request exists -> ignore
			if (requests.find(record.request_id) != requests.end()) {
				return;
			}
		}

		request r(body.flags & detail::FLAGS::FCGI_KEEP_CONN);

		switch (body.role) {
		case detail::ROLE::FCGI_AUTHORIZER:
		case detail::ROLE::FCGI_FILTER:
		case detail::ROLE::FCGI_RESPONDER: {
			auto& factory = role_factories[body.role - 1];

			if (factory) {
				auto role = factory->create();

				// launch thread
				std::thread(&request_manager::request_hanlder, this, std::move(role));

				break;
			} // else fall through, because role is unimplemented
		}
		default: {
			// reject because role is unknown
			detail::record(detail::FCGI_VERSION_1, record.request_id)
				.write(output_manager, detail::end_request{ 0, detail::PROTOCOL_STATUS::FCGI_UNKNOWN_ROLE });

			return;
		}
		}

		{
			std::lock_guard<std::mutex> lock(mutex);

			// add request
			requests.insert({ record.request_id, r });
		}
	}
	void handle_abort_request(reader& reader, output_manager& output_manager, detail::record record)
	{
		// assert body size

		// request does not exist -> ignore
		request* r;

		{
			std::lock_guard<std::mutex> lock(mutex);

			auto re = requests.find(record.request_id);

			if (re == requests.end()) {
				return;
			}

			r = re->second;
		}

		// trigger cancel
		r->second.cancel();
	}
	void handle_params(reader& reader, output_manager& output_manager, detail::record record)
	{
		auto r = requests.find(record.request_id);

		// request not found
		if (r == requests.end()) {
		}

		if (r->second.params_finished) {
		}

		// finished
		if (record.content_length == 0) {
			r->second.params_finished = true;
		} else {
			r->second.params.read_parameter(reader, record);
		}
	}

private:
	typedef std::map<id_type, std::unique_ptr<request>> requests_type;

	std::mutex _mutex;
	requests_type _requests;
	std::shared_ptr<allocator> _allocator;
	const std::shared_ptr<role_factory> _role_factories[3];

	void request_hanlder(std::unique_ptr<role> role, request& request)
	{
		auto version = detail::VERSION::FCGI_VERSION_1;

		// read all parameters

		// initialize input streams
		input_streambuf sin;
		input_streambuf sdata;
		byte_istream input_stream(&sin);
		byte_istream data_stream(&sdata);

		if (request.role_type == detail::ROLE::FCGI_FILTER || request.role_type == detail::ROLE::FCGI_RESPONDER) {
			static_cast<responder*>(role.get())->_input_stream = &input_stream;

			// initialize data stream
			if (request.role_type == detail::ROLE::FCGI_FILTER) {
				static_cast<filter*>(role.get())->_data_stream = &data_stream;

				// wait until input stream finished reading
				sin.wait_for_all_input();
			}
		}

		// create output streams
		// todo: buffers nead to be copied
		output_streambuf sout(_allocator.get(), 1024, [&request, version](const void* buffer, std::size_t size) {
			detail::record(version, request.id).write(request.output_manager, detail::stdout_stream{ buffer, size });
		});
		output_streambuf serr(_allocator.get(), 1024, [&request, version](const void* buffer, std::size_t size) {
			detail::record(version, request.id).write(request.output_manager, detail::stderr_stream{ buffer, size });
		});
		byte_ostream output_stream(&sout);
		byte_ostream error_stream(&serr);

		role->_output_stream = &output_stream;
		role->_error_stream  = &error_stream;
		role->_cancelled	 = &request->cancelled;

		// execute the role
		auto status = role->run();

		// flush and finish all output streams
		sout.pubsync();
		detail::record(version, request.id).write(request.output_manager, detail::stdout_stream{ nullptr, 0 });
		serr.pubsync();
		detail::record(version, request.id).write(request.output_manager, detail::stderr_stream{ nullptr, 0 });

		// end request
		detail::record(version, request.id)
			.write(request.output_manager,
				   detail::end_request{ status, detail::PROTOCOL_STATUS::FCGI_REQUEST_COMPLETE });
	}
};

} // namespace fast_cgi