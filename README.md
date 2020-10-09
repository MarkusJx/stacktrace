# stacktrace
Stack traces for windows, linux and macOs. Somewhat equal to the implementation in ``boost::stacktrace``.
Uses an edited version of the addr2line to get line numbers on linux. If you are using linux,
you must install ``binutils-dev`` and ``libiberty-dev`` in order to compile ``addr2line.c``.

## Examples
On **windows**, stack traces may look like this (built in debug mode):
```
 0# markusjx::stacktrace::stacktrace::stacktrace in stacktrace.cpp:390
 1# test_1 in test.cpp:6
 2# main in main.cpp:33
 3# invoke_main in exe_common.inl:79
 4# __scrt_common_main_seh in exe_common.inl:288
 5# __scrt_common_main in exe_common.inl:331
 6# mainCRTStartup in exe_main.cpp:17
```

On **linux**, stack traces may look like this:
```
 0# markusjx::stacktrace::stacktrace::stacktrace(unsigned long, unsigned long) in stacktrace.cpp:405
 1# test_1() in test.cpp:6
 2# main in main.cpp:33
 3# __libc_start_main in libc.so.6
 4# 0x000055944645CAEA in stacktrace
```

On **macOs**, stack traces may look like this:
```
 0# markusjx::stacktrace::stacktrace::stacktrace(unsigned long, unsigned long) in stacktrace
 1# markusjx::stacktrace::stacktrace::stacktrace(unsigned long, unsigned long) in stacktrace
 2# test_1() in stacktrace
 3# main in stacktrace
 4# start in libdyld.dylib
 5# 0   ???                                 0x0000000000000001 0x0 + 1
```

## Compiling
In order to compile this library, you could create a ``CMakeLists.txt`` similar to this:
```CMake
cmake_minimum_required(VERSION 3.17)
project(PROJECT_NAME)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# If the addr2line library is not used, you must set:
#set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")

add_executable(${PROJECT_NAME} main.cpp)

# Add this after add_executable or add_library
include(initStacktrace.cmake)
initStacktrace(${PROJECT_NAME})
```
