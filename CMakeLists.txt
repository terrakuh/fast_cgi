cmake_minimum_required(VERSION 3.1)

project("fast_cgi"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FAST_CGI_LOGGER_NAME "fastcgi-default" CACHE STRING "FastCGI default logger name. Only used if logging is enabled.")
option(FAST_CGI_BUILD_EXAMPLES "Build examples." ON)
option(FAST_CGI_ENABLE_LOGGING "Enable logging." ON)
option(FAST_CGI_BUILD_TESTS "Build tests." ON)

find_package(Threads REQUIRED)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/log.hpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/fast_cgi/log.hpp")

add_library("fast_cgi" INTERFACE)
target_include_directories("fast_cgi"
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/spdlog/include"
	INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/Catch2/include")
target_link_libraries("fast_cgi"
	INTERFACE Threads::Threads)

if(FAST_CGI_BUILD_EXAMPLES)
	file(GLOB FAST_CGI_EXAMPLE_SOURCES
		"${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp")

	foreach(FAST_CGI_EXAMPLE IN ITEMS ${FAST_CGI_EXAMPLE_SOURCES})
		get_filename_component(FAST_CGI_EXAMPLE_NAME ${FAST_CGI_EXAMPLE} NAME_WE)

		add_executable(${FAST_CGI_EXAMPLE_NAME} ${FAST_CGI_EXAMPLE})
		target_link_libraries(${FAST_CGI_EXAMPLE_NAME}
			PUBLIC "fast_cgi")
	endforeach()
endif()

if(FAST_CGI_BUILD_TESTS)
	include(CTest)
	enable_testing()

endif()
