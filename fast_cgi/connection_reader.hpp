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
	connection_reader(const std::shared_ptr<connection>& connection) : connection(connection), buffer(new std::uint8_t[max_size])
	{
		cursor = 0;
		size   = 0;
	}
	virtual std::size_t read(void* buffer, std::size_t size) override
	{
	}
	void require_size(std::size_t size)
	{
		if (cursor + size > this->size) {
			this->size -= cursor;

			std::memmove(buffer.get(), buffer.get() + cursor, this->size);

			cursor = 0;

			// read
			this->size += connection->read(buffer.get() + this->size, size - this->size, max_size - this->size);
		}
	}
	virtual std::size_t skip(std::size_t size) override
	{
		while (size > 0) {
		}
	}

private:
	constexpr static auto max_size = 4096;

	std::shared_ptr<connection> connection;
	std::unique_ptr<std::uint8_t[]> buffer;
	std::size_t cursor;
	std::size_t size;
};

} // namespace fast_cgi