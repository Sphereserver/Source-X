INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/OSX-AppleClang_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: OSX-AppleClang-x86_64.cmake.")
	SET(CMAKE_SYSTEM_NAME	"OSX"		PARENT_SCOPE)
	SET(ARCH_BITS			64		PARENT_SCOPE)

	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86_64"	PARENT_SCOPE)

	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=x86-64 -m64" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=x86-64 -m64" PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()
	
	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
endfunction()
