set(TOOLCHAIN_LOADED 1)

function(toolchain_after_project_common)
    enable_language(RC)
    include("${CMAKE_SOURCE_DIR}/cmake/DetectArch.cmake")
endfunction()

function(toolchain_exe_stuff_common)
    #-- Configure the Windows application type.

    # Subsystem is already managed by is_win32_app_linker. GCC doesn't need us to specify the entry point.
    #IF (${WIN_SPAWN_CONSOLE})
    #    add_link_options ("LINKER:SHELL:-mconsole")
    #    SET (PREPROCESSOR_DEFS_EXTRA    _WINDOWS_CONSOLE)
    ##ELSE ()
    ##    add_link_options ("LINKER:SHELL:-mwindows")
    #ENDIF ()

    set(cxx_local_opts
        -mno-ms-bitfields # it's needed to fix structure packing
    )

    set(cxx_compiler_options_common ${list_explicit_compiler_options_all} ${cxx_local_opts})

    #-- Apply compiler flags, only the ones specific per build type.

    if(TARGET spheresvr_release)
        target_compile_options(spheresvr_release PUBLIC ${custom_compile_options_release})
    endif()
    if(TARGET spheresvr_nightly)
        target_compile_options(spheresvr_nightly PUBLIC ${custom_compile_options_nightly})
    endif()
    if(TARGET spheresvr_debug)
        target_compile_options(spheresvr_debug PUBLIC ${custom_compile_options_debug})
    endif()

    #-- Store common linker flags.

    set(cxx_linker_options_common
        ${list_explicit_linker_options_all}
        $<$<BOOL:${RUNTIME_STATIC_LINK}>:
        -static-libstdc++
        -static-libgcc> # no way to statically link against libc? maybe we can on windows?
    )

    #-- Apply linker flags, only the ones specific per build type.

    if(TARGET spheresvr_release)
        target_link_options(spheresvr_release PUBLIC ${custom_link_options_release})
    endif()
    if(TARGET spheresvr_nightly)
        target_link_options(spheresvr_nightly PUBLIC ${custom_link_options_nightly})
    endif()
    if(TARGET spheresvr_debug)
        target_link_options(spheresvr_debug PUBLIC ${custom_link_options_debug})
    endif()

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
