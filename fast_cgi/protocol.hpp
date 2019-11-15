#pragma once

#include "connection.hpp"
#include "detail/record.hpp"
#include "output_manager.hpp"
#include "reader.hpp"
#include "request_manager.hpp"
#include "writer.hpp"

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <string>

namespace fast_cgi {

class protocol
{
public:
	protocol(std::shared_ptr<connection> main_connection);

	void run(const std::string& server_addresses)
	{}
	static std::string get_server_addresses()
	{
		return std::getenv("FCGI_WEB_SERVER_ADDRS");
	}

private:
	detail::VERSION version;
	request_manager request_manager;

	void input_handler(std::shared_ptr<connection> transport_connection, output_manager& output_manager)
	{
		reader reader(transport_connection);

		while (true) {
			auto record = detail::record::read(reader);

			// version mismatch
			if (record.version != version) {
			}

			// process record
			switch (record.type) {
			case detail::TYPE::GET_VALUES: {
				get_values(reader, record);

				break;
			}
			case detail::TYPE::ABORT_REQUEST:
			case detail::TYPE::DATA:
			case detail::TYPE::PARAMS:
			case detail::TYPE::STDIN:
			case detail::TYPE::BEGIN_REQUEST: {
				request_manager.handle_request(reader, output_manager, record);

				break;
			}
			default: {
				if (record.request_id) {
					request_manager.handle_request(reader, output_manager, record);

					break;
				}

				// ignore body
				record.skip(reader);

				// tell the server that the record was ignored
				detail::record::write(output_manager, detail::unknown_type{ record.type });

				break;
			}
			}

			// skip padding
			reader.skip(record.padding_length);
		}
	}
	void get_values(reader& reader, detail::record& record)
	{
		constexpr auto max_conns  = "FCGI_MAX_CONNS";
		constexpr auto max_reqs   = "FCGI_MAX_REQS";
		constexpr auto mpxs_conns = "FCGI_MPXS_CONNS";
		constexpr auto max_size   = 15;
		std::string name;
		auto answer = std::make_shared<std::map<const char*, std::string>>();

		while (true) {
			auto pair = detail::name_value_pair::read(reader);

			// read name
			if (pair.name_length <= max_size) {
				name.resize(pair.name_length);
				reader.read(&name[0], pair.name_length);
			} else {
				name.clear();
			}

			// ignore value
			reader.skip(pair.value_length);

			if (name == max_conns) {
			} else if (name == max_reqs) {
			} else if (name == mpxs_conns) {
				answer[mpxs_conns] = "1";
			}
		}

		// answer
		auto begin = answer->begin();
		auto end   = answer->end();

		detail::record(version, 0).write(output_manager, detail::name_value_pair::from_generator([answer, begin, end]() mutable{
			if (begin != end) {
				auto t = *begin;

				++begin;

				return t;
			}

			return {};
		});
	}
};

} // namespace fast_cgi