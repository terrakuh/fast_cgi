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

mkdir build
cd build

cmake ..

# cmake -DFAST_CGI_BUILD_EXAMPLES=OFF

cmake --build .
cmake --build . --target install
```

### Without CMake

Just copy the *fast_cgi* folder with *.hpp* files to your project or set it as include directory.

## Cheatsheet

### Roles

Every role is provided an output stream by `output()`, an error stream by `error()` and request parameters by `params()`. Only one user specific role for each type can be integrated. Integration can look like:

```cpp
protocol.set_role<my_responder>();
```

#### Responder (`fast_cgi::responder`)

A responder additionally receives optinal input by `input()`.

#### Filter (`fast_cgi::filter`)

#### Authorizer (`fast_cgi::authorizer`)

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
