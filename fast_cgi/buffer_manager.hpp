#pragma once

#include "allocator.hpp"
#include "log.hpp"

#include <atomic>
#include <cstddef>
#include <memory>
#include <set>
#include <vector>

namespace fast_cgi {

class buffer_manager
{
public:
	buffer_manager(std::size_t page_size, const std::shared_ptr<allocator>& allocator) : _allocator(allocator)
	{
		_page_size = page_size;
	}
	buffer_manager(const buffer_manager& copy) = delete;
	buffer_manager(buffer_manager&& copy)      = delete;
	~buffer_manager()
	{
		for (auto& page : _pages) {
			_allocator->deallocate(page, _page_size);
		}

		for (auto& page : _free_pages) {
			_allocator->deallocate(page.first, _page_size);
		}
	}
	void free_page(void* page, const std::shared_ptr<std::atomic_bool>& tracker = nullptr)
	{
		auto result = _pages.find(page);

		if (result != _pages.end()) {
			auto ptr = *result;

			_pages.erase(result);

			_free_pages.push_back({ ptr, tracker });
		} else {
			FAST_CGI_LOG(CRITICAL, "requesting freeing of unkown page: {}", page);
		}
	}
	void* new_page()
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
	std::size_t page_size() const noexcept
	{
		return _page_size;
	}

private:
	std::shared_ptr<allocator> _allocator;
	std::size_t _page_size;
	std::set<void*> _pages;
	std::vector<std::pair<void*, std::shared_ptr<std::atomic_bool>>> _free_pages;
};

} // namespace fast_cgi