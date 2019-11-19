#pragma once

#include "detail/config.hpp"

#include <cstddef>
#include <type_traits>

namespace fast_cgi {

class reader
{
public:
	virtual ~reader() = default;
	detail::quadruple_type read_variable()
	{
		char buffer[sizeof(detail::quadruple_type)];
		detail::quadruple_type value = 0;

		if (read(buffer, 1) != 1) {
		}

		value = buffer[0];

		if (value > 127) {
			// read more
			if (read(buffer + 1, sizeof(buffer) - 1) != sizeof(buffer) - 1) {
			}

			value = ((value & 0x7f) << 24) + (buffer[1] << 16) + (buffer[2] << 8) +
					buffer[3];
		}

		return value;
	}
	template<typename T>
	typename std::enable_if<(std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4)), T>::value read()
	{
		char buffer[sizeof(T)];
		T value = 0;

		// end reached
		if (read(buffer, sizeof(T)) != sizeof(T)) {
		}

		if (sizeof(T) == 1) {
			value = buffer[0];
		} else if (sizeof(T) == 2) {
			value = buffer[0] << 8 + buffer[1];
		} else {
			value = ((buffer[0] & 0x7f) << 24) + (buffer[1] << 16) +
					(buffer[2] << 8) + buffer[3];
		}

		return value;
	}
	virtual std::size_t read(void* buffer, std::size_t size) = 0;
	virtual std::size_t skip(std::size_t size) = 0;
};

} // namespace fast_cgi