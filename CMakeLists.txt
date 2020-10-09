cmake_minimum_required(VERSION 3.17)
project(stacktrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

add_executable(stacktrace main.cpp test.cpp test.hpp)

#set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

include(initStacktrace.cmake)
initStacktrace(${PROJECT_NAME})
