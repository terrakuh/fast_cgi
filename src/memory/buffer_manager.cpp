#include "fast_cgi/log.hpp"
#include "fast_cgi/memory/buffer_manager.hpp"

namespace fast_cgi {
namespace memory {

buffer_manager::buffer_manager(std::size_t page_size, std::shared_ptr<allocator> allocator)
    : _allocator(std::move(allocator))
{
	_page_size = page_size;
}

buffer_manager::~buffer_manager()
{
	for (auto& page : _pages) {
		_allocator->deallocate(page, _page_size);
	}

	for (auto& page : _free_pages) {
		_allocator->deallocate(page.first, _page_size);
	}
}

void buffer_manager::free_page(void* page, std::shared_ptr<std::atomic_bool> tracker)
{
	auto result = _pages.find(page);

	if (result != _pages.end()) {
		auto ptr = *result;

		_pages.erase(result);

		_free_pages.push_back({ ptr, std::move(tracker) });
	} else {
		FAST_CGI_LOG(CRITICAL, "requesting freeing of unkown page: {}", page);
	}
}

void* buffer_manager::new_page()
{
	// check for free page
	for (auto i = _free_pages.begin(); i != _free_pages.end(); ++i) {
		if (!i->second || i->second->load(std::memory_order_acquire)) {
			auto ptr = i->first;

			_free_pages.erase(i);
			_pages.insert(ptr);

			return ptr;
		}
	}

	// allocate new page
	auto ptr = _allocator->allocate(_page_size, 1);

	_pages.insert(ptr);

	return ptr;
}

std::size_t buffer_manager::page_size() const noexcept
{
	return _page_size;
}

} // namespace memory
} // namespace fast_cgi