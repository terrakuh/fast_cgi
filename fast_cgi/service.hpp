#pragma once

#include "connector.hpp"
#include "detail/record.hpp"
#include "io/input_manager.hpp"
#include "io/reader.hpp"
#include "memory/allocator.hpp"
#include "role.hpp"

#include <array>
#include <functional>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace fast_cgi {

class service
{
public:
	service(std::shared_ptr<connector> connector, std::shared_ptr<memory::allocator> allocator);
	template<typename T>
	typename std::enable_if<std::is_base_of<role, T>::value>::type set_role()
	{
		if (std::is_base_of<responder, T>::value) {
			_role_factories[0] = [] { return std::unique_ptr<role>(new T()); };
		}
	}
	void run();
	void join();

private:
	detail::VERSION _version;
	std::shared_ptr<connector> _connector;
	std::vector<std::thread> _connections;
	std::shared_ptr<memory::allocator> _allocator;
	std::array<std::function<std::unique_ptr<role>()>, 3> _role_factories;

	void _connection_thread(std::shared_ptr<connection> connection);
	void _input_handler(std::shared_ptr<io::reader> reader, std::shared_ptr<io::output_manager> output_manager);
	void _get_values(io::reader& reader, io::output_manager& output_manager, detail::record record);
};

} // namespace fast_cgi