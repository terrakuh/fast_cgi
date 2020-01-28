#ifndef FAST_CGI_IO_OUTPUT_MANAGER_HPP_
#define FAST_CGI_IO_OUTPUT_MANAGER_HPP_

#include "../connection.hpp"
#include "../memory/allocator.hpp"
#include "../memory/buffer_manager.hpp"
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

	output_manager(std::shared_ptr<connection> connection, std::shared_ptr<memory::allocator> allocator);
	~output_manager();
	memory::buffer_manager& buffer_manager() noexcept;
	/**
	  Adds a writing task to the queue. The task are executed on a different thread at an unspecified time.

	  @param task is the writing task
	  @returns a future that will be completed when the writing task finished
	 */
	std::shared_ptr<std::atomic_bool> add(task_type task);

private:
	typedef std::pair<task_type, std::shared_ptr<std::atomic_bool>> queue_type;

	bool _alive;
	std::deque<queue_type> _queue;
	std::mutex _mutex;
	std::condition_variable _cv;
	writer _writer;
	std::thread _thread;
	memory::buffer_manager _buffer_manager;

	void _run();
};

} // namespace io
} // namespace fast_cgi

#endif