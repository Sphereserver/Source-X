include("${CMAKE_CURRENT_LIST_DIR}/include/Linux-Clang_common.inc.cmake")

function(toolchain_force_compiler)
    if(CROSSCOMPILING_ARCH)
        message(FATAL_ERROR "Can't cross-compile with a 'native' toolchain.")
    endif()

    set(CMAKE_C_COMPILER "clang" CACHE STRING "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "C++ compiler" FORCE)
endfunction()

function(toolchain_after_project)
    message(STATUS "Toolchain: Linux-Clang-native.cmake.")
    # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
    #SET(CMAKE_SYSTEM_NAME    "Linux"      CACHE INTERNAL "" FORCE) # target OS
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native64" PARENT_SCOPE)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native32" PARENT_SCOPE)
    endif()

    # possible native/host lib locations
    set(local_usr_lib_subfolders "libmariadb3" "mysql")
    foreach(local_usr_lib_subfolders subfolder)
        # ARCH variable is set by CMakeGitStatus
        set(local_lib_search_paths ${local_lib_search_paths} "/usr/${ARCH}-linux-gnu/${subfolder}")
    endforeach()
    set(lib_search_paths
        ${local_lib_search_paths}
        "/usr/lib64/mysql"
        "/usr/lib64"
        "/usr/lib/mysql"
        "/usr/lib"
        CACHE STRING
        "Library search paths"
        FORCE
    )

    toolchain_after_project_common()

    if("${ARCH_BASE}" STREQUAL "arm")
        # Clang doesn't support -march=native for ARM (but it IS supported by Apple Clang, btw)
        # Also:
        #  https://community.arm.com/arm-community-blogs/b/tools-software-ides-blog/posts/compiler-flags-across-architectures-march-mtune-and-mcpu
        #  https://code.fmsolvr.fz-juelich.de/i.lilikakis/fmsolvr/commit/bd010058b8b8c92385d1ccc321912bf1359d66b0
        set(local_arch_cmd -mcpu=native)
    else()
        set(local_arch_cmd -march=native -mtune=native)
    endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   ${local_arch_cmd}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${local_arch_cmd}" PARENT_SCOPE)
endfunction()

function(toolchain_exe_stuff)
    toolchain_exe_stuff_common()

    # Propagate global variables set in toolchain_exe_stuff_common to the upper scope
    #SET (CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"        PARENT_SCOPE)
    #SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"    PARENT_SCOPE)
    #SET (CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}"                PARENT_SCOPE)
endfunction()
