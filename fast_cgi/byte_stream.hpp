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

typedef std::uint8_t byte_type;

class byte_ostream : public std::basic_ostream<byte_type>
{
public:
	using std::basic_ostream<byte_type>::basic_ostream;
};

class byte_istream : public std::basic_istream<byte_type>
{
public:
	using std::basic_istream<byte_type>::basic_istream;
};

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
	typedef std::function<std::pair<byte_type*, std::size_t>(const void*, std::size_t)> writer_type;

	output_streambuf(byte_type* buffer, std::size_t size, writer_type writer) : _writer(writer)
	{
		setp(buffer, buffer + size);
	}

protected:
	virtual int sync() override
	{
		if (pptr() > pbase()) {
			auto _buffer = _writer(pptr(), static_cast<std::size_t>(pptr() - pbase()));

			setp(_buffer.first, _buffer.first + _buffer.second);
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