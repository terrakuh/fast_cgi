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
            _buffer      = move._buffer;
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
          @returns the buffer pointer and its size; if `size==0` the buffer is full or the token has been closed
         */
        std::pair<void*, std::size_t> request_buffer(std::size_t desired)
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
        /**
          Closes this token. Calling this function on a closed token has no effect.
         */
        void close() noexcept
        {
            if (!closed()) {
                _lock.unlock();
                _buffer->_waiter.notify_one();

                _buffer = nullptr;
            }
        }
        /**
          Checks whether this token has been closed.
         */
        bool closed() noexcept
        {
            return _buffer == nullptr || !_lock.owns_lock();
        }

    private:
        friend buffer;

        buffer* _buffer;
        std::unique_lock<std::mutex> _lock;

        writer(buffer* buffer) : _lock(buffer->_mutex)
        {
            _buffer = buffer;
        }
    };

    /**
      Creates a new buffer with the given max size.

      @param allocator the memory allocator
      @param max_size the maximum allowed buffer size
     */
    buffer(const std::shared_ptr<allocator>& allocator, std::size_t max_size) : _allocator(allocator)
    {
        _write_total   = 0;
        _consume_total = 0;
        _max_size      = max_size;
    }
    buffer(const buffer& copy) = delete;
    buffer(buffer&& move)      = delete;
    ~buffer()
    {
        for (auto& page : _pages) {
            _allocator->deallocate(page.begin, page.size);
        }
    }
    /**
      Blocks until the buffer has reached max size.
     */
    void wait_for_all_input()
    {
        std::unique_lock<std::mutex> lock(_mutex);

        _waiter.wait(lock, [this] { return _write_total >= _max_size; });
    }
    /**
      Waits until new input is available. Every input returned by previous calls to this function are invalidated.

      @returns the new input or `{nullptr, 0}` if no more input is available
     */
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

        return { begin, size };
    }
    /**
      Closes the buffer and prevents any more writes. Calling this function on a closed buffer has no effect.
     */
    void close()
    {
        _max_size = _write_total;
    }
    bool output_closed() const noexcept
    {
        return _write_total >= _max_size;
    }
    bool input_closed() const noexcept
    {
        return _consume_total >= _max_size;
    }
    /**
      Locks the buffer and starts writing. The lock is released when the returned token is destroyed or closed. Sharing
      a token between threads results in undefined behavior. This token may **not** exist longer than this instance.

      @returns the writer token
     */
    writer begin_writing()
    {
        return writer(this);
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

    std::shared_ptr<allocator> _allocator;
    std::mutex _mutex;
    std::condition_variable _waiter;
    std::deque<page> _pages;
    std::size_t _write_total;
    std::size_t _consume_total;
    /* the maximum allowed size */
    std::size_t _max_size;

    page& append_new_page()
    {
        page p{};

        p.begin = _allocator->allocate(page::max_size, 1);
        p.size  = page::max_size;

        _pages.push_back(p);

        return _pages.back();
    }
};

} // namespace fast_cgi