cmake_minimum_required (VERSION 3.19)
project(sqlite C)

set (lib_SOURCES
    sqlite/sqlite3.c
    sqlite/sqlite3.h
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\sqlite FILES ${lib_SOURCES})

# create the target. equivalent to add_executable, but for libraries
add_library (sqlite STATIC ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (sqlite PUBLIC .)

# set compiler flags
include("../lib_build_flags_common_c.cmake")
target_compile_options      (sqlite PRIVATE ${c_compiler_options_common})
target_compile_definitions  (sqlite PRIVATE ${c_compiler_definitions_common})
target_link_options		    (sqlite PRIVATE ${c_linker_options_common})
