include("${CMAKE_CURRENT_LIST_DIR}/include/Windows-Clang_common.inc.cmake")

function(toolchain_force_compiler)
    if(CROSSCOMPILING_ARCH)
        message(FATAL_ERROR "Can't cross-compile with a 'native' toolchain.")
    endif()

    set(CMAKE_C_COMPILER "clang" CACHE STRING "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "C++ compiler" FORCE)

    # In order to enable ninja to be verbose
    #set(CMAKE_VERBOSE_MAKEFILE             ON CACHE    BOOL "ON")

    if(CLANG_USE_GCC_LINKER)
        # Not working, see CLANG_USE_GCC_LINKER option. Use MSVC.
        set(CLANG_VENDOR "gnu" PARENT_SCOPE)
    else()
        set(CLANG_VENDOR "msvc" PARENT_SCOPE)
    endif()
endfunction()

function(toolchain_after_project)
    message(STATUS "Toolchain: Windows-Clang-native.cmake.")
    # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
    #SET(CMAKE_SYSTEM_NAME    "Windows"      INTERNAL "" FORCE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native64" PARENT_SCOPE)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native32" PARENT_SCOPE)
    endif()

    toolchain_after_project_common() # To enable RC language, to compile Windows Resource files

    #SET (CLANG_TARGET     "--target=x86_64-pc-windows-${CLANG_VENDOR}")
    #SET (C_ARCH_OPTS    "-march=native ${CLANG_TARGET}")
    #SET (CXX_ARCH_OPTS    "-march=native ${CLANG_TARGET}")
    #SET (RC_FLAGS        "${CLANG_TARGET}")

    # TODO: fix the target, maybe using CMAKE_HOST_SYSTEM_PROCESSOR ?
    link_directories("lib/_bin/{ARCH}/mariadb/")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -march=native -mtune=native" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -mtune=native" PARENT_SCOPE)
endfunction()

function(toolchain_exe_stuff)
    toolchain_exe_stuff_common()

    # Propagate global variables set in toolchain_exe_stuff_common to the upper scope
    #SET (CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
    #SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"    PARENT_SCOPE)
    #SET (CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}"                PARENT_SCOPE)
    #SET (CMAKE_RC_FLAGS            "${RC_FLAGS}"                            PARENT_SCOPE)
endfunction()
