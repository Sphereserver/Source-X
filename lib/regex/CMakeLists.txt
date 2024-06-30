cmake_minimum_required (VERSION 3.19)
project(regex CXX)

set (lib_SOURCES
    regex/deelx.h
)

# create the target. equivalent to add_executable, but for libraries
add_library(regex INTERFACE ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (regex INTERFACE .)
# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\regex FILES ${lib_SOURCES})
