#include "fast_cgi/exception/interrupted_error.hpp"
#include "fast_cgi/log.hpp"
#include "fast_cgi/memory/buffer.hpp"

#include <algorithm>

namespace fast_cgi {
namespace memory {

buffer::writer::writer(writer&& move) : _lock(std::move(move._lock))
{
	_buffer      = move._buffer;
	move._buffer = nullptr;
}

buffer::writer::~writer()
{
	close();
}

std::pair<void*, std::size_t> buffer::writer::request_buffer(std::size_t desired)
{
	if (closed()) {
		return { nullptr, 0 };
	}

	// limit buffer to max size
	desired = std::min(desired, _buffer->_max_size - _buffer->_write_total);

	if (desired == 0) {
		return { nullptr, 0 };
	}

	page* ptr = nullptr;

	for (auto& page : _buffer->_pages) {
		if (page.written < page.size) {
			ptr = &page;

			break;
		}
	}

	// no space available
	if (ptr == nullptr) {
		ptr = &_buffer->append_new_page();
	}

	auto size  = std::min(ptr->size - ptr->written, desired);
	auto begin = static_cast<std::int8_t*>(ptr->begin) + ptr->written;

	ptr->written += size;
	_buffer->_write_total += size;

	return { begin, size };
}

void buffer::writer::close() noexcept
{
	if (!closed()) {
		_lock.unlock();
		_buffer->_waiter.notify_one();

		_buffer = nullptr;
	}
}

bool buffer::writer::closed() noexcept
{
	return _buffer == nullptr || !_lock.owns_lock();
}

buffer::writer::writer(buffer* buffer) : _lock(buffer->_mutex)
{
	_buffer = buffer;
}

buffer::buffer(std::shared_ptr<allocator> allocator, std::size_t max_size) : _allocator(std::move(allocator))
{
	_interrupted   = false;
	_write_total   = 0;
	_consume_total = 0;
	_max_size      = max_size;
}

buffer::~buffer()
{
	for (auto& page : _pages) {
		_allocator->deallocate(page.begin, page.size);
	}
}

void buffer::interrupt_all_waiting()
{
	std::lock_guard<std::mutex> lock(_mutex);

	_interrupted = true;
	_waiter.notify_all();
}

bool buffer::interrupted()
{
	std::lock_guard<std::mutex> lock(_mutex);

	return _interrupted;
}

void buffer::set_max(std::size_t max)
{
	std::lock_guard<std::mutex> lock(_mutex);

	_max_size = max;
}

void buffer::wait_for_all_input()
{
	std::unique_lock<std::mutex> lock(_mutex);

	_waiter.wait(lock, [this] { return _write_total >= _max_size || _interrupted; });

	if (_interrupted) {
		throw exception::interrupted_error("waiting was interrupted");
	}
}

std::pair<void*, std::size_t> buffer::wait_for_input()
{
	std::unique_lock<std::mutex> lock(_mutex);
	page* ptr = nullptr;

	FAST_CGI_LOG(TRACE, "waiting for input, consumed={} written={} max={}", _consume_total, _write_total, _max_size);

	// wait for input
	_waiter.wait(lock, [this, &ptr] {
		if (_interrupted) {
			throw exception::interrupted_error("waiting was interrupted");
		} // reached end
		else if (_consume_total >= _max_size) {
			return true;
		}

		FAST_CGI_LOG(TRACE, "page count={}", _pages.size());

		for (auto& page : _pages) {
			FAST_CGI_LOG(TRACE, "page: consumed={} written={} max={}", page.consumed, page.written, page.size);

			if (page.written > page.consumed) {
				ptr = &page;

				return true;
			}
		}

		return false;
	});

	// reached end
	if (_consume_total >= _max_size) {
		return { nullptr, 0 };
	}

	auto begin = static_cast<std::int8_t*>(ptr->begin) + ptr->consumed;
	auto size  = ptr->written - ptr->consumed;

	// update page
	ptr->consumed = ptr->written;
	_consume_total += size;

	return { begin, size };
}

void buffer::close()
{
	std::lock_guard<std::mutex> lock(_mutex);

	_max_size = _write_total;

	_waiter.notify_all();
}

bool buffer::output_closed() noexcept
{
	std::lock_guard<std::mutex> lock(_mutex);

	return _write_total >= _max_size;
}

bool buffer::input_closed() noexcept
{
	std::lock_guard<std::mutex> lock(_mutex);

	return _consume_total >= _max_size;
}

buffer::writer buffer::begin_writing()
{
	return writer(this);
}

buffer::page& buffer::append_new_page()
{
	page p{};

	p.begin = _allocator->allocate(page::max_size, 1);
	p.size  = page::max_size;

	_pages.push_back(p);

	return _pages.back();
}

} // namespace fast_cgi
} // namespace fast_cgi