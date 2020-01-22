#pragma once

#include <exception>
#include <string>

namespace fast_cgi {
namespace exception {

class fastcgi_exception : public std::exception
{
public:
	fastcgi_exception(const std::string& msg) : _msg(msg)
	{}
	fastcgi_exception(std::string&& msg) : _msg(std::move(msg))
	{}

	virtual const char* what() const noexcept override
	{
		return _msg.c_str();
	}

private:
	std::string _msg;
};

} // namespace exception
} // namespace fast_cgi