# Main config.
PROJECT (sphereserver)
 
CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

INCLUDE ("cmake/CMakeSources.cmake")
INCLUDE ("cmake/CMakeGitStatus.cmake")

# Need to clear shared library flags. If not, cmake sets -rdynamic and this
# add to the executable the full symbol table (included unused symbols).
# This is a problem because the binary is ~700 KB bigger.
SET (CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)
SET (CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS)

ADD_EXECUTABLE (spheresvr
		${game_SRCS}
		${items_SRCS}
		${chars_SRCS}
		${clients_SRCS}
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

#[[
# Nightly project, maybe we should create it only with enabling a CMake option like 'BuildNightly' or something.
ADD_EXECUTABLE (spheresvrNightly
		${game_SRCS}
		${items_SRCS}
		${chars_SRCS}
		${clients_SRCS}
		${common_SRCS}
		${network_SRCS}
		${sphere_SRCS}
		${crashdump_SRCS}
		${libev_SRCS}
		${sqlite_SRCS}
		${spherelibrary_SRCS}
		${zlib_SRCS}
		${tables_SRCS}
		${docs_TEXT}
) ]]

# If we have not specified a toolchain, let's detect which one we should use
IF (NOT TOOLCHAIN)

MESSAGE (STATUS "Toolchain not specified. Detecting the one to use.")
IF (MSVC)
	INCLUDE ("cmake/toolchains/Windows-MSVC.cmake")
ELSE (MSVC)
	INCLUDE ("cmake/CMakeDetectArch.cmake")
	target_architecture("ARCH")

	IF (UNIX)
		SET(CMAKE_SYSTEM_NAME Linux)
		IF (ARCH STREQUAL "x86")
			MESSAGE (STATUS "32 bits architecture detected.")
			INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		ELSEIF (ARCH STREQUAL "x86_64")
			MESSAGE (STATUS "64 bits architecture detected.")
			INCLUDE ("cmake/toolchains/Linux-GNU-64.cmake")
		ELSE (ARCH STREQUAL "x86")
			MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Linux-GNU-32.cmake")
		ENDIF (ARCH STREQUAL "x86")
	ELSE (UNIX)
		SET(CMAKE_SYSTEM_NAME Windows)
		IF (ARCH STREQUAL "x86")
			MESSAGE (STATUS "32 bits architecture detected.")
			INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		ELSEIF (ARCH STREQUAL "x86_64")
			MESSAGE (STATUS "64 bits architecture detected.")
			INCLUDE ("cmake/toolchains/Windows-GNU-64.cmake")
		ELSE (ARCH STREQUAL "x86")
			MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits.")
			INCLUDE ("cmake/toolchains/Windows-GNU-32.cmake")
		ENDIF (ARCH STREQUAL "x86")
	ENDIF (UNIX)
ENDIF (MSVC)

ENDIF (NOT TOOLCHAIN)

INCLUDE ("cmake/platforms/${CMAKE_SYSTEM_NAME}.cmake")
