function (toolchain_exe_stuff_common)
	#-- Setting compiler flags common to all builds.

	SET (C_WARNING_OPTS
		"-Wall -Wextra -Wno-pragmas -Wno-unknown-pragmas -Wno-format -Wno-switch -Wno-parentheses -Wno-implicit-fallthrough\
		-Wno-unused-variable -Wno-unused-function -Wno-unused-parameter -Wno-uninitialized -Wno-error=maybe-uninitialized -Wno-error=unused-but-set-variable\
		-Wno-implicit-function-declaration -Wno-type-limits -Wno-incompatible-pointer-types -Wno-array-bounds")
		# last 2 lines are for warnings issued by 3rd party C code
	SET (CXX_WARNING_OPTS
		"-Wall -Wextra -Wno-pragmas -Wno-unknown-pragmas -Wno-format -Wno-switch -Wno-parentheses -Wno-conversion-null -Wno-misleading-indentation -Wno-implicit-fallthrough")
	#SET (C_ARCH_OPTS	) # set in parent toolchain
	#SET (CXX_ARCH_OPTS	) # set in parent toolchain
	SET (C_OPTS		"-std=c11   -pthread -fexceptions -fnon-call-exceptions")
	SET (CXX_OPTS		"-std=c++17 -pthread -fexceptions -fnon-call-exceptions -mno-ms-bitfields")
	 # -mno-ms-bitfields is needed to fix structure packing;
	 # -mwindows: specify the subsystem, avoiding the opening of a console when launching the application.
	SET (C_SPECIAL		"-pipe -mwindows -fno-expensive-optimizations")
	SET (CXX_SPECIAL	"-pipe -mwindows -ffast-math")

	SET (CMAKE_RC_FLAGS	"--target=pe-i386"							PARENT_SCOPE)
	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_OPTS} ${C_SPECIAL} ${C_FLAGS_EXTRA}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_OPTS} ${CXX_SPECIAL} ${CXX_FLAGS_EXTRA}"	PARENT_SCOPE)


	#-- Setting common linker flags

	 # Force dynamic linking but include into exe libstdc++ and libgcc.
	 # -pthread, -s and -g need to be added/removed also to/from linker flags!
	SET (CMAKE_EXE_LINKER_FLAGS	"-pthread -dynamic -static-libstdc++ -static-libgcc"		PARENT_SCOPE)


	#-- Adding compiler flags per build.

	 # (note: since cmake 3.3 the generator $<COMPILE_LANGUAGE> exists).
	 # do not use " " to delimitate these flags!
	 # -s: strips debug info (remove it when debugging); -g: adds debug informations;
	 # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
	IF (TARGET spheresvr_release)
		TARGET_COMPILE_OPTIONS ( spheresvr_release	PUBLIC -s -O3 	)
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_COMPILE_OPTIONS ( spheresvr_nightly	PUBLIC -s -O3    )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_OPTIONS ( spheresvr_debug	PUBLIC -ggdb3 -Og -fno-omit-frame-pointer	)
	ENDIF (TARGET spheresvr_debug)


	#-- Setting per-build linker flags.

	 # Linking Unix (MinGW) libs.
	 # same here, do not use " " to delimitate these flags!
	IF (TARGET spheresvr_release)
		TARGET_LINK_LIBRARIES ( spheresvr_release	mysql ws2_32 )
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	mysql ws2_32 )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_LINK_LIBRARIES ( spheresvr_debug		mysql ws2_32 )
	ENDIF (TARGET spheresvr_debug)


	#-- Set common define macros.

	SET (COMMON_DEFS "_WIN32;Z_PREFIX;_GITVERSION;_EXCEPTIONS_DEBUG;_CRT_SECURE_NO_WARNINGS;_WINSOCK_DEPRECATED_NO_WARNINGS")
		# _WIN32: always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		# Z_PREFIX: Use the "z_" prefix for the zlib functions
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
		# _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		# _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.
	FOREACH (DEF ${COMMON_DEFS})
		IF (TARGET spheresvr_release)
			TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC ${DEF} )
		ENDIF (TARGET spheresvr_release)
		IF (TARGET spheresvr_nightly)
			TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC ${DEF} )
		ENDIF (TARGET spheresvr_nightly)
		IF (TARGET spheresvr_debug)
			TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC ${DEF} )
		ENDIF (TARGET spheresvr_debug)
	ENDFOREACH (DEF)


	#-- Set per-build define macros.

	IF (TARGET spheresvr_release)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK )
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP )
	ENDIF (TARGET spheresvr_debug)
endfunction()