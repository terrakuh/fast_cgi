#include "fast_cgi/io/input_manager.hpp"
#include "fast_cgi/log.hpp"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <thread>

namespace fast_cgi {
namespace io {

std::shared_ptr<reader> input_manager::launch_input_manager(std::shared_ptr<connection> connection,
                                                            std::shared_ptr<memory::allocator> allocator)
{
	std::shared_ptr<input_manager> im(new input_manager(std::move(connection), std::move(allocator)));
	auto r = std::make_shared<reader>(im->_buffer);

	// launch reader
	std::thread(&input_manager::_run, std::move(im)).detach();

	return r;
}

input_manager::input_manager(std::shared_ptr<connection> connection, std::shared_ptr<memory::allocator> allocator)
    : _buffer(new memory::buffer(std::move(allocator), 999999)), _connection(std::move(connection))
{}

void input_manager::_run(std::shared_ptr<input_manager> self)
{
	std::uint8_t buffer[1024];

	while (true) {
		// spin
		while (!self->_connection->in_available()) {
			if (self->_buffer->interrupted()) {
				FAST_CGI_LOG(INFO, "buffer was interrupted; exiting input thread");

				return;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		// read from connection
		auto read = self->_connection->read(buffer, 1, sizeof(buffer));

		if (!read) {
			FAST_CGI_LOG(INFO, "received nothing; exiting input thread");

			break;
		}

		// forward to buffer
		auto token = self->_buffer->begin_writing();
		auto ptr   = buffer;

		while (read > 0) {
			auto buf = token.request_buffer(read);

			std::memcpy(buf.first, ptr, buf.second);

			read -= buf.second;
			ptr += buf.second;
		}
	}
}

} // namespace io
} // namespace fast_cgi