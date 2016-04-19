SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-GNU-32.cmake.")
	SET(CMAKE_SYSTEM_NAME Linux)

	SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
	SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
	SET (C_ARCH_OPTS "-march=i686 -m32")
	SET (CXX_ARCH_OPTS "-march=i686 -m32")
	SET (C_OPTS "-s -fno-omit-frame-pointer -O3 -fno-expensive-optimizations -std=c11")
	SET (CXX_OPTS "-s -fno-omit-frame-pointer -ffast-math -fpermissive -O3 -std=c++11")
	SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}"	PARENT_SCOPE)

	# Force dynamic linking.
	SET (CMAKE_EXE_LINKER_FLAGS "-dynamic")

	# Nightly compiler and linker flags
	SET (CMAKE_CXX_FLAGS_NIGHTLY ${CMAKE_CXX_FLAGS}			PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY ${CMAKE_EXE_LINKER_FLAGS}	PARENT_SCOPE)

	LINK_DIRECTORIES ("/usr/lib")
endfunction()

function (toolchain_exe_stuff)
	# Architecture defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr 		PUBLIC _32B )
endfunction()
