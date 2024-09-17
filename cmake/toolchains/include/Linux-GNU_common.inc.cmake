set(TOOLCHAIN_LOADED 1)

function(toolchain_after_project_common)
    include("${CMAKE_SOURCE_DIR}/cmake/DetectArch.cmake")
endfunction()

function(toolchain_exe_stuff_common)
    #-- Find libraries to be linked to.

    message(STATUS "Locating libraries to be linked to...")

    set(LIBS_LINK_LIST mariadb dl)
    foreach(LIB_NAME ${LIBS_LINK_LIST})
        find_library(LIB_${LIB_NAME}_WITH_PATH ${LIB_NAME} PATH ${lib_search_paths})
        message(STATUS "Library ${LIB_NAME}: ${LIB_${LIB_NAME}_WITH_PATH}")
    endforeach()

    #-- Validate sanitizers options and store them between the common compiler flags.

    set(ENABLED_SANITIZER false)

    if(${USE_ASAN})
        set(CXX_FLAGS_EXTRA
            ${CXX_FLAGS_EXTRA}
            -fsanitize=address
            -fno-sanitize-recover=address
            #-fsanitize-cfi # cfi: control flow integrity, not supported by GCC
            -fsanitize-address-use-after-scope
            -fsanitize=pointer-compare
            -fsanitize=pointer-subtract
            # Flags for additional instrumentation not strictly needing asan to be enabled
            #-fvtable-verify=preinit # This causes a GCC internal error! Tested with 13.2.0
            -fstack-check
            -fstack-protector-all
            -fcf-protection=full
            # GCC 14?
            #-fharden-control-flow-redundancy
            #-fhardcfr-check-exceptions
            # Other
            #-fsanitize-trap=all
        )
        set(CMAKE_EXE_LINKER_FLAGS_EXTRA
            ${CMAKE_EXE_LINKER_FLAGS_EXTRA}
            -fsanitize=address
            $<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libasan>
        )
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} ADDRESS_SANITIZER)
        set(ENABLED_SANITIZER true)
    endif()
    if(${USE_MSAN})
        message(FATAL_ERROR "Linux GCC doesn't yet support MSAN")
        set(USE_MSAN false)
        #SET (CXX_FLAGS_EXTRA     ${CXX_FLAGS_EXTRA} -fsanitize=memory -fsanitize-memory-track-origins=2 -fPIE)
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=memory )#$<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libmsan>)
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} MEMORY_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_LSAN})
        set(CXX_FLAGS_EXTRA ${CXX_FLAGS_EXTRA} -fsanitize=leak)
        set(CMAKE_EXE_LINKER_FLAGS_EXTRA
            ${CMAKE_EXE_LINKER_FLAGS_EXTRA}
            -fsanitize=leak
            $<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-liblsan>
        )
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} LEAK_SANITIZER)
        set(ENABLED_SANITIZER true)
    endif()
    if(${USE_UBSAN})
        set(UBSAN_FLAGS
            -fsanitize=undefined,float-divide-by-zero
            -fno-sanitize=enum
            # Unsupported (yet?) by GCC 13
            #-fsanitize=unsigned-integer-overflow #Unlike signed integer overflow, this is not undefined behavior, but it is often unintentional.
            #-fsanitize=implicit-conversion, local-bounds
        )
        set(CXX_FLAGS_EXTRA ${CXX_FLAGS_EXTRA} ${UBSAN_FLAGS} -fsanitize=return,vptr)
        set(CMAKE_EXE_LINKER_FLAGS_EXTRA
            ${CMAKE_EXE_LINKER_FLAGS_EXTRA}
            -fsanitize=undefined
            $<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libubsan>
        )
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} UNDEFINED_BEHAVIOR_SANITIZER)
        set(ENABLED_SANITIZER true)
    endif()

    if(${ENABLED_SANITIZER})
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS)
    endif()

    #-- Store compiler flags common to all builds.

    set(cxx_local_opts_warnings
        -Werror
        -Wall
        -Wextra
        -Wpedantic

        -Wmissing-include-dirs # Warns when an include directory provided with -I does not exist.
        -Wformat=2
        #-Wcast-qual # Warns about casts that remove a type's const or volatile qualifier.
        #-Wconversion # Temporarily disabled. Warns about implicit type conversions that might change a value, such as narrowing conversions.
        -Wdisabled-optimization
        #-Winvalid-pch
        -Wzero-as-null-pointer-constant
        #-Wnull-dereference # Don't: on GCC 12 causes some false positives...
        -Wduplicated-cond

        # Supported by Clang, but unsupported by GCC:
        #-Wweak-vtables

        # Unsupported by Clang, but supported by GCC:
        -Wtrampolines # Warns when trampolines (a technique to implement nested functions) are generated (don't want this for security reasons).
        -Wvector-operation-performance
        -Wsized-deallocation
        -Wduplicated-cond
        -Wshift-overflow=2

        # Disable errors:
        -Wno-format-nonliteral # Since -Wformat=2 is stricter, you would need to disable this warning.
        -Wno-nonnull-compare # GCC only
        -Wno-switch
        -Wno-implicit-fallthrough
        -Wno-parentheses
        -Wno-misleading-indentation
        -Wno-unused-result
        -Wno-format-security # TODO: disable that when we'll have time to fix every printf format issue
        -Wno-nested-anon-types
    )
    set(cxx_local_opts
        -std=c++20
        -pthread
        -fexceptions
        -fnon-call-exceptions
        -pipe
        -ffast-math
    )
    set(cxx_compiler_options_common ${cxx_local_opts_warnings} ${cxx_local_opts} ${CXX_FLAGS_EXTRA})

    #-- Apply compiler flags, only the ones specific per build type.

    # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
    set(COMPILE_OPTIONS_EXTRA)
    if(ENABLED_SANITIZER OR TARGET spheresvr_debug)
        set(COMPILE_OPTIONS_EXTRA -fno-omit-frame-pointer -fno-inline)
    endif()
    if(TARGET spheresvr_release)
        target_compile_options(spheresvr_release PUBLIC -s -O3 ${COMPILE_OPTIONS_EXTRA})
    endif()
    if(TARGET spheresvr_nightly)
        if(ENABLED_SANITIZER)
            target_compile_options(spheresvr_nightly PUBLIC -ggdb3 -Og ${COMPILE_OPTIONS_EXTRA})
        else()
            target_compile_options(spheresvr_nightly PUBLIC -O3 ${COMPILE_OPTIONS_EXTRA})
        endif()
    endif()
    if(TARGET spheresvr_debug)
        target_compile_options(spheresvr_debug PUBLIC -ggdb3 -O0 ${COMPILE_OPTIONS_EXTRA})
    endif()

    #-- Store common linker flags.

    if(${USE_MSAN})
        set(CMAKE_EXE_LINKER_FLAGS_EXTRA ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -pie)
    endif()
    set(cxx_linker_options_common
        ${CMAKE_EXE_LINKER_FLAGS_EXTRA}
        -pthread
        -dynamic
        -Wl,--fatal-warnings
        $<$<BOOL:${RUNTIME_STATIC_LINK}>:
        -static-libstdc++
        -static-libgcc # no way to safely statically link against libc
        #$<$<BOOL:${ENABLED_SANITIZER}>: -fsanitize-link-runtime -fsanitize-link-c++-runtime>
        >
    )

    #-- Store common define macros.

    set(cxx_compiler_definitions_common
        ${PREPROCESSOR_DEFS_EXTRA}
        $<$<NOT:$<BOOL:${CMAKE_NO_GIT_REVISION}>>:_GITVERSION>
        _EXCEPTIONS_DEBUG
        # _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
        #   on a running environment. Also it makes sphere more stable since exceptions are local.
    )

    #-- Apply define macros, only the ones specific per build type.

    if(TARGET spheresvr_release)
        target_compile_definitions(spheresvr_release PUBLIC NDEBUG)
    endif(TARGET spheresvr_release)
    if(TARGET spheresvr_nightly)
        target_compile_definitions(spheresvr_nightly PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD)
    endif(TARGET spheresvr_nightly)
    if(TARGET spheresvr_debug)
        target_compile_definitions(spheresvr_debug PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP)
    endif(TARGET spheresvr_debug)

    #-- Now apply the common compiler options, preprocessor macros, linker options.

    foreach(tgt ${TARGETS})
        target_compile_options(${tgt} PRIVATE ${cxx_compiler_options_common})
        target_compile_definitions(${tgt} PRIVATE ${cxx_compiler_definitions_common})
        target_link_options(${tgt} PRIVATE ${cxx_linker_options_common})
        target_link_libraries(${tgt} PRIVATE ${LIB_mariadb_WITH_PATH} ${LIB_dl_WITH_PATH})
    endforeach()

    #-- Set different output folders for each build type
    # (When we'll have support for multi-target builds...)
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"    )
endfunction()
