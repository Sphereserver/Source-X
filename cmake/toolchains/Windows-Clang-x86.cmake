SET (TOOLCHAIN 1)
INCLUDE("${CMAKE_CURRENT_LIST_DIR}/Windows-Clang_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-Clang-x86.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"		PARENT_SCOPE)
	SET(ARCH_BITS			32				PARENT_SCOPE)

	toolchain_after_project_common()

	LINK_DIRECTORIES ("lib/bin/x86_64/mariadb/")
	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86"	PARENT_SCOPE)
	
endfunction()


function (toolchain_exe_stuff)
	SET (CLANG_TARGET 	"--target=i686-pc-windows-${CLANG_VENDOR}")
	SET (C_ARCH_OPTS	"-march=i686 -m32 ${CLANG_TARGET}")
	SET (CXX_ARCH_OPTS	"-march=i686 -m32 ${CLANG_TARGET}")
	SET (RC_FLAGS		"${CLANG_TARGET}")

	toolchain_exe_stuff_common()

	# Propagate variables set in toolchain_exe_stuff_common to the upper scope
	SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
	SET (CMAKE_RC_FLAGS			"${RC_FLAGS}"							PARENT_SCOPE)

	# Set different output folders for each build type
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"	)
	
endfunction()
