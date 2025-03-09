include(CheckLinkerFlag)
message(STATUS "Setting base compiler flags...")

set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} ${CUSTOM_C_FLAGS}")
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} ${CUSTOM_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CUSTOM_EXE_LINKER_FLAGS}")

if(NOT MSVC)
    # Compiler option flags (minimum).
    list(
        APPEND
        compiler_options_base
        -pthread
        -fexceptions
        -fnon-call-exceptions
        -pipe
        -ffast-math
        # Place each function and variable into its own section, allowing the linker to remove unused ones.
        #-ffunction-sections
        #-fdata-sections
        #-flto # Supported even by ancient compilers... also needed to benefit the most from the two flags above.
    )
    list(
        APPEND
        compiler_options_warning_base
        -Werror
        -Wall
        -Winitialized
        -Wformat-signedness
        -Wextra
        -Wpedantic
        -Wdouble-promotion
        -Wconversion
        -Wsign-conversion
        -Wduplicated-branches
        #-Wcast-qual
        #-Wnrvo
    )

    # Linker option flags (minimum).
    list(
        APPEND
        linker_options_base
        -pthread
        -dynamic
        #-flto
    )

    message(STATUS "Adding the following base compiler options: ${compiler_options_base}.")
    message(STATUS "Adding the following base compiler warning options: ${compiler_options_warning_base}.")
    message(STATUS "Adding the following base linker options: ${linker_options_base}.")

    #-- Store once the compiler flags, only the ones specific per build type.

    if("${ENABLED_SANITIZER}" STREQUAL "TRUE")
        # -fno-omit-frame-pointer disables a good optimization which might corrupt the debugger stack trace.
        set(local_compile_options_nondebug -ggdb3 -Og -fno-omit-frame-pointer -fno-inline)
        set(local_link_options_nondebug)
    else()
        # If using ThinLTO: -flto=thin -fsplit-lto-unit
        # If using classic/monolithic LTO: -flto=full -fvirtual-function-elimination
        # Probably useful when using lto: -ffunction-sections -fdata-sections
        set(local_compile_options_nondebug -O3)# -flto)
        set(local_link_options_nondebug)#-flto)
    endif()

    check_linker_flag(CXX -Wl,--gc-sections LINKER_HAS_GC_SECTIONS)
    if(LINKER_HAS_GC_SECTIONS)
      list(APPEND local_link_options_nondebug -Wl,--gc-sections)
    endif()

    set(custom_compile_options_release
        ${local_compile_options_nondebug}
        -flto=full
        -fvirtual-function-elimination
        -ffunction-sections
        -fdata-sections
        CACHE INTERNAL STRING
    )
    set(custom_compile_options_nightly
        ${local_compile_options_nondebug}
        CACHE INTERNAL STRING
    )
    set(custom_compile_options_debug
        -ggdb3 -O0 -fno-omit-frame-pointer -fno-inline
        CACHE INTERNAL STRING
    )

    set(custom_link_options_release
        ${local_link_options_nondebug}
        -flto=full
        CACHE INTERNAL STRING
    )
    set(custom_link_options_nightly
        ${local_link_options_nondebug}
        CACHE INTERNAL STRING
    )
    set(custom_link_options_debug
        ""
        CACHE INTERNAL STRING
    )

    if(TARGET spheresvr_release)
        message(STATUS "Adding the following compiler options specific to 'Release' build: ${custom_compile_options_release}.")
        message(STATUS "Adding the following linker options specific to 'Release' build: ${custom_link_options_release}.")
    endif()
    if(TARGET spheresvr_nightly)
        message(STATUS "Adding the following compiler options specific to 'Nightly' build: ${custom_compile_options_nightly}.")
        message(STATUS "Adding the following linker options specific to 'Nightly' build: ${custom_link_options_nightly}.")
    endif()
    if(TARGET spheresvr_debug)
        message(STATUS "Adding the following compiler options specific to 'Debug' build: ${custom_compile_options_debug}.")
        message(STATUS "Adding the following linker options specific to 'Debug' build: ${custom_link_options_debug}.")
    endif()

endif()
