INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Windows-Clang_common.inc.cmake")


function (toolchain_force_compiler)
  if (CROSSCOMPILE_ARCH)
    message(FATAL_ERROR "This toolchain (as it is now) doesn't support cross-compilation.")
  endif ()

	SET (CMAKE_C_COMPILER 	"clang" 	  CACHE STRING "C compiler" 	FORCE)
	SET (CMAKE_CXX_COMPILER "clang++"	  CACHE STRING "C++ compiler" FORCE)

	# In order to enable ninja to be verbose
	#set(CMAKE_VERBOSE_MAKEFILE 			ON CACHE	BOOL "ON")

  IF (CLANG_USE_GCC_LINKER)
		# Not working, see CLANG_USE_GCC_LINKER option. Use MSVC.
		SET (CLANG_VENDOR "gnu" PARENT_SCOPE)
	ELSE ()
		SET (CLANG_VENDOR "msvc" PARENT_SCOPE)
	ENDIF ()
endfunction ()


function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-Clang-x86_64.cmake.")
  # Do not set CMAKE_SYSTEM_NAME if compiling for the same OS, otherwise CMAKE_CROSSCOMPILING will be set to TRUE
	#SET(CMAKE_SYSTEM_NAME	"Windows"		CACHE INTERNAL "" FORCE)
  SET(CMAKE_SYSTEM_PROCESSOR "x86_64" CACHE INTERNAL "" FORCE)
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86_64"	PARENT_SCOPE)
  #set(ARCH_BITS 64 CACHE INTERNAL "" FORCE) # provide it

	toolchain_after_project_common()	# To enable RC language, to compile Windows Resource files

  LINK_DIRECTORIES ("lib/_bin/x86_64/mariadb/")
	SET (CLANG_TARGET 		"--target=x86_64-pc-windows-${CLANG_VENDOR}")
	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=x86-64 -m64 ${CLANG_TARGET}" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=x86-64 -m64 ${CLANG_TARGET}" PARENT_SCOPE)
	SET (RC_FLAGS			"${CLANG_TARGET}" PARENT_SCOPE)

	# We can't override -fuse-ld=lld-link to use GNU ld, nor we can change --dependent-lib=msvcrtd
	# Maybe changing CMAKE_${lang}_LINK_EXECUTABLE ?
	# Check CMake code here: https://gitlab.kitware.com/cmake/cmake/-/blob/v3.25.1/Modules/Platform/Windows-Clang.cmake#L76-80
endfunction()


function (toolchain_exe_stuff)
	#SET (CMAKE_EXE_LINKER_FLAGS_COMMON "${CMAKE_EXE_LINKER_FLAGS_COMMON} ${CLANG_TARGET}" PARENT_SCOPE) # -stdlib=libc++ -lc++abi
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
	#SET (CMAKE_RC_FLAGS			"${RC_FLAGS}"							PARENT_SCOPE)

endfunction()
