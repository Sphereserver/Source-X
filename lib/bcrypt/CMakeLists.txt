cmake_minimum_required (VERSION 3.19)
project(bcrypt C)

set (lib_SOURCES
    bcrypt/crypt_blowfish.c
    bcrypt/crypt_blowfish.h
    bcrypt/crypt_gensalt.c
    bcrypt/crypt_gensalt.h
    bcrypt/ow-crypt.h
    bcrypt/wrapper.c
)

add_library(bcrypt STATIC ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (bcrypt PUBLIC .)
# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\bcrypt FILES ${lib_SOURCES})

# set compiler flags
include("../lib_build_flags_common_c.cmake")
target_compile_options      (bcrypt PRIVATE ${c_compiler_options_common})
target_compile_definitions  (bcrypt PRIVATE ${c_compiler_definitions_common})
target_link_options		    (bcrypt PRIVATE ${c_linker_options_common})
