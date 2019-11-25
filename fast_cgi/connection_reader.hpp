#pragma once

#include "connection.hpp"
#include "reader.hpp"

#include <cstdint>
#include <cstring>
#include <memory>

namespace fast_cgi {

class connection_reader : public reader
{
public:
    connection_reader(const std::shared_ptr<connection>& connection) : _connection(connection)
    {}
    virtual std::size_t read(void* buffer, std::size_t size) override
    {
        return _connection->read(buffer, size, size);
    }
    virtual std::size_t skip(std::size_t size) override
    {
        std::int8_t buffer[1024];

        for (std::size_t i = 0; i < size;) {
            auto s = std::min(sizeof(buffer), size - i);
            auto r = _connection->read(buffer, s, s);

            i += r;

            if (s != r) {
                return i;
            }
        }

        return size;
    }

private:
    std::shared_ptr<connection> _connection;
};

} // namespace fast_cgi