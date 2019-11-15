project("fast_cgi"
	LANGUAGES CXX)

file(GLOB_RECURSE FAST_CGI_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library("fast_cgi" INTERFACE)
target_include_directories("fast_cgi"
	INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})