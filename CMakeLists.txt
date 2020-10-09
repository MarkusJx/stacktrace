cmake_minimum_required(VERSION 3.17)
project(stacktrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if (NOT WIN32 AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -g")
endif ()

add_executable(stacktrace main.cpp test.cpp test.hpp)

#set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

include(initStacktrace.cmake)
initStacktrace(${PROJECT_NAME})
