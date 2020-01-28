#ifndef FAST_CGI_MEMORY_ALLOCATOR_HPP_
#define FAST_CGI_MEMORY_ALLOCATOR_HPP_

#include <cstddef>

namespace fast_cgi {
namespace memory {

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
	virtual void deallocate(void* ptr, std::size_t size)        = 0;
};

} // namespace memory
} // namespace fast_cgi

#endif