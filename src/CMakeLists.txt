#	Main config	#

CMAKE_MINIMUM_REQUIRED (VERSION 3.1)


MESSAGE (STATUS "Scanning system for compilers...")
PROJECT (SphereServer)	# does a scan for C++ and C compilers

# Supress the ZERO_CHECK generation
SET (CMAKE_SUPPRESS_REGENERATION true)

# If we have not specified a toolchain, let's detect which one we should use
IF (NOT TOOLCHAIN)
	INCLUDE ("cmake/CMakeDefaultToolchain.cmake")
ENDIF ()


toolchain_after_project()   # stuff that need to be executed before ADD_EXECUTABLE

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)


STRING (FIND "${CMAKE_GENERATOR}" "Makefiles" GEN_IS_MAKEFILE)
STRING (FIND "${CMAKE_GENERATOR}" "Ninja" GEN_IS_NINJA)

IF ( ((NOT GEN_IS_MAKEFILE EQUAL -1) OR (NOT GEN_IS_NINJA EQUAL -1)) AND (NOT MSVC) )
	SET (SINGLE_TARGET 1)
ELSEIF ( MSVC )
	#SET (SINGLE_TARGET)
ENDIF ()

IF (SINGLE_TARGET)
	# If you want to manually specify the build type, call cmake with parameter: -DCMAKE_BUILD_TYPE=something

	MESSAGE (STATUS "Single-target build system (${CMAKE_GENERATOR}) detected: generating multiple projects!")
	IF (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
		IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
			MESSAGE (WARNING "Invalid parameter -DCMAKE_BUILD_TYPE, defaulting to Release.")
			# -> needed only for MAKEFILE-STYLE generators, which can't switch between different configs
			SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
		ELSE ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
			MESSAGE (STATUS "Generating only specified project: ${CMAKE_BUILD_TYPE}.")
		ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
	ELSE (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")
		MESSAGE (STATUS "No target specified: building all the projects (Release, Debug, Nightly).")
		# The only situation supported here is using MSVC, and /MP (multi-core) is set in its compiler flags.
	ENDIF (NOT ${CMAKE_BUILD_TYPE} STREQUAL "")

	# Setting parallel make
	INCLUDE (ProcessorCount)
	ProcessorCount (MAKE_THREADS)
	IF (NOT MAKE_THREADS EQUAL 0)
		MATH (EXPR MAKE_THREADS "${MAKE_THREADS} + (${MAKE_THREADS}/2)")		# Suggested number of threads: cores * 1.5
		SET (CMAKE_MAKE_PROGRAM "${CMAKE_MAKE_PROGRAM} -j${MAKE_THREADS}")
	ELSE (NOT MAKE_THREADS EQUAL 0)
		MESSAGE (STATUS "Can't determine CPU cores number. Parallel compilation turned off.")
	ENDIF (NOT MAKE_THREADS EQUAL 0)

ELSE (SINGLE_TARGET)

    MESSAGE (STATUS "Multi-target build system detected: generating single project with multiple targets!")
    SET (CMAKE_CONFIGURATION_TYPES "Debug;Release;Nightly" CACHE STRING "" FORCE)

ENDIF (SINGLE_TARGET)


# Include the list of our source files
INCLUDE ("cmake/CMakeSources.cmake")

SET (ALL_SRCS 
	${game_SRCS}
	${items_SRCS}
	${chars_SRCS}
	${clients_SRCS}
	${components_SRCS}
	${uofiles_SRCS}
	${common_SRCS}
	${resource_SRCS}
	${resourcesections_SRCS}
	${network_SRCS}
	${crypto_SRCS}
	${sphere_SRCS}
	${crashdump_SRCS}
	${libev_SRCS}
	${spherelibrary_SRCS}
	${sqlite_SRCS}
	${zlib_SRCS}
	${tables_SRCS}
)

IF (SINGLE_TARGET)

	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(R|r?)elease"))
		SET (TARGETS ${TARGETS} spheresvr_release)
		ADD_EXECUTABLE (spheresvr_release
					${ALL_SRCS}
		#			${docs_TEXT}
			)
        	SET_TARGET_PROPERTIES (spheresvr_release PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_release)
	ENDIF ()
	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(N|n?)ightly"))
		SET (TARGETS ${TARGETS} spheresvr_nightly)
		ADD_EXECUTABLE (spheresvr_nightly
					${ALL_SRCS}
		#			${docs_TEXT}
			)
        	SET_TARGET_PROPERTIES (spheresvr_nightly PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_nightly)
	ENDIF ()
	IF (("${CMAKE_BUILD_TYPE}" STREQUAL "") OR (${CMAKE_BUILD_TYPE} MATCHES "(D|d?)ebug"))
		SET (TARGETS ${TARGETS} spheresvr_debug)
		ADD_EXECUTABLE (spheresvr_debug
					${ALL_SRCS}
		#			${docs_TEXT}
			)
        	SET_TARGET_PROPERTIES (spheresvr_debug PROPERTIES OUTPUT_NAME SphereSvrX${ARCH_BITS}_debug)
	ENDIF ()

ELSE (SINGLE_TARGET)

	SET (TARGETS ${TARGETS} spheresvr)
    	ADD_EXECUTABLE (spheresvr
            ${ALL_SRCS}
            ${docs_TEXT}
		)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_RELEASE SphereSvrX${ARCH_BITS}_release)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_NIGHTLY SphereSvrX${ARCH_BITS}_nightly)
	SET_TARGET_PROPERTIES (spheresvr PROPERTIES OUTPUT_NAME_DEBUG SphereSvrX${ARCH_BITS}_debug)

ENDIF (SINGLE_TARGET)


toolchain_exe_stuff()   # stuff to be executed after ADD_EXECUTABLE


# Get the Git revision number
INCLUDE ("cmake/CMakeGitStatus.cmake")

# Generate config.h file for libev (if we are using it)
INCLUDE ("cmake/CMakeConfigureLibev.cmake")
