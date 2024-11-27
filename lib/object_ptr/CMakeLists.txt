cmake_minimum_required(VERSION 3.19)
project(object_ptr CXX)

# provide the headers headers.
set(lib_SOURCES object_ptr/object_ptr.hpp)

# create the target. equivalent to add_executable, but for libraries
add_library(object_ptr INTERFACE ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories(object_ptr INTERFACE .)
# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group(lib\\object_ptr FILES ${lib_SOURCES})
