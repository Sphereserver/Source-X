#	Main config	#

CMAKE_MINIMUM_REQUIRED (VERSION 3.19)
set (CMAKE_SUPPRESS_REGENERATION TRUE)	# Supress the ZERO_CHECK generation

# GUI checkboxes
option (CMAKE_NO_GIT_REVISION
	"Do not try to retrieve the current git revision. Useful for building source not git-cloned from Github."
	FALSE
)
option (RUNTIME_STATIC_LINK
	"Statically link inside the executable the runtime libraries? (MSVC, libc/libcc, libstdc++)."
	FALSE
)

set (predef_LIB_LIBEV_BUILD 	FALSE)
if (NOT WIN32)
	set (predef_LIB_LIBEV_BUILD	TRUE)
endif()
option (LIB_LIBEV_BUILD			"Build libev." ${predef_LIB_LIBEV_BUILD})
option (LIB_SQLITE_BUILD		"Build sqlite." TRUE)
option (LIB_ZLIB_BUILD			"Build zlib." TRUE)

if (WIN32)
	option (WIN32_SPAWN_CONSOLE	"Spawn an additional console, useful for debugging." FALSE)
endif()

option (USE_ASAN				"Enable AddressSanitizer." FALSE)
option (USE_MSAN				"Enable MemorySanitizer." FALSE)
option (USE_LSAN				"Enable LeakSanitizer." FALSE)
option (USE_UBSAN				"Enable Undefined Behavior Sanitizer." FALSE)


# Utility functions
FUNCTION (booleanize_str_find VAR)
	IF (${VAR} EQUAL -1)
		UNSET (${VAR} PARENT_SCOPE)
	ELSE ()
		SET (${VAR} 1 PARENT_SCOPE)
	ENDIF ()
ENDFUNCTION ()


# -- Start: initializations

# Should i load the toolchain passed as argument?
IF ((NOT TOOLCHAIN_LOADED) AND (NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL ""))
	INCLUDE (${CMAKE_TOOLCHAIN_FILE})
	toolchain_force_compiler ()
ENDIF ()


MESSAGE (STATUS "Scanning system build tools...")

PROJECT (SphereServer CXX C)	# does a scan for C++ and C compilers

# Setup global flags, to be done before creating targets.

if (NOT DEFINED CMAKE_CXX_STANDARD)
  # only set CMAKE_CXX_STANDARD if not already set
  # this allows the standard to be set by the caller, for example with -DCMAKE_CXX_STANDARD:STRING=20
    set (CMAKE_CXX_STANDARD 20)
elseif (CMAKE_CXX_STANDARD LESS 20)
    message (WARNING "Current C++ standard ({CMAKE_CXX_STANDARD}) too low, defaulting to the minimum supported, 20.")
    set (CMAKE_CXX_STANDARD 20)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is >700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

# CMake also sets other predefined flags, which we don't want...
SET (CMAKE_C_FLAGS "")
SET (CMAKE_C_FLAGS_DEBUG "")
SET (CMAKE_C_FLAGS_NIGHTLY "")
SET (CMAKE_C_FLAGS_RELEASE "")

SET (CMAKE_CXX_FLAGS "")
SET (CMAKE_CXX_FLAGS_DEBUG "")
SET (CMAKE_CXX_FLAGS_NIGHTLY "")
SET (CMAKE_CXX_FLAGS_RELEASE "")

SET (CMAKE_EXE_LINKER_FLAGS "")
SET (CMAKE_EXE_LINKER_FLAGS_DEBUG "")
SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY "")
SET (CMAKE_EXE_LINKER_FLAGS_RELEASE "")

set (is_win32_app_linker)
IF (WIN32 AND NOT ${WIN32_SPAWN_CONSOLE})
	set (is_win32_app_linker WIN32)	# if not set, it defaults to console subsystem
ENDIF ()


# If we have not specified a toolchain, let's detect which one we should use, using the detected arch.
IF (NOT TOOLCHAIN_LOADED)
	INCLUDE ("cmake/CMakeDefaultToolchain.cmake")
ENDIF ()


#-- Stuff that needs to be executed after PROJECT but before ADD_EXECUTABLE
toolchain_after_project()

STRING (FIND "${CMAKE_GENERATOR}" "Makefiles" GEN_IS_MAKEFILE)
STRING (FIND "${CMAKE_GENERATOR}" "Ninja" GEN_IS_NINJA)
booleanize_str_find(GEN_IS_MAKEFILE)
booleanize_str_find(GEN_IS_NINJA)

IF ( (GEN_IS_MAKEFILE OR GEN_IS_NINJA) AND (NOT MSVC) )
	SET (SINGLE_TARGET 1)
ENDIF ()

# TODO: check if Ninja can handle different targets.
IF (SINGLE_TARGET)
	# If you want to manually specify the build type, call cmake with parameter: -DCMAKE_BUILD_TYPE=something

	MESSAGE (STATUS "Single-target build system (${CMAKE_GENERATOR}) detected: generating multiple projects!")
	IF (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
		IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
			MESSAGE (WARNING "Invalid parameter -DCMAKE_BUILD_TYPE, defaulting to Nightly.")
			# -> needed only for MAKEFILE-STYLE generators, which can't switch between different configs
			SET (CMAKE_BUILD_TYPE "Nightly" CACHE STRING "" FORCE)
		ELSE ()
			MESSAGE (STATUS "Generating only specified project: ${CMAKE_BUILD_TYPE}.")
		ENDIF ()
	ELSE ()
		MESSAGE (STATUS "No target specified: building all the projects (Release, Debug, Nightly).")
		# The only situation supported here is using MSVC, and /MP (multi-core) is set in its compiler flags.
	ENDIF ()

	IF (GEN_IS_MAKEFILE)
		# Setting parallel make; ninja does that by default.
		INCLUDE (ProcessorCount)
		ProcessorCount (MAKE_THREADS)
		IF (NOT MAKE_THREADS EQUAL 0)
			MATH (EXPR MAKE_THREADS "${MAKE_THREADS} + (${MAKE_THREADS}/2)")		# Suggested number of threads: cores * 1.5
			SET (CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MAKE_THREADS}")
		ELSE ()
			MESSAGE (STATUS "Can't determine CPU cores number. Parallel compilation turned off.")
		ENDIF ()
	ENDIF ()

ELSE (SINGLE_TARGET)

    MESSAGE (STATUS "Multi-target build system detected: generating single project with multiple targets!")
    SET (CMAKE_CONFIGURATION_TYPES "Debug;Release;Nightly" CACHE STRING "" FORCE)

ENDIF (SINGLE_TARGET)


# Include the list of all Sphere source files
include("src/CMakeSources.cmake")

# Configure output binaries
set(TARGETS "" CACHE INTERNAL "List of Sphere targets to build.")
IF (SINGLE_TARGET)

	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(R|r?)elease"))
		SET (TARGETS ${TARGETS} spheresvr_release)
		ADD_EXECUTABLE (spheresvr_release ${is_win32_app_linker}
			${SPHERE_SOURCES}
		#	${docs_TEXT}
		)
			SET_TARGET_PROPERTIES (spheresvr_release PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_release)
	ENDIF ()
	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(N|n?)ightly"))
		SET (TARGETS ${TARGETS} spheresvr_nightly)
		ADD_EXECUTABLE (spheresvr_nightly ${is_win32_app_linker}
			${SPHERE_SOURCES}
		#	${docs_TEXT}
		)
			SET_TARGET_PROPERTIES (spheresvr_nightly PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_nightly)
	ENDIF ()
	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(D|d?)ebug"))
		SET (TARGETS ${TARGETS} spheresvr_debug)
		ADD_EXECUTABLE (spheresvr_debug ${is_win32_app_linker}
			${SPHERE_SOURCES}
		#	${docs_TEXT}
		)
		SET_TARGET_PROPERTIES (spheresvr_debug PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_debug)
	ENDIF ()

ELSE (SINGLE_TARGET)

	SET (TARGETS ${TARGETS} spheresvr)
	ADD_EXECUTABLE (spheresvr ${is_win32_app_linker}
		${SPHERE_SOURCES}
		${docs_TEXT}
	)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_RELEASE SphereSvrX${ARCH_BITS}_release)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_NIGHTLY SphereSvrX${ARCH_BITS}_nightly)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_DEBUG   SphereSvrX${ARCH_BITS}_debug)

ENDIF (SINGLE_TARGET)

# Now include external libraries
add_subdirectory(lib)

toolchain_exe_stuff()   # stuff to be executed after ADD_EXECUTABLE

# Get the Git revision number
INCLUDE ("cmake/CMakeGitStatus.cmake")

