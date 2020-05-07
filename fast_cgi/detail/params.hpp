#ifndef FAST_CGI_DETAIL_PARAMS_HPP_
#define FAST_CGI_DETAIL_PARAMS_HPP_

#include "../io/reader.hpp"
#include "record.hpp"

#include <map>
#include <string>
#include <utility>

namespace fast_cgi {
namespace detail {

class params
{
public:
	typedef std::map<std::string, std::string> map_type;

	constexpr static auto request_uri     = "REQUEST_URI";
	constexpr static auto query_string    = "QUERY_STRING";
	constexpr static auto content_length  = "CONTENT_LENGTH";
	constexpr static auto content_type    = "CONTENT_TYPE";
	constexpr static auto document_uri    = "DOCUMENT_URI";
	constexpr static auto document_root   = "DOCUMENT_ROOT";
	constexpr static auto remote_addr     = "REMOTE_ADDR";
	constexpr static auto remote_port     = "REMOTE_PORT";
	constexpr static auto script_filename = "SCRIPTE_FILENAME";
	constexpr static auto http_host       = "HTTP_HOST";
	constexpr static auto request_method  = "REQUEST_METHOD";

	map_type::const_iterator begin() const noexcept;
	map_type::const_iterator end() const noexcept;
	bool has(const std::string& key) const;
	const std::string& operator[](const std::string& key) const;

private:
	friend class request_manager;

	map_type _parameters;

	void _read_parameters(io::reader& reader);
};

} // namespace detail
} // namespace fast_cgi

#endif