cmake_minimum_required(VERSION 3.17)
project(stacktrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

if (NOT WIN32 AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -g")

    #set(CMAKE_EXE_LINKER_FLAGS "-rdynamic -g")

    set(addr2line addr2lineLib/addr2line.c addr2lineLib/addr2line.h addr2lineLib/addr2line.hpp addr2lineLib/addr2line.cpp)

    #if (APPLE)
    #    include_directories("/opt/local/include")
    #    link_directories("/opt/local/lib")
    #endif ()
endif ()

add_executable(stacktrace main.cpp stacktrace.hpp stacktrace.cpp ${addr2line} test.cpp test.hpp)

if (NOT WIN32 AND NOT APPLE)
    target_link_libraries(stacktrace PRIVATE bfd dl)
elseif (APPLE)
    target_link_libraries(stacktrace PRIVATE dl)
endif ()
