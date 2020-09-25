cmake_minimum_required(VERSION 3.17)
project(stacktrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -g")

#set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -g")

add_executable(stacktrace main.cpp addr2line.c addr2line.h addr2line.hpp)
target_link_libraries(stacktrace PRIVATE bfd)
