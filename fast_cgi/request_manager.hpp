#pragma once

#include "buffer_reader.hpp"
#include "detail/record.hpp"
#include "output_manager.hpp"
#include "params.hpp"
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
	typedef detail::double_type id_type;

	bool handle_request(reader& reader, output_manager& output_manager, detail::record record)
	{
		std::shared_ptr<request> request;

		// ignore if request id is unknown
		if (record.type != detail::TYPE::FCGI_BEGIN_REQUEST) {
			std::lock_guard<std::mutex> lock(_mutex);
			auto r = _requests.find(record.request_id);

			if (r == _requests.end()) {
				reader.skip(record.content_length);

				return true;
			}

			request = r->second;
		}

		// request is set if type is not FCGI_BEGIN_REQUEST
		switch (record.type) {
		case detail::TYPE::FCGI_BEGIN_REQUEST: {
			handle_begin_request(reader, output_manager, record);

			break;
		}
		case detail::TYPE::FCGI_ABORT_REQUEST: {
			reader.skip(record.content_length);

			request->cancelled = true;

			break;
		}
		case detail::TYPE::FCGI_PARAMS: {
			_forward_to_buffer(record.content_length, request->params_buffer, reader);

			break;
		}
		case detail::TYPE::FCGI_DATA: {
			_forward_to_buffer(record.content_length, request->data_buffer, reader);

			break;
		}
		case detail::TYPE::FCGI_STDIN: {
			_forward_to_buffer(record.content_length, request->input_buffer, reader);

			break;
		}
		default: return false;
		}

		return true;
	}

private:
	typedef std::map<id_type, std::shared_ptr<request>> requests_type;

	std::mutex _mutex;
	requests_type _requests;
	std::shared_ptr<allocator> _allocator;
	const std::shared_ptr<role_factory> _role_factories[3];

	void _forward_to_buffer(detail::double_type length, buffer& buffer, reader& reader)
	{
		// end of stream
		if (length == 0) {
			buffer.close();
		} else {
			auto token = buffer.begin_writing();

			for (detail::double_type sent = 0; sent < length;) {
				auto buf = token.request_buffer(length - sent);

				// buffer is full -> ignore
				if (buf.second == 0) {
					reader.skip(length - sent);

					break;
				}

				reader.read(buf.first, buf.second);

				sent += buf.second;
			}
		}
	}
	void _request_hanlder(std::unique_ptr<role> role, std::shared_ptr<request> request)
	{
		auto version = detail::VERSION::FCGI_VERSION_1;

		// read all parameters
		{
			buffer_reader reader(request.params_buffer);

			request.params._read_parameters(reader);
		}

		// initialize input streams
		input_streambuf sin(request.input_buffer);
		input_streambuf sdata(request.data_buffer);
		byte_istream input_stream(&sin);
		byte_istream data_stream(&sdata);

		if (request.role_type == detail::ROLE::FCGI_FILTER || request.role_type == detail::ROLE::FCGI_RESPONDER) {
			static_cast<responder*>(role.get())->_input_stream = &input_stream;

			// initialize data stream
			if (request.role_type == detail::ROLE::FCGI_FILTER) {
				static_cast<filter*>(role.get())->_data_stream = &data_stream;

				// wait until input stream finished reading
				request.input_buffer.wait_for_all_input();
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
	void _begin_request(reader& reader, const std::shared_ptr<output_manager>& output_manager, detail::record record)
	{
		auto body	= detail::begin_request::read(reader);
		auto request = std::make_shared<request>(record.request_id, body.role, output_manager,
												 (body.flags & detail::FLAGS::FCGI_KEEP_CONN) == 0);

		switch (body.role) {
		case detail::ROLE::FCGI_AUTHORIZER:
		case detail::ROLE::FCGI_FILTER:
		case detail::ROLE::FCGI_RESPONDER: {
			auto& factory = role_factories[body.role - 1];

			if (factory) {
				auto role = factory->create();

				// launch thread
				request->handler_thread =
					std::thread(&request_manager::_request_hanlder, this, std::move(role), request);

				break;
			} // else fall through, because role is unimplemented
		}
		default: {
			// reject because role is unknown
			detail::record(detail::FCGI_VERSION_1, record.request_id)
				.write(*output_manager, detail::end_request{ 0, detail::PROTOCOL_STATUS::FCGI_UNKNOWN_ROLE });

			return;
		}
		}

		// add request
		{
			std::lock_guard<std::mutex> lock(mutex);

			// add request
			requests.insert({ record.request_id, std::move(request) });
		}
	}
};

} // namespace fast_cgi