#pragma once

#include "../output_manager.hpp"
#include "../reader.hpp"
#include "config.hpp"

#include <functional>
#include <ostream>

namespace fast_cgi {
namespace detail {

enum VERSION : single_type
{
	FCGI_VERSION_1 = 1
};

enum TYPE : single_type
{
	FCGI_BEGIN_REQUEST	 = 1,
	FCGI_ABORT_REQUEST	 = 2,
	FCGI_END_REQUEST	   = 3,
	FCGI_PARAMS			   = 4,
	FCGI_STDIN			   = 5,
	FCGI_STDOUT			   = 6,
	FCGI_STDERR			   = 7,
	FCGI_DATA			   = 8,
	FCGI_GET_VALUES		   = 9,
	FCGI_GET_VALUES_RESULT = 10,
	FCGI_UNKNOWN_TYPE	  = 11
};

enum ROLE : double_type
{
	FCGI_RESPONDER  = 1,
	FCGI_AUTHORIZER = 2,
	FCGI_FILTER		= 3
};

enum FLAGS : single_type
{
	FCGI_KEEP_CONN = 1
};

enum PROTOCOL_STATUS : single_type
{
	FCGI_REQUEST_COMPLETE,
	FCGI_CANT_MPX_CONN,
	FCGI_OVERLOADED,
	FCGI_UNKNOWN_ROLE
};

struct name_value_pair
{
	typedef const std::pair<std::string, std::string> generated_type;

	const quadruple_type name_length;
	const quadruple_type value_length;

	static name_value_pair read(reader& reader)
	{
		auto name_length  = reader.read_variable();
		auto value_length = reader.read_variable();

		return { name_length, value_length };
	}
	static name_value_pair from_generator(std::function<generated_type> generator)
	{
		// caclulate size
		name_value_pair pair{};

		pair._generator = generator;

		while (true) {
			auto value = generator();
			auto s	 = value.first.size() + value.second.size();

			if (s == 0) {
				break;
			}

			pair._size += s;
		}

		return pair;
	}
	void write(writer& writer)
	{
		while (true) {
			auto value = generator();

			if (value.first.empty() && value.second.empty()) {
				break;
			}

			writer.write_variable(value.first.size());
			writer.write_variable(value.second.size());
			writer.write(value.first.data(), value.first.size());
			writer.write(value.second.data(), value.second.size());
		}
	}
	double_type size() noexcept
	{
		return (name_length > 127 ? 4 : 1) + (value_length > 127 ? 4 : 1);
	}

private:
	double_type _size;
	std::function<generated_type> _generator;
};

struct unknown_type
{
	const TYPE type;
	// 7 bytes padding

	void write(writer& writer)
	{
		writer.write_all(unknown_type, single_type(), single_type(), single_type(), single_type(), single_type(),
						 single_type(), single_type());
	}
	constexpr static double_type size() noexcept
	{
		return 8;
	}
	constexpr static TYPE type() noexcept
	{
		return TYPE::FCGI_UNKNOWN_TYPE;
	}
};

struct end_request
{
	const quadruple_type app_status;
	const PROTOCOL_STATUS protocol_status;
	// 3 bytes reserved

	void write(writer& writer)
	{
		writer.write_all(app_status, protocol_status, single_type(), single_type(), single_type());
	}
	constexpr static double_type size() noexcept
	{
		return 8;
	}
	constexpr static TYPE type() noexcept
	{
		return TYPE::FCGI_END_REQUEST;
	}
};

struct begin_request
{
	const ROLE role;
	const single_type flags;
	// 5 bytes padding

	static begin_request read(reader& reader)
	{
		reader.require_size(8);
		auto role  = reader.read<ROLE>();
		auto flags = reader.read<single_type>();
		reader.skip(5);

		return { role, flags };
	}
	void write(writer& writer)
	{
		writer.write_all(role, flags, single_type(), single_type(), single_type(), single_type(), single_type());
	}
	constexpr static double_type size() noexcept
	{
		return 8;
	}
	constexpr static TYPE type() noexcept
	{
		return TYPE::FCGI_BEGIN_REQUEST;
	}
};

struct stdout_stream
{
	const void* const content;
	const double_type content_size;

	void write(writer& writer)
	{
		writer.write(content, content_size);
	}
	double_type size() noexcept
	{
		return content_size;
	}
	constexpr static TYPE type() noexcept
	{
		return TYPE::FCGI_STDOUT;
	}
};

struct stderr_stream
{
	const void* const content;
	const double_type content_size;

	void write(writer& writer)
	{
		writer.write(content, content_size);
	}
	double_type size() noexcept
	{
		return content_size;
	}
	constexpr static TYPE type() noexcept
	{
		return TYPE::FCGI_STDERR;
	}
};

struct record
{
	constexpr static auto default_padding_boundary = 8;
	const VERSION version;
	const TYPE type;
	const double_type request_id;
	const double_type content_length;
	const single_type padding_length;
	// 1 byte padding

	record(VERSION version, double_type request_id)
		: version(version), type(type), request_id(request_id), content_length(content_length)
	{}
	record(const record& copy) = default;
	static record read(reader& reader)
	{
		VERSION version;
		TYPE type;
		double_type request_id;
		double_type content_length;
		single_type padding_length;

		reader.require_size(sizeof(VERSION) + sizeof(TYPE) + 2 * sizeof(double_type) + 2 * sizeof(single_type));
		reader.read(version);
		reader.read(type);
		reader.read(request_id);
		reader.read(content_length);
		reader.read(padding_length);
		reader.skip(1);
	}
	template<typename T>
	void write(output_manager& output_manager, const T& data)
	{
		output_manager.add([unknown_type](writer& writer) {
			double_type size	= data.size();
			single_type padding = size % default_padding_boundary;

			if (padding) {
				padding = default_padding_boundary - padding;
			}

			// write header
			writer.write_all(version, T::type(), request_id, size, padding, single_type());

			// write data
			data.write(writer);

			// write padding
			if (padding) {
				std::uint8_t buf[default_padding_boundary]{};

				writer.write(buf, padding);
			}
		});
	}
};

} // namespace detail
} // namespace fast_cgi