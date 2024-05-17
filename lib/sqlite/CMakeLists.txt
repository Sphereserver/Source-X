cmake_minimum_required (VERSION 3.19)
project(sqlite C)

set (lib_SOURCES
    sqlite/sqlite3.c
    sqlite/sqlite3.h
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\sqlite FILES ${lib_SOURCES})

add_library (sqlite STATIC ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (sqlite PUBLIC .)

# set compiler flags
include("../lib_compiler_flags_common_c.cmake")
