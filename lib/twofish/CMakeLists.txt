cmake_minimum_required (VERSION 3.19)
project(twofish C)

set (lib_SOURCES
    twofish/aes.h
    twofish/debug.h
    twofish/platform.h
    twofish/table.h
    twofish/twofish.c
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\twofish FILES ${lib_SOURCES})

# create the target. equivalent to add_executable, but for libraries
add_library (twofish STATIC ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (twofish PUBLIC .)

# set compiler flags
include("../lib_compiler_flags_common_c.cmake")
target_compile_options(twofish PRIVATE ${c_compiler_options_common})
