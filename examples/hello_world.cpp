#include <arpa/inet.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fast_cgi/fast_cgi.hpp>
#include <fast_cgi/simple_allocator.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

class responder : public fast_cgi::responder
{
public:
    virtual status_code_type run() override
    {
        using namespace fast_cgi::manipulator;

        output() << "Content-type: text/html" << feed << feed;
        output() << "<html>"
                 << "<h1>"
                 << "Hello, World!"
                 << "</h1>"
                 << "Your URI: "
                 << (params().has("REQUEST_URI") ? params()["REQUEST_URI"] : "<i>No request uri specified.</i>")
                 << "<br/>";

        for (auto& i : params()) {
            output() << i.first << "=" << i.second << "<br/>";
        }

        output() << "</html>" << input().rdbuf();

        return 0;
    }
};

class conn : public fast_cgi::connection
{
public:
    int s;
    char* buffer;
    char* ptr;
    char* end;

    conn(int s) : connection(true), s(s)
    {
        buffer = new char[4096];
        ptr    = buffer;
        end    = buffer + 4096;
    }
    ~conn()
    {
        delete[] buffer;
        ::close(s);
        s = 0;
        LOG(DEBUG, "killing connection");
    }
    virtual size_type do_in_available() override
    {
        int count = 0;
        ioctl(s, FIONREAD, &count);
        return count;
    }
    virtual void do_flush() override
    {
        if (ptr - buffer) {
            LOG(DEBUG, "flushing {}", buff{ buffer, std::size_t(ptr - buffer) });

            send(s, buffer, ptr - buffer, 0);
            ptr = buffer;
        }
    }
    virtual size_type do_read(void* buffer, size_type at_least, size_type at_most) override
    {
        auto t = recv(s, (char*) buffer, at_most, 0);

        if (t < 0) {
            LOG(CRITICAL, "faile0 to receive d {}", t);
        }

        return t;
    }
    virtual size_type do_write(const void* buffer, size_type size) override
    {
        for (size_type i = 0; i < size;) {
            auto s = std::min(static_cast<size_type>(end - ptr), size - i);
            std::memcpy(ptr, buffer, s);
            ptr += s;

            i += s;

            if (i != size) {
                flush();
            }
        }

        return size;
    }
};

class con : public fast_cgi::connector
{
public:
    int s;

    con()
    {
        s = socket(AF_INET, SOCK_STREAM, 0);

        struct sockaddr_in server;
        server.sin_family      = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port        = htons(32158);
        bind(s, (struct sockaddr*) &server, sizeof(server));
        listen(s, 3);
    }

    // Inherited via connector
    virtual std::shared_ptr<fast_cgi::connection> accept() override
    {
        int c = sizeof(struct sockaddr_in);
        struct sockaddr_in client;

        return std::shared_ptr<fast_cgi::connection>(
            new conn(::accept(s, (struct sockaddr*) &client, (socklen_t*) &c)));
    }
};

int main()
{
#if defined(FAST_CGI_ENABLE_LOGGING)
    spdlog::default_logger_raw()->set_pattern("[%T:%e | %=5t | %-20s at %-3# (%-20!)] [%^%=8l%$]\n\t%v");
    //spdlog::default_logger_raw()->set_level(spdlog::level::trace);
#endif

    // create server
    fast_cgi::protocol protocol(std::make_shared<con>(), std::make_shared<fast_cgi::simple_allocator>());

    protocol.set_role<responder>();

    protocol.run();
    protocol.join();
}
