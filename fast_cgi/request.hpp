#pragma once

#include "buffer.hpp"
#include "detail/record.hpp"
#include "output_manager.hpp"
#include "params.hpp"

#include <memory>
#include <thread>

namespace fast_cgi {

struct request
{
    const detail::double_type id;
    const detail::ROLE role_type;
    std::thread handler_thread;
    params params;
    std::shared_ptr<buffer> params_buffer;
    std::shared_ptr<buffer> input_buffer;
    std::shared_ptr<buffer> data_buffer;
    std::shared_ptr<output_manager> output_manager;
    volatile bool cancelled;
    const bool close_connection;

    request(detail::double_type id, detail::ROLE role_type, const std::shared_ptr<class output_manager>& output_manager,
            bool close_connection)
        : id(id), role_type(role_type), params_buffer(nullptr, 0), output_manager(output_manager),
          close_connection(close_connection)
    {
        cancelled = false;
    }
};

} // namespace fast_cgi