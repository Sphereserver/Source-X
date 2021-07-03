SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-GNU-x86.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Linux"		PARENT_SCOPE)
	SET(ARCH_BITS		32		PARENT_SCOPE)

	ENABLE_LANGUAGE(RC)

	LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/../DLLs/64/")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin"	PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
    INCLUDE("cmake/toolchains/Windows-GNU_common.inc.cmake")
    
	SET (C_ARCH_OPTS	"-march=i686 -m32")
	SET (CXX_ARCH_OPTS	"-march=i686 -m32")

	toolchain_exe_stuff_common()
	
	# Propagate variables set in toolchain_exe_stuff_common to the upper scope
	SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
	SET (CMAKE_RC_FLAGS			"${CMAKE_RC_FLAGS}"						PARENT_SCOPE)
endfunction()
