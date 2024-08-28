INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Linux-Clang_common.inc.cmake")


function (toolchain_force_compiler)
  SET (CMAKE_C_COMPILER 	"clang" 	  CACHE STRING "C compiler" 	FORCE)
  SET (CMAKE_CXX_COMPILER "clang++" 	CACHE STRING "C++ compiler" FORCE)

  if (CROSSCOMPILING_ARCH)
    message(FATAL_ERROR "Incomplete/to be tested.") # are the names/paths correct?

    # where is the target environment located
    set(CMAKE_FIND_ROOT_PATH "/usr/i386-pc-linux-gnu" CACHE INTERNAL "" FORCE)

    # adjust the default behavior of the FIND_XXX() commands:
    # search programs in the host environment
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM "NEVER" CACHE INTERNAL "" FORCE)

    # search headers and libraries in the target environment
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE "ONLY" CACHE INTERNAL "" FORCE)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY "ONLY" CACHE INTERNAL "" FORCE)
  endif()
endfunction ()


function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-Clang-x86.cmake.")
  # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
	#SET(CMAKE_SYSTEM_NAME	"Linux"		CACHE INTERNAL "" FORCE) # target OS
  SET(CMAKE_SYSTEM_PROCESSOR "x86" CACHE INTERNAL "" FORCE) # target arch
  SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86"	PARENT_SCOPE)
  #set(ARCH_BITS 32 CACHE INTERNAL "" FORCE) # provide it

  if (CROSSCOMPILING_ARCH)
    # possible cross-compilation foreign arch lib locations
    set (lib_search_paths
      "/usr/i686-linux-gnu/usr/lib/libmariadb3"
      "/usr/i686-linux-gnu/usr/lib/mysql"
      "/usr/i686-linux-gnu/usr/lib/"
      "/usr/i386-linux-gnu/usr/lib/libmariadb3"
      "/usr/i386-linux-gnu/usr/lib/mysql"
      "/usr/i386-linux-gnu/usr/lib/"
      CACHE STRING "Library search paths" FORCE
    )
  else (CROSSCOMPILING_ARCH)
    set (local_lib_search_paths
      "/usr/lib/i686-linux-gnu/libmariadb3"
      "/usr/lib/i686-linux-gnu/mysql"
      "/usr/lib/i686-linux-gnu"
      "/usr/lib/i386-linux-gnu/libmariadb3"
      "/usr/lib/i386-linux-gnu/mysql"
      "/usr/lib/i386-linux-gnu"
      "/usr/lib32/mysql"
      "/usr/lib32"
    )

    if ("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
      # I'm compiling for x86 on an x86_64 host.
      set (CMAKE_LIBRARY_PATH ${local_lib_search_paths} CACHE PATH "")
    else ()
      # I'm compiling for x86 on an x86 OS (32 bits), so natively: i have libs on /usr/lib and not /usr/lib32.
      set (local_extra_lib_search_paths
        "/usr/lib/mysql"
        "/usr/lib"
      )
    endif()

    set (lib_search_paths
      ${local_lib_search_paths}
      ${local_extra_lib_search_paths}
      CACHE STRING "Library search paths" FORCE
    )
  endif (CROSSCOMPILING_ARCH)

	toolchain_after_project_common()

  # --target=-x86-unknown-linux-gnu?
	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=i686 -m32" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=i686 -m32" PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
endfunction()
