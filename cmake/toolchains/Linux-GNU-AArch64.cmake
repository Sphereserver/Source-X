include("${CMAKE_CURRENT_LIST_DIR}/include/Linux-GNU_common.inc.cmake")

function(toolchain_force_compiler)
    if(NOT CROSSCOMPILING_ARCH)
        set(CMAKE_C_COMPILER "gcc" CACHE STRING "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "g++" CACHE STRING "C++ compiler" FORCE)
    else()
        set(CMAKE_C_COMPILER "aarch64-linux-gnu-gcc" CACHE STRING "C compiler" FORCE)
        set(CMAKE_CXX_COMPILER "aarch64-linux-gnu-g++" CACHE STRING "C++ compiler" FORCE)

        # where is the target environment located
        set(CMAKE_FIND_ROOT_PATH "/usr/aarch64-linux-gnu" CACHE INTERNAL "" FORCE)

        # adjust the default behavior of the FIND_XXX() commands:
        # search programs in the host environment
        set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "NEVER" CACHE INTERNAL "" FORCE)

        # search headers and libraries in the target environment
        set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "ONLY" CACHE INTERNAL "" FORCE)
        set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "ONLY" CACHE INTERNAL "" FORCE)
    endif()
endfunction()

function(toolchain_after_project)
    message(STATUS "Toolchain: Linux-GNU-AArch64.cmake.")
    # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
    #unset(CMAKE_SYSTEM_NAME)
    #set(CMAKE_SYSTEM_NAME    "Linux"        CACHE INTERNAL "" FORCE)
    unset(CMAKE_SYSTEM_PROCESSOR)
    set(CMAKE_SYSTEM_PROCESSOR "aarch64" CACHE INTERNAL "" FORCE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-aarch64" CACHE INTERNAL "" FORCE)
    set(ARCH_BITS 64 CACHE INTERNAL "" FORCE) # provide it

    if(CROSSCOMPILING_ARCH)
        # possible cross-compilation foreign arch lib locations
        set(lib_search_paths
            "/usr/aarch64-linux-gnu/usr/lib/libmariadb3"
            "/usr/aarch64-linux-gnu/usr/lib/mysql"
            "/usr/aarch64-linux-gnu/usr/lib/"
            CACHE STRING
            "Library search paths"
            FORCE
        )
    else()
        # possible native/host lib locations
        set(lib_search_paths
            "/usr/lib/aarch64-linux-gnu/libmariadb3"
            "/usr/lib/aarch64-linux-gnu/mysql"
            "/usr/lib/aarch64-linux-gnu"
            "/usr/lib64/mysql"
            "/usr/lib64"
            "/usr/lib/mysql"
            "/usr/lib"
            CACHE STRING
            "Library search paths"
            FORCE
        )
    endif()

    toolchain_after_project_common()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -march=armv8-a" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a" PARENT_SCOPE)
endfunction()

function(toolchain_exe_stuff)
    toolchain_exe_stuff_common()

    # Propagate global variables set in toolchain_exe_stuff_common to the upper scope
    #SET (CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
    #SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"    PARENT_SCOPE)
    #SET (CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}"                PARENT_SCOPE)
endfunction()
