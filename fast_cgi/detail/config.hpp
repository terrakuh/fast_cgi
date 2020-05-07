#ifndef FAST_CGI_DETAIL_CONFIG_HPP_
#define FAST_CGI_DETAIL_CONFIG_HPP_

#include <cstdint>

namespace fast_cgi {
namespace detail {

typedef std::uint8_t single_type;
typedef std::uint16_t double_type;
typedef std::uint32_t quadruple_type;

constexpr auto default_buffer_size = 1024;

} // namespace detail
} // namespace fast_cgi

#endif