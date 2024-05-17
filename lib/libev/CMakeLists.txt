cmake_minimum_required (VERSION 3.19)
project(libev C)

# Generate config.h file for libev (if we are using it)
INCLUDE ("configure.cmake")

set (lib_SOURCES
    wrapper_ev.c
    libev/config.h
    libev/ev++.h
    libev/ev.h
    #libev/ev_epoll.c
    #libev/ev_kqueue.c
    #libev/ev_poll.c
    #libev/ev_port.c
    #libev/ev_select.c
    libev/ev_vars.h
    #libev/ev_win32.c
    libev/ev_wrap.h
    #libev/event.c
    libev/event.h
)

# add to the proper directories in the solution explorer (eg. Visual Studio)
source_group (lib\\libev FILES ${lib_SOURCES})

add_library (libev STATIC ${lib_SOURCES})

# make the headers available in the include path (accessible with < >).
target_include_directories (libev PUBLIC .)

# set compiler flags
include("../lib_compiler_flags_common_c.cmake")
