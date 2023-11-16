function (toolchain_force_compiler)
	SET (CMAKE_C_COMPILER 	"gcc" 	CACHE STRING "C compiler" 	FORCE)
	SET (CMAKE_CXX_COMPILER "g++" 	CACHE STRING "C++ compiler" FORCE)
endfunction ()


function (toolchain_after_project_common)
	ENABLE_LANGUAGE(RC)
endfunction ()


function (toolchain_exe_stuff_common)
	IF (${WIN32_SPAWN_CONSOLE})
		SET (CMAKE_EXE_LINKER_FLAGS_EXTRA  "${CMAKE_EXE_LINKER_FLAGS_EXTRA} -mconsole")
		SET (PREPROCESSOR_DEFS_EXTRA	"_WINDOWS_CONSOLE")
	#ELSE ()
	#	SET (CMAKE_EXE_LINKER_FLAGS_EXTRA "${CMAKE_EXE_LINKER_FLAGS_EXTRA} -mwindows")
	ENDIF ()

	#SET (ENABLED_SANITIZER false)
	IF (${USE_ASAN})
		MESSAGE (FATAL_ERROR "MinGW-GCC doesn't yet support ASAN")
		SET (USE_ASAN false)
		#SET (C_FLAGS_EXTRA 		"${C_FLAGS_EXTRA}   -fsanitize=address -fsanitize-address-use-after-scope")
		#SET (CXX_FLAGS_EXTRA 	"${CXX_FLAGS_EXTRA} -fsanitize=address -fsanitize-address-use-after-scope")
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_MSAN})
		MESSAGE (FATAL_ERROR "MinGW-GCC doesn't yet support MSAN")
		SET (USE_MSAN false)
		#SET (C_FLAGS_EXTRA 		"${C_FLAGS_EXTRA}   -fsanitize=memory -fsanitize-memory-track-origins=2 -fPIE")
		#SET (CXX_FLAGS_EXTRA 	"${CXX_FLAGS_EXTRA} -fsanitize=memory -fsanitize-memory-track-origins=2 -fPIE")
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_LSAN})
		MESSAGE (FATAL_ERROR "MinGW-GCC doesn't yet support LSAN")
		SET (USE_LSAN false)
		#SET (C_FLAGS_EXTRA 		"${C_FLAGS_EXTRA}   -fsanitize=leak")
		#SET (CXX_FLAGS_EXTRA 	"${CXX_FLAGS_EXTRA} -fsanitize=leak")
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_UBSAN})
		MESSAGE (FATAL_ERROR "MinGW-GCC doesn't yet support UBSAN")
		SET (USE_UBSAN false)
#		SET (UBSAN_FLAGS		"-fsanitize=undefined,\
#shift,integer-divide-by-zero,vla-bound,null,signed-integer-overflow,bounds,\
#float-divide-by-zero,float-cast-overflow,pointer-overflow,\
#unreachable,nonnull-attribute,returns-nonnull-attribute \
#-fno-sanitize=enum")
		#SET (C_FLAGS_EXTRA 		"${C_FLAGS_EXTRA}   ${UBSAN_FLAGS}")
		#SET (CXX_FLAGS_EXTRA 	"${CXX_FLAGS_EXTRA} ${UBSAN_FLAGS} -fsanitize=return,vptr")
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	#IF (${ENABLED_SANITIZER})
	#	SET (PREPROCESSOR_DEFS_EXTRA "${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS")
	#ENDIF ()


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
	SET (CXX_OPTS	"-std=c++17 -pthread -fexceptions -fnon-call-exceptions -mno-ms-bitfields")
	 # -mno-ms-bitfields is needed to fix structure packing
	SET (C_SPECIAL		"-pipe -fno-expensive-optimizations")
	SET (CXX_SPECIAL	"-pipe -ffast-math")

	SET (CMAKE_C_FLAGS		"${C_WARNING_OPTS} ${C_OPTS} ${C_SPECIAL} ${C_FLAGS_EXTRA}"			PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS	"${CXX_WARNING_OPTS} ${CXX_OPTS} ${CXX_SPECIAL} ${CXX_FLAGS_EXTRA}"	PARENT_SCOPE)


	#-- Setting common linker flags

	IF (${USE_MSAN})
		SET (CMAKE_EXE_LINKER_FLAGS_EXTRA	"${CMAKE_EXE_LINKER_FLAGS_EXTRA} -pie")
	ENDIF()

	 # Force dynamic linking but include into exe libstdc++ and libgcc.
	 # -pthread, -s and -g need to be added/removed also to/from linker flags!
	SET (CMAKE_EXE_LINKER_FLAGS_COMMON	"${CMAKE_EXE_LINKER_FLAGS_COMMON} -pthread -dynamic -static-libstdc++ -static-libgcc")


	#-- Adding compiler flags per build.

	 # (note: since cmake 3.3 the generator $<COMPILE_LANGUAGE> exists).
	 # do not use " " to delimitate these flags!
	 # -s: strips debug info (remove it when debugging); -g: adds debug informations;
	 # -fno-omit-frame-pointer disables a good optimization which may corrupt the debugger stack trace.
	SET (COMPILE_OPTIONS_EXTRA)
	IF (ENABLED_SANITIZER OR TARGET spheresvr_debug)
		SET (COMPILE_OPTIONS_EXTRA -fno-omit-frame-pointer -fno-inline)
	ENDIF ()
	IF (TARGET spheresvr_release)
		TARGET_COMPILE_OPTIONS ( spheresvr_release	PUBLIC -s -O3 ${COMPILE_OPTIONS_EXTRA})
	ENDIF ()
	IF (TARGET spheresvr_nightly)
		IF (ENABLED_SANITIZER)
			TARGET_COMPILE_OPTIONS ( spheresvr_nightly	PUBLIC -ggdb3 -O1 ${COMPILE_OPTIONS_EXTRA})
		ELSE ()
			TARGET_COMPILE_OPTIONS ( spheresvr_nightly	PUBLIC -O3 ${COMPILE_OPTIONS_EXTRA})
		ENDIF ()
	ENDIF ()
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_OPTIONS ( spheresvr_debug	PUBLIC -ggdb3 -Og ${COMPILE_OPTIONS_EXTRA})
	ENDIF ()


	#-- Setting per-build linker options.

	 # Linking libs the MinGW way and setting linker flags.
	IF (TARGET spheresvr_release)
		TARGET_LINK_LIBRARIES ( spheresvr_release	mariadb ws2_32 )
		TARGET_LINK_OPTIONS ( spheresvr_release PUBLIC  "SHELL:${CMAKE_EXE_LINKER_FLAGS_COMMON} ${CMAKE_EXE_LINKER_FLAGS_EXTRA}")
	ENDIF ()
	IF (TARGET spheresvr_nightly)
		TARGET_LINK_LIBRARIES ( spheresvr_nightly	mariadb ws2_32 )
		TARGET_LINK_OPTIONS ( spheresvr_nightly PUBLIC  "SHELL:${CMAKE_EXE_LINKER_FLAGS_COMMON} ${CMAKE_EXE_LINKER_FLAGS_EXTRA}")
	ENDIF ()
	IF (TARGET spheresvr_debug)
		TARGET_LINK_LIBRARIES ( spheresvr_debug		mariadb ws2_32 )
		TARGET_LINK_OPTIONS ( spheresvr_debug PUBLIC  "SHELL:${CMAKE_EXE_LINKER_FLAGS_COMMON} ${CMAKE_EXE_LINKER_FLAGS_EXTRA}")
	ENDIF ()
	

	#-- Set common define macros.

	add_compile_definitions(${PREPROCESSOR_DEFS_EXTRA} _WIN32 Z_PREFIX _GITVERSION _EXCEPTIONS_DEBUG _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS)
		# _WIN32: always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		# Z_PREFIX: Use the "z_" prefix for the zlib functions
		# _EXCEPTIONS_DEBUG: Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
		#   on a running environment. Also it makes sphere more stable since exceptions are local.
		# _CRT_SECURE_NO_WARNINGS: Temporary setting to do not spam so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		# _WINSOCK_DEPRECATED_NO_WARNINGS: Removing warnings until the code gets updated or reviewed.


	#-- Add per-build define macros.

	IF (TARGET spheresvr_release)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_release	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK )
	ENDIF ()
	IF (TARGET spheresvr_nightly)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_nightly	PUBLIC NDEBUG THREAD_TRACK_CALLSTACK _NIGHTLYBUILD )
	ENDIF ()
	IF (TARGET spheresvr_debug)
		TARGET_COMPILE_DEFINITIONS ( spheresvr_debug	PUBLIC _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP )
	ENDIF ()


	#-- Set different output folders for each build type
	# (When we'll have support for multi-target builds...)
	#SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Release"	)
	#SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG		"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Debug"	)
	#SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY	"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Nightly"	)

endfunction()