#ifndef FAST_CGI_EXCEPTION_IO_ERROR_HPP_
#define FAST_CGI_EXCEPTION_IO_ERROR_HPP_

#include "fastcgi_error.hpp"

namespace fast_cgi {
namespace exception {

class io_error : public fastcgi_error
{
public:
	using fastcgi_error::fastcgi_error;
};

} // namespace exception
} // namespace fast_cgi

#endif