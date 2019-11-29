#pragma once

#include "detail/record.hpp"
#include "reader.hpp"
#include "log.hpp"

#include <map>
#include <string>
#include <utility>

namespace fast_cgi {

class params
{
public:
    constexpr static auto REQUEST_URI    = "REQUEST_URI";
    constexpr static auto QUERY_STRING   = "QUERY_STRING";
    constexpr static auto CONTENT_LENGTH = "CONTENT_LENGTH";
    constexpr static auto CONTENT_TYPE   = "CONTENT_TYPE";

    const std::string& operator[](const std::string& key) const
    {
        return _parameters.at(key);
    }

private:
    friend class request_manager;

    std::map<std::string, std::string> _parameters;

    void _read_parameters(reader& reader)
    {
        try {
            while (true) {
                auto pair = detail::name_value_pair::read(reader);
                std::string name;
                std::string value;

                name.resize(pair.name_length);
                reader.read(&name[0], pair.name_length);

                value.resize(pair.value_length);
                reader.read(&value[0], pair.value_length);

                LOG(debug("read parameter: {}={}", name, value));

                _parameters[std::move(name)] = std::move(value);
            }
        } catch (const exception::io_exception& e) {
        }
    }
};

} // namespace fast_cgi