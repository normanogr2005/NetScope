cmake_minimum_required(VERSION 3.16)
project(netscope VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(netscope
    src/main.cpp
    src/netstats.cpp
    src/tcp.cpp
)

target_compile_options(netscope PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)

include(CTest)
if(BUILD_TESTING)
    add_executable(netscope_tests
        tests/test_netstats.cpp
        src/netstats.cpp
    )
    target_compile_options(netscope_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    add_test(NAME netscope_tests COMMAND netscope_tests)
endif()
