#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "detail/record.hpp"
#include "output_manager.hpp"

#include <memory>
#include <thread>

namespace fast_cgi {

struct request
{
	detail::double_type id;
	detail::ROLE role_type;
	std::thread handler;
	buffer input_buffer;
	buffer data_buffer;
	output_manager& output_manager;
	bool cancelled;
};

} // namespace fast_cgi