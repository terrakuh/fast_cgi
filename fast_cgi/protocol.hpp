#pragma once

#include "allocator.hpp"
#include "connection.hpp"
#include "connector.hpp"
#include "detail/record.hpp"
#include "io/input_manager.hpp"
#include "io/output_manager.hpp"
#include "log.hpp"
#include "request_manager.hpp"
#include "role.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace fast_cgi {

class protocol
{
public:
    protocol(const std::shared_ptr<connector>& connector, const std::shared_ptr<allocator>& allocator)
        : _connector(connector), _allocator(allocator)
    {
        _version = detail::VERSION::FCGI_VERSION_1;
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<role, T>::value>::type set_role()
    {
        if (std::is_base_of<responder, T>::value) {
            _role_factories[0] = [] { return std::unique_ptr<role>(new T()); };
        }
    }
    void run()
    {
        _connector->run([this](std::shared_ptr<connection> conn) {
            FAST_CGI_LOG(INFO, "accepted new connection; launching new thread");

            _connections.push_back(std::thread(&protocol::_connection_thread, this, std::move(conn)));
        });
    }
    void join()
    {
        for (auto& thread : _connections) {
            thread.join();
        }

        _connections.clear();
    }

private:
    detail::VERSION _version;
    std::shared_ptr<connector> _connector;
    std::vector<std::thread> _connections;
    std::shared_ptr<allocator> _allocator;
    std::array<std::function<std::unique_ptr<role>()>, 3> _role_factories;

    void _connection_thread(std::shared_ptr<connection> connection)
    {
        auto output_manager = std::make_shared<io::output_manager>(connection, _allocator);
        auto reader         = io::input_manager::launch_input_manager(connection, _allocator);

        try {
            _input_handler(reader, output_manager);
        } catch (const exception::io_exception& e) {
            FAST_CGI_LOG(INFO, "buffer closed ({})", e.what());
        }

        FAST_CGI_LOG(INFO, "connection thread terminating");
    }
    void _input_handler(const std::shared_ptr<io::reader>& reader,
                        const std::shared_ptr<io::output_manager>& output_manager)
    {
        request_manager request_manager(_allocator, reader, _role_factories);

        while (!request_manager.should_terminate_connection()) {
            auto record = detail::record::read(*reader);

            FAST_CGI_LOG(INFO, "received record: version={}, type={}, id={}, length={}, padding={}", record.version, record.type,
                record.request_id, record.content_length, record.padding_length);

            // version mismatch
            if (record.version != _version) {
                FAST_CGI_LOG(CRITICAL, "version mismatch (supported: {}|given: {})", _version, record.version);

                return;
            }

            // process record
            switch (record.type) {
            case detail::TYPE::FCGI_GET_VALUES: {
                _get_values(*reader, *output_manager, record);

                break;
            }
            default: {
                // a request -> handled
                if (record.request_id && request_manager.handle_request(output_manager, record)) {
                    break;
                }

                FAST_CGI_LOG(WARN, "skipping record because of unkown type {}", record.type);

                // ignore body
                reader->skip(record.content_length);

                // tell the server that the record was ignored
                detail::record::write(_version, record.request_id, *output_manager,
                                      detail::unknown_type{ record.type });

                break;
            }
            }

            // skip padding
            reader->skip(record.padding_length);
        }
    }
    void _get_values(io::reader& reader, io::output_manager& output_manager, detail::record record)
    {
        constexpr auto max_conns  = "FCGI_MAX_CONNS";
        constexpr auto max_reqs   = "FCGI_MAX_REQS";
        constexpr auto mpxs_conns = "FCGI_MPXS_CONNS";
        constexpr auto max_size   = 15;
        std::string name;
        auto answer = std::make_shared<std::map<const char*, std::string>>();

        while (true) {
            auto pair = detail::name_value_pair::read(reader);

            // read name
            if (pair.name_length <= max_size) {
                name.resize(pair.name_length);
                reader.read(&name[0], pair.name_length);
            } else {
                name.clear();
            }

            // ignore value
            reader.skip(pair.value_length);

            if (name == max_conns) {
            } else if (name == max_reqs) {
            } else if (name == mpxs_conns) {
                (*answer)[mpxs_conns] = "1";
            }
        }

        // answer
        auto begin = answer->begin();
        auto end   = answer->end();

        detail::record::write(_version, 0, output_manager,
                              detail::get_values_result::from_generator(
                                  [answer, begin, end]() mutable -> detail::get_values_result::generated_type {
                                      if (begin != end) {
                                          auto t = *begin;

                                          ++begin;

                                          return t;
                                      }

                                      return {};
                                  }));
    }
};

} // namespace fast_cgi