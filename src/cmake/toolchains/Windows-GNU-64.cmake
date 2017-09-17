SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-GNU-64.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)
	SET(ARCH_BITS		64		PARENT_SCOPE)
	ENABLE_LANGUAGE(RC)

	LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/../dlls")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin64"	PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	#-- Setting compiler flags common to all builds.

	SET (C_WARNING_OPTS	"-Wall -Wextra -Wno-unknown-pragmas -Wno-switch -Wno-error=unused-but-set-variable")
	SET (CXX_WARNING_OPTS	"-Wall -Wextra -Wno-unknown-pragmas -Wno-switch -Wno-invalid-offsetof")
	SET (C_ARCH_OPTS	"-march=x86-64 -m64")
	SET (CXX_ARCH_OPTS	"-march=x86-64 -m64")
	SET (C_OPTS		"-std=c11   -pthread -fexceptions -fnon-call-exceptions")
	SET (CXX_OPTS		"-std=c++11 -pthread -fexceptions -fnon-call-exceptions -mno-ms-bitfields")
	 # -mno-ms-bitfields is needed to fix structure packing;
	 # -mwindows: specify the subsystem, avoiding the opening of a console when launching the application.
	SET (C_SPECIAL		"-pipe -mwindows -fno-expensive-optimizations")
	SET (CXX_SPECIAL	"-pipe -mwindows -ffast-math")

	SET (CMAKE_RC_FLAGS	"--target=pe-x86-64"							PARENT_SCOPE)
	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL}"	PARENT_SCOPE)


	#-- Setting common linker flags

	 # Force dynamic linking but include into exe libstdc++ and libgcc.
	 # -pthread, -s and -g need to be added/removed also to/from linker flags!
	SET (CMAKE_EXE_LINKER_FLAGS	"-pthread -dynamic -static-libstdc++ -static-libgcc"		PARENT_SCOPE)


	#-- Adding compiler flags per build.

	 # (note: since cmake 3.3 the generator $<COMPILE_LANGUAGE> exists).
	 # do not use " " to delimitate these flags!
	 # -s: strips debug info (remove it when debugging); -g: adds debug informations;
	 # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
	TARGET_COMPILE_OPTIONS ( spheresvr_release	PUBLIC -s -O3 				)
	TARGET_COMPILE_OPTIONS ( spheresvr_debug	PUBLIC -g -Og -fno-omit-frame-pointer	)
	TARGET_COMPILE_OPTIONS ( spheresvr_nightly	PUBLIC -s -O3 				)


	#-- Setting per-build linker flags.

	 # Linking Unix (MinGW) libs.
	 # same here, do not use " " to delimitate these flags!
	TARGET_LINK_LIBRARIES ( spheresvr_release	mysql64 ws2_32	-s )
	TARGET_LINK_LIBRARIES ( spheresvr_debug		mysql64 ws2_32	-g )
	TARGET_LINK_LIBRARIES ( spheresvr_nightly	mysql64 ws2_32	-s )


	#-- Set common define macros.

	SET (COMMON_DEFS "_WIN32;_WIN64;_64BITS;Z_PREFIX;_GITVERSION;_MTNETWORK;_EXCEPTIONS_DEBUG;_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS")
		# _WIN32: always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		# _WIN64: 64 bits windows application. Keeping it for compatibility with external code and libraries.
		# _64BITS: 64 bits architecture.
		# _MTNETWORK: multi-threaded networking support.
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
		# _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		# _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.
	FOREACH (DEF ${COMMON_DEFS})
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC ${DEF} )
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC ${DEF} )
	ENDFOREACH (DEF)


	#-- Set per-build define macros.

	TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD )
	TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP )
endfunction()
