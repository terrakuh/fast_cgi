#pragma once

#include "../buffer_manager.hpp"
#include "../connection.hpp"
#include "../log.hpp"
#include "writer.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace fast_cgi {
namespace io {

class output_manager
{
public:
    typedef std::function<void(writer&)> task_type;

    output_manager(const std::shared_ptr<connection>& connection, const std::shared_ptr<allocator>& allocator)
        : _alive(true), _writer(connection), _thread(&output_manager::_run, this), _buffer_manager(1024, allocator)
    {
        LOG(TRACE, "output manager thread started");
    }
    ~output_manager()
    {
        _mutex.lock();
        _alive = false;
        _mutex.unlock();
        _cv.notify_one();

        // wait
        _thread.join();

        LOG(TRACE, "output manager thread terminated");
    }
    class buffer_manager& buffer_manager() noexcept
    {
        return _buffer_manager;
    }
    /**
      Adds a writing task to the queue. The task are executed on a different thread at an unspecified time.

      @param task is the writing task
      @returns a future that will be completed when the writing task finished
     */
    std::shared_ptr<std::atomic_bool> add(task_type&& task)
    {
        LOG(DEBUG, "adding output task");

        std::lock_guard<std::mutex> lock(_mutex);
        std::shared_ptr<std::atomic_bool> ret(new std::atomic_bool(false));

        _queue.push_back({ std::move(task), ret });
        _cv.notify_one();

        return ret;
    }

private:
    typedef std::pair<task_type, std::shared_ptr<std::atomic_bool>> queue_type;

    bool _alive;
    std::deque<queue_type> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    writer _writer;
    std::thread _thread;
    class buffer_manager _buffer_manager;

    void _run()
    {
        while (true) {
            queue_type task;

            // poll queue
            {
                std::unique_lock<std::mutex> lock(_mutex);

                if (_queue.empty()) {
                    _writer.flush();
                }

                _cv.wait(lock, [&] { return !_queue.empty() || !_alive; });

                if (!_alive && _queue.empty()) {
                    break;
                }

                task = std::move(_queue.front());
                _queue.pop_front();
            }

            LOG(TRACE, "executing writer task");

            // execute task
            try {
                task.first(_writer);
            } catch (const std::exception& e) {
                LOG(CRITICAL, "failed to execute writer task ({})", e.what());
            } catch (...) {
                LOG(CRITICAL, "failed to execute writer task");
            }

            task.second->store(true, std::memory_order_release);
        }
    }
};

} // namespace io
} // namespace fast_cgi