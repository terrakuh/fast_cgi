#pragma once

#include "allocator.hpp"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>

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
		/**
		  Returns a buffer where the user can write his contents. The returned buffer size may be smaller than the
		  desired size.

		  @param desired the desired size of the buffer
		  @returns the buffer pointer and its size; if `size==0` the buffer is full and closed
		 */
		std::pair<void*, std::size_t> request_buffer(std::size_t desired)
		{
			// limit buffer to max size
			desired = std::min(desired, _buffer->_max_size - _buffer->_write_total);

			if (desired == 0) {
				return { nullptr, 0 };
			}

			page p{};

			for (auto& page : _buffer->_pages) {
				if (page.written < page.size) {
					p = page;

					break;
				}
			}

			// no space available
			if (p.begin == nullptr) {
				p = _buffer->append_new_page();
			}

			auto size  = std::min(p.size - p.written, desired);
			auto begin = static_cast<std::int8_t*>(p.begin) + p.written;

			p.written += size;
			_buffer->_write_total += size;

			return { begin, size };
		}
		void close() noexcept
		{
			if (_buffer && _lock.owns_lock()) {
				_lock.unlock();
				_buffer->_waiter.notify_one();
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
			return { nullptr, nullptr };
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

		return { begin, size };
	}
	void close()
	{}
	writer begin_writing()
	{
		return { this };
	}

private:
	struct page
	{
		constexpr static auto max_size = 4096;
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

	page append_new_page()
	{
		page p{};

		p.begin = _allocator->allocate(0);
		p.size  = 0;

		_pages.push_back(p);

		return p;
	}
};

} // namespace fast_cgi