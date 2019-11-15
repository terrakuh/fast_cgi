#pragma once

#include "byte_stream.hpp"
#include "detail/config.hpp"
#include "params.hpp"

#include <memory>

namespace fast_cgi {

class role
{
public:
	role() noexcept
	{
		_cancelled	 = nullptr;
		_output_stream = nullptr;
		_error_stream  = nullptr;
	}
	virtual ~role()						 = default;
	virtual detail::quadruple_type run() = 0;
	volatile bool is_cancelled() const noexcept
	{
		return *_cancelled;
	}
	fast_cgi::params& params() noexcept
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
	params* _params;
	byte_ostream* _output_stream;
	byte_ostream* _error_stream;
};

class responder : public role
{
public:
	responder() noexcept
	{
		input_stream = nullptr;
	}
	byte_istream& input() noexcept
	{
		return *input_stream;
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
	byte_istream& data() noexcept
	{
		return *_input_stream;
	}

private:
	friend class request_manager;

	byte_istream* _data_stream;
};

class role_factory
{
public:
	virtual ~role_factory()				   = default;
	virtual std::shared_ptr<role> create() = 0;
};

class my_responder : public responder
{
public:
	virtual void run() override
	{
		// get address
		auto call = calls.find(params()["REQUEST_URI"]);

		if (call != calls.end()) {
			(this->*call)();
		}
	}

private:
	typedef void (my_responder::*callback_type)();

	static std::map<std::string, callback_type> calls;

	void get_file()
	{
		// read file

		output() << std::ifstream("file", std::ios::in | std::ios::binary);
	}
};

} // namespace fast_cgi