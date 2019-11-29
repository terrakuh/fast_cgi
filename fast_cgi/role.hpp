#pragma once

#include "byte_stream.hpp"
#include "detail/config.hpp"
#include "exception/invalid_role_exception.hpp"
#include "params.hpp"

#include <memory>

namespace fast_cgi {

class role
{
public:
    typedef detail::quadruple_type status_code_type;

    role() noexcept
    {
        _cancelled     = nullptr;
        _output_stream = nullptr;
        _error_stream  = nullptr;
    }
    virtual ~role() = default;
    /**
      Executes this role
     */
    virtual status_code_type run() = 0;
    volatile bool is_cancelled() const noexcept
    {
        return *_cancelled;
    }
    /**
      Returns the associated parameters given by the web server.

      @returns a reference to the param container
     */
    class params& params() noexcept
    {
        return *_params;
    }
    byte_ostream& output() noexcept
    {
        return *_output_stream;
    }
    byte_ostream& error() noexcept
    {
        return *_error_stream;
    }

private:
    friend class request_manager;

    volatile bool* _cancelled;
    class params* _params;
    byte_ostream* _output_stream;
    byte_ostream* _error_stream;
};

class responder : public role
{
public:
    responder() noexcept
    {
        _input_stream = nullptr;
    }
    byte_istream& input()
    {
        if (!_input_stream) {
            throw exception::invalid_role_exception("this role wasn't initialized as responder or filter");
        }

        return *_input_stream;
    }

private:
    friend class request_manager;

    byte_istream* _input_stream;
};

class authorizer : public role
{};

class filter : public responder
{
public:
    filter() noexcept
    {
        _data_stream = nullptr;
    }
    byte_istream& data()
    {
        if (!_data_stream) {
            throw exception::invalid_role_exception("this role wasn't initialized as filter");
        }

        return *_data_stream;
    }

private:
    friend class request_manager;

    byte_istream* _data_stream;
};

} // namespace fast_cgi