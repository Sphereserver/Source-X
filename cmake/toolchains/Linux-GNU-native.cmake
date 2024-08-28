INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Linux-GNU_common.inc.cmake")


function (toolchain_force_compiler)
  if (CROSSCOMPILING_ARCH)
    message(FATAL_ERROR "Can't cross-compile with a 'native' toolchain.")
  endif ()

  SET (CMAKE_C_COMPILER 	"gcc" 	CACHE STRING "C compiler" 	FORCE)
  SET (CMAKE_CXX_COMPILER "g++" 	CACHE STRING "C++ compiler" FORCE)
endfunction ()


function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-GNU-native.cmake.")
  # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
	#SET(CMAKE_SYSTEM_NAME	"Linux"      CACHE INTERNAL "" FORCE) # target OS
	IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
		SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native64"	PARENT_SCOPE)
	ELSE ()
		SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native32"	PARENT_SCOPE)
	ENDIF ()

  # possible native/host lib locations
  set (local_usr_lib_subfolders
    "libmariadb3"
    "mysql"
  )
  foreach (local_usr_lib_subfolders subfolder)
    # ARCH variable is set by CMakeGitStatus
    set (local_lib_search_paths
      ${local_lib_search_paths}
      "/usr/${ARCH}-linux-gnu/${subfolder}"
    )
  endforeach()
  set (lib_search_paths
    ${local_lib_search_paths}
    "/usr/lib64/mysql"
    "/usr/lib64"
    "/usr/lib/mysql"
    "/usr/lib"
    CACHE STRING "Library search paths" FORCE
  )

	toolchain_after_project_common()

	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=native -mtune=native" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=native -mtune=native" PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS	"${CMAKE_EXE_LINKER_FLAGS}"				PARENT_SCOPE)
endfunction()
