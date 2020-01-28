#ifndef FAST_CGI_EXCEPTION_INTERRUPTED_ERROR_HPP_
#define FAST_CGI_EXCEPTION_INTERRUPTED_ERROR_HPP_

#include "io_error.hpp"

namespace fast_cgi {
namespace exception {

class interrupted_error : public io_error
{
public:
	using io_error::io_error;
};

} // namespace exception
} // namespace fast_cgi

#endif