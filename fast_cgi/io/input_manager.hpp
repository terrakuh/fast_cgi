#pragma once

#include "../allocator.hpp"
#include "../buffer.hpp"
#include "../connection.hpp"
#include "../log.hpp"
#include "reader.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace fast_cgi {
namespace io {

class input_manager
{
public:
	~input_manager()
	{}
	static std::shared_ptr<reader> launch_input_manager(const std::shared_ptr<connection>& connection,
	                                                    const std::shared_ptr<allocator>& allocator)
	{
		std::shared_ptr<input_manager> im(new input_manager(connection, allocator));
		auto r = std::make_shared<reader>(im->_buffer);

		// launch reader
		std::thread(&input_manager::_run, std::move(im)).detach();

		return r;
	}

private:
	std::shared_ptr<buffer> _buffer;
	std::shared_ptr<connection> _connection;

	input_manager(const std::shared_ptr<connection>& connection, const std::shared_ptr<allocator>& allocator)
	    : _buffer(new buffer(allocator, 999999)), _connection(connection)
	{}
	static void _run(std::shared_ptr<input_manager> self)
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
};

} // namespace io
} // namespace fast_cgi