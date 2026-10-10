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
        src/tcp.cpp
    )
    target_compile_options(netscope_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    add_test(NAME netscope_tests COMMAND netscope_tests)

    add_executable(netscope_event_id_tests
        tests/test_event_id.cpp
    )
    target_compile_options(netscope_event_id_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    add_test(NAME netscope_event_id_tests COMMAND netscope_event_id_tests)

    add_executable(netscope_json_util_tests
        tests/test_json_util.cpp
    )
    target_compile_options(netscope_json_util_tests PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
    add_test(NAME netscope_json_util_tests COMMAND netscope_json_util_tests)

    add_test(
        NAME netscope_rejects_invalid_interval
        COMMAND netscope --once --interval 1abc
    )
    set_tests_properties(
        netscope_rejects_invalid_interval
        PROPERTIES WILL_FAIL TRUE
    )

    add_test(
        NAME netscope_rejects_invalid_alert_threshold
        COMMAND netscope --once --alert-mbps 0
    )
    set_tests_properties(
        netscope_rejects_invalid_alert_threshold
        PROPERTIES WILL_FAIL TRUE
    )

    add_test(
        NAME netscope_rejects_overflowing_alert_threshold
        COMMAND netscope --once --alert-mbps 1e308
    )
    set_tests_properties(
        netscope_rejects_overflowing_alert_threshold
        PROPERTIES WILL_FAIL TRUE
    )
endif()
