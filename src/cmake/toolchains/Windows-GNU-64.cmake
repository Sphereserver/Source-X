SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-GNU-64.cmake.")
	SET(CMAKE_SYSTEM_NAME Windows)

	SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
	SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
	SET (C_ARCH_OPTS "-march=x86-64 -m64")
	SET (CXX_ARCH_OPTS "-march=x86-64 -m64")
	#SET (CMAKE_RC_FLAGS "--target=pe-x86-64")
	SET (C_OPTS "-s -fno-omit-frame-pointer -O3 -fno-expensive-optimizations -std=c11")
	SET (CXX_OPTS "-s -fno-omit-frame-pointer -ffast-math -fpermissive -O3 -std=c++11")
	SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
	SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}"	PARENT_SCOPE)

	# Force dynamic linking.
	SET (CMAKE_EXE_LINKER_FLAGS "-dynamic -static-libstdc++ -static-libgcc")

	# Nightly compiler and linker flags
	SET (CMAKE_CXX_FLAGS_NIGHTLY ${CMAKE_CXX_FLAGS})
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY ${CMAKE_EXE_LINKER_FLAGS})

	LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/common/mysql/lib/x86_64")
endfunction()

function (toolchain_exe_stuff)
	# Architecture defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr 	
		PUBLIC _64B
		PUBLIC _WIN64
	)

	# Linux (MinGW) libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		pthread
		libmysql
		ws2_32
		wsock32
	)
endfunction()