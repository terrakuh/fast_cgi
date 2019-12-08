# fast_cgi

- [Installation](#installation)
  - [With CMake](#with-cmake)
  - [Without CMake](#without-cmake)
- [Cheatsheet](#cheatsheet)
  - [Roles](#roles)
    - [Responder (`fast_cgi::responder`)](#responder-fast_cgiresponder)
    - [Filter (`fast_cgi::filter`)](#filter-fast_cgifilter)
    - [Authorizer (`fast_cgi::authorizer`)](#authorizer-fast_cgiauthorizer)
  - [Parameters](#parameters)
- [License](#license)

## Installation

### With CMake

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

### Without CMake

Just copy the *fast_cgi* folder with all the *.hpp* files to your project or set it as include directory. If logging is enabled (logging can be disabled by manually deleting the macro `FAST_CGI_ENABLE_LOGGING` in `log.hpp`) *spdlog* is also required.

## Cheatsheet

### Roles

Every role is provided an output stream by `output()`, an error stream by `error()` and request parameters by `params()`. Only one user specific role can be integrated for each type. Integration can look like:

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
