INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/OSX-AppleClang_common.inc.cmake")


function (toolchain_force_compiler)
  if (CROSSCOMPILING_ARCH)
    message(FATAL_ERROR "Can't cross-compile with a 'native' toolchain.")
  endif ()

	SET (CMAKE_C_COMPILER 	"clang" 	  CACHE STRING "C compiler" 	FORCE)
	SET (CMAKE_CXX_COMPILER "clang++" 	CACHE STRING "C++ compiler" FORCE)

	# In order to enable ninja to be verbose
	set(CMAKE_VERBOSE_MAKEFILE 			ON CACHE	BOOL "ON")
endfunction ()


function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: OSX-AppleClang-native.cmake.")
  # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
	#SET(CMAKE_SYSTEM_NAME	"Darwin" CACHE INTERNAL "" FORCE)
  IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native64"	PARENT_SCOPE)
  ELSE ()
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native32"	PARENT_SCOPE)
  ENDIF ()

	toolchain_after_project_common()

  # Official Clang doesn't support -march=native for ARM arch, but Apple Clang does...
  # Otherwise, mind this for ARM crossbuilding -march=apple-m1'
	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=native -mcpu=native" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=native -mcpu=native" PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
endfunction()
