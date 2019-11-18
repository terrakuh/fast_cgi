#pragma once

#include "../byte_stream.hpp"

namespace fast_cgi {
namespace manipulator {

inline byte_ostream& feed(byte_ostream& stream)
{
	return stream << "\r\n";
}

} // namespace manipulator
} // namespace fast_cgi