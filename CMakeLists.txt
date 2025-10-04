cmake_minimum_required(VERSION 3.14)
project(multi_vector LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add Google Test
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/googletest)

# Create the test executable
add_executable(tests tests.cpp)

# Link with Google Test
target_link_libraries(tests
    gtest
    gtest_main
)

# Add the current directory to include path so multi_vector.hpp can be found
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Enable testing
enable_testing()
add_test(NAME multi_vector_tests COMMAND tests)

# Add compiler warnings
if(MSVC)
    target_compile_options(tests PRIVATE /W4)
else()
    target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)
endif()