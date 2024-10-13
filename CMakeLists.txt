#    Main config    #

cmake_minimum_required(VERSION 3.24)
set(CMAKE_SUPPRESS_REGENERATION TRUE) # Supress the ZERO_CHECK generation

# -------------------- Utility functions --------------------

function(booleanize_str_find VAR)
    if(${VAR} EQUAL -1)
        unset(${VAR} PARENT_SCOPE)
    else()
        set(${VAR} 1 PARENT_SCOPE)
    endif()
endfunction()

# -------------------- Start: initializations --------------------

# Should i load the toolchain passed as argument?
if((NOT TOOLCHAIN_LOADED) AND (NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL ""))
    include(${CMAKE_TOOLCHAIN_FILE})
    toolchain_force_compiler()
endif()

message(STATUS "Scanning system build tools...")
project(SphereServer CXX C) # does a scan for C++ and C compilers

# If we have not specified a toolchain, let's detect which one we should use, using the detected arch.
# We need to do this after "project", otherwise CMake doesn't know where it's running.
if(NOT TOOLCHAIN_LOADED)
    include("cmake/DetectDefaultToolchain.cmake")
endif()

# Setup global flags, to be done before creating targets.

if(NOT DEFINED CMAKE_CXX_STANDARD)
    # only set CMAKE_CXX_STANDARD if not already set
    # this allows the standard to be set by the caller, for example with -DCMAKE_CXX_STANDARD:STRING=20
    set(CMAKE_CXX_STANDARD 20)
elseif(CMAKE_CXX_STANDARD LESS 20)
    message(WARNING "Current C++ standard ({CMAKE_CXX_STANDARD}) too low, defaulting to the minimum supported, 20.")
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is >700 KB bigger.
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# In some cases, CMake also sets other predefined flags, which we don't want.
#[[
set(CMAKE_C_FLAGS "")
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_NIGHTLY "")
set(CMAKE_C_FLAGS_RELEASE "")

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_NIGHTLY "")
set(CMAKE_CXX_FLAGS_RELEASE "")

set(CMAKE_EXE_LINKER_FLAGS "")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "")
]]
# Example: using MSVC, /RTC1 is globally added (all projects) to DEBUG builds. It's mostly fine, except in some cases where we don't want it...
# We might want to delete only /RTC* flags, instead of erasing all of them altogether.
#STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
#STRING (REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")

# Also, it fails to create those predefined flags, which it needs, for custom build types (e.g. Nightly).
set(CMAKE_EXE_LINKER_FLAGS_NIGHTLY "")

# --------------------  GUI checkboxes --------------------

option(
    CMAKE_NO_GIT_REVISION
    "Do not try to retrieve the current git revision. Useful for building source not git-cloned from Github."
    FALSE
)
option(CROSSCOMPILE "Are we compiling for a different target architecture?" FALSE)
option(
    RUNTIME_STATIC_LINK
    "Statically link inside the executable the runtime libraries? (MSVC, libc/libcc, libstdc++)."
    FALSE
)
option(CMAKE_EXPORT_COMPILE_COMMANDS "Export compiler commands to compile_commands.json" TRUE)

set(predef_LIB_LIBEV_BUILD FALSE)
if(NOT WIN32)
    set(predef_LIB_LIBEV_BUILD TRUE)
endif()
option(LIB_LIBEV_BUILD "Build libev." ${predef_LIB_LIBEV_BUILD})
option(LIB_SQLITE_BUILD "Build sqlite." TRUE)
option(LIB_ZLIB_BUILD "Build zlib." TRUE)

if(WIN32)
    option(WIN_SPAWN_CONSOLE "Spawn an additional console, useful for debugging." FALSE)
    if(MSVC)
        option(WIN_GENERATE_CRASHDUMP "For non-Debug builds, add Windows Structured Exception Handling and generate a Crash Dump if a fatal SE is thrown." FALSE)
    endif()
endif()

if(NOT MSVC)
    option(USE_COMPILER_HARDENING_OPTIONS "Enable compiler (even runtime) safety checks and code hardening." FALSE)
endif()
option(USE_ASAN "Enable AddressSanitizer." FALSE)
option(USE_UBSAN "Enable Undefined Behavior Sanitizer." FALSE)
option(USE_LSAN "Enable LeakSanitizer." FALSE)
option(USE_MSAN "Enable MemorySanitizer." FALSE)

set(ENABLED_SANITIZER CACHE INTERNAL BOOL FALSE)
if(USE_ASAN OR USE_MSAN OR USE_LSAN OR USE_MSAN)
    set(ENABLED_SANITIZER TRUE)
endif()


# -------------------- Stuff to set right away, after having parsed the checkboxes/CLI arguments.

set(is_win32_app_linker)
if(WIN32 AND NOT ${WIN_SPAWN_CONSOLE})
    set(is_win32_app_linker WIN32) # if not set, it defaults to console subsystem
endif()


# -------------------- Stuff that needs to be executed after PROJECT but before ADD_EXECUTABLE

message(STATUS)
toolchain_after_project()

string(FIND "${CMAKE_GENERATOR}" "Makefiles" GEN_IS_MAKEFILE)
string(FIND "${CMAKE_GENERATOR}" "Ninja" GEN_IS_NINJA)
booleanize_str_find(GEN_IS_MAKEFILE)
booleanize_str_find(GEN_IS_NINJA)

if((GEN_IS_MAKEFILE OR GEN_IS_NINJA) AND (NOT MSVC))
    set(SINGLE_TARGET 1)
endif()

# TODO: check if Ninja can handle different targets.
if(SINGLE_TARGET)
    # If you want to manually specify the build type, call cmake with parameter: -DCMAKE_BUILD_TYPE=something
    # TODO: add NightlyWithDebInfo ?
    message(STATUS "Single-target build system (${CMAKE_GENERATOR}) detected: generating multiple projects!")
    set (CMAKE_BUILD_TYPE "Nightly" CACHE STRING "Set the build type. Expected values: Debug, Nightly or Release.") # Set only if unset.
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Nightly" "Release")
    if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
        if(
            (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
            AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
            AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly")
        )
            message(WARNING "Invalid parameter CMAKE_BUILD_TYPE (${CMAKE_BUILD_TYPE}), defaulting to Nightly.")
            # -> needed only for MAKEFILE-STYLE generators, which can't switch between different configs
            set(CMAKE_BUILD_TYPE "Nightly" CACHE STRING "" FORCE)
        else()
            message(STATUS "Generating only specified project: ${CMAKE_BUILD_TYPE}.")
        endif()
    else()
        message(STATUS "No target specified: building all the projects (Release, Debug, Nightly).")
        # The only situation supported here is using MSVC, and /MP (multi-core) is set in its compiler flags.
    endif()

    if(GEN_IS_MAKEFILE)
        # Setting parallel make; ninja does that by default.
        include(ProcessorCount)
        processorcount(MAKE_THREADS)
        if(NOT MAKE_THREADS EQUAL 0)
            math(EXPR MAKE_THREADS "${MAKE_THREADS} + (${MAKE_THREADS}/2)") # Suggested number of threads: cores * 1.5
            set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MAKE_THREADS}")
        else()
            message(STATUS "Can't determine CPU cores number. Parallel compilation turned off.")
        endif()
    endif()
else(SINGLE_TARGET)
    message(STATUS "Multi-target build system detected: generating single project with multiple targets!")
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Nightly" CACHE STRING "" FORCE)
endif(SINGLE_TARGET)

# Include the list of all Sphere source files
include("src/CMakeSources.cmake")

# Configure output binaries
set(TARGETS "" CACHE INTERNAL "List of Sphere targets to build.")
if(SINGLE_TARGET)
    if(("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(R|r?)elease"))
        set(TARGETS ${TARGETS} spheresvr_release)
        add_executable(
            spheresvr_release
            ${is_win32_app_linker}
            ${SPHERE_SOURCES}
            #${docs_TEXT}
        )
        set_target_properties(spheresvr_release PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_release)
        #set_target_properties(spheresvr_release PROPERTIES CXX_EXTENSIONS OFF)
        #target_compile_features(spheresvr_release PUBLIC cxx_std_20)
        target_precompile_headers(spheresvr_release ${pch_options})
    endif()
    if(("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(N|n?)ightly"))
        set(TARGETS ${TARGETS} spheresvr_nightly)
        add_executable(
            spheresvr_nightly
            ${is_win32_app_linker}
            ${SPHERE_SOURCES}
            #${docs_TEXT}
        )
        set_target_properties(spheresvr_nightly PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_nightly)
        #set_target_properties(spheresvr_nightly PROPERTIES CXX_EXTENSIONS OFF)
        #target_compile_features(spheresvr_nightly PUBLIC cxx_std_20)
        target_precompile_headers(spheresvr_nightly ${pch_options})
    endif()
    if(("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(D|d?)ebug"))
        set(TARGETS ${TARGETS} spheresvr_debug)
        add_executable(
            spheresvr_debug
            ${is_win32_app_linker}
            ${SPHERE_SOURCES}
            #${docs_TEXT}
        )
        set_target_properties(spheresvr_debug PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_debug)
        #set_target_properties(spheresvr_debug PROPERTIES CXX_EXTENSIONS OFF)
        #target_compile_features(spheresvr_debug PUBLIC cxx_std_20)
        target_precompile_headers(spheresvr_debug ${pch_options})
    endif()
else(SINGLE_TARGET)
    set(TARGETS ${TARGETS} spheresvr)
    add_executable(spheresvr ${is_win32_app_linker} ${SPHERE_SOURCES} ${docs_TEXT})
    set_target_properties(spheresvr PROPERTIES OUTPUT_NAME_RELEASE SphereSvrX${ARCH_BITS}_release)
    set_target_properties(spheresvr PROPERTIES OUTPUT_NAME_NIGHTLY SphereSvrX${ARCH_BITS}_nightly)
    set_target_properties(spheresvr PROPERTIES OUTPUT_NAME_DEBUG SphereSvrX${ARCH_BITS}_debug)
    #set_target_properties(spheresvr PROPERTIES CXX_EXTENSIONS OFF)
    #target_compile_features(spheresvr PUBLIC cxx_std_20)
    target_precompile_headers(spheresvr ${pch_options})
endif(SINGLE_TARGET)

# --------------------

message(STATUS)
include("${CMAKE_SOURCE_DIR}/cmake/CompilerFlagsChecker.cmake")

# --------------------

if(USE_ASAN)
    set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} ADDRESS_SANITIZER)
endif()
if(USE_UBSAN)
    set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} UNDEFINED_BEHAVIOR_SANITIZER)
endif()
if(USE_LSAN)
    set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} LEAK_SANITIZER)
endif()
if(USE_MSAN)
    set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} MEMORY_SANITIZER)
endif()
if(ENABLED_SANITIZER)
    set(PREPROCESSOR_DEFS_EXTRA ${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS)
endif()

# --------------------

# Now include external libraries

message(STATUS)
message(STATUS "Configuring external libraries...")
add_subdirectory(lib)

toolchain_exe_stuff() # stuff to be executed after ADD_EXECUTABLE

# Get the Git revision number
message(STATUS)
message(STATUS "Retrieving git data...")
include("cmake/GitStatus.cmake")
message(STATUS)
