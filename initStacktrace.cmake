# init the stacktrace library
# arguments:
#   target - the name of the target to build
function(initStacktrace target)
    if (NOT WIN32 AND NOT APPLE)
        # Find required libraries and headers
        find_library(BFD_LIBRARY NAMES "bfd")
        find_file(DEMANGLE_H NAMES "libiberty/demangle.h")
        find_file(BFD_H NAMES "bfd.h")

        if (NOT EXISTS ${DEMANGLE_H})
            message(STATUS "libiberty/demangle.h was not found. Probably libiberty-dev is not installed.")
        endif ()

        if (NOT EXISTS ${BFD_H})
            message(STATUS "bfd.h was not found. Probably binutils-dev is not installed.")
        endif ()

        if (NOT EXISTS ${BFD_LIBRARY})
            message(STATUS "libbfd was not found. Probably binutils-dev is not installed.")
        endif ()

        set(BUILD_ADDR2LINE EXISTS ${BFD_LIBRARY} AND EXISTS ${DEMANGLE_H} AND EXISTS ${BFD_H})

        if (${BUILD_ADDR2LINE})
            set(addr2line addr2lineLib/addr2line.c addr2lineLib/addr2line.h
                    addr2lineLib/addr2line.hpp addr2lineLib/addr2line.cpp)
            message(STATUS "libbfd and libiberty found, building addr2line.c")
        else ()
            set(addr2line "")
            message(STATUS "libbfd or libiberty not found, not building addr2line.c")
        endif ()

        #if (APPLE)
        #    include_directories("/opt/local/include")
        #    link_directories("/opt/local/lib")
        #endif ()
    endif ()

    target_sources(${target} PRIVATE stacktrace.hpp stacktrace.cpp ${addr2line})

    if (NOT WIN32 AND NOT APPLE)
        if (${BUILD_ADDR2LINE})
            target_link_libraries(${target} PRIVATE bfd dl)
            message(STATUS "libbfd and libiberty found, linking libbfd")
        else ()
            target_link_libraries(${target} PRIVATE dl)
            message(STATUS "libbfd or libiberty not found, not linking libbfd")
            target_compile_definitions(${target} PRIVATE STACKTRACE_NO_ADDR2LINE)
        endif ()
    elseif (APPLE)
        target_link_libraries(${target} PRIVATE dl)
        target_compile_definitions(${target} PRIVATE STACKTRACE_NO_ADDR2LINE)
    endif ()
endfunction()