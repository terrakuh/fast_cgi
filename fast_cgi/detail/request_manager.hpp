#ifndef FAST_CGI_DETAIL_REQUEST_MANAGER_HPP_
#define FAST_CGI_DETAIL_REQUEST_MANAGER_HPP_

#include "../io/output_manager.hpp"
#include "../io/reader.hpp"
#include "../memory/allocator.hpp"
#include "../role.hpp"
#include "record.hpp"
#include "request.hpp"

#include <array>
#include <atomic>
#include <map>
#include <memory>

namespace fast_cgi {
namespace detail {

class request_manager
{
public:
	typedef double_type id_type;

	/**
	 * Constructor.
	 *
	 * @param allocator the memory allocator
	 * @param reader the input source for this specific request
	 * @param role_factories the role factories for a responder, filter and/or authorizer
	 */
	request_manager(std::shared_ptr<memory::allocator> allocator, std::shared_ptr<io::reader> reader,
	                std::array<std::function<std::unique_ptr<role>()>, 3> role_factories);
	~request_manager();
	/**
	 * Checks whether this request is finished and if the connection can be savely closed.
	 *
	 * @returns `true` if the connection can be closed, otherwise `false`
	 */
	bool should_terminate_connection() const noexcept;
	/**
	 *
	 * @returns `true` if the request has been handled, otherwise `false`
	 */
	bool handle_request(std::shared_ptr<io::output_manager> output_manager, struct record record);

private:
	typedef std::map<id_type, std::shared_ptr<request>> requests_type;

	/** if set, the connection is queued to be terminated; no more request will be accepted */
	std::atomic_bool _terminate_connection;
	/** a map of requests */
	requests_type _requests;
	/** the memory allocator */
	std::shared_ptr<memory::allocator> _allocator;
	/** the connection input */
	std::shared_ptr<io::reader> _reader;
	/** the 3 main roles for FastCGI */
	std::array<std::function<std::unique_ptr<role>()>, 3> _role_factories;

	/**
	 * Forwards `length` bytes to `buffer` read by `reader`. If `length` is zero, the buffer is closed by specification.
	 *
	 * @param length the length of the forward content
	 * @param[in] buffer the buffer
	 */
	void _forward_to_buffer(detail::double_type length, memory::buffer& buffer);
	void _request_handler(std::unique_ptr<role> role, std::shared_ptr<request> request);
	/**
	 * Starts a new request handler if the role is implemented.
	 *
	 * @param output_manager the output of the connection
	 * @param record the record that triggered the request creation
	 */
	void _begin_request(std::shared_ptr<io::output_manager> output_manager, struct record record);
};

} // namespace detail
} // namespace fast_cgi

#endif