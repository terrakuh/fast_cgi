#ifndef FAST_CGI_MEMORY_BUFFER_MANAGER_HPP_
#define FAST_CGI_MEMORY_BUFFER_MANAGER_HPP_

#include "allocator.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <set>
#include <vector>

namespace fast_cgi {
namespace memory {

class buffer_manager
{
public:
	buffer_manager(std::size_t page_size, std::shared_ptr<allocator> allocator);
	buffer_manager(const buffer_manager& copy) = delete;
	buffer_manager(buffer_manager&& copy)      = delete;
	~buffer_manager();
	void free_page(void* page, std::shared_ptr<std::atomic_bool> tracker = nullptr);
	void* new_page();
	std::size_t page_size() const noexcept;

private:
	std::shared_ptr<allocator> _allocator;
	std::size_t _page_size;
	std::set<void*> _pages;
	std::vector<std::pair<void*, std::shared_ptr<std::atomic_bool>>> _free_pages;
};

} // namespace memory
} // namespace fast_cgi

#endif