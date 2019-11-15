#pragma once

#include "writer.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace fast_cgi {

class output_manager
{
public:
	typedef std::function<void(writer&)> task_type;

	void add(task_type&& task)
	{
		std::lock_guard<std::mutex> lock(_mutex);

		_queue.push_back(std::move(task));
		_cv.notify_one();
	}
	void run(writer& writer)
	{
		while (true) {
			task_type task;

			// poll queue
			{
				std::unique_lock<std::mutex> lock(_mutex);

				_cv.wait(lock, [&] { return !_queue.empty(); });

				task = std::move(_queue.front());
				_queue.pop_front();
			}

			// execute task
			task(writer);
			writer.flush();
		}
	}

private:
	std::deque<task_type> _queue;
	std::mutex _mutex;
	std::condition_variable _cv;
};

} // namespace fast_cgi