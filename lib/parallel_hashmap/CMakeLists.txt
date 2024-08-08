cmake_minimum_required (VERSION 3.19)
project(parallel_hashmap CXX)

set (lib_SOURCES
    parallel_hashmap/btree.h
    #parallel_hashmap/meminfo.h
    parallel_hashmap/phmap.h
    #parallel_hashmap/phmap_base.h
    #parallel_hashmap/phmap_bits.h
    #parallel_hashmap/phmap_config.h
    #parallel_hashmap/phmap_dump.h
    #parallel_hashmap/phmap_fwd_decl.h
    #parallel_hashmap/phmap_utils.h
)

# create the target. equivalent to add_executable, but for libraries
add_library(parallel_hashmap INTERFACE ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (parallel_hashmap INTERFACE .)
# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\parallel_hashmap FILES ${lib_SOURCES})

#target_compile_definitions(parallel_hashmap INTERFACE
#  $<$<BOOL:${USE_ASAN}>: ADDRESS_SANITIZER>
#  $<$<BOOL:${USE_MSAN}>: MEMORY_SANITIZER>
#  $<$<BOOL:${USE_UBSAN}>:UNDEFINED_BEHAVIOR_SANITIZER>
#  $<$<BOOL:${USE_TSAN}>: THREAD_SANITIZER>
#)
