#pragma once

#include <cstddef>
#include <mutex>

namespace fast_cgi {

class connection
{
public:
    typedef std::size_t size_type;

    virtual ~connection()
    {
        delete _mutex;
    }
    void flush()
    {
        if (_mutex) {
            std::lock_guard<std::mutex> lock(*_mutex);

            do_flush();
        } else {
            do_flush();
        }
    }
    size_type in_available()
    {
        if (_mutex) {
            std::lock_guard<std::mutex> lock(*_mutex);

            return do_in_available();
        } else {
            return do_in_available();
        }
    }
    size_type read(void* buffer, size_type at_least, size_type at_most)
    {
        if (_mutex) {
            std::lock_guard<std::mutex> lock(*_mutex);

            return do_read(buffer, at_least, at_most);
        } else {
            return do_read(buffer, at_least, at_most);
        }
    }
    size_type write(const void* buffer, size_type size)
    {
        if (_mutex) {
            std::lock_guard<std::mutex> lock(*_mutex);

            return do_write(buffer, size);
        } else {
            return do_write(buffer, size);
        }
    }

protected:
    connection(bool synchronize)
    {
        _mutex = synchronize ? new std::mutex() : nullptr;
    }
    connection(const connection& copy) = delete;
    connection(connection&& move)
    {
        _mutex      = move._mutex;
        move._mutex = nullptr;
    }
    virtual void do_flush()                                                        = 0;
    virtual size_type do_in_available()                                            = 0;
    virtual size_type do_read(void* buffer, size_type at_least, size_type at_most) = 0;
    virtual size_type do_write(const void* buffer, size_type size)                 = 0;

private:
    std::mutex* _mutex;
};

} // namespace fast_cgi