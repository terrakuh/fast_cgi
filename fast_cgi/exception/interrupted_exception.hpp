#pragma once

#include "io_exception.hpp"

namespace fast_cgi {
namespace exception {

class interrupted_exception : public io_exception
{
public:
    using io_exception::io_exception;
};

} // namespace exception
} // namespace fast_cgi