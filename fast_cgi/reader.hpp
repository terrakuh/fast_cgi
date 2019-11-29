#pragma once

#include "exception/io_exception.hpp"
#include "detail/config.hpp"

#include <cstddef>
#include <type_traits>
#include <cstdint>

namespace fast_cgi {

class reader
{
public:
    template<typename T>
    using valid_type = std::integral_constant<bool, std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4)>;
    template<typename T>
    using underlying_type = typename std::enable_if<std::is_enum<T>::value, std::underlying_type<T>>::type;

    virtual ~reader() = default;
    detail::quadruple_type read_variable()
    {
        std::uint8_t buffer[sizeof(detail::quadruple_type)];
        detail::quadruple_type value = 0;

        if (read(buffer, 1) != 1) {
            throw exception::io_exception("buffer exhausted");
        }

        value = buffer[0];

        if (value & 0x80) {
            // read more
            if (read(buffer + 1, sizeof(buffer) - 1) != sizeof(buffer) - 1) {
                throw exception::io_exception("buffer exhausted");
            }

            value = ((value & 0x7f) << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        }

        return value;
    }
    template<typename T>
    typename std::enable_if<valid_type<typename underlying_type<T>::type>::value, T>::type read()
    {
        return static_cast<T>(read<typename std::underlying_type<T>::type>());
    }
    template<typename T>
    typename std::enable_if<valid_type<T>::value, T>::type read()
    {
        std::uint8_t buffer[sizeof(T)];
        T value = 0;

        // end reached
        if (read(buffer, sizeof(T)) != sizeof(T)) {
            throw exception::io_exception("buffer exhausted");
        }

        if (sizeof(T) == 1) {
            value = buffer[0];
        } else if (sizeof(T) == 2) {
            value = (buffer[0] << 8) | buffer[1];
        } else {
            value = ((buffer[0] & 0x7f) << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        }

        return value;
    }
    virtual std::size_t read(void* buffer, std::size_t size) = 0;
    virtual std::size_t skip(std::size_t size)               = 0;
};

} // namespace fast_cgi