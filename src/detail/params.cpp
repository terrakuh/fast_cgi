#include "fast_cgi/detail/params.hpp"

#include "fast_cgi/exception/io_error.hpp"
#include "fast_cgi/log.hpp"

namespace fast_cgi {
namespace detail {

params::map_type::const_iterator params::begin() const noexcept
{
	return _parameters.begin();
}

params::map_type::const_iterator params::end() const noexcept
{
	return _parameters.end();
}

bool params::has(const std::string& key) const
{
	return _parameters.find(key) != _parameters.end();
}

const std::string& params::operator[](const std::string& key) const
{
	return _parameters.at(key);
}

void params::_read_parameters(io::reader& reader)
{
	try {
		while (true) {
			const auto pair = detail::name_value_pair::read(reader);
			std::string name;
			std::string value;

			name.resize(pair.name_length);
			reader.read(&*name.begin(), pair.name_length);

			value.resize(pair.value_length);
			reader.read(&*value.begin(), pair.value_length);

			FAST_CGI_LOG(DEBUG, "read parameter: {}={}", name, value);

			_parameters[std::move(name)].swap(value);
		}
	} catch (const exception::io_error& e) {
	}
}

} // namespace detail
} // namespace fast_cgi