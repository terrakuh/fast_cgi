#pragma once

#include "detail/record.hpp"
#include "reader.hpp"

#include <map>
#include <string>
#include <utility>

namespace fast_cgi {

class params
{
public:
	constexpr static auto REQUEST_URI	= "REQUEST_URI";
	constexpr static auto QUERY_STRING   = "QUERY_STRING";
	constexpr static auto CONTENT_LENGTH = "CONTENT_LENGTH";
	constexpr static auto CONTENT_TYPE   = "CONTENT_TYPE";

	const std::string& operator[](const std::string& key) const
	{
		auto r = parameters.find(key);

		if (r == parameters.end()) {
		}

		return r->second;
	}

private:
	friend class request_manager;

	std::map<std::string, std::string> parameters;

	void _read_parameters(reader& reader)
	{
		while (true) {
			auto pair = detail::name_value_pair::read(reader);
			std::string name;

			name.resize(header.name_length);

			reader.read(name.data(), header.name_length);

			auto& value = parameters[std::move(name)];

			value.resize(header.value_length);

			reader.read(value.data(), header.value_length);
		}
	}
};

} // namespace fast_cgi