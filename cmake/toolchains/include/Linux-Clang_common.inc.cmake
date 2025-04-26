set(TOOLCHAIN_LOADED 1)

function(toolchain_after_project_common)
    include("${CMAKE_SOURCE_DIR}/cmake/DetectArch.cmake")
endfunction()

function(toolchain_exe_stuff_common)
    #-- Find libraries to be linked to.

    message(STATUS)
    message(STATUS "Locating libraries to be linked to...")

    set(libs_link_list mariadb dl)
    foreach(lib_name ${libs_link_list})
        find_library(lib_${lib_name}_with_path ${lib_name} PATH ${lib_search_paths})
        message(STATUS "Library ${lib_name}: ${lib_${lib_name}_with_path}")
    endforeach()

    #-- Validate sanitizers options and store them between the common compiler flags.

    # From https://clang.llvm.org/docs/ClangCommandLineReference.html
    # -static-libsan Statically link the sanitizer runtime (Not supported for ASan, TSan or UBSan on Darwin)

    #string(REPLACE ";" " " CXX_FLAGS_EXTRA "${CXX_FLAGS_EXTRA}")

    set(cxx_compiler_options_common ${list_explicit_compiler_options_all})
    #separate_arguments(cxx_compiler_options_common)

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
        -static-libgcc> # no way to safely statically link against libc
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

    #-- Now add back the common compiler options, preprocessor macros, linker targets and options.

    foreach(tgt ${TARGETS})
        target_compile_options(${tgt} PRIVATE ${cxx_compiler_options_common})
        target_compile_definitions(${tgt} PRIVATE ${cxx_compiler_definitions_common})
        target_link_options(${tgt} PRIVATE ${cxx_linker_options_common})
        target_link_libraries(${tgt} PRIVATE ${lib_mariadb_with_path} ${lib_dl_with_path})
    endforeach()

    #-- Set different output folders for each build type
    # (When we'll have support for multi-target builds...)
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"    )
    #SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"    )
endfunction()
