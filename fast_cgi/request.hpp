#pragma once

#include "buffer.hpp"
#include "detail/record.hpp"
#include "io/output_manager.hpp"
#include "params.hpp"

#include <atomic>
#include <memory>
#include <thread>

namespace fast_cgi {

struct request
{
    const detail::double_type id;
    const detail::ROLE role_type;
    std::thread handler_thread;
    std::atomic_bool finished;
    class params params;
    std::shared_ptr<buffer> params_buffer;
    std::shared_ptr<buffer> input_buffer;
    std::shared_ptr<buffer> data_buffer;
    std::shared_ptr<io::output_manager> output_manager;
    std::atomic_bool cancelled;
    const bool close_connection;

    request(detail::double_type id, detail::ROLE role_type, const std::shared_ptr<io::output_manager>& output_manager,
            bool close_connection, const std::shared_ptr<allocator>& allocator)
        : id(id), role_type(role_type), finished(false), params_buffer(new buffer(allocator, 99999)),
          input_buffer(new buffer(allocator, 99999)), data_buffer(new buffer(allocator, 99999)),
          output_manager(output_manager), cancelled(false), close_connection(close_connection)
    {}
};

} // namespace fast_cgi