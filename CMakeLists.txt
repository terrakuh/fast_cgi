cmake_minimum_required(VERSION 3.1)

project("fast_cgi"
	LANGUAGES CXX)

option(FAST_CGI_BUILD_EXAMPLES "Build examples." ON)
option(FAST_CGI_ENABLE_LOGGING "Enable logging." ON)

find_package(Threads REQUIRED)

add_library("fast_cgi" INTERFACE)
target_include_directories("fast_cgi"
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
	INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/spdlog/include")
target_link_libraries("fast_cgi"
	INTERFACE ${CMAKE_THREAD_LIBS_INIT})

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