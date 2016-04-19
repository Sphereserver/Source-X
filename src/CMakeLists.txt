#	Main config	#

CMAKE_MINIMUM_REQUIRED (VERSION 2.8.10)

# If you want to manually specify the build type, call cmake with parameter: -DCMAKE_BUILD_TYPE=something
IF (CMAKE_BUILD_TYPE)

IF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))
	MESSAGE (STATUS "Invalid parameter -DCMAKE_BUILD_TYPE, defaulting to Release.")
	# -> needed only for MAKEFILE-STYLE generators, which can't switch between different configs
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
ENDIF ((NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Nightly"))

ELSE (CMAKE_BUILD_TYPE)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Nightly" CACHE STRING "" FORCE)
ENDIF (CMAKE_BUILD_TYPE)


MESSAGE (STATUS "Scanning system for compilers...")
PROJECT (sphereserver)	# does a scan for C++ and C compilers


# If we have not specified a toolchain, let's detect which one we should use
IF (NOT TOOLCHAIN)

MESSAGE (STATUS "Toolchain not specified. Detecting the one to use.")
IF (MSVC)
	INCLUDE ("cmake/toolchains/Windows-MSVC.cmake")
ELSE (MSVC)
	MESSAGE (STATUS "Toolchain not specified. Defaulting to 32 bits compiler.")
	# INCLUDE ("cmake/CMakeDetectArch.cmake")
	# target_architecture("ARCH")

	IF (UNIX)
		#IF (ARCH STREQUAL "x86")
		#	MESSAGE (STATUS "32 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		#ELSEIF (ARCH STREQUAL "x86_64")
		#	MESSAGE (STATUS "64 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Linux-GNU-64.cmake")
		#ELSE (ARCH STREQUAL "x86")
		#	MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		#ENDIF (ARCH STREQUAL "x86")
	ELSE (UNIX)
		#IF (ARCH STREQUAL "x86")
		#	MESSAGE (STATUS "32 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		#ELSEIF (ARCH STREQUAL "x86_64")
		#	MESSAGE (STATUS "64 bits architecture detected.")
		#	INCLUDE ("cmake/toolchains/Windows-GNU-64.cmake")
		#ELSE (ARCH STREQUAL "x86")
		#	MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		#ENDIF (ARCH STREQUAL "x86")
	ENDIF (UNIX)
ENDIF (MSVC)

ENDIF (NOT TOOLCHAIN)

toolchain_after_project()

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

INCLUDE ("cmake/CMakeSources.cmake")
INCLUDE ("cmake/CMakeGitStatus.cmake")

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

toolchain_exe_stuff()
INCLUDE ("cmake/platforms/${CMAKE_SYSTEM_NAME}.cmake")