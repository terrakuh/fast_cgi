#pragma once

#include "allocator.hpp"
#include "buffer.hpp"

#include <cstdint>
#include <functional>
#include <istream>
#include <ostream>
#include <streambuf>
#include <utility>

namespace fast_cgi {

typedef char byte_type;

typedef std::ostream byte_ostream;
typedef std::istream byte_istream;

class input_streambuf : public std::basic_streambuf<byte_type>
{
public:
    input_streambuf(const std::shared_ptr<buffer>& buffer) : _buffer(buffer)
    {}

protected:
    virtual int_type underflow() override
    {
        if (!_buffer) {
            return traits_type::eof();
        }

        // wait for input
        auto input = _buffer->wait_for_input();

        // reached end
        if (input.second == 0) {
            _buffer.reset();

            return traits_type::eof();
        }

        auto ptr = static_cast<byte_type*>(input.first);

        setg(ptr, ptr, ptr + input.second);

        return traits_type::to_int_type(*ptr);
    }

private:
    std::shared_ptr<buffer> _buffer;
};

class output_streambuf : public std::basic_streambuf<byte_type>
{
public:
    typedef std::function<std::pair<void*, std::size_t>(void*, std::size_t)> writer_type;

    output_streambuf(writer_type writer) : _writer(writer)
    {}

protected:
    virtual int sync() override
    {
        if (pptr() > pbase()) {
            auto _buffer = _writer(pptr(), static_cast<std::size_t>(pptr() - pbase()));

            setp(static_cast<byte_type*>(_buffer.first), static_cast<byte_type*>(_buffer.first) + _buffer.second);
        }

        return 0;
    }
    virtual int_type overflow(int_type c = traits_type::eof()) override
    {
        // write
        sync();

        // buffer is full
        if (pptr() == epptr()) {
            return traits_type::eof();
        } else if (c != traits_type::eof()) {
            sputc(traits_type::to_char_type(c));
        }

        return traits_type::to_int_type(0);
    }

private:
    writer_type _writer;
};

} // namespace fast_cgi