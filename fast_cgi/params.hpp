#pragma once

#include "detail/record.hpp"
#include "io/reader.hpp"
#include "log.hpp"

#include <map>
#include <string>
#include <utility>

namespace fast_cgi {

class params
{
public:
    typedef std::map<std::string, std::string> map_type;

    constexpr static auto REQUEST_URI    = "REQUEST_URI";
    constexpr static auto QUERY_STRING   = "QUERY_STRING";
    constexpr static auto CONTENT_LENGTH = "CONTENT_LENGTH";
    constexpr static auto CONTENT_TYPE   = "CONTENT_TYPE";

    map_type::const_iterator begin() const
    {
        return _parameters.begin();
    }
    map_type::const_iterator end() const
    {
        return _parameters.end();
    }
    bool has(const std::string& key) const
    {
        return _parameters.find(key) != _parameters.end();
    }
    const std::string& operator[](const std::string& key) const
    {
        return _parameters.at(key);
    }

private:
    friend class request_manager;

    map_type _parameters;

    void _read_parameters(io::reader& reader)
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

                LOG(DEBUG, "read parameter: {}={}", name, value);

                _parameters[std::move(name)] = std::move(value);
            }
        } catch (const exception::io_exception& e) {
        }
    }
};

} // namespace fast_cgi