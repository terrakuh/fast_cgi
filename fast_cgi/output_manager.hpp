#pragma once

#include "writer.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <utility>

namespace fast_cgi {

class output_manager
{
public:
	typedef std::function<void(writer&)> task_type;

	/**
	  Adds a writing task to the queue. The task are executed on a different thread at an unspecified time.

	  @param task is the writing task
	  @returns a future that will be completed when the writing task finished
	 */
	std::future<void> add(task_type&& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		_queue.push_back({ std::move(task), {} });
		_cv.notify_one();

		return _queue.back().second.get_future();
	}
	void run(writer& writer)
	{
		while (true) {
			std::pair<task_type, std::promise<void>> task;

			// poll queue
			{
				std::unique_lock<std::mutex> lock(_mutex);

				_cv.wait(lock, [&] { return !_queue.empty(); });

				task = std::move(_queue.front());
				_queue.pop_front();
			}

			// execute task
			task.first(writer);
			task.second.set_value();
			writer.flush();
		}
	}

private:
	std::deque<std::pair<task_type, std::promise<void>>> _queue;
	std::mutex _mutex;
	std::condition_variable _cv;
};

} // namespace fast_cgi