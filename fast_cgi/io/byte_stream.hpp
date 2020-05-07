#ifndef FAST_CGI_IO_BUFFER_STREAM_HPP_
#define FAST_CGI_IO_BUFFER_STREAM_HPP_

#include "../memory/buffer.hpp"

#include <cstddef>
#include <functional>
#include <istream>
#include <memory>
#include <ostream>
#include <streambuf>

namespace fast_cgi {
namespace io {

typedef char byte_type;

typedef std::ostream byte_ostream;
typedef std::istream byte_istream;

class input_streambuf : public std::basic_streambuf<byte_type>
{
public:
	input_streambuf(std::shared_ptr<memory::buffer> buffer);

protected:
	int_type underflow() override;

private:
	std::shared_ptr<memory::buffer> _buffer;
};

class output_streambuf : public std::basic_streambuf<byte_type>
{
public:
	typedef std::function<std::pair<void*, std::size_t>(void*, std::size_t)> writer_type;

	output_streambuf(writer_type writer);

protected:
	std::streamsize xsputn(const char_type* s, std::streamsize count) override;
	int sync() override;
	int_type overflow(int_type c = traits_type::eof()) override;

private:
	writer_type _writer;
};

} // namespace io
} // namespace fast_cgi

#endif