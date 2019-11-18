#pragma once

#include "buffer.hpp"
#include "reader.hpp"

#include <cstdint>

namespace fast_cgi {

class buffer_reader : public reader
{
public:
	buffer_reader(buffer& buffer) : _buffer(buffer)
	{}
	virtual std::size_t read(void* buffer, std::size_t size) override
	{}
	virtual std::size_t skip(std::size_t size) override
	{}

private:
	buffer& _buffer;
	std::int8_t* begin;
	std::int8_t* end;
};

} // namespace fast_cgi