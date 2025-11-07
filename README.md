# multi_vector

A C++17 header-only library implementing `multi_vector<Ts...>` - a container that stores multiple vectors of different types in a single contiguous memory block.

## Features

- **Header-only**: Just include `multi_vector.hpp`
- **Memory efficient**: Single contiguous allocation for all type vectors
- **Flexible access**: Both type-based and index-based API

## Quick Start

```cpp
#include "multi_vector.hpp"

// Create a multi_vector that can store ints, doubles, and strings
auto vec = multi_vector<int, double, std::string>::builder()
    .capacity<int>(100)
    .capacity<double>(50)
    .capacity<std::string>(200)
    .build();

vec.push_back<int>(42);
vec.push_back<double>(3.14);
vec.push_back<std::string>("hello");

// Access elements
int* ints = vec.data<int>();
std::size_t count = vec.size<int>();
std::size_t cap = vec.capacity<int>();
```

### Default Values

Initialize all elements of a type with a default value:

```cpp
auto vec = multi_vector<int, std::string>::builder()
    .capacity<int>(10)
    .capacity<std::string>(5)
    .default_value<int>(0)           // Initialize all 10 ints to 0
    .default_value<std::string>("?") // Initialize all 5 strings to "?"
    .build();

// vec.size<int>() == 10, all elements are 0
// vec.size<std::string>() == 5, all elements are "?"
```

## Memory Layout

`multi_vector` allocates a single memory block with proper alignment for all types:

```
┌─────────────────────────────────────────────────┐
│  [int array]  │  [double array]  │  [string...] │
└─────────────────────────────────────────────────┘
         Single contiguous allocation
```

## Constraints

- **Fixed capacity**: Capacity is set at construction time and cannot be changed
- **Capacity enforcement**: Throws `std::length_error` when capacity exceeded

## Building and Testing

### Prerequisites

- CMake 3.14 or later
- C++17 compatible compiler

### Build Steps

```bash
# Clone and initialize submodules
git clone <repository-url>
cd multi_vector
git submodule update --init --recursive

# Build
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Run tests
./Release/tests.exe     # Windows
# or
./tests                 # Linux/macOS
```
