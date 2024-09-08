set(TOOLCHAIN_LOADED 1)

function(toolchain_after_project_common)
    enable_language(RC)
    include("${CMAKE_SOURCE_DIR}/cmake/CMakeDetectArch.cmake")
endfunction()

function(toolchain_exe_stuff_common)
    #-- Configure the Windows application type.

    # Subsystem is already managed by is_win32_app_linker. GCC doesn't need us to specify the entry point.
    #IF (${WIN32_SPAWN_CONSOLE})
    #    add_link_options ("LINKER:SHELL:-mconsole")
    #    SET (PREPROCESSOR_DEFS_EXTRA    _WINDOWS_CONSOLE)
    ##ELSE ()
    ##    add_link_options ("LINKER:SHELL:-mwindows")
    #ENDIF ()

    #-- Validate sanitizers options and store them between the common compiler flags.

    set(ENABLED_SANITIZER false)
    if(${USE_ASAN})
        message(FATAL_ERROR "MinGW-GCC doesn't yet support ASAN")
        set(USE_ASAN false)
        #[[
    SET (CXX_FLAGS_EXTRA    ${CXX_FLAGS_EXTRA}
      -fsanitize=address -fno-sanitize-recover=address #-fsanitize-cfi # cfi: control flow integrity, not currently supported by GCC (even on Linux)
      -fsanitize-address-use-after-scope -fsanitize=pointer-compare -fsanitize=pointer-subtract
      # Flags for additional instrumentation not strictly needing asan to be enabled
      #-fvtable-verify=preinit # This causes a GCC internal error! Tested with 13.2.0
      -fstack-check -fstack-protector-all
      -fcf-protection=full
      # GCC 14?
      #-fharden-control-flow-redundancy -fhardcfr-check-exceptions
      # Other
      #-fsanitize-trap=all
    )
    ]]
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=address $<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libasan>)
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} ADDRESS_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_MSAN})
        message(FATAL_ERROR "MinGW-GCC doesn't yet support MSAN")
        set(USE_MSAN false)
        #SET (CXX_FLAGS_EXTRA     ${CXX_FLAGS_EXTRA} -fsanitize=memory -fsanitize-memory-track-origins=2 -fPIE)
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=memory )#$<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libmsan>)
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} MEMORY_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_LSAN})
        message(FATAL_ERROR "MinGW-GCC doesn't yet support LSAN")
        set(USE_LSAN false)
        #SET (CXX_FLAGS_EXTRA     ${CXX_FLAGS_EXTRA} -fsanitize=leak)
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=leak  #$<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-liblsan>)
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} LEAK_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_UBSAN})
        message(FATAL_ERROR "MinGW-GCC doesn't yet support UBSAN")
        set(USE_UBSAN false)
        #[[
    SET (UBSAN_FLAGS
            -fsanitize=undefined,float-divide-by-zero
            -fno-sanitize=enum
            # Unsupported (yet?) by GCC 13
            #-fsanitize=unsigned-integer-overflow #Unlike signed integer overflow, this is not undefined behavior, but it is often unintentional.
      #-fsanitize=implicit-conversion, local-bounds
        )
    ]]
        #SET (CXX_FLAGS_EXTRA     ${CXX_FLAGS_EXTRA} ${UBSAN_FLAGS} -fsanitize=return,vptr)
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=undefined #$<$<BOOL:${RUNTIME_STATIC_LINK}>:-static-libubsan>)
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} UNDEFINED_BEHAVIOR_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()

    #IF (${ENABLED_SANITIZER})
    #    SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS)
    #ENDIF ()

    #-- Store compiler flags common to all builds.

    set(cxx_local_opts_warnings
        -Werror
        -Wall
        -Wextra
        -Wno-nonnull-compare
        -Wno-unknown-pragmas
        -Wno-switch
        -Wno-implicit-fallthrough
        -Wno-parentheses
        -Wno-misleading-indentation
        -Wno-conversion-null
        -Wno-unused-result
        -Wno-format-security # TODO: disable that when we'll have time to fix every printf format issue
    )
    set(cxx_local_opts
        -std=c++20
        -pthread
        -fexceptions
        -fnon-call-exceptions
        -mno-ms-bitfields
        # -mno-ms-bitfields is needed to fix structure packing
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
            target_compile_options(spheresvr_nightly PUBLIC -ggdb3 -O1 ${COMPILE_OPTIONS_EXTRA})
        else()
            target_compile_options(spheresvr_nightly PUBLIC -O3 ${COMPILE_OPTIONS_EXTRA})
        endif()
    endif()
    if(TARGET spheresvr_debug)
        target_compile_options(spheresvr_debug PUBLIC -ggdb3 -Og ${COMPILE_OPTIONS_EXTRA})
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
        -static-libgcc> # no way to statically link against libc? maybe we can on windows?
    )

    #-- Store common define macros.

    set(cxx_compiler_definitions_common
        ${PREPROCESSOR_DEFS_EXTRA}
        $<$<NOT:$<BOOL:${CMAKE_NO_GIT_REVISION}>>:_GITVERSION>
        _EXCEPTIONS_DEBUG
        # _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
        #   on a running environment. Also it makes sphere more stable since exceptions are local.
        _CRT_SECURE_NO_WARNINGS
        # _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
        _WINSOCK_DEPRECATED_NO_WARNINGS
        # _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.
    )

    #-- Apply define macros, only the ones specific per build type.

    if(TARGET spheresvr_release)
        target_compile_definitions(spheresvr_release PUBLIC NDEBUG THREAD_TRACK_CALLSTACK)
    endif()
    if(TARGET spheresvr_nightly)
        target_compile_definitions(spheresvr_nightly PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD)
    endif()
    if(TARGET spheresvr_debug)
        target_compile_definitions(spheresvr_debug PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP)
    endif()

    #-- Now apply the common compiler options, preprocessor macros, linker options.

    foreach(tgt ${TARGETS})
        target_compile_options(${tgt} PRIVATE ${cxx_compiler_options_common})
        target_compile_definitions(${tgt} PRIVATE ${cxx_compiler_definitions_common})
        target_link_options(${tgt} PRIVATE ${cxx_linker_options_common})
        target_link_libraries(${tgt} PRIVATE mariadb ws2_32)
    endforeach()

    #-- Set different output folders for each build type
    # (When we'll have support for multi-target builds...)
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"    )
endfunction()
