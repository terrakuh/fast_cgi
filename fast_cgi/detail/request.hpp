#ifndef FAST_CGI_DETAIL_REQUEST_HPP_
#define FAST_CGI_DETAIL_REQUEST_HPP_

#include "../io/output_manager.hpp"
#include "../memory/buffer.hpp"
#include "params.hpp"
#include "record.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace fast_cgi {
namespace detail {

struct request
{
	const detail::double_type id;
	const detail::ROLE role_type;
	std::thread handler_thread;
	std::atomic_bool finished;
	class params params;
	std::shared_ptr<memory::buffer> params_buffer;
	std::shared_ptr<memory::buffer> input_buffer;
	std::shared_ptr<memory::buffer> data_buffer;
	std::shared_ptr<io::output_manager> output_manager;
	std::atomic_bool cancelled;
	const bool close_connection;

	request(detail::double_type id, detail::ROLE role_type, std::shared_ptr<io::output_manager> output_manager,
	        bool close_connection, std::shared_ptr<memory::allocator> allocator)
	    : id(id), role_type(role_type), finished(false), output_manager(std::move(output_manager)),
	      params_buffer(new memory::buffer(allocator, 99999)), input_buffer(new memory::buffer(allocator, 99999)),
	      data_buffer(new memory::buffer(allocator, 99999)), cancelled(false), close_connection(close_connection)
	{}
};

} // namespace detail
} // namespace fast_cgi

#endif