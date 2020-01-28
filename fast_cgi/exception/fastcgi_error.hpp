#ifndef FAST_CGI_EXCEPTION_FASTCGI_ERROR_HPP_
#define FAST_CGI_EXCEPTION_FASTCGI_ERROR_HPP_

#include <stdexcept>

namespace fast_cgi {
namespace exception {

class fastcgi_error : public std::runtime_error
{
public:
	using runtime_error::runtime_error;
};

} // namespace exception
} // namespace fast_cgi

#endif