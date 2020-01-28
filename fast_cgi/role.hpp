#pragma once

#include "detail/config.hpp"
#include "detail/params.hpp"
#include "exception/invalid_role_exception.hpp"
#include "io/byte_stream.hpp"

#include <atomic>
#include <memory>
#include <type_traits>

namespace fast_cgi {
namespace detail {

class request_manager;

}

class role
{
public:
	typedef typename std::make_signed<detail::quadruple_type>::type status_code_type;

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
	bool is_cancelled() const volatile noexcept
	{
		return _cancelled->load(std::memory_order_acquire);
	}
	const std::string& params(const std::string& key) const
	{
		return (*_params)[key];
	}
	/**
	  Returns the associated parameters given by the web server.

	  @returns a reference to the param container
	 */
	detail::params& params() noexcept
	{
		return *_params;
	}
	io::byte_ostream& output() noexcept
	{
		return *_output_stream;
	}
	io::byte_ostream& error() noexcept
	{
		return *_error_stream;
	}

private:
	friend detail::request_manager;

	std::atomic_bool* _cancelled;
	detail::params* _params;
	io::byte_ostream* _output_stream;
	io::byte_ostream* _error_stream;
};

class responder : public virtual role
{
public:
	responder() noexcept
	{
		_input_stream = nullptr;
	}
	io::byte_istream& input()
	{
		if (!_input_stream) {
			throw exception::invalid_role_exception("this role wasn't initialized as responder or filter");
		}

		return *_input_stream;
	}

private:
	friend detail::request_manager;

	io::byte_istream* _input_stream;
};

class authorizer : public virtual role
{};

class filter : public virtual responder
{
public:
	filter() noexcept
	{
		_data_stream = nullptr;
	}
	io::byte_istream& data()
	{
		if (!_data_stream) {
			throw exception::invalid_role_exception("this role wasn't initialized as filter");
		}

		return *_data_stream;
	}

private:
	friend detail::request_manager;

	io::byte_istream* _data_stream;
};

} // namespace fast_cgi