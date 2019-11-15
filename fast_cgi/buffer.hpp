#pragma once

#include "allocator.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <utility>
#include <deque>

namespace fast_cgi {

class buffer
{
public:
	class writer
	{
	public:
		writer(const writer& copy) = delete;
		writer(writer&& move) : _lock(std::move(move._lock))
		{
			_buffer		 = move._buffer;
			move._buffer = nullptr;
		}
		~writer()
		{
			close();
		}
		std::pair<void*, std::size_t> request_buffer(std::size_t desired)
		{}
		void close() noexcept
		{
			if (_buffer && _lock.owns_lock()) {
				_buffer->_waiter.notify_one();
				_lock.unlock();
			}
		}

	private:
		buffer* _buffer;
		std::unique_lock<std::mutex> _lock;

		writer(buffer* buffer) : _lock(buffer->_mutex)
		{
			_buffer = buffer;
		}
	};

	buffer(allocator* allocator, std::size_t max_size)
	{
		_allocator = allocator;

		auto page_count = max_size / page::max_size;

		if (max_size % page::max_size) {
			++page_count;
		}

		_pages.reserve();
	}
	buffer(const buffer& copy) = delete;
	buffer(buffer&& move)
	{}
	std::pair<void*, std::size_t> wait_for_input()
	{
		std::unique_lock<std::mutex> lock(_mutex);
		page* ptr = nullptr;

		// reached end
		if (_consume_total >= _max_size) {
			return { nullptr, 0 };
		}

		// wait for input
		_waiter.wait(lock, [this, &ptr] {
			for (auto& page : _pages) {
				if (page.written > page.consumed) {
					ptr = &page;

					return true;
				}
			}

			return false;
		});

		auto begin = static_cast<std::int8_t*>(ptr->begin) + ptr->consumed;
		auto size  = ptr->written - ptr->consumed;

		// update page
		ptr->consumed = ptr->written;
		_consume_total += size;

		// move page to back
		if (ptr->consumed == ptr->size) {
			
		}

		return { begin, size };
	}
	writer begin_writing()
	{
		return { this };
	}

private:
	struct page
	{
		constexpr static auto max_size;
		void* begin;
		std::size_t size;
		std::size_t consumed;
		std::size_t written;
	};

	allocator* _allocator;
	std::mutex _mutex;
	std::condition_variable _waiter;
	std::deque<page> _pages;
	std::size_t _write_total;
	std::size_t _consume_total;
	std::size_t _max_size;
};

} // namespace fast_cgi