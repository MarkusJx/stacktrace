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

        # Set BUILD_ADDR2LINE option to 'true', if bfd.h, libiberty/demangle.h and libbfd exist
        set(BUILD_ADDR2LINE EXISTS ${BFD_LIBRARY} AND EXISTS ${DEMANGLE_H} AND EXISTS ${BFD_H})

        if (${BUILD_ADDR2LINE})
            # Set addr2lineLib sources
            set(ADDR2LINE_SRC addr2lineLib/addr2line.c addr2lineLib/addr2line.h
                    addr2lineLib/addr2line.hpp addr2lineLib/addr2line.cpp)
            message(STATUS "libbfd and libiberty found, building addr2line.c")

            # Try to compile addr2line.c with standard definitions
            message(STATUS "Trying to compile addr2line.c without custom function definitions")
            set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
            try_compile(CAN_COMPILE_ADDR2LINE "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/addr2lineLib/addr2line.c"
                LINK_LIBRARIES bfd dl)

            if (CAN_COMPILE_ADDR2LINE)
                # The compilation was successful, we can continue
                message(STATUS "Compilation was successful, not adding any compile definitions")
            else()
                # The compilation failed, try with the ADDR2LINE_LIB_DEFINE_BFD_WRAPPERS compile defintion
                message(STATUS "Compilation failed, trying to compile addr2line.c with custom function definitions")
                try_compile(CAN_COMPILE_ADDR2LINE "${CMAKE_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/addr2lineLib/addr2line.c"
                    COMPILE_DEFINITIONS -DADDR2LINE_LIB_DEFINE_BFD_WRAPPERS
                    LINK_LIBRARIES bfd dl)

                if (CAN_COMPILE_ADDR2LINE)
                    # Compilation was successful, add ADDR2LINE_LIB_DEFINE_BFD_WRAPPERS compile definition to the target
                    message(STATUS "Compilation was successful, adding 'ADDR2LINE_LIB_DEFINE_BFD_WRAPPERS' compile definition")
                    target_compile_definitions(${target} PRIVATE ADDR2LINE_LIB_DEFINE_BFD_WRAPPERS)
                else()
                    # The compilation failed, set addr2line sources to an empty string
                    # and set BUILD_ADDR2LINE to FALSE
                    message(STATUS "Compilation failed, not building addr2line.c")
                    set(ADDR2LINE_SRC "")
                    set(BUILD_ADDR2LINE FALSE)
                endif(CAN_COMPILE_ADDR2LINE)
            endif(CAN_COMPILE_ADDR2LINE)
        else ()
            set(ADDR2LINE_SRC "")
            message(STATUS "libbfd or libiberty not found, not building addr2line.c")
        endif ()

        #if (APPLE)
        #    include_directories("/opt/local/include")
        #    link_directories("/opt/local/lib")
        #endif ()
    endif ()

    target_sources(${target} PRIVATE stacktrace.hpp stacktrace.cpp ${ADDR2LINE_SRC})

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