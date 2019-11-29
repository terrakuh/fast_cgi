#pragma once

#include "fastcgi_exception.hpp"

namespace fast_cgi {
namespace exception {

class io_exception : public fastcgi_exception
{
public:
    using fastcgi_exception::fastcgi_exception;
};

} // namespace exception
} // namespace fast_cgi