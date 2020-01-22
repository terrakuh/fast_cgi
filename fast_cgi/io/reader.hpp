#pragma once

#include "../buffer.hpp"
#include "../detail/config.hpp"
#include "../exception/io_exception.hpp"
#include "../log.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

namespace fast_cgi {
namespace io {

class reader
{
public:
	template<typename T>
	using valid_type = std::integral_constant<bool, std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4)>;
	template<typename T>
	using underlying_type = typename std::enable_if<std::is_enum<T>::value, std::underlying_type<T>>::type;

	reader(const std::shared_ptr<buffer>& buffer) : _buffer(buffer)
	{
		_begin = nullptr;
		_end   = nullptr;
	}
	void interrupt()
	{
		_buffer->interrupt_all_waiting();
	}
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
	std::size_t read(void* buffer, std::size_t size)
	{
		const auto initial_size = size;
		auto ptr                = static_cast<std::int8_t*>(buffer);

		// need more
		while (size > 0) {
			if (_begin >= _end) {
				FAST_CGI_LOG(TRACE, "waiting for input");
				auto buf = _buffer->wait_for_input();
				FAST_CGI_LOG(TRACE, "got input {}", buf.second);

				// buffer is empty
				if (!buf.second) {
					break;
				}

				_begin = static_cast<std::int8_t*>(buf.first);
				_end   = _begin + buf.second;
			}

			auto s = std::min(size, static_cast<std::size_t>(_end - _begin));

			std::memcpy(ptr, _begin, s);

			_begin += s;
			ptr += s;
			size -= s;
		}

		return initial_size - size;
	}
	std::size_t skip(std::size_t size)
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

} // namespace io
} // namespace fast_cgi