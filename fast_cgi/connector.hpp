#ifndef FAST_CGI_CONNECTOR_HPP_
#define FAST_CGI_CONNECTOR_HPP_

#include "connection.hpp"

#include <functional>
#include <memory>

namespace fast_cgi {

class connector
{
public:
	typedef std::function<void(std::shared_ptr<connection>)> acceptor_type;

	virtual ~connector() = default;
	/**
	 Accepts incoming connections.

	 @acceptor the callback that takes one connection
	 @throws may throw anything
	*/
	virtual void run(const acceptor_type& acceptor) = 0;
};

} // namespace fast_cgi

#endif