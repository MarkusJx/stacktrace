cmake_minimum_required(VERSION 3.16)
project(stacktrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

option(BUILD_TESTS OFF)

if (NOT WIN32 AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -g")
endif ()

add_library(stacktrace STATIC)

#set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

include(initStacktrace.cmake)
initStacktrace(stacktrace)

if (BUILD_TESTS)
    add_executable(stacktrace_test main.cpp test.cpp test.hpp)
    target_link_libraries(stacktrace_test stacktrace)
endif ()
