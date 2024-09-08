cmake_minimum_required(VERSION 3.19)
project(rand CXX)

set(lib_SOURCES rand/xorshift.hpp)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group(lib\\rand FILES ${lib_SOURCES})

# create the target. equivalent to add_executable, but for libraries
add_library(rand INTERFACE ${lib_SOURCES})
#set_target_properties(rand PROPERTIES LINKER_LANGUAGE CXX)

# make the headers available in the include path (accessible with < >).
target_include_directories(rand INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
