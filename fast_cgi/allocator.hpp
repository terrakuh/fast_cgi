#pragma once

#include <cstddef>

namespace fast_cgi {

class allocator
{
public:
	virtual ~allocator() = default;
	template<typename T>
	T* allocate(std::size_t count)
	{
		return static_cast<T*>(allocate(count * sizeof(T), alignof(T)));
	}
	virtual void* allocate(std::size_t size, std::size_t align) = 0;
	virtual void deallocate(void* ptr, std::size_t size)		= 0;
};

} // namespace fast_cgi