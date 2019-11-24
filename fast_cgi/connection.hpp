#pragma once

#include <cstddef>

namespace fast_cgi {

class connection
{
public:
	typedef std::size_t size_type;

	virtual ~connection()														= default;
	virtual void close()														= 0;
	virtual void flush()														= 0;
	virtual size_type read(void* buffer, size_type at_least, size_type at_most) = 0;
	virtual size_type write(const void* buffer, size_type size)					= 0;
};

} // namespace fast_cgi