#pragma once

#include <cstdint>

namespace fast_cgi
{

class handler
{
public:
	virtual ~handler() = default;
	virtual std::int32_t handle_request() = 0;
};

}