#pragma once

#include "buffer.hpp"
#include "reader.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>

namespace fast_cgi {

class buffer_reader : public reader
{
public:
    buffer_reader(const std::shared_ptr<buffer>& buffer) : _buffer(buffer)
    {}
    virtual std::size_t read(void* buffer, std::size_t size) override
    {
        const auto initial_size = size;

        // need more
        while (size > 0) {
            if (_begin >= _end) {
                auto buf = _buffer->wait_for_input();

                // buffer is empty
                if (!buf.second) {
                    break;
                }

                _begin = static_cast<std::int8_t*>(buf.first);
                _end   = _begin + buf.second;
            }

            auto s = std::min(size, static_cast<std::size_t>(_end - _begin));

            std::memcpy(buffer, _begin, s);

            _begin += s;
            size -= s;
        }

        return initial_size - size;
    }
    virtual std::size_t skip(std::size_t size) override
    {
        const auto initial_size = size;

        // need more
        while (size > 0) {
            if (_begin >= _end) {
                auto buf = _buffer->wait_for_input();

                // buffer is empty
                if (!buf.second) {
                    break;
                }

                _begin = static_cast<std::int8_t*>(buf.first);
                _end   = _begin + buf.second;
            }

            auto s = std::min(size, static_cast<std::size_t>(_end - _begin));

            _begin += s;
            size -= s;
        }

        return initial_size - size;
    }

private:
    std::shared_ptr<buffer> _buffer;
    std::int8_t* _begin;
    std::int8_t* _end;
};

} // namespace fast_cgi