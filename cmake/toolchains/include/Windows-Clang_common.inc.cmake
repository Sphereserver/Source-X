set(TOOLCHAIN_LOADED 1)

option(
    CLANG_USE_GCC_LINKER
    "NOT CURRENTLY WORKING. By default, Clang requires MSVC (Microsoft's) linker. With this flag, it can be asked to use MinGW-GCC's one."
    FALSE
)

function(toolchain_after_project_common)
    enable_language(RC)
    include("${CMAKE_SOURCE_DIR}/cmake/CMakeDetectArch.cmake")
endfunction()

function(toolchain_exe_stuff_common)
    #-- Configure the Windows application type and add global linker flags.

    if(${WIN32_SPAWN_CONSOLE})
        set(PREPROCESSOR_DEFS_EXTRA _WINDOWS_CONSOLE)
        if(${CLANG_USE_GCC_LINKER})
            # --entry might not work
            add_link_options("LINKER:--entry=WinMainCRTStartup") # Handled by is_win32_app_linker -> "LINKER:SHELL:-m$<IF:$<BOOL:${WIN32_SPAWN_CONSOLE}>,CONSOLE,WINDOWS>"
        else()
            add_link_options("LINKER:/ENTRY:WinMainCRTStartup") # Handled by is_win32_app_linker -> "LINKER:SHELL:/SUBSYSTEM:$<IF:$<BOOL:${WIN32_SPAWN_CONSOLE}>,CONSOLE,WINDOWS>"
        endif()
    endif()

    #-- Validate sanitizers options and store them between the common compiler flags.

    set(ENABLED_SANITIZER false)
    # From https://clang.llvm.org/docs/ClangCommandLineReference.html
    # -static-libsan Statically link the sanitizer runtime (Not supported for ASan, TSan or UBSan on darwin)

    if(${USE_ASAN})
        set(CXX_FLAGS_EXTRA
            ${CXX_FLAGS_EXTRA} # -fsanitize=safe-stack # Can't be used with asan!
            -fsanitize=address
            -fno-sanitize-recover=address #-fsanitize-cfi # cfi: control flow integrity
            -fsanitize-address-use-after-scope
            -fsanitize=pointer-compare
            -fsanitize=pointer-subtract
            # Flags for additional instrumentation not strictly needing asan to be enabled
            -fcf-protection=full
            -fstack-check
            -fstack-protector-all
            -fstack-clash-protection
            # Not supported by Clang, but supported by GCC
            #-fvtable-verify=preinit -fharden-control-flow-redundancy -fhardcfr-check-exceptions
            # Other
            #-fsanitize-trap=all
        )
        if(${CLANG_USE_GCC_LINKER})
            set(CMAKE_EXE_LINKER_FLAGS_EXTRA ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=address)
        endif()
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} ADDRESS_SANITIZER)
        set(ENABLED_SANITIZER true)
    endif()
    if(${USE_MSAN})
        message(FATAL_ERROR "Windows Clang doesn't yet support MSAN")
        set(USE_MSAN false)
        #SET (CXX_FLAGS_EXTRA         ${CXX_FLAGS_EXTRA} -fsanitize=memory -fsanitize-memory-track-origins=2 -fPIE)
        #IF (${CLANG_USE_GCC_LINKER})
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=memory)
        #ENDIF
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} MEMORY_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_LSAN})
        message(FATAL_ERROR "Windows Clang doesn't yet support LSAN")
        set(USE_LSAN false)
        #SET (CXX_FLAGS_EXTRA        ${CXX_FLAGS_EXTRA} -fsanitize=leak)
        #IF (${CLANG_USE_GCC_LINKER})
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=leak)
        #ENDIF
        #SET (PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} LEAK_SANITIZER)
        #SET (ENABLED_SANITIZER true)
    endif()
    if(${USE_UBSAN})
        set(UBSAN_FLAGS
            -fsanitize=undefined,float-divide-by-zero,local-bounds
            -fno-sanitize=enum
            # Supported?
            -fsanitize=unsigned-integer-overflow #Unlike signed integer overflow, this is not undefined behavior, but it is often unintentional.
            -fsanitize=implicit-conversion
        )
        set(CXX_FLAGS_EXTRA ${CXX_FLAGS_EXTRA} ${UBSAN_FLAGS} -fsanitize=return)
        #IF (${CLANG_USE_GCC_LINKER})
        #set (CMAKE_EXE_LINKER_FLAGS_EXTRA     ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -fsanitize=undefined)
        #ENDIF
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} UNDEFINED_BEHAVIOR_SANITIZER)
        set(ENABLED_SANITIZER true)
    endif()

    if(${ENABLED_SANITIZER})
        set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS)
        if(${RUNTIME_STATIC_LINK})
            set(CMAKE_EXE_LINKER_FLAGS_EXTRA ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -static-libsan)
        else()
            set(CMAKE_EXE_LINKER_FLAGS_EXTRA ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -shared-libsan)
        endif()
    endif()

    #-- Store compiler flags common to all builds.

    set(cxx_local_opts_warnings
        -Werror
        -Wall
        -Wextra
        -Wno-unknown-pragmas
        -Wno-switch
        -Wno-implicit-fallthrough
        -Wno-parentheses
        -Wno-misleading-indentation
        -Wno-conversion-null
        -Wno-unused-result
        -Wno-format-security # TODO: disable that when we'll have time to fix every printf format issue
        # clang -specific:
        # -fforce-emit-vtables
    )
    set(cxx_local_opts
        -std=c++20
        -fexceptions
        -fnon-call-exceptions
        -pipe
        -ffast-math
        -mno-ms-bitfields
        # -mno-ms-bitfields is needed to fix structure packing;
        # -pthread unused here? we only need to specify that to the linker?
        -Wno-format-security # TODO: disable that when we'll have time to fix every printf format issue
        # clang-specific:
        #
        # Flags supported by GCC but not by Clang: -fno-expensive-optimizations, -Wno-error=maybe-uninitialized
    )
    set(cxx_compiler_options_common ${cxx_local_opts_warnings} ${cxx_local_opts} ${CXX_FLAGS_EXTRA})

    #-- Apply compiler flags, only the ones specific per build type.

    # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
    set(COMPILE_OPTIONS_EXTRA)
    if(ENABLED_SANITIZER OR TARGET spheresvr_debug)
        set(COMPILE_OPTIONS_EXTRA -fno-omit-frame-pointer -fno-inline)
    endif()
    if(TARGET spheresvr_release)
        target_compile_options(
            spheresvr_release
            PUBLIC -O3 -flto=full -fvirtual-function-elimination ${COMPILE_OPTIONS_EXTRA}
        )
        #[[
        IF (NOT ${CLANG_USE_GCC_LINKER})
            if (${RUNTIME_STATIC_LINK})
                TARGET_COMPILE_OPTIONS ( spheresvr_release    PUBLIC /MT )
            else ()
                TARGET_COMPILE_OPTIONS ( spheresvr_release    PUBLIC /MD )
            endif ()
        endif ()
        ]]
    endif()
    if(TARGET spheresvr_nightly)
        if(ENABLED_SANITIZER)
            target_compile_options(spheresvr_nightly PUBLIC -ggdb3 -O1 ${COMPILE_OPTIONS_EXTRA})
        else()
            target_compile_options(
                spheresvr_nightly
                PUBLIC -O3 -flto=full -fvirtual-function-elimination ${COMPILE_OPTIONS_EXTRA}
            )
        endif()
        #[[
        IF (NOT ${CLANG_USE_GCC_LINKER})
            if (${RUNTIME_STATIC_LINK})
                TARGET_COMPILE_OPTIONS ( spheresvr_nightly    PUBLIC /MT )
            else ()
                TARGET_COMPILE_OPTIONS ( spheresvr_nightly    PUBLIC /MD )
            endif ()
        endif ()
        ]]
    endif()
    if(TARGET spheresvr_debug)
        target_compile_options(spheresvr_debug PUBLIC -ggdb3 -Og)
        #[[
        IF (NOT ${CLANG_USE_GCC_LINKER})
            if (${RUNTIME_STATIC_LINK})
                TARGET_COMPILE_OPTIONS ( spheresvr_debug    PUBLIC /MTd )
            else ()
                TARGET_COMPILE_OPTIONS ( spheresvr_debug    PUBLIC /MDd )
            endif ()
        endif ()
        ]]
    endif()

    #-- Store common linker flags.

    if(${USE_MSAN})
        set(CMAKE_EXE_LINKER_FLAGS_EXTRA ${CMAKE_EXE_LINKER_FLAGS_EXTRA} -pie)
    endif()

    set(cxx_linker_options_common ${CMAKE_EXE_LINKER_FLAGS_EXTRA})
    if(${CLANG_USE_GCC_LINKER})
        set(cxx_linker_options_common ${cxx_linker_options_common} -pthread -dynamic -Wl,--fatal-warnings)
        if(${RUNTIME_STATIC_LINK})
            set(cxx_linker_options_common ${cxx_linker_options_common} -static-libstdc++ -static-libgcc) # no way to statically link against libc? maybe we can on windows?
        endif()
        #[[
    else ()
        if (${RUNTIME_STATIC_LINK})
            set (cxx_linker_options_common    ${cxx_linker_options_common} /MTd )
        else ()
            set (cxx_linker_options_common    ${cxx_linker_options_common} /MDd )
        endif ()
]]
    endif()

    #-- Store common define macros.

    set(cxx_compiler_definitions_common
        ${PREPROCESSOR_DEFS_EXTRA}
        _EXCEPTIONS_DEBUG
        # _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
        #   on a running environment. Also it makes sphere more stable since exceptions are local.
        _CRT_SECURE_NO_WARNINGS
        # _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
        _WINSOCK_DEPRECATED_NO_WARNINGS
        # _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.
    )
    if(NOT ${CMAKE_NO_GIT_REVISION})
        set(cxx_compiler_definitions_common ${cxx_compiler_definitions_common} _GITVERSION)
    endif()

    #-- Apply define macros, only the ones specific per build type.

    if(TARGET spheresvr_release)
        target_compile_definitions(spheresvr_release PUBLIC NDEBUG THREAD_TRACK_CALLSTACK)
    endif()
    if(TARGET spheresvr_nightly)
        target_compile_definitions(spheresvr_nightly PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD)
    endif()
    if(TARGET spheresvr_debug)
        target_compile_definitions(spheresvr_debug PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP)
        if(USE_ASAN AND NOT CLANG_USE_GCC_LINKER)
            target_compile_definitions(spheresvr_debug PUBLIC _HAS_ITERATOR_DEBUGGING=0 _ITERATOR_DEBUG_LEVEL=0)
        endif()
    endif()

    #-- Apply linker options, only the ones specific per build type.

    #if (NOT ${CLANG_USE_GCC_LINKER})
    #    IF (TARGET spheresvr_release)
    #        target_link_options ( spheresvr_release    PUBLIC "LINKER:SHELL:" )
    #    ENDIF ()
    #    IF (TARGET spheresvr_nightly)
    #        target_link_options ( spheresvr_nightly    PUBLIC  "LINKER:SHELL:" )
    #    ENDIF ()
    #    IF (TARGET spheresvr_debug)
    #        target_link_options ( spheresvr_debug    PUBLIC "LINKER:/DEBUG" )
    #    ENDIF ()
    #endif ()

    #-- Now add back the common compiler options, preprocessor macros, linker targets and options.

    if(NOT ${CLANG_USE_GCC_LINKER})
        set(libs_prefix lib)
        #[[
        if (ENABLED_SANITIZER)
            # one should add to the linker path its LLVM lib folder...
            if (ARCH_BITS EQUAL 64)
                if (TARGET spheresvr_release)
                    target_link_libraries(spheresvr_release PUBLIC clang_rt.asan_dll_thunk-x86_64) #clang_rt.asan_dynamic-x86_64)
                endif ()
                if (TARGET spheresvr_nightly)
                    target_link_libraries(spheresvr_nightly PUBLIC clang_rt.asan_dll_thunk-x86_64) #clang_rt.asan_dynamic-x86_64)
                endif ()
                if (TARGET spheresvr_debug)
                    target_link_libraries(spheresvr_debug PUBLIC clang_rt.asan_dll_thunk-x86_64) #clang_rt.asan_dbg_dynamic-x86_64)
                endif ()
            else ()
            if (TARGET spheresvr_release)
                target_link_libraries(spheresvr_release PUBLIC clang_rt.asan_dll_thunk-i386) #clang_rt.asan_dynamic-x86)
            endif ()
            if (TARGET spheresvr_nightly)
                target_link_libraries(spheresvr_nightly PUBLIC clang_rt.asan_dll_thunk-i386) #clang_rt.asan_dynamic-x86)
            endif ()
            if (TARGET spheresvr_debug)
                target_link_libraries(spheresvr_debug PUBLIC clang_rt.asan_dll_thunk-i386) #clang_rt.asan_dbg_dynamic-x86)
            endif ()
                endif()
            endif ()
]]
    endif()
    set(libs_to_link_against ${libs_to_link_against} ws2_32 ${libs_prefix}mariadb)

    foreach(tgt ${TARGETS})
        target_compile_options(${tgt} PRIVATE ${cxx_compiler_options_common})
        target_compile_definitions(${tgt} PRIVATE ${cxx_compiler_definitions_common})
        target_link_options(${tgt} PRIVATE LINKER:${cxx_linker_options_common})
        target_link_libraries(${tgt} PRIVATE ${libs_to_link_against})
    endforeach()

    #-- Set different output folders for each build type
    # (When we'll have support for multi-target builds...)
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"    )
endfunction()
