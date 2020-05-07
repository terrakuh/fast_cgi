#ifndef FAST_CGI_DETAIL_REQUEST_HPP_
#define FAST_CGI_DETAIL_REQUEST_HPP_

#include "../io/output_manager.hpp"
#include "../memory/buffer.hpp"
#include "config.hpp"
#include "params.hpp"
#include "record.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace fast_cgi {
namespace detail {

struct request
{
	/** id of the request */
	const detail::double_type id;
	/** request role */
	const detail::ROLE role_type;
	/** the thread where this request is being handled */
	std::thread handler_thread;
	/** if `true`, this request has successfully finished */
	std::atomic_bool finished;
	/** the associated parameters */
	class params params;
	std::shared_ptr<memory::buffer> params_buffer;
	std::shared_ptr<memory::buffer> input_buffer;
	std::shared_ptr<memory::buffer> data_buffer;
	std::shared_ptr<io::output_manager> output_manager;
	/** if `true`, this request has been cancelled by the server; the handler should stop as soon as possible */
	std::atomic_bool cancelled;
	/** if `true`, the connection will be closed after completion of this request */
	const bool close_connection;

	request(detail::double_type id, detail::ROLE role_type, std::shared_ptr<io::output_manager> output_manager,
	        bool close_connection, std::shared_ptr<memory::allocator> allocator)
	    : id{ id }, role_type{ role_type }, finished{ false }, output_manager{ std::move(output_manager) },
	      params_buffer{ new memory::buffer(allocator, default_buffer_size) }, input_buffer{ new memory::buffer(
		                                                                           allocator, default_buffer_size) },
	      data_buffer{ new memory::buffer(allocator, default_buffer_size) }, cancelled{ false }, close_connection{
		      close_connection
	      }
	{}
};

} // namespace detail
} // namespace fast_cgi

#endif