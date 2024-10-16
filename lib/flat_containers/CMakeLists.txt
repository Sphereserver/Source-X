cmake_minimum_required(VERSION 3.19)
project(flat_containers CXX)

set(lib_SOURCES
    flat_containers/flat_map.hpp
    flat_containers/flat_multimap.hpp
    flat_containers/flat_multiset.hpp
    flat_containers/flat_set.hpp
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group(lib\\flat_containers FILES ${lib_SOURCES})

# create the target. equivalent to add_executable, but for libraries
add_library(flat_containers INTERFACE ${lib_SOURCES})
#set_target_properties(flat_containers PROPERTIES LINKER_LANGUAGE CXX)

# make the headers available in the include path (accessible with < >).
target_include_directories(flat_containers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
