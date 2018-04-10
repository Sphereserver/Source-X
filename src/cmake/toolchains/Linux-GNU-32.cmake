SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Linux-GNU-32.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Linux"      PARENT_SCOPE)
	SET(ARCH_BITS		32		PARENT_SCOPE)

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY	"${CMAKE_BINARY_DIR}/bin"	PARENT_SCOPE)
endfunction()


function (toolchain_exe_stuff)
	#-- Setting compiler flags common to all builds.

	SET (C_WARNING_OPTS
		"-Wall -Wextra -Wno-nonnull-compare -Wno-unknown-pragmas -Wno-format -Wno-switch -Wno-implicit-fallthrough\
		-Wno-parentheses -Wno-misleading-indentation -Wno-strict-aliasing -Wno-unused-result\
		-Wno-error=unused-but-set-variable -Wno-maybe-uninitialized -Wno-implicit-function-declaration") # this line is for warnings issued by 3rd party C code
	SET (CXX_WARNING_OPTS
		"-Wall -Wextra -Wno-nonnull-compare -Wno-unknown-pragmas -Wno-format -Wno-switch -Wno-implicit-fallthrough\
		-Wno-parentheses -Wno-misleading-indentation -Wno-conversion-null -Wno-invalid-offsetof -Wno-unused-result")
	SET (C_ARCH_OPTS	"-march=i686 -m32")
	SET (CXX_ARCH_OPTS	"-march=i686 -m32")
	SET (C_OPTS		"-std=c11   -pthread -fexceptions -fnon-call-exceptions")
	SET (CXX_OPTS		"-std=c++17 -pthread -fexceptions -fnon-call-exceptions")
	SET (C_SPECIAL		"-pipe -fno-expensive-optimizations")
	SET (CXX_SPECIAL	"-pipe -ffast-math")

	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL} ${C_FLAGS_EXTRA}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL} ${CXX_FLAGS_EXTRA}"	PARENT_SCOPE)


	#-- Setting common linker flags

	 # -s and -g need to be added/removed also to/from linker flags!
	SET (CMAKE_EXE_LINKER_FLAGS	"-pthread -dynamic\
					-L/usr/lib/mysql -L/usr/lib/i386-linux-gnu/mysql\
					-Wl,-rpath=/usr/lib/mysql -Wl,-rpath=/usr/lib/i386-linux-gnu/mysql"
					PARENT_SCOPE)


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

	 # Linking Unix libs.
	 # same here, do not use " " to delimitate these flags!
	IF (TARGET spheresvr_release)
		TARGET_LINK_LIBRARIES ( spheresvr_release	mysqlclient rt dl )
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	mysqlclient rt dl )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_LINK_LIBRARIES ( spheresvr_debug		mysqlclient rt dl )
	ENDIF (TARGET spheresvr_debug)


	#-- Set common define macros.

	SET (COMMON_DEFS "_32BITS;_LINUX;_LIBEV;Z_PREFIX;_POSIX_SOURCE;_GITVERSION;_MTNETWORK;_EXCEPTIONS_DEBUG")
		# _32BITS: 32 bits architecture.
		# _LINUX: linux OS.
		# _LIBEV: use libev
		# Z_PREFIX: Use the "z_" prefix for the zlib functions
		# _POSIX_SOURCE: needed for libev compilation in some linux distributions (doesn't seem to affect compilation on distributions that don't need it)
		# _MTNETWORK: multi-threaded networking support.
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
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
