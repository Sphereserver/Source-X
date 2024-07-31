INCLUDE("${CMAKE_CURRENT_LIST_DIR}/include/Windows-Clang_common.inc.cmake")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-Clang-native.cmake.")

	#SET(CMAKE_SYSTEM_NAME	"Windows"      PARENT_SCOPE)

	toolchain_after_project_common()	# To enable RC language, to compile Windows Resource files

	IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
        #MESSAGE (STATUS "Detected 64 bits architecture")
		#SET(ARCH_BITS	64	CACHE INTERNAL) # override
		SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native64"	PARENT_SCOPE)
		LINK_DIRECTORIES ("lib/_bin/x86_64/mariadb/")
	ELSE (CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE (STATUS "Detected 32 bits architecture")
		#SET(ARCH_BITS	32	CACHE INTERNAL) # override
		#SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin-native32"	PARENT_SCOPE)
		LINK_DIRECTORIES ("lib/_bin/x86/mariadb/")
	ENDIF (CMAKE_SIZEOF_VOID_P EQUAL 8)

	#SET (CLANG_TARGET 	"--target=x86_64-pc-windows-${CLANG_VENDOR}")
	#SET (C_ARCH_OPTS	"-march=native ${CLANG_TARGET}")
	#SET (CXX_ARCH_OPTS	"-march=native ${CLANG_TARGET}")
	#SET (RC_FLAGS		"${CLANG_TARGET}")

	# TODO: fix the target, maybe using CMAKE_HOST_SYSTEM_PROCESSOR ?
	SET (CMAKE_C_FLAGS		"${CMAKE_C_FLAGS}   -march=native" PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -march=native" PARENT_SCOPE)

endfunction()


function (toolchain_exe_stuff)
	toolchain_exe_stuff_common()

	# Propagate global variables set in toolchain_exe_stuff_common to the upper scope
	#SET (CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${C_ARCH_OPTS}"       PARENT_SCOPE)
	#SET (CMAKE_CXX_FLAGS		"${CMAKE_CXX_FLAGS} ${CXX_ARCH_OPTS}"	PARENT_SCOPE)
	#SET (CMAKE_EXE_LINKER_FLAGS	"${CMAKE_EXE_LINKER_FLAGS}"				PARENT_SCOPE)
	#SET (CMAKE_RC_FLAGS			"${RC_FLAGS}"							PARENT_SCOPE)

endfunction()
