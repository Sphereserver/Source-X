SET (TOOLCHAIN 1)
MESSAGE (STATUS "Toolchain: Windows-GNU-64.cmake.")

SET (C_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch")
SET (CXX_WARNING_FLAGS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
SET (C_ARCH_OPTS "-march=x86-64 -m64")
SET (CXX_ARCH_OPTS "-march=x86-64 -m64")
#SET (CMAKE_RC_FLAGS "--target=pe-x86-64")
SET (C_OPTS "-s -fno-omit-frame-pointer -ffast-math -O3 -fno-expensive-optimizations")
SET (CXX_OPTS "-s -fno-omit-frame-pointer -ffast-math -fpermissive -O3")
SET (C_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions")
SET (CMAKE_C_FLAGS "${C_WARNING_FLAGS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL} -std=c11")
SET (CMAKE_CXX_FLAGS "${CXX_WARNING_FLAGS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL} -std=c++11")

# Force dynamic linking.
SET (CMAKE_EXE_LINKER_FLAGS "-dynamic -static-libstdc++ -static-libgcc")

LINK_DIRECTORIES ("common/mysql/lib/x86_64")

function (toolchain_exe_stuff)
	## Optimization flags set to max.	# redundant
	#SET_TARGET_PROPERTIES (spheresvr		PROPERTIES	COMPILE_FLAGS -O3 )
	##SET_TARGET_PROPERTIES (spheresvrNightly	PROPERTIES	COMPILE_FLAGS -O3 )

	# Architecture defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr 	
		PUBLIC _64B
		PUBLIC _WIN64	)
	#TARGET_COMPILE_DEFINITIONS ( spheresvrNightly
	#	PUBLIC _64B
	#	PUBLIC _WIN64	)

	# Linux (MinGW) libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		pthread
		libmysql
		ws2_32
		wsock32
	)
	#TARGET_LINK_LIBRARIES ( spheresvrNightly
	#	pthread
	#	libmysql
	#	ws2_32
	#	wsock32
	#)
endfunction()