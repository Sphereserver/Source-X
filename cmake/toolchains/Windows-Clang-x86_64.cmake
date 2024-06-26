INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Windows-Clang_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-Clang-x86_64.cmake.")
	#SET(CMAKE_SYSTEM_NAME	"Windows"		PARENT_SCOPE)
	SET(ARCH_BASE			"x86"		CACHE INTERNAL "" FORCE) # override
	SET(ARCH_BITS			64			CACHE INTERNAL "" FORCE) # override
	SET(ARCH				"x86_64"	CACHE INTERNAL "" FORCE) # override
	SET(CMAKE_SYSTEM_PROCESSOR "${ARCH}" CACHE INTERNAL "" FORCE)

	toolchain_after_project_common()	# To enable RC language, to compile Windows Resource files

	LINK_DIRECTORIES ("lib/_bin/x86_64/mariadb/")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86_64"	PARENT_SCOPE)

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
