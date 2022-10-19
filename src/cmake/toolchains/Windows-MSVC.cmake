SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)


	#-- Base compiler and linker flags are the same for every build type.
	 # For vcxproj structure or for cmake generator's fault, CXX flags will be used also for C sources;
	 # until this gets fixed, CXX and C compiler and linker flags should be the same.

	 # Setting the Visual Studio warning level to 4 and forcing MultiProccessor compilation
	SET (C_FLAGS_COMMON		"${C_FLAGS_EXTRA} /W4 /MP /GR /fp:fast\
					/wd4127 /wd4131 /wd4310 /wd4996 /wd4701 /wd4703 /wd26812"		)
	 #				# Disable warnings caused by external c libraries.
	 # For zlib: C4244, C4245

	SET (CXX_FLAGS_COMMON		"${CXX_FLAGS_EXTRA} /W4 /MP /GR /fp:fast /std:c++17\
					/wd4127 /wd4131 /wd4310 /wd4996 /wd4701 /wd4703 /wd26812"		)

	 # Setting the exe to be a GUI application and not a console one.
	SET (LINKER_FLAGS_COMMON	"/SUBSYSTEM:WINDOWS"		)

	 # These linker flags shouldn't be applied to Debug release.
	IF (CMAKE_CL_64)	# 64 bits
		SET(ARCH_BITS		64			PARENT_SCOPE)
		SET(LINKER_FLAGS_NODEBUG "/OPT:REF,ICF"			)
	ELSE (CMAKE_CL_64)	# 32 bits
		SET(ARCH_BITS		32			PARENT_SCOPE)
		SET(LINKER_FLAGS_NODEBUG "/OPT:REF,ICF"			)
	ENDIF (CMAKE_CL_64)


	#-- Release compiler and linker flags.
	
	SET (CMAKE_C_FLAGS_RELEASE		"${C_FLAGS_COMMON}   /O2 /EHsc /GL /GA /Gw"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_RELEASE		"${CXX_FLAGS_COMMON} /O2 /EHsc /GL /GA /Gw /Gy"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_RELEASE	"${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_NODEBUG} /LTCG"	PARENT_SCOPE)


	#-- Debug compiler and linker flags.

	SET (CMAKE_C_FLAGS_DEBUG		"${C_FLAGS_COMMON}   /Od /EHsc /Oy-"			PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_DEBUG		"${CXX_FLAGS_COMMON} /Od /EHsc /Oy- /MDd /ZI /ob0"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_DEBUG	"${LINKER_FLAGS_COMMON} /DEBUG /SAFESEH:NO"		PARENT_SCOPE)


	#-- Nightly compiler and linker flags.

	SET (CMAKE_C_FLAGS_NIGHTLY		"${C_FLAGS_COMMON}   /O2 /EHa /GL /GA /Gw"		PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"${CXX_FLAGS_COMMON} /O2 /EHa /GL /GA /Gw /Gy"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${LINKER_FLAGS_COMMON} ${LINKER_FLAGS_NODEBUG}\
						/LTCG"							PARENT_SCOPE)


	#-- Set mysql .lib directory for the linker.

	IF (CMAKE_CL_64)
		LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/../dlls/64/")
	ELSE (CMAKE_CL_64)
		LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/../dlls/32/")
	ENDIF (CMAKE_CL_64)
	
endfunction()


function (toolchain_exe_stuff)
	#-- Windows libraries to link against.

	TARGET_LINK_LIBRARIES ( spheresvr	libmysql ws2_32 )


	#-- Set define macros.

	 # Architecture defines
	IF (CMAKE_CL_64)
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	PUBLIC _64BITS _WIN64	)
	ELSE (CMAKE_CL_64)
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	PUBLIC _32BITS		)
	ENDIF (CMAKE_CL_64)

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

		$<$<CONFIG:Nightly>:	_NIGHTLYBUILD THREAD_TRACK_CALLSTACK>

		$<$<CONFIG:Debug>:	_DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP>
	)


	#-- Custom output directory.

	IF (CMAKE_CL_64)
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin64/")
	ELSE (CMAKE_CL_64)
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin/")
	ENDIF (CMAKE_CL_64)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DIRECTORY	"${OUTDIR}"		)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE	"${OUTDIR}/Release"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG		"${OUTDIR}/Debug"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY	"${OUTDIR}/Nightly"	)

	#-- Custom .vcxproj settings (for now, it only affects the debugger working directory).

	SET(SRCDIR ${CMAKE_SOURCE_DIR}) # for the sake of shortness
	CONFIGURE_FILE("cmake/spheresvr.vcxproj.user.in" "${CMAKE_BINARY_DIR}/spheresvr.vcxproj.user" @ONLY)
endfunction()
