#ifndef FAST_CGI_SERVICE_HPP_
#define FAST_CGI_SERVICE_HPP_

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
	/**
	 * Sets the given type as the default role handler for this service.
	 *
	 * @tparam Role the role type; must implement only one of: responder, authorizer or filter; this type must be
	 * default constructible
	 */
	template<typename Role>
	void set_role() noexcept
	{
		// filter derives from responder
		static_assert(static_cast<int>(std::is_base_of<responder, Role>::value) +
		                      static_cast<int>(std::is_base_of<authorizer, Role>::value) ==
		                  1,
		              "the given role must be implement only one of: responder, authorizer or filter");

		const auto factory = [] { return std::unique_ptr<role>(new Role{}); };

		if (std::is_base_of<filter, Role>::value) {
			_role_factories[2] = factory;
		} else if (std::is_base_of<authorizer, Role>::value) {
			_role_factories[1] = factory;
		} else {
			_role_factories[0] = factory;
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

#endif