cmake_minimum_required(VERSION 3.19)
project(mariadb_connector C)
# we do not build the library here, only provide the headers for a precompiled one.
#include("lib_compiler_flags_common_c.cmake")

# Configure MariaDB C Connector source headers
include("mysql/configure.cmake")

set(lib_SOURCES mysql/config.h mysql/mysql.h)

# create the target. equivalent to add_executable, but for libraries
add_library(mariadb_connector INTERFACE ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories(mariadb_connector INTERFACE .)
# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group(lib\\mariadb_connector_c FILES ${lib_SOURCES})
