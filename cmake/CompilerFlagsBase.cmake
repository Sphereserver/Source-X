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
        -fvisibility=hidden
        -ffunction-sections
        -fdata-sections
    )

    list(
        APPEND
        compiler_options_warnings_base
        -Werror
        -Wall
        -Wuninitialized
        -Wextra
        -Wpedantic
        -Wnarrowing
        -Wno-format         # TODO: not disabling it generates a fair amount of warnings! These are potential bugs to fix!
        #-Wdouble-promotion # TODO: fix warnings.
        #-Wconversion       # TODO: generates TONS of warnings! These are potential bugs to fix!
        #-Wsign-conversion  # Like the comment above.
        -Wdisabled-optimization
        -Winvalid-pch
        -Wshadow
        #-Wcast-qual
        #-Wnrvo
    )

    list(
        APPEND
        compiler_options_warnings_disabled_base
        -Wno-switch
        -Wno-implicit-fallthrough
        -Wno-unused-function
        -Wno-format-security    # TODO:  TODO: remove that when we'll have time to fix every printf format issue
        -Wno-format-nonliteral  # Since -Wformat=2 is stricter, you would need to disable this warning.
        -Wno-misleading-indentation
        -Wno-parentheses
        -Wno-unused-result
        -Wno-deprecated-declarations
    )

    # Linker option flags (minimum).
    list(
        APPEND
        linker_options_base
        -pthread
    )
    #[[
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        list(APPEND linker_options_base     -Wl,-fatal_warnings)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # For some reason, Clang 17 blocks --fatal-warnings if using mold linker, even if it supports the flag.
        #  We have to bypass its flags parsing by passing it directly to the linker
        list(APPEND linker_options_base     -Xlinker --fatal-warnings)
    else()
        list(APPEND linker_options_base     -Wl,--fatal-warnings)
    endif()
    ]]

    message(STATUS "Adding the following base compiler options: ${compiler_options_base}.")
    message(STATUS "Adding the following base compiler warning options: ${compiler_options_warnings_base}.")
    message(STATUS "Adding the following base compiler warning ignore options: ${compiler_options_warnings_disabled_base}.")
    message(STATUS "Adding the following base linker options: ${linker_options_base}.")

    list(
        APPEND
        list_explicit_compiler_options_all
        ${compiler_options_base}
        ${compiler_options_warnings_base}
        ${compiler_options_warnings_disabled_base}
        ${linker_options_base}
    )
    list(
        APPEND
        list_explicit_linker_options_all
        ${linker_options_base}
    )


    #-- Store once the compiler flags, only the ones specific per build type.

    if("${ENABLED_SANITIZER}" STREQUAL "TRUE")
        # -fno-omit-frame-pointer disables a good optimization which might corrupt the debugger stack trace.
        set(local_compile_options_nondebug -ggdb3 -Og -fno-omit-frame-pointer -fno-inline)
        set(local_link_options_nondebug) #-gsplit-dwarf or -gmacro if i want to add lto and there are some missing symbols
    else()
        # If using ThinLTO: -flto=thin -fsplit-lto-unit
        # If using classic/monolithic LTO: -flto=full -fvirtual-function-elimination
        set(local_compile_options_nondebug -O3)# -flto)
        set(local_link_options_nondebug)#-flto)
    endif()

    if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        list(APPEND local_link_options_nondebug -Wl,-dead_strip)
    else()
        list(APPEND local_link_options_nondebug -Wl,--gc-sections)
    endif()


    set(custom_compile_options_release
        ${local_compile_options_nondebug}
        -flto=full
        -fvirtual-function-elimination
        -ffunction-sections
        -fdata-sections
        #-fno-unique-section-names
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
