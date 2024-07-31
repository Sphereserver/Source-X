cmake_minimum_required (VERSION 3.19)
project(cdrc CXX)

set (lib_SOURCES
    cdrc/atomic_rc_ptr.h
    cdrc/atomic_weak_ptr.h
    cdrc/marked_arc_ptr.h
    cdrc/rc_ptr.h
    cdrc/snapshot_ptr.h
    cdrc/weak_ptr.h
    cdrc/weak_snapshot_ptr.h
)
set (lib_custom_SOURCES
    cdrc/custom/rand.cpp
    cdrc/custom/rand.h
    cdrc/custom/threadid.cpp
    cdrc/custom/threadid.h
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\cdrc FILES ${lib_SOURCES})
source_group (lib\\cdrc\\custom FILES ${lib_custom_SOURCES})

add_library(cdrc STATIC ${lib_SOURCES} ${lib_custom_SOURCES})

# set compiler flags
if (MSVC)
    set(my_comp_options
        /wd4244
        /wd4267
        /wd4324
    )
endif ()

include("../lib_build_flags_common_c.cmake")
target_compile_options	(cdrc PRIVATE ${c_compiler_options_common} ${my_comp_options})
target_link_options	(cdrc PRIVATE ${c_linker_options_common})

# make the headers available in the include path (accessible with < >).
target_include_directories (cdrc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
