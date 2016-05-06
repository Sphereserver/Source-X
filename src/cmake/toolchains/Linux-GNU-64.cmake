SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-GNU-64.cmake.")
	SET(CMAKE_SYSTEM_NAME Linux)

	SET (C_WARNING_OPTS "-Wall -Wextra -Wno-unknown-pragmas -Wno-switch  -Wno-error=unused-but-set-variable")
	SET (CXX_WARNING_OPTS "-Wall -Wextra -Wno-unknown-pragmas -Wno-invalid-offsetof -Wno-switch")
	SET (C_ARCH_OPTS "-march=x86-64 -m64")
	SET (CXX_ARCH_OPTS "-march=x86-64 -m64")
	SET (C_OPTS "-s -fno-omit-frame-pointer -std=c11 -O3 -fno-expensive-optimizations")
	SET (CXX_OPTS "-s -fno-omit-frame-pointer -std=c++11 -O3 -ffast-math")
	SET (C_SPECIAL "-fexceptions -fnon-call-exceptions -pipe")
	SET (CXX_SPECIAL "-fexceptions -fnon-call-exceptions -pthread -pipe")

	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}"	PARENT_SCOPE)

	# Force dynamic linking.
	SET (CMAKE_EXE_LINKER_FLAGS "-dynamic"	PARENT_SCOPE)
	LINK_DIRECTORIES ("/usr/lib64")

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin64"	PARENT_SCOPE)
endfunction()

function (toolchain_exe_stuff)
	# Unix (MinGW) libs.
	FOREACH (LINK_LIB "mysqlclient;rt;dl")
		TARGET_LINK_LIBRARIES ( spheresvr_release	${LINK_LIB} )
		TARGET_LINK_LIBRARIES ( spheresvr_debug		${LINK_LIB} )
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	${LINK_LIB} )
	ENDFOREACH (LINK_LIB)

	# Defines.
	SET (COMMON_DEFS "_64B;_LINUX;_GITVERSION;_EXCEPTIONS_DEBUG")
		# _64B: 32 bits architecture
		# _LINUX: linux OS
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
	FOREACH (DEF ${COMMON_DEFS})
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC ${DEF} )
	ENDFOREACH (DEF)
	TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC THREAD_TRACK_CALLSTACK NDEBUG )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC THREAD_TRACK_CALLSTACK NDEBUG _NIGHTLYBUILD )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG _PACKETDUMP _TESTEXCEPTION DEBUG_CRYPT_MSGS )
endfunction()
