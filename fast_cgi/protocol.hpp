#pragma once

#include "connection.hpp"
#include "connection_reader.hpp"
#include "connector.hpp"
#include "detail/record.hpp"
#include "output_manager.hpp"
#include "request_manager.hpp"
#include "role.hpp"
#include "writer.hpp"

#include <cstddef>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <type_traits>
#include <vector>

namespace fast_cgi {

class protocol
{
public:
    protocol(std::shared_ptr<connector> connector) : _connector(std::move(connector))
    {
        _version = detail::VERSION::FCGI_VERSION_1;
    }

    template<typename T>
    typename std::enable_if<std::is_base_of<role, T>::value>::type set_role()
    {}
    void run()
    {
        // accept
        std::shared_ptr<connection> connection;

        while (connection = _connector->accept()) {
            spdlog::info("accepting new connection; launching new thread");

            _connections.push_back(std::thread(&protocol::_connection_thread, this, std::move(connection)));
        }
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

    void _connection_thread(std::shared_ptr<connection> connection)
    {
        volatile auto alive = true;
        connection_reader reader(connection);
        output_manager output_manager(connection);
        std::thread output_thread(&fast_cgi::output_manager::run, &output_manager, std::ref(alive));

        _input_handler(alive, reader, output_manager);

        output_thread.join();
    }
    void _input_handler(volatile bool& alive, reader& reader, output_manager& output_manager)
    {
        request_manager request_manager;

        while (alive) {
            auto record = detail::record::read(reader);

            // version mismatch
            if (record.version != _version) {
                spdlog::critical("version mismatch (supported: {}|given: {})", _version, record.version);

                alive = false;

                return;
            }

            // process record
            switch (record.type) {
            case detail::TYPE::GET_VALUES: {
                _get_values(reader, output_manager, record);

                break;
            }
            default: {
                // a request -> handled
                if (record.request_id && request_manager.handle_request(reader, output_manager, record)) {
                    break;
                }

                // ignore body
                reader.skip(record.content_length);

                // tell the server that the record was ignored
                detail::record::write(_version, record.request_id, output_manager, detail::unknown_type{ record.type });

                break;
            }
            }

            // skip padding
            reader.skip(record.padding_length);
        }
    }
    void _get_values(reader& reader, output_manager& output_manager, detail::record record)
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
                answer[mpxs_conns] = "1";
            }
        }

        // answer
        auto begin = answer->begin();
        auto end   = answer->end();

        detail::record::write(_version, 0, output_manager, detail::name_value_pair::from_generator([answer, begin, end]() mutable -> detail::name_value_pair::generated_type {
            if (begin != end) {
                auto t = *begin;

                ++begin;

                return t;
            }

            return {};
        });
    }
};

} // namespace fast_cgi