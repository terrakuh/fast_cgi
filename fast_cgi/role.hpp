#ifndef FASST_CGI_ROLE_HPP_
#define FASST_CGI_ROLE_HPP_

#include "detail/config.hpp"
#include "detail/params.hpp"
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

	virtual ~role() = default;
	/**
	 * Executes the logic of this role.
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
	 * Returns the associated parameters given by the web server.
	 *
	 * @note calling this function outside of the fast_cgi context results in undefined behavior
	 *
	 * @returns a reference to the param container
	 */
	detail::params& params() noexcept
	{
		return *_params;
	}
	/**
	 * Returns the default output stream.
	 *
	 * @note calling this function outside of the fast_cgi context results in undefined behavior
	 *
	 * @returns a reference to the stdout stream
	 */
	io::byte_ostream& output() noexcept
	{
		return *_output_stream;
	}
	/**
	 * Returns the default error output stream.
	 *
	 * @note calling this function outside of the fast_cgi context results in undefined behavior
	 *
	 * @returns a reference to the stderr stream
	 */
	io::byte_ostream& error() noexcept
	{
		return *_error_stream;
	}

private:
	/** the request manager sets the required fields */
	friend detail::request_manager;

	std::atomic_bool* _cancelled     = nullptr;
	detail::params* _params          = nullptr;
	io::byte_ostream* _output_stream = nullptr;
	io::byte_ostream* _error_stream  = nullptr;
};

class responder : public virtual role
{
public:
	/**
	 * Returns the default input stream.
	 *
	 * @note calling this function outside of the fast_cgi context results in undefined behavior
	 *
	 * @returns a reference to the stdin stream
	 */
	io::byte_istream& input() noexcept
	{
		return *_input_stream;
	}

private:
	/** sets the required input stream */
	friend detail::request_manager;

	io::byte_istream* _input_stream = nullptr;
};

class authorizer : public virtual role
{};

class filter : public virtual responder
{
public:
	/**
	 * Returns the data stream.
	 *
	 * @note calling this function outside of the fast_cgi context results in undefined behavior
	 *
	 * @returns a reference to the data stream
	 */
	io::byte_istream& data() noexcept
	{
		return *_data_stream;
	}

private:
	/** sets the required data stream */
	friend detail::request_manager;

	io::byte_istream* _data_stream = nullptr;
};

} // namespace fast_cgi

#endif