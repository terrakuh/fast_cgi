#pragma once

#include "detail/record.hpp"

#include <functional>

namespace fast_cgi {

class task_distributor
{
public:
	virtual ~task_distributor()							= default;
	virtual bool distribute(detail::ROLE role, std::function<void()> task) = 0;
};

} // namespace fast_cgi