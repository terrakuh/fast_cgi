#pragma once

#include "connection.hpp"
#include "detail/config.hpp"

#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace fast_cgi {

class reader
{
public:
	reader(const std::shared_ptr<fast_cgi::connection>& connection)
		: connection(connection), buffer(new std::uint8_t[max_size])
	{
		cursor = 0;
		size   = 0;
	}
	detail::quadruple_type read_variable()
	{
		require_size(4);

		detail::quadruple_type value = 0;

		value = buffer.get()[cursor];

		if (value > 127) {
			value = ((value & 0x7f) << 24) + (buffer.get()[cursor + 1] << 16) + (buffer.get()[cursor + 2] << 8) +
					buffer.get()[cursor + 3];
			cursor += 4;
		} else {
			cursor += 1;
		}

		return value;
	}
	template<typename T>
	typename std::enable_if<(std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4)), T>::value read()
	{
		require_size(sizeof(T));

		T value = 0;

		if (sizeof(T) == 1) {
			value = buffer.get()[cursor];
		} else if (sizeof(T) == 2) {
			value = buffer.get()[cursor] << 8 + buffer.get()[cursor + 1];
		} else {
			value = ((buffer.get()[cursor] & 0x7f) << 24) + (buffer.get()[cursor + 1] << 16) +
					(buffer.get()[cursor + 2] << 8) + buffer.get()[cursor + 3];
		}

		cursor += sizeof(T);

		return value;
	}
	void read(void* buffer, std::size_t size)
	{}
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
	void skip(std::size_t size)
	{
		while (size > 0) {
		}
	}

private:
	constexpr static auto max_size = 4096;

	std::shared_ptr<fast_cgi::connection> connection;
	std::unique_ptr<std::uint8_t[]> buffer;
	std::size_t cursor;
	std::size_t size;
};

} // namespace fast_cgi