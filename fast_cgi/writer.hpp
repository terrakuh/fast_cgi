#pragma once

#include "connection.hpp"
#include "detail/config.hpp"

#include <memory>

namespace fast_cgi {

class writer
{
public:
	template<typename T>
	typename std::enable_if<(std::is_unsigned<T>::value && (sizeof(T) <= 2 || sizeof(T) == 4))>::value write(T value)
	{}
	void write(const void* src, std::size_t size)
	{}
	void write_variable(detail::quadruple_type value)
	{}
	void flush()
	{}
	/*
	 Writes all values to the stream encoded as big endian.

	*/
	template<typename... T>
	std::size_t write_all(T&&... values)
	{
		constexpr auto size = size_of<T...>();

		reserve(size);

		return size;
	}

protected:
	virtual void reserve(std::size_t length) = 0;

private:
	std::shared_ptr<connection> connection;

	template<typename T>
	constexpr static std::size_t size_of() noexcept
	{
		return sizeof(T);
	}
	template<typename T, typename... Other>
	constexpr static std::size_t size_of() noexcept
	{
		return sizeof(T) + size_of<Other...>();
	}
};

} // namespace fast_cgi