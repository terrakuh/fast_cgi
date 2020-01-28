#ifndef FAST_CGI_MANIPULATOR_MANIPULATORS_HPP_
#define FAST_CGI_MANIPULATOR_MANIPULATORS_HPP_

#include "../io/byte_stream.hpp"

namespace fast_cgi {
namespace manipulator {

inline io::byte_ostream& feed(io::byte_ostream& stream)
{
	return stream << "\r\n";
}

} // namespace manipulator
} // namespace fast_cgi

#endif