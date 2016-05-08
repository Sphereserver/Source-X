#	Main config	#

CMAKE_MINIMUM_REQUIRED (VERSION 2.8.10)


MESSAGE (STATUS "Scanning system for compilers...")
PROJECT (SphereServer)	# does a scan for C++ and C compilers


# If we have not specified a toolchain, let's detect which one we should use
IF (NOT TOOLCHAIN)

MESSAGE (STATUS "Toolchain not specified. Detecting the one to use.")
IF (MSVC)
	INCLUDE ("cmake/toolchains/Windows-MSVC.cmake")
ELSE (MSVC)
	MESSAGE (STATUS "Toolchain: defaulting to 32 bits compiler.")
	# INCLUDE ("cmake/CMakeDetectArch.cmake")
	# target_architecture("ARCH_DETECT")

	IF (UNIX)
		#IF (ARCH_DETECT STREQUAL "x86")
		#	MESSAGE (STATUS "32 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		#ELSEIF (ARCH_DETECT STREQUAL "x86_64")
		#	MESSAGE (STATUS "64 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Linux-GNU-64.cmake")
		#ELSE (ARCH_DETECT STREQUAL "x86")
		#	MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		#ENDIF (ARCH_DETECT STREQUAL "x86")
	ELSE (UNIX)
		#IF (ARCH_DETECT STREQUAL "x86")
		#	MESSAGE (STATUS "32 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		#ELSEIF (ARCH_DETECT STREQUAL "x86_64")
		#	MESSAGE (STATUS "64 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Windows-GNU-64.cmake")
		#ELSE (ARCH_DETECT STREQUAL "x86")
		#	MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		#ENDIF (ARCH_DETECT STREQUAL "x86")
	ENDIF (UNIX)
ENDIF (MSVC)

ENDIF (NOT TOOLCHAIN)

toolchain_after_project()


MARK_AS_ADVANCED(SINGLE_TARGET MAKE_THREADS)
string(FIND "${CMAKE_GENERATOR}" "Makefiles" MAKEFILE)
IF ( (MAKEFILE GREATER 0) AND (NOT MSVC) )
	SET (SINGLE_TARGET 1)
ELSEIF ( MSVC )
	#SET (SINGLE_TARGET)
ENDIF ( (MAKEFILE GREATER 0) AND (NOT MSVC) )

IF (SINGLE_TARGET)
	# If you want to manually specify the build type, call cmake with parameter: -DCMAKE_BUILD_TYPE=something

	MESSAGE (STATUS "Single-target build system (Makefile) detected: generating multiple projects!")
	IF (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
		IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
			MESSAGE (STATUS "Invalid parameter -DCMAKE_BUILD_TYPE, defaulting to Release.")
			# -> needed only for MAKEFILE-STYLE generators, which can't switch between different configs
			set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
		ELSE ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
			MESSAGE (STATUS "Generating only specified project: ${CMAKE_BUILD_TYPE}.")
		ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
	ELSE (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
		MESSAGE (STATUS "No target specified: building all the projects (Release, Debug, Nightly).")
		# The only situation supported here is using MSVC, and /MP (multi-core) is set in its compiler flags.
	ENDIF (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")

	# Setting parallel make
	include(ProcessorCount)
	ProcessorCount(MAKE_THREADS)
	IF (NOT MAKE_THREADS EQUAL 0)
		MATH(EXPR MAKE_THREADS "${MAKE_THREADS} + (${MAKE_THREADS}/2)")		# Suggested number of threads: cores * 1.5
		set(CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MAKE_THREADS}")
	ELSE (NOT MAKE_THREADS EQUAL 0)
		message(STATUS "Can't determine CPU cores number. Parallel compilation turned off.")
	ENDIF (NOT MAKE_THREADS EQUAL 0)

ELSE (SINGLE_TARGET)

    MESSAGE (STATUS "Multi-target build system (Non-Makefile) detected: generating single project with multiple targets!")
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Nightly" CACHE STRING "" FORCE)

ENDIF (SINGLE_TARGET)


# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

INCLUDE ("cmake/CMakeSources.cmake")
INCLUDE ("cmake/CMakeGitStatus.cmake")


IF (SINGLE_TARGET)

	IF (NOT CMAKE_BUILD_TYPE)
		SET (CMAKE_BUILD_TYPE "filler")	# workaround, cmake is so buggy...
	ENDIF (NOT CMAKE_BUILD_TYPE)

	IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Release"))
		ADD_EXECUTABLE (spheresvr_release
					${game_SRCS}
					${items_SRCS}
					${chars_SRCS}
					${clients_SRCS}
					${uofiles_SRCS}
					${common_SRCS}
					${network_SRCS}
					${sphere_SRCS}
					${crashdump_SRCS}
					${libev_SRCS}
					${spherelibrary_SRCS}
					${sqlite_SRCS}
					${zlib_SRCS}
					${tables_SRCS}
		//			${docs_TEXT}
			)
	ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Release"))
	IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
		ADD_EXECUTABLE (spheresvr_debug
					${game_SRCS}
					${items_SRCS}
					${chars_SRCS}
					${clients_SRCS}
					${uofiles_SRCS}
					${common_SRCS}
					${network_SRCS}
					${sphere_SRCS}
					${crashdump_SRCS}
					${libev_SRCS}
					${spherelibrary_SRCS}
					${sqlite_SRCS}
					${zlib_SRCS}
					${tables_SRCS}
		//			${docs_TEXT}
			)
	ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Debug"))
	IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
		ADD_EXECUTABLE (spheresvr_nightly
					${game_SRCS}
					${items_SRCS}
					${chars_SRCS}
					${clients_SRCS}
					${uofiles_SRCS}
					${common_SRCS}
					${network_SRCS}
					${sphere_SRCS}
					${crashdump_SRCS}
					${libev_SRCS}
					${spherelibrary_SRCS}
					${sqlite_SRCS}
					${zlib_SRCS}
					${tables_SRCS}
		//			${docs_TEXT}
			)
	ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "") OR (${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))

ELSE (SINGLE_TARGET)

    ADD_EXECUTABLE (spheresvr
            ${game_SRCS}
            ${items_SRCS}
            ${chars_SRCS}
            ${clients_SRCS}
            ${uofiles_SRCS}
            ${common_SRCS}
            ${network_SRCS}
            ${sphere_SRCS}
            ${crashdump_SRCS}
            ${libev_SRCS}
            ${spherelibrary_SRCS}
            ${sqlite_SRCS}
            ${zlib_SRCS}
            ${tables_SRCS}
            ${docs_TEXT}
		)

ENDIF (SINGLE_TARGET)


toolchain_exe_stuff()
