#pragma once

#include "../detail/config.hpp"
#include "../exception/io_exception.hpp"
#include "../memory/buffer.hpp"

#include <cstddef>
#include <cstdint>
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

	reader(std::shared_ptr<memory::buffer> buffer);
	void interrupt();
	detail::quadruple_type read_variable();
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
	std::size_t read(void* buffer, std::size_t size);
	std::size_t skip(std::size_t size);

private:
	std::shared_ptr<memory::buffer> _buffer;
	std::int8_t* _begin;
	std::int8_t* _end;
};

} // namespace io
} // namespace fast_cgi