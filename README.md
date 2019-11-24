# fast_cgi

- [Installation](#installation)
  - [With CMake](#with-cmake)
  - [Without CMake](#without-cmake)
- [Cheatsheet](#cheatsheet)
  - [Roles](#roles)
    - [Responder](#responder)
    - [Filter](#filter)
    - [Authorizer](#authorizer)
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

Every role is provided an output stream by `output()`, an error stream by `error()` and request parameters by `params()`.

#### Responder

A responder additionally receives optinal input by `input()`.

#### Filter

#### Authorizer

## License
