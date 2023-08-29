SET (TOOLCHAIN 1)

function (toolchain_exe_stuff_common)
	#-- Setting compiler flags common to all builds.

	SET (C_WARNING_OPTS
		"-w") # this line is for warnings issued by 3rd party C code
	SET (CXX_WARNING_OPTS
		"-w")
	SET (C_OPTS		"-std=c11   -pthread -fexceptions -fnon-call-exceptions")
	SET (CXX_OPTS		"-std=c++17 -pthread -fexceptions -fnon-call-exceptions")
	SET (C_SPECIAL		"-pipe -fno-expensive-optimizations")
	SET (CXX_SPECIAL	"-pipe -ffast-math")

	SET (CMAKE_C_FLAGS	"${C_WARNING_OPTS} ${C_ARCH_OPTS} ${C_OPTS} ${C_SPECIAL} ${C_FLAGS_EXTRA}"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_ARCH_OPTS} ${CXX_OPTS} ${CXX_SPECIAL} ${CXX_FLAGS_EXTRA}"	PARENT_SCOPE)


	#-- Setting common linker flags

	 # -s and -g need to be added/removed also to/from linker flags!
	SET (CMAKE_EXE_LINKER_FLAGS	"-pthread -dynamic\
					-I/usr/local/opt/mariadb-connector-c/include/mariadb\
					-L/usr/local/opt/mariadb-connector-c/lib/mariadb\
					-lmariadb\
					${CMAKE_EXE_LINKER_FLAGS_EXTRA}"
					PARENT_SCOPE)

	IF (${ENABLE_SANITIZERS})
		SET (SANITIZER_OPTS "-fno-inline -fsanitize=address,undefined,leak -fsanitize-address-use-after-scope -fstack-protector-strong -fvtable-verify=preinit")
	ENDIF ()

	#-- Adding compiler flags per build.

	 # (note: since cmake 3.3 the generator $<COMPILE_LANGUAGE> exists).
	 # do not use " " to delimitate these flags!
	 # -s: strips debug info (remove it when debugging); -g: adds debug informations;
	 # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
	IF (TARGET spheresvr_release)
		TARGET_COMPILE_OPTIONS ( spheresvr_release	PUBLIC -s -O3 	)
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_COMPILE_OPTIONS ( spheresvr_nightly	PUBLIC -O3    )
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZERS_OPTS}")
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_OPTIONS ( spheresvr_debug	PUBLIC -ggdb3 -Og -fno-omit-frame-pointer )
		SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZERS_OPTS}")
	ENDIF (TARGET spheresvr_debug)


	#-- Setting per-build linker flags.

	 # Linking Unix libs.
	 # same here, do not use " " to delimitate these flags!
	IF (TARGET spheresvr_release)
		TARGET_LINK_LIBRARIES ( spheresvr_release	mariadb dl )
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	mariadb dl )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_LINK_LIBRARIES ( spheresvr_debug		mariadb dl )
	ENDIF (TARGET spheresvr_debug)

	#-- Set common define macros.
	
	add_definitions(-D_64BITS)
	add_definitions(-DZ_PREFIX)
	add_definitions(-D_EXCEPTIONS_DEBUG)
	# _64BITS: 64 bits architecture.
	# Z_PREFIX: Use the "z_" prefix for the zlib functions
	# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
	#   on a running environment. Also it makes sphere more stable since exceptions are local.

	#-- Set per-build define macros.

	IF (TARGET spheresvr_release)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC NDEBUG )
	ENDIF (TARGET spheresvr_release)
	IF (TARGET spheresvr_nightly)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD )
	ENDIF (TARGET spheresvr_nightly)
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP )
	ENDIF (TARGET spheresvr_debug)

endfunction()
