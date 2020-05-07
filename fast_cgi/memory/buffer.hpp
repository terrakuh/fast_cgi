#ifndef FAST_CGI_MEMORY_BUFFER_HPP_
#define FAST_CGI_MEMORY_BUFFER_HPP_

#include "allocator.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>

namespace fast_cgi {
namespace memory {

class buffer
{
public:
	class writer
	{
	public:
		writer(const writer& copy) = delete;
		writer(writer&& move);
		~writer();
		/**
		 * Returns a buffer where the user can write his contents. The returned buffer size may be smaller than the
		 * desired size.
		 *
		 * @param desired the desired size of the buffer
		 * @returns the buffer pointer and its size; if `size==0` the buffer is full or the token has been closed
		 */
		std::pair<void*, std::size_t> request_buffer(std::size_t desired);
		/**
		 * Closes this token. Calling this function on a closed token has no effect.
		 */
		void close() noexcept;
		/**
		 * Checks whether this token has been closed.
		 */
		bool closed() noexcept;

	private:
		friend buffer;

		buffer* _buffer;
		std::unique_lock<std::mutex> _lock;

		writer(buffer* buffer);
	};

	/**
	 * Creates a new buffer with the given max size.
	 *
	 * @param allocator the memory allocator
	 * @param max_size the maximum allowed buffer size
	 */
	buffer(std::shared_ptr<allocator> allocator, std::size_t max_size);
	buffer(const buffer& copy) = delete;
	buffer(buffer&& move)      = delete;
	~buffer();
	void interrupt_all_waiting();
	bool interrupted();
	void set_max(std::size_t max);
	/**
	 * Blocks until the buffer has reached max size.
	 *
	 * @throw exception::interrupted_exception if reading was interrupted
	 */
	void wait_for_all_input();
	/**
	 * Waits until new input is available. Every input returned by previous calls to this function are invalidated.
	 *
	 * @returns the new input or `{nullptr, 0}` if no more input is available
	 * @throw exception::interrupted_exception if reading was interrupted
	 */
	std::pair<void*, std::size_t> wait_for_input();
	/**
	 * Closes the buffer and prevents any more writes. Calling this function on a closed buffer has no effect.
	 */
	void close();
	bool output_closed() noexcept;
	bool input_closed() noexcept;
	/**
	 *  Locks the buffer and starts writing. The lock is released when the returned token is destroyed or closed.
	 * Sharing a token between threads results in undefined behavior. This token may **not** exist longer than this
	 * instance.
	 *
	 *  @returns the writer token
	 */
	writer begin_writing();

private:
	struct page
	{
		constexpr static auto max_size = 4096;
		void* begin;
		std::size_t size;
		std::size_t consumed;
		std::size_t written;
	};

	bool _interrupted;
	std::shared_ptr<allocator> _allocator;
	std::mutex _mutex;
	std::condition_variable _waiter;
	std::deque<page> _pages;
	/** how much has already been written */
	std::size_t _write_total;
	/** how much has already been consumed */
	std::size_t _consume_total;
	/** the maximum allowed size */
	std::size_t _max_size;

	page& append_new_page();
};

} // namespace memory
} // namespace fast_cgi

#endif