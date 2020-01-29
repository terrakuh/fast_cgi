# fast_cgi

- [Installation](#installation)
- [Cheatsheet](#cheatsheet)
  - [Roles](#roles)
    - [Responder (`fast_cgi::responder`)](#responder-fast_cgiresponder)
    - [Filter (`fast_cgi::filter`)](#filter-fast_cgifilter)
    - [Authorizer (`fast_cgi::authorizer`)](#authorizer-fast_cgiauthorizer)
  - [Parameters](#parameters)
- [License](#license)

## Installation

Requirements:

- CMake *>= 3.1*
- Git
- C++11 compiler

```cmd
git clone https://github.com/terrakuh/fast_cgi.git
cd fast_cgi
git submodule init
git submodule update

mkdir build
cd build

cmake ..

# cmake -DFAST_CGI_BUILD_EXAMPLES=OFF
# cmake -DFAST_CGI_ENABLE_LOGGING=OFF

cmake --build .
cmake --build . --target install
```

## Cheatsheet

### Roles

Every role is provided an output stream by `output()`, an error stream by `error()` and request parameters by `params()`. For every incoming request from the web server a new role instance is created. Only one user specific role can be integrated for each type. Integration can look like this:

```cpp
protocol.set_role<my_responder>();
```

A detailed definition of the following roles can be found [here](https://fastcgi-archives.github.io/FastCGI_Specification.html#S6.1).

#### Responder (`fast_cgi::responder`)

A responder additionally receives optinal input by `input()`. This role has the same purpose as simple CGI/1.1 programs (this is probably what you want). A simple *Hello, Wolrd* might look like this:

```cpp
#include <fast_cgi/fast_cgi.hpp>

class hello_world : public fast_cgi::responder
{
public:
    virtual status_code_type run() override
    {
        using namespace fast_cgi::manipulator;

        output() << "Content-type: text/html" << feed << feed;
        output() << "<html><body>"
                 << "<h1>" << "Hello, World!" << "</h1><br/><br/>"
                 << "<span>Here are all parameters:</span><br/>";

        // Print all parameters
        for (auto& i : params()) {
            output() << i.first << "=" << i.second << "<br/>";
        }
        
        output() << "<span>Your payload:</span><br/>";
        output() << input().rdbuf();
        output() << "</body></html>"

        return 0;
    }
};
```

More information can be found [here](https://fastcgi-archives.github.io/FastCGI_Specification.html#S6.2).

#### Filter (`fast_cgi::filter`)

See [here](https://fastcgi-archives.github.io/FastCGI_Specification.html#S6.4).

#### Authorizer (`fast_cgi::authorizer`)

See [here](https://fastcgi-archives.github.io/FastCGI_Specification.html#S6.3).

### Parameters

Parameters can be iterated like:

```cpp
for (auto& parameter : params()) {
    std::cout << parameter.first << "=" << parameter.second << "\n";
}
```

Getting a parameter (this example may throw an `std::out_of_range` exception if the key does not exist):

```cpp
auto& value = params()["REQUEST_URI"];
// or
auto& value = params("REQUEST_URI");
```

Checking if a parameter is available:

```cpp
auto has_uri = params().has("REQUEST_URI");
```

## License

[MIT License](https://github.com/terrakuh/fast_cgi/blob/master/LICENSE)
