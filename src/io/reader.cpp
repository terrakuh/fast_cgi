#include "fast_cgi/io/reader.hpp"
#include "fast_cgi/log.hpp"

#include <algorithm>
#include <cstring>

namespace fast_cgi {
namespace io {

reader::reader(std::shared_ptr<memory::buffer> buffer) : _buffer(std::move(buffer))
{
	_begin = nullptr;
	_end   = nullptr;
}

void reader::interrupt()
{
	_buffer->interrupt_all_waiting();
}

detail::quadruple_type reader::read_variable()
{
	std::uint8_t buffer[sizeof(detail::quadruple_type)];
	detail::quadruple_type value = 0;

	if (read(buffer, 1) != 1) {
		throw exception::io_error("buffer exhausted");
	}

	value = buffer[0];

	if (value & 0x80) {
		// read more
		if (read(buffer + 1, sizeof(buffer) - 1) != sizeof(buffer) - 1) {
			throw exception::io_error("buffer exhausted");
		}

		value = ((value & 0x7f) << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
	}

	return value;
}

std::size_t reader::read(void* buffer, std::size_t size)
{
	const auto initial_size = size;
	auto ptr                = static_cast<std::int8_t*>(buffer);

	// need more
	while (size > 0) {
		if (_begin >= _end) {
			FAST_CGI_LOG(TRACE, "waiting for input");
			auto buf = _buffer->wait_for_input();
			FAST_CGI_LOG(TRACE, "got input {}", buf.second);

			// buffer is empty
			if (!buf.second) {
				break;
			}

			_begin = static_cast<std::int8_t*>(buf.first);
			_end   = _begin + buf.second;
		}

		auto s = std::min(size, static_cast<std::size_t>(_end - _begin));

		std::memcpy(ptr, _begin, s);

		_begin += s;
		ptr += s;
		size -= s;
	}

	return initial_size - size;
}

std::size_t reader::skip(std::size_t size)
{
	const auto initial_size = size;

	// need more
	while (size > 0) {
		if (_begin >= _end) {
			auto buf = _buffer->wait_for_input();

			// buffer is empty
			if (!buf.second) {
				break;
			}

			_begin = static_cast<std::int8_t*>(buf.first);
			_end   = _begin + buf.second;
		}

		auto s = std::min(size, static_cast<std::size_t>(_end - _begin));

		_begin += s;
		size -= s;
	}

	return initial_size - size;
}

} // namespace io
} // namespace fast_cgi