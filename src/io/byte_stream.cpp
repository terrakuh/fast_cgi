#include "fast_cgi/io/byte_stream.hpp"

#include <cstring>

namespace fast_cgi {
namespace io {

input_streambuf::input_streambuf(std::shared_ptr<memory::buffer> buffer) : _buffer(std::move(buffer))
{}

input_streambuf::int_type input_streambuf::underflow()
{
	if (!_buffer) {
		return traits_type::eof();
	}

	// wait for input
	auto input = _buffer->wait_for_input();

	// reached end
	if (input.second == 0) {
		_buffer.reset();

		return traits_type::eof();
	}

	auto ptr = static_cast<byte_type*>(input.first);

	setg(ptr, ptr, ptr + input.second);

	return traits_type::to_int_type(*ptr);
}

output_streambuf::output_streambuf(writer_type writer) : _writer(writer)
{}

std::streamsize output_streambuf::xsputn(const char_type* s, std::streamsize count)
{
	const auto initial_count = count;

	while (count) {
		if (epptr() == pptr()) {
			sync();

			if (epptr() == pptr()) {
				break;
			}
		}

		auto size = std::min(count, static_cast<std::streamsize>(epptr() - pptr()));

		std::memcpy(pptr(), s, size);
		pbump(size);

		s += size;
		count -= size;
	}

	return initial_count - count;
}

int output_streambuf::sync()
{
	if (!pptr() || pptr() > pbase()) {
		auto _buffer = _writer(pbase(), static_cast<std::size_t>(pptr() - pbase()));

		setp(static_cast<byte_type*>(_buffer.first), static_cast<byte_type*>(_buffer.first) + _buffer.second);
	}

	return 0;
}

output_streambuf::int_type output_streambuf::overflow(int_type c)
{
	// write
	sync();

	// buffer is full
	if (pptr() == epptr()) {
		return traits_type::eof();
	} else if (c != traits_type::eof()) {
		sputc(traits_type::to_char_type(c));
	}

	return traits_type::to_int_type(0);
}

} // namespace io
} // namespace fast_cgi