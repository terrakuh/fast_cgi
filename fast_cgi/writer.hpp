#pragma once

#include "connection.hpp"
#include "detail/config.hpp"

#include <cstdint>
#include <memory>
#include <utility>

namespace fast_cgi {

class writer
{
public:
    template<typename T>
    typename std::enable_if<(std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4)), std::size_t>::type
        write(T value)
    {
        if (sizeof(T) == 1) {
            _connection->write(&value, 1);
        } else if (sizeof(T) == 2) {
            std::uint8_t buf[] = { static_cast<std::uint8_t>(value >> 8), static_cast<std::uint8_t>(value & 0xff) };

            return _connection->write(buf, 2);
        }

        std::uint8_t buf[] = { static_cast<std::uint8_t>(value >> 24 | 0x80),
                               static_cast<std::uint8_t>(value >> 16 & 0xff),
                               static_cast<std::uint8_t>(value >> 8 & 0xff), static_cast<std::uint8_t>(value & 0xff) };

        return _connection->write(buf, 4);
    }
    std::size_t write(const void* src, std::size_t size)
    {
        return _connection->write(src, size);
    }
    std::size_t write_variable(detail::quadruple_type value)
    {
        if (value <= 127) {
            return write(static_cast<detail::single_type>(value));
        }

        return write(value);
    }
    void flush()
    {
        _connection->flush();
    }
    /*
     Writes all values to the stream encoded as big endian.

    */
    template<typename T, typename... Other>
    std::size_t write_all(T&& value, Other&&... other)
    {
        auto s = write(std::forward<T>(value));

        return s + write_all(std::forward<Other>(other)...);
    }
    std::size_t write_all()
    {
        return 0;
    }

private:
    friend class output_manager;

    std::shared_ptr<connection> _connection;

    writer(const std::shared_ptr<connection>& connection) : _connection(connection)
    {}
};

} // namespace fast_cgi