#pragma once

#include "connection.hpp"
#include "writer.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <atomic>

namespace fast_cgi {

class output_manager
{
public:
	typedef std::function<void(writer&)> task_type;

	output_manager(const std::shared_ptr<connection>& connection) : _writer(connection)
	{}
	/**
	  Adds a writing task to the queue. The task are executed on a different thread at an unspecified time.

	  @param task is the writing task
	  @returns a future that will be completed when the writing task finished
	 */
	std::shared_ptr<std::atomic_bool> add(task_type&& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		std::shared_ptr<std::atomic_bool> ret(new std::atomic_bool(false));

		_queue.push_back({ std::move(task), ret });
		_cv.notify_one();

		return ret;
	}
	void run(volatile bool& alive)
	{
		while (alive) {
			queue_type task;

			// poll queue
			{
				std::unique_lock<std::mutex> lock(_mutex);

				if (_queue.empty()) {
					_writer.flush();
				}

				_cv.wait(lock, [&] { return !_queue.empty() || !alive; });

				if (!alive) {
					break;
				}

				task = std::move(_queue.front());
				_queue.pop_front();
			}

			// execute task
			try {
				task.first(_writer);
			} catch (...) {
			}

			task.second->store(true, std::memory_order_release);
		}
	}

private:
	typedef std::pair<task_type, std::shared_ptr<std::atomic_bool>> queue_type;

	std::deque<queue_type> _queue;
	std::mutex _mutex;
	std::condition_variable _cv;
	writer _writer;
};

} // namespace fast_cgi