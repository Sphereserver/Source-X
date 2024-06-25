cmake_minimum_required (VERSION 3.19)
project(zlib C)

set (lib_SOURCES
    zlib/adler32.c
    zlib/compress.c
    zlib/crc32.c
    zlib/crc32.h
    zlib/deflate.c
    zlib/deflate.h
    zlib/gzclose.c
    zlib/gzguts.h
    zlib/gzlib.c
    zlib/gzread.c
    zlib/gzwrite.c
    zlib/infback.c
    zlib/inffast.c
    zlib/inffast.h
    zlib/inffixed.h
    zlib/inflate.c
    zlib/inflate.h
    zlib/inftrees.c
    zlib/inftrees.h
    zlib/trees.c
    zlib/trees.h
    zlib/uncompr.c
    zlib/zconf.h
    zlib/zlib.h
    zlib/zutil.c
    zlib/zutil.h
)

source_group (lib\\zlib FILES
    ${lib_SOURCES}
)

# make the headers available in the include path (accessible with < >).
add_library (zlib STATIC ${lib_SOURCES})
# add to the proper directories in the solution explorer (eg. Visual Studio)
target_include_directories (zlib PUBLIC .)

# set compiler flags
include("../lib_build_flags_common_c.cmake")
target_compile_options      (zlib PRIVATE ${c_compiler_options_common})
target_compile_definitions  (zlib PRIVATE ${c_compiler_definitions_common})
if (MSVC)
    target_compile_definitions (zlib PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
endif ()
target_link_options		    (zlib PRIVATE ${c_linker_options_common})
