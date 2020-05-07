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

	request_manager(std::shared_ptr<memory::allocator> allocator, std::shared_ptr<io::reader> reader,
	                std::array<std::function<std::unique_ptr<role>()>, 3> role_factories);
	~request_manager();
	bool should_terminate_connection() const;
	bool handle_request(std::shared_ptr<io::output_manager> output_manager, struct record record);

private:
	typedef std::map<id_type, std::shared_ptr<request>> requests_type;

	std::atomic_bool _terminate_connection;
	requests_type _requests;
	std::shared_ptr<memory::allocator> _allocator;
	std::shared_ptr<io::reader> _reader;
	std::array<std::function<std::unique_ptr<role>()>, 3> _role_factories;

	/**
	 * Forwards *length* bytes to *buffer* read by *reader*. If *length* is zero the buffer is closed.
	 *
	 * @param length the length of the forward content
	 * @param[in] buffer the buffer
	 */
	void _forward_to_buffer(detail::double_type length, memory::buffer& buffer);
	void _request_hanlder(std::unique_ptr<role> role, std::shared_ptr<request> request);
	void _begin_request(std::shared_ptr<io::output_manager> output_manager, struct record record);
};

} // namespace detail
} // namespace fast_cgi

#endif