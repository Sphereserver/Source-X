SET (TOOLCHAIN 1)

SET(ARCH_BITS			32)
SET(ENABLE_SANITIZERS	true CACHE BOOL "Use sanitizers for the nightly build. ONLY FOR TESTING PURPOSES!")

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-clang-MSVC-32.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)
	ENABLE_LANGUAGE(RC)


	#-- Base compiler and linker flags are the same for every build type.

	 # Setting the Visual Studio warning level to 4 and forcing MultiProccessor compilation
	 # For vcxproj structure or for cmake generator's fault, CXX flags will be used also for C sources;
	 # until this gets fixed, CXX and C compiler and linker flags should be the same.
	SET (C_FLAGS_COMMON		"${C_FLAGS_EXTRA} /W4 /MP /GR\
					-Wno-unused-value -Wno-tautological-undefined-compare -Wno-overloaded-virtual"		)

	SET (CXX_FLAGS_COMMON	"${CXX_FLAGS_EXTRA} /W4 /MP /GR /std:c++17\
					-Wno-unused-value -Wno-tautological-undefined-compare -Wno-overloaded-virtual"		)

	 # Setting the exe to be a GUI application and not a console one.
	SET (LINKER_FLAGS_COMMON		"/SUBSYSTEM:WINDOWS /OPT:REF,ICF")
	IF (${ENABLE_SANITIZERS})
		SET (LINKER_FLAGS_COMMON 	"${LINKER_FLAGS_COMMON} /DEBUG")
	ENDIF (${ENABLE_SANITIZERS})

	#-- Release compiler and linker flags.
	
	IF (${ENABLE_SANITIZERS})
		# We need to optimize for speed (/O2) when using sanitizers otherwise the execution will be too slow
		SET (SANITIZERS_OPTS_R "/O2 /Oy- -Xclang -fno-inline /Zo -gcodeview -gdwarf -fsanitize=address,undefined")
	ELSE (${ENABLE_SANITIZERS})
		SET (SANITIZERS_OPTS_R "/O2 /Gy /flto=full -fwhole-program-vtables")
	ENDIF (${ENABLE_SANITIZERS})
	SET (CMAKE_C_FLAGS_RELEASE			"${C_FLAGS_COMMON}   /EHsc /GA /Gw ${SANITIZERS_OPTS_R}"	PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_RELEASE		"${CXX_FLAGS_COMMON} /EHsc /GA /Gw ${SANITIZERS_OPTS_R}"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_RELEASE	"${LINKER_FLAGS_COMMON}"									PARENT_SCOPE)


	#-- Nightly compiler and linker flags.

	IF (${ENABLE_SANITIZERS})
		# We need to optimize for speed (/O2) when using sanitizers otherwise the execution will be too slow
		SET (SANITIZERS_OPTS_N "/O2 /Oy- -Xclang -fno-inline /Zo -gcodeview -gdwarf -fsanitize=address,undefined")
	ELSE (${ENABLE_SANITIZERS})
		SET (SANITIZERS_OPTS_N "/O2 /Gy /flto=full -fwhole-program-vtables")
	ENDIF (${ENABLE_SANITIZERS})
	SET (CMAKE_C_FLAGS_NIGHTLY			"${C_FLAGS_COMMON}   ${SANITIZERS_OPTS_N} /EHsc /GA /Gw"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"${CXX_FLAGS_COMMON} ${SANITIZERS_OPTS_N} /EHsc /GA /Gw"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${LINKER_FLAGS_COMMON}"										PARENT_SCOPE)


	#-- Debug compiler and linker flags.

	IF (${ENABLE_SANITIZERS})
		# We need to optimize for speed (/O2) when using sanitizers otherwise the execution will be too slow
		SET (SANITIZERS_OPTS_D "/O2 /Oy- -Xclang -fno-inline -fsanitize=address,undefined")
	ELSE (${ENABLE_SANITIZERS})
		SET (SANITIZERS_OPTS_D "/Od /Oy-")
	ENDIF (${ENABLE_SANITIZERS})
	SET (CMAKE_C_FLAGS_DEBUG			"${C_FLAGS_COMMON}   /EHsc ${SANITIZERS_OPTS_D}"			PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_DEBUG			"${CXX_FLAGS_COMMON} /EHsc /MDd /Zi -gcodeview -gdwarf\
										 ${SANITIZERS_OPTS_D}"										PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_DEBUG	"${LINKER_FLAGS_COMMON} /DEBUG"								PARENT_SCOPE)


	#-- Set .lib directory for the linker.

	LINK_DIRECTORIES ("lib/bin/x86/mariadb/")
	
endfunction()


function (toolchain_exe_stuff)
	#-- Windows libraries to link against.

	IF (${ENABLE_SANITIZERS})
		SET (SANITIZERS_LNK clang_rt.asan_dynamic_runtime_thunk-x86_64 clang_rt.asan_dynamic-x86_64 clang_rt.asan-preinit-x86_64 clang_rt.builtins-x86_64)
	ENDIF (${ENABLE_SANITIZERS})
	TARGET_LINK_LIBRARIES ( spheresvr	libmariadb ws2_32 ${SANITIZERS_LNK})


	#-- Set define macros.

	 # Architecture defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr 	PUBLIC _32BITS		)

	 # Common defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr PUBLIC
	  # _WIN32 is always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		_WIN32
      # Use the "z_" prefix for the zlib functions
		Z_PREFIX
	  # GIT defs.
		_GITVERSION
	  # Temporary setting _CRT_SECURE_NO_WARNINGS to do not spam
	  #  so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		_CRT_SECURE_NO_WARNINGS
	  # Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
	  #  on a running environment. Also it makes sphere more stable since exceptions are local.
		_EXCEPTIONS_DEBUG
	  
	  # Removing WINSOCK warnings until the code gets updated or reviewed.
		_WINSOCK_DEPRECATED_NO_WARNINGS

	 # Per-build defines
		$<$<OR:$<CONFIG:Release>,$<CONFIG:Nightly>>: NDEBUG >

		$<$<CONFIG:Nightly>: _NIGHTLYBUILD $<$<NOT:$<BOOL:${ENABLE_SANITIZERS}>>:THREAD_TRACK_CALLSTACK> >

		$<$<CONFIG:Debug>:	 _DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP>
	)


	#-- Custom output directory.

	SET(OUTDIR "${CMAKE_BINARY_DIR}/bin/")
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DIRECTORY	"${OUTDIR}"		)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE	"${OUTDIR}/Release"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG		"${OUTDIR}/Debug"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY	"${OUTDIR}/Nightly"	)

	#-- Custom .vcxproj settings (for now, it only affects the debugger working directory).

	CONFIGURE_FILE("cmake/spheresvr.vcxproj.user.in" "${CMAKE_BINARY_DIR}/spheresvr.vcxproj.user" @ONLY)
	
endfunction()
