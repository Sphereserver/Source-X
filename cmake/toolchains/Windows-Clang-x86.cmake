include("${CMAKE_CURRENT_LIST_DIR}/include/Windows-Clang_common.inc.cmake")

function(toolchain_force_compiler)
    if(CROSSCOMPILE_ARCH)
        message(FATAL_ERROR "This toolchain (as it is now) doesn't support cross-compilation.")
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
    message(STATUS "Toolchain: Windows-Clang-x86.cmake.")
    # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
    #SET(CMAKE_SYSTEM_NAME    "Windows"        CACHE INTERNAL "" FORCE)
    set(CMAKE_SYSTEM_PROCESSOR "x86" CACHE INTERNAL "" FORCE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-x86" PARENT_SCOPE)
    #set(ARCH_BITS 32 CACHE INTERNAL "" FORCE) # provide it

    toolchain_after_project_common() # To enable RC language, to compile Windows Resource files

    link_directories("lib/_bin/x86/mariadb/")
    set(CLANG_TARGET "--target=i686-pc-windows-${CLANG_VENDOR}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}   -march=i686 -m32 ${CLANG_TARGET}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=i686 -m32 ${CLANG_TARGET}" PARENT_SCOPE)
    set(RC_FLAGS "${CLANG_TARGET}" PARENT_SCOPE)
endfunction()

function(toolchain_exe_stuff)
    toolchain_exe_stuff_common()

    # Propagate global variables set in toolchain_exe_stuff_common to the upper scope
    #SET (CMAKE_C_FLAGS            "${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"        PARENT_SCOPE)
    #SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"    PARENT_SCOPE)
    #SET (CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}"                PARENT_SCOPE)
    #SET (CMAKE_RC_FLAGS            "${RC_FLAGS}"                            PARENT_SCOPE)

    # Set different output folders for each build type
    set_target_properties(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release")
    set_target_properties(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug")
    set_target_properties(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly")
endfunction()
