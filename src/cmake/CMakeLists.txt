# Main config.
PROJECT (sphereserver)
 
CMAKE_MINIMUM_REQUIRED (VERSION 3.0)

INCLUDE ("CMakeSources.cmake")
INCLUDE ("CMakeGitStatus.cmake")

INCLUDE ("CMakeDetectArch.cmake")
target_architecture("ARCH")

IF (UNIX)
	SET(CMAKE_SYSTEM_NAME Linux)
	IF (ARCH STREQUAL "x86")
		MESSAGE (STATUS "32 bits architecture detected")
		LINK_DIRECTORIES ("/usr/lib")
	ELSEIF (ARCH STREQUAL "x86_64")
		MESSAGE (STATUS "64 bits architecture detected")
		LINK_DIRECTORIES ("/usr/lib64")
	ELSE (ARCH STREQUAL "x86")
		MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits")
		LINK_DIRECTORIES ("/usr/lib")
	ENDIF (ARCH STREQUAL "x86")
ELSE (UNIX)
	SET(CMAKE_SYSTEM_NAME Windows)
	IF (ARCH STREQUAL "x86")
		MESSAGE (STATUS "32 bits architecture detected")
		LINK_DIRECTORIES ("../common/mysql/lib/x86")
	ELSEIF (ARCH STREQUAL "x86_64")
		MESSAGE (STATUS "64 bits architecture detected")
		LINK_DIRECTORIES ("../common/mysql/lib/x86_64")
	ELSE (ARCH STREQUAL "x86")
		MESSAGE (STATUS "Unknown architecture, defaulting to 32 bits")
		LINK_DIRECTORIES ("../common/mysql/lib/x86")
	ENDIF (ARCH STREQUAL "x86")
ENDIF (UNIX)

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

INCLUDE ("platforms/${CMAKE_SYSTEM_NAME}.cmake")
INCLUDE ("platforms/${CMAKE_SYSTEM_NAME}-${CMAKE_CXX_COMPILER_ID}.cmake")