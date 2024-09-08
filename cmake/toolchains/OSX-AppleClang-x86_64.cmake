include("${CMAKE_CURRENT_LIST_DIR}/include/OSX-AppleClang_common.inc.cmake")
# UNTESTED !

function(toolchain_force_compiler)
    if(CROSSCOMPILE_ARCH)
        message(FATAL_ERROR "This toolchain (as it is now) doesn't support cross-compilation.")
    endif()

    set(CMAKE_C_COMPILER "clang" CACHE STRING "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "C++ compiler" FORCE)

    # In order to enable ninja to be verbose
    set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON")
endfunction()

function(toolchain_after_project)
    message(STATUS "Toolchain: OSX-AppleClang-x86_64.cmake.")
    # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
    #SET(CMAKE_SYSTEM_NAME    "Darwin" CACHE INTERNAL "" FORCE)
    set(CMAKE_SYSTEM_PROCESSOR "x86_64" CACHE INTERNAL "" FORCE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-x86_64" PARENT_SCOPE)
    #set(ARCH_BITS 64 CACHE INTERNAL "" FORCE) # provide it

    toolchain_after_project_common()

    # -target=-x86_64-apple-darwin ?
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -march=x86-64 -m64" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=x86-64 -m64" PARENT_SCOPE)
endfunction()

function(toolchain_exe_stuff)
    toolchain_exe_stuff_common()

    # Propagate global variables set in toolchain_exe_stuff_common to the upper scope
    #SET (CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
    #SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"    PARENT_SCOPE)
    #SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}"             PARENT_SCOPE)
endfunction()
