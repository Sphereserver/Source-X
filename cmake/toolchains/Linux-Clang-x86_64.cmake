SET (TOOLCHAIN 1)
INCLUDE("${CMAKE_CURRENT_LIST_DIR}/Linux-Clang_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-Clang-x86_64.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Linux"		PARENT_SCOPE)
	SET(ARCH_BITS			64			PARENT_SCOPE)

	SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin-x86_64"	PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	SET (C_ARCH_OPTS	"-march=x86-64 -m64")
	SET (CXX_ARCH_OPTS	"-march=x86-64 -m64")
	
	SET (CMAKE_EXE_LINKER_FLAGS_EXTRA 
		"-L/usr/lib64/mysql -Wl,-rpath=/usr/lib64/mysql\
		 -L/usr/lib/x86_64-linux-gnu/mysql -Wl,-rpath=/usr/lib/x86_64-linux-gnu/mysql"
		PARENT_SCOPE)

	toolchain_exe_stuff_common()

	# Propagate variables set in toolchain_exe_stuff_common to the upper scope
	SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" 			PARENT_SCOPE)
endfunction()
