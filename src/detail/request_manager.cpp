#include "fast_cgi/detail/params.hpp"
#include "fast_cgi/detail/request_manager.hpp"
#include "fast_cgi/log.hpp"

namespace fast_cgi {
namespace detail {

request_manager::request_manager(std::shared_ptr<memory::allocator> allocator, std::shared_ptr<io::reader> reader,
                                 std::array<std::function<std::unique_ptr<role>()>, 3> role_factories)
    : _terminate_connection(false), _allocator(std::move(allocator)), _reader(std::move(reader)),
      _role_factories(std::move(role_factories))
{}

request_manager::~request_manager()
{
	for (auto& request : _requests) {
		if (request.second->handler_thread.joinable()) {
			FAST_CGI_LOG(DEBUG, "joining request({}) thread", request.first);

			request.second->handler_thread.join();
		}
	}
}

bool request_manager::should_terminate_connection() const
{
	if (!_terminate_connection.load(std::memory_order_acquire)) {
		return false;
	}

	for (auto& request : _requests) {
		if (!request.second->finished.load(std::memory_order_acquire)) {
			return false;
		}
	}

	return true;
}

bool request_manager::handle_request(std::shared_ptr<io::output_manager> output_manager, detail::record record)
{
	std::shared_ptr<request> request;

	{
		auto r = _requests.find(record.request_id);

		// request not found
		if (r != _requests.end()) {
			// check if request is finished
			if (r->second->finished.load(std::memory_order_acquire)) {
				request->handler_thread.join();
				_requests.erase(r);
			} else {
				request = r->second;
			}
		}

		// ignore record
		if ((request && record.type == detail::TYPE::FCGI_BEGIN_REQUEST) ||
		    (!request && record.type != detail::TYPE::FCGI_BEGIN_REQUEST)) {
			FAST_CGI_LOG(WARN, "ignoring record because of invalid type and request state (id: {})", record.request_id);

			return false;
		}
	}

	// do not accept any more new request when connection is queued to be closed
	if (!request && _terminate_connection.load(std::memory_order_acquire)) {
		return false;
	}

	// request is set if type is not FCGI_BEGIN_REQUEST
	switch (record.type) {
	case detail::TYPE::FCGI_BEGIN_REQUEST: {
		_begin_request(output_manager, record);

		break;
	}
	case detail::TYPE::FCGI_ABORT_REQUEST: {
		_reader->skip(record.content_length);

		request->cancelled.store(true, std::memory_order_release);

		break;
	}
	case detail::TYPE::FCGI_PARAMS: {
		_forward_to_buffer(record.content_length, *request->params_buffer);

		break;
	}
	case detail::TYPE::FCGI_DATA: {
		_forward_to_buffer(record.content_length, *request->data_buffer);

		break;
	}
	case detail::TYPE::FCGI_STDIN: {
		_forward_to_buffer(record.content_length, *request->input_buffer);

		break;
	}
	default: return false;
	}

	return true;
}

void request_manager::_forward_to_buffer(detail::double_type length, memory::buffer& buffer)
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
				FAST_CGI_LOG(WARN, "buffer is full...skipping {} bytes", length - sent);

				_reader->skip(length - sent);

				break;
			}

			_reader->read(buf.first, buf.second);

			sent += buf.second;
		}
	}
}

void request_manager::_request_hanlder(std::unique_ptr<role> role, std::shared_ptr<request> request)
{
	auto version = detail::VERSION::FCGI_VERSION_1;

	// read all parameters
	{
		io::reader reader(request->params_buffer);

		FAST_CGI_LOG(DEBUG, "reading all parameters");

		request->params._read_parameters(reader);

		// initialize input buffers
		std::size_t content_size = 0;

		try {
			content_size = static_cast<std::size_t>(std::stoull(request->params["CONTENT_LENGTH"]));
		} catch (const std::exception& e) {
			FAST_CGI_LOG(WARN, "failed to parse content length ({})", e.what());
		}

		request->input_buffer->set_max(content_size);
	}

	// initialize input streams
	io::input_streambuf sin(request->input_buffer);
	io::input_streambuf sdata(request->data_buffer);
	io::byte_istream input_stream(&sin);
	io::byte_istream data_stream(&sdata);

	if (request->role_type == detail::ROLE::FCGI_FILTER || request->role_type == detail::ROLE::FCGI_RESPONDER) {
		dynamic_cast<responder*>(role.get())->_input_stream = &input_stream;

		// initialize data stream
		if (request->role_type == detail::ROLE::FCGI_FILTER) {
			dynamic_cast<filter*>(role.get())->_data_stream = &data_stream;

			std::size_t content_size = 0;

			try {
				content_size = static_cast<std::size_t>(std::stoull(request->params["FCGI_DATA_LENGTH"]));
			} catch (const std::exception& e) {
				FAST_CGI_LOG(WARN, "failed to parse data content length ({})", e.what());
			}

			request->data_buffer->set_max(content_size);

			// wait until input stream finished reading
			request->input_buffer->wait_for_all_input();
		} else {
			request->data_buffer->close();
		}
	} else {
		request->data_buffer->close();
	}

	// create output streams
	io::output_streambuf sout([&request, version](void* buffer, std::size_t size) -> std::pair<void*, std::size_t> {
		if (buffer) {
			auto flag = detail::record::write(version, request->id, *request->output_manager,
			                                  detail::stdout_stream{ buffer, static_cast<detail::double_type>(size) });

			request->output_manager->buffer_manager().free_page(buffer, flag);
		}

		return { request->output_manager->buffer_manager().new_page(),
			     request->output_manager->buffer_manager().page_size() };
	});
	io::output_streambuf serr([&request, version](void* buffer, std::size_t size) -> std::pair<void*, std::size_t> {
		if (buffer) {
			auto flag = detail::record::write(version, request->id, *request->output_manager,
			                                  detail::stderr_stream{ buffer, static_cast<detail::double_type>(size) });

			request->output_manager->buffer_manager().free_page(buffer, flag);
		}

		return { request->output_manager->buffer_manager().new_page(),
			     request->output_manager->buffer_manager().page_size() };
	});
	io::byte_ostream output_stream(&sout);
	io::byte_ostream error_stream(&serr);

	role->_params        = &request->params;
	role->_output_stream = &output_stream;
	role->_error_stream  = &error_stream;
	role->_cancelled     = &request->cancelled;

	// execute the role
	role::status_code_type status = -1;

	try {
		status = role->run();
	} catch (const std::exception& e) {
		FAST_CGI_LOG(ERROR, "role executor threw an exception ({})", e.what());
	} catch (...) {
		FAST_CGI_LOG(ERROR, "role executor threw an exception");
	}

	FAST_CGI_LOG(INFO, "role finished with status code={}", static_cast<detail::quadruple_type>(status));

	// flush and finish all output streams
	output_stream.flush();
	detail::record::write(version, request->id, *request->output_manager, detail::stdout_stream{ nullptr, 0 });
	error_stream.flush();
	detail::record::write(version, request->id, *request->output_manager, detail::stderr_stream{ nullptr, 0 });

	// end request
	detail::record::write(version, request->id, *request->output_manager,
	                      detail::end_request{ static_cast<detail::quadruple_type>(status),
	                                           detail::PROTOCOL_STATUS::FCGI_REQUEST_COMPLETE });

	FAST_CGI_LOG(INFO, "request {} finished; removing", request->id);

	// trigger end and interrupt reading buffer
	if (request->close_connection) {
		FAST_CGI_LOG(DEBUG, "terminating connection");

		_terminate_connection.store(true, std::memory_order_release);
		_reader->interrupt();
	}

	request->finished.store(true, std::memory_order_release);
}

void request_manager::_begin_request(std::shared_ptr<io::output_manager> output_manager, detail::record record)
{
	auto body    = detail::begin_request::read(*_reader);
	auto request = std::make_shared<struct request>(record.request_id, body.role, std::move(output_manager),
	                                                (body.flags & detail::FLAGS::FCGI_KEEP_CONN) == 0, _allocator);

	switch (body.role) {
	case detail::ROLE::FCGI_AUTHORIZER:
	case detail::ROLE::FCGI_FILTER:
	case detail::ROLE::FCGI_RESPONDER: {
		auto& factory = _role_factories[body.role - 1];

		if (factory) {
			auto role = factory();

			FAST_CGI_LOG(INFO, "created role; launching request thread");

			// launch thread
			request->handler_thread = std::thread(&request_manager::_request_hanlder, this, std::move(role), request);

			break;
		} // else fall through, because role is unimplemented
	}
	default: {
		FAST_CGI_LOG(ERROR, "begin request record rejected because of unknown/unimplemented role {}", body.role);

		// reject because role is unknown
		detail::record::write(detail::FCGI_VERSION_1, record.request_id, *request->output_manager,
		                      detail::end_request{ 0, detail::PROTOCOL_STATUS::FCGI_UNKNOWN_ROLE });

		return;
	}
	}

	// add request
	_requests.insert({ record.request_id, std::move(request) });
}

} // namespace detail
} // namespace fast_cgi