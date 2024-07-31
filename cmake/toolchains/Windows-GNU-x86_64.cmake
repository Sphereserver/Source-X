INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Windows-GNU_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-GNU-x86_64.cmake.")
	#SET(CMAKE_SYSTEM_NAME	"Windows"		PARENT_SCOPE)
	SET(ARCH_BASE			"x86"		CACHE INTERNAL "" FORCE) # override
	SET(ARCH_BITS			64			CACHE INTERNAL "" FORCE) # override
	SET(ARCH				"x86_64"	CACHE INTERNAL "" FORCE) # override
	SET(CMAKE_SYSTEM_PROCESSOR "${ARCH}" CACHE INTERNAL "" FORCE)

	toolchain_after_project_common()	# To enable RC language, to compile Windows Resource files

	LINK_DIRECTORIES ("lib/_bin/x86_64/mariadb/")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86_64"	PARENT_SCOPE)

	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=x86-64 -m64"	PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=x86-64 -m64"	PARENT_SCOPE)
	SET (RC_FLAGS			"--target=pe-x86-64"	PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
	#SET (CMAKE_RC_FLAGS			"${RC_FLAGS}"							PARENT_SCOPE)

endfunction()
