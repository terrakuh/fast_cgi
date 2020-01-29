cmake_minimum_required(VERSION 3.1)

project("fast_cgi"
	LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(FAST_CGI_LOGGER_NAME "fastcgi-default" CACHE STRING "FastCGI default logger name. Only used if logging is enabled.")
set(FAST_CGI_LOGGER_PATTERN "[%T.%e | %=5t | %-20s at %-3# (%-20!)] [%n: %^%8l%$]\\n\\t%v" CACHE STRING "FastCGI logging pattern. Only used if logging is enabled.")
option(FAST_CGI_BUILD_EXAMPLES "Build examples." ON)
option(FAST_CGI_ENABLE_LOGGING "Enable logging." ON)
option(FAST_CGI_BUILD_TESTS "Build tests." ON)

find_package(Threads REQUIRED)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/log.hpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/fast_cgi/log.hpp")

# set build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type." FORCE)
endif()

# add library
file(GLOB_RECURSE FAST_CGI_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(fast_cgi ${FAST_CGI_SOURCE_FILES})
target_include_directories(fast_cgi
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>)
target_link_libraries(fast_cgi
	PRIVATE Threads::Threads)

if(FAST_CGI_ENABLE_LOGGING)
	find_package(spdlog QUIET)

	if(spdlog_FOUND)
		message(STATUS "Found spdlog at ${spdlog_DIR}")

		target_link_libraries(fast_cgi
			PRIVATE spdlog::spdlog)
	else()
		message(STATUS "spdlog not found. Using header-only version from ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/spdlog/include.")

		target_include_directories(fast_cgi
			PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/spdlog/include")
	endif()
endif()

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

install(TARGETS fast_cgi
	EXPORT fast_cgi
	ARCHIVE
		DESTINATION "lib"
	LIBRARY
		DESTINATION "lib")
install(DIRECTORY "fast_cgi"
	DESTINATION "include")
install(EXPORT fast_cgi
	DESTINATION "lib/cmake/fast_cgi"
	FILE "fast_cgi-config.cmake"
	NAMESPACE fast_cgi::)
