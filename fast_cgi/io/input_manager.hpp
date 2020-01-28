#ifndef FAST_CGI_IO_INPUT_MANAGER_HPP_
#define FAST_CGI_IO_INPUT_MANAGER_HPP_

#include "../connection.hpp"
#include "../memory/allocator.hpp"
#include "../memory/buffer.hpp"
#include "reader.hpp"

#include <memory>

namespace fast_cgi {
namespace io {

class input_manager
{
public:
	static std::shared_ptr<reader> launch_input_manager(std::shared_ptr<connection> connection,
	                                                    std::shared_ptr<memory::allocator> allocator);

private:
	std::shared_ptr<memory::buffer> _buffer;
	std::shared_ptr<connection> _connection;

	input_manager(std::shared_ptr<connection> connection, std::shared_ptr<memory::allocator> allocator);
	static void _run(std::shared_ptr<input_manager> self);
};

} // namespace io
} // namespace fast_cgi

#endif