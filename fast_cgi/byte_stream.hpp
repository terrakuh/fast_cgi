#pragma once

#include "allocator.hpp"
#include "buffer.hpp"

#include <cstdint>
#include <functional>
#include <istream>
#include <ostream>
#include <streambuf>

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
	input_streambuf(buffer& buffer) : _buffer(buffer)
	{}
	input_streambuf(input_streambuf&& move) : std::basic_streambuf(std::move(move))
	{
		_end	  = move._end;
		move._end = nullptr;
	}
	void wait_for_all_input()
	{}

protected:
	virtual int_type underflow() override
	{
		// wait for input
		auto input = _buffer.wait_for_input();

		// reached end
		if (input.second == 0) {
			return traits_type::eof();
		}

		auto ptr = static_cast<byte_type*>(input.first);

		setg(ptr, ptr, ptr + input.second);

		return traits_type::to_int_type(*ptr);
	}

private:
	byte_type* _end;
	buffer& _buffer;
};

class output_streambuf : public std::basic_streambuf<byte_type>
{
public:
	typedef std::function<void(const void*, std::size_t)> writer_type;

	output_streambuf(allocator* allocator, std::size_t size, writer_type writer) : _writer(writer)
	{
		_allocator = allocator;
		_buffer	= allocator->allocate(size);
		_size	  = size;
	}
	~output_streambuf()
	{
		if (_buffer) {
			_allocator->deallocate(_buffer, _size);
		}
	}

protected:
	virtual int sync() override
	{
		if (pptr() != pbase()) {
			_writer(pptr(), static_cast<std::size_t>(pptr() - pbase()));
			setp(_buffer, _buffer + _size);
		}
	}
	virtual int_type overflow(int_type c = traits_type::eof()) override
	{
		// write
		sync();

		if (c != traits_type::eof()) {
			sputc(traits_type::to_char_type(c));
		}

		return traits_type::to_int_type(0);
	}

private:
	allocator* _allocator;
	byte_type* _buffer;
	std::size_t _size;
	std::function<void(const void*, std::size_t)> _writer;
};

} // namespace fast_cgi