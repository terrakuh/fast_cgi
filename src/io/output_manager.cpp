#include "fast_cgi/io/output_manager.hpp"
#include "fast_cgi/log.hpp"

namespace fast_cgi {
namespace io {

output_manager::output_manager(std::shared_ptr<connection> connection, std::shared_ptr<memory::allocator> allocator)
    : _alive(true), _writer(std::move(connection)), _thread(&output_manager::_run, this),
      _buffer_manager(1024, std::move(allocator))
{
	FAST_CGI_LOG(TRACE, "output manager thread started");
}

output_manager::~output_manager()
{
	_mutex.lock();
	_alive = false;
	_mutex.unlock();
	_cv.notify_one();

	// wait
	_thread.join();

	FAST_CGI_LOG(TRACE, "output manager thread terminated");
}

memory::buffer_manager& output_manager::buffer_manager() noexcept
{
	return _buffer_manager;
}

std::shared_ptr<std::atomic_bool> output_manager::add(task_type task)
{
	FAST_CGI_LOG(DEBUG, "adding output task");

	std::lock_guard<std::mutex> lock(_mutex);
	std::shared_ptr<std::atomic_bool> ret(new std::atomic_bool(false));

	_queue.push_back({ std::move(task), ret });
	_cv.notify_one();

	return ret;
}

void output_manager::_run()
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

		FAST_CGI_LOG(TRACE, "executing writer task");

		// execute task
		try {
			task.first(_writer);
		} catch (const std::exception& e) {
			FAST_CGI_LOG(CRITICAL, "failed to execute writer task ({})", e.what());
		} catch (...) {
			FAST_CGI_LOG(CRITICAL, "failed to execute writer task");
		}

		task.second->store(true, std::memory_order_release);
	}
}

} // namespace io
} // namespace fast_cgi