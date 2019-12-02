#pragma once

#include "buffer_manager.hpp"
#include "buffer_reader.hpp"
#include "detail/record.hpp"
#include "log.hpp"
#include "output_manager.hpp"
#include "params.hpp"
#include "request.hpp"
#include "role.hpp"

#include <array>
#include <atomic>
#include <iostream>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <streambuf>
#include <type_traits>

namespace fast_cgi {

class request_manager
{
public:
    typedef detail::double_type id_type;

    request_manager(const std::shared_ptr<allocator>& allocator,
                    const std::array<std::function<std::unique_ptr<role>()>, 3>& role_factories)
        : _terminate_connection(false), _allocator(allocator), _role_factories(role_factories)
    {}
    ~request_manager()
    {
        for (auto& request : _requests) {
            if (request.second->handler_thread.joinable()) {
                LOG(DEBUG, "joining request({}) thread", request.first);

                request.second->handler_thread.join();
            }
        }
    }
    bool should_terminate_connection() const
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
    bool handle_request(reader& reader, const std::shared_ptr<output_manager>& output_manager, detail::record record)
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
                LOG(WARN, "ignoring record because of invalid type and request state (id: {})", record.request_id);

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
            _begin_request(reader, output_manager, record);

            break;
        }
        case detail::TYPE::FCGI_ABORT_REQUEST: {
            reader.skip(record.content_length);

            request->cancelled.store(true, std::memory_order_release);

            break;
        }
        case detail::TYPE::FCGI_PARAMS: {
            _forward_to_buffer(record.content_length, *request->params_buffer, reader);

            break;
        }
        case detail::TYPE::FCGI_DATA: {
            _forward_to_buffer(record.content_length, *request->data_buffer, reader);

            break;
        }
        case detail::TYPE::FCGI_STDIN: {
            //_forward_to_buffer(record.content_length, *request->input_buffer, reader);
            reader.skip(record.content_length);
            break;
        }
        default: return false;
        }

        return true;
    }

private:
    typedef std::map<id_type, std::shared_ptr<request>> requests_type;

    std::atomic_bool _terminate_connection;
    requests_type _requests;
    std::shared_ptr<allocator> _allocator;
    std::array<std::function<std::unique_ptr<role>()>, 3> _role_factories;

    /**
      Forwards *length* bytes to *buffer* read by *reader*. If *length* is zero the buffer is closed.

      @param length the length of the forward content
      @param[in] buffer the buffer
      @param[in] reader the reader
     */
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
                    LOG(WARN, "buffer is full...skipping {} bytes", length - sent);

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
            buffer_reader reader(request->params_buffer);

            LOG(DEBUG, "reading all parameters");

            request->params._read_parameters(reader);

            // initialize input buffers
        }

        // initialize input streams
        input_streambuf sin(request->input_buffer);
        input_streambuf sdata(request->data_buffer);
        byte_istream input_stream(&sin);
        byte_istream data_stream(&sdata);

        if (request->role_type == detail::ROLE::FCGI_FILTER || request->role_type == detail::ROLE::FCGI_RESPONDER) {
            static_cast<responder*>(role.get())->_input_stream = &input_stream;

            // initialize data stream
            if (request->role_type == detail::ROLE::FCGI_FILTER) {
                static_cast<filter*>(role.get())->_data_stream = &data_stream;

                // wait until input stream finished reading
                request->input_buffer->wait_for_all_input();
            }
        }

        // create output streams
        output_streambuf sout([&request, version](void* buffer, std::size_t size) -> std::pair<void*, std::size_t> {
            if (buffer) {
                auto flag =
                    detail::record::write(version, request->id, *request->output_manager,
                                          detail::stdout_stream{ buffer, static_cast<detail::double_type>(size) });

                request->output_manager->buffer_manager().free_page(buffer, flag);
            }

            return { request->output_manager->buffer_manager().new_page(),
                     request->output_manager->buffer_manager().page_size() };
        });
        output_streambuf serr([&request, version](void* buffer, std::size_t size) -> std::pair<void*, std::size_t> {
            if (buffer) {
                auto flag =
                    detail::record::write(version, request->id, *request->output_manager,
                                          detail::stderr_stream{ buffer, static_cast<detail::double_type>(size) });

                request->output_manager->buffer_manager().free_page(buffer, flag);
            }

            return { request->output_manager->buffer_manager().new_page(),
                     request->output_manager->buffer_manager().page_size() };
        });
        byte_ostream output_stream(&sout);
        byte_ostream error_stream(&serr);

        role->_params        = &request->params;
        role->_output_stream = &output_stream;
        role->_error_stream  = &error_stream;
        role->_cancelled     = &request->cancelled;

        // execute the role
        role::status_code_type status = -1;

        try {
            status = role->run();
        } catch (const std::exception& e) {
            LOG(ERROR, "role executor threw an exception ({})", e.what());
        } catch (...) {
            LOG(ERROR, "role executor threw an exception");
        }

        LOG(INFO, "role finished with status code={}", static_cast<detail::quadruple_type>(status));

        // flush and finish all output streams
        output_stream.flush();
        detail::record::write(version, request->id, *request->output_manager, detail::stdout_stream{ nullptr, 0 });
        error_stream.flush();
        detail::record::write(version, request->id, *request->output_manager, detail::stderr_stream{ nullptr, 0 });

        // end request
        detail::record::write(version, request->id, *request->output_manager,
                              detail::end_request{ static_cast<detail::quadruple_type>(status),
                                                   detail::PROTOCOL_STATUS::FCGI_REQUEST_COMPLETE });

        LOG(INFO, "request {} finished; removing", request->id);

        // trigger end
        if (request->close_connection) {
            _terminate_connection.store(true, std::memory_order_release);
        }

        request->finished.store(true, std::memory_order_release);
    }
    void _begin_request(reader& reader, const std::shared_ptr<output_manager>& output_manager, detail::record record)
    {
        auto body    = detail::begin_request::read(reader);
        auto request = std::make_shared<class request>(record.request_id, body.role, output_manager,
                                                       (body.flags & detail::FLAGS::FCGI_KEEP_CONN) == 0);

        switch (body.role) {
        case detail::ROLE::FCGI_AUTHORIZER:
        case detail::ROLE::FCGI_FILTER:
        case detail::ROLE::FCGI_RESPONDER: {
            auto& factory = _role_factories[body.role - 1];

            if (factory) {
                auto role = factory();

                LOG(INFO, "created role; launching request thread");
                request->params_buffer.reset(new buffer(_allocator, 99999));

                // launch thread
                request->handler_thread =
                    std::thread(&request_manager::_request_hanlder, this, std::move(role), request);

                break;
            } // else fall through, because role is unimplemented
        }
        default: {
            LOG(ERROR, "begin request record rejected because of unknown/unimplemented role {}", body.role);

            // reject because role is unknown
            detail::record::write(detail::FCGI_VERSION_1, record.request_id, *output_manager,
                                  detail::end_request{ 0, detail::PROTOCOL_STATUS::FCGI_UNKNOWN_ROLE });

            return;
        }
        }

        // add request
        _requests.insert({ record.request_id, std::move(request) });
    }
};

} // namespace fast_cgi