#pragma once

#include "connection.hpp"

#include <memory>

namespace fast_cgi {

class connector
{
public:
    virtual ~connector() = default;
    virtual std::shared_ptr<connection> accept() = 0;

};

} // namespace fast_cgi