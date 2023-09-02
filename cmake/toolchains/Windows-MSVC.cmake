SET (TOOLCHAIN 1)

function (toolchain_force_compiler)
	# Already managed by the generator.
	#SET (CMAKE_C_COMPILER 	"...cl.exe" 	CACHE STRING "C compiler" 	FORCE)
	#SET (CMAKE_CXX_COMPILER "...cl.exe" 	CACHE STRING "C++ compiler" FORCE)
endfunction ()


function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)

	SET (EXE_LINKER_EXTRA "")
	IF (${WIN32_SPAWN_CONSOLE} EQUAL TRUE)
		SET (EXE_LINKER_EXTRA "${EXE_LINKER_EXTRA} /SUBSYSTEM:CONSOLE /ENTRY:WinMainCRTStartup")
		SET (PREPROCESSOR_DEFS_EXTRA	"_WINDOWS_CONSOLE")
	ELSE ()
		SET (EXE_LINKER_EXTRA "${EXE_LINKER_EXTRA} /SUBSYSTEM:WINDOWS")
	ENDIF ()

	SET (ENABLED_SANITIZER false)
	IF (${USE_ASAN})
		IF (${MSVC_TOOLSET_VERSION} LESS_EQUAL 141) # VS 2017
			MESSAGE (FATAL_ERROR "This MSVC version doesn't yet support LSAN.")
		ENDIF()
		SET (C_FLAGS_EXTRA 		"${C_FLAGS_EXTRA}   /fsanitize=address")
		SET (CXX_FLAGS_EXTRA 	"${CXX_FLAGS_EXTRA} /fsanitize=address")
		SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_LSAN})
		MESSAGE (FATAL_ERROR "MSVC doesn't yet support LSAN.")
	ENDIF ()
	IF (${USE_UBSAN})
		MESSAGE (FATAL_ERROR "MSVC doesn't yet support LSAN.")
	ENDIF ()
	IF (${ENABLED_SANITIZER})
		SET (PREPROCESSOR_DEFS_EXTRA "${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS")
	ENDIF ()

	#-- Base compiler and linker flags are the same for every build type.
	 # For vcxproj structure or for cmake generator's fault, CXX flags will be used also for C sources;
	 # until this gets fixed, CXX and C compiler and linker flags should be the same.

	 # Setting the Visual Studio warning level to 4 and forcing MultiProccessor compilation
	SET (C_FLAGS_COMMON		"${C_FLAGS_EXTRA} /W4 /MP /GR /fp:fast\
							/wd4127 /wd4131 /wd4310 /wd4996 /wd4701 /wd4703 /wd26812")
	 #						# Disable warnings caused by external c libraries.
	 # For zlib: C4244, C4245

	SET (CXX_FLAGS_COMMON	"${CXX_FLAGS_EXTRA} /W4 /MP /GR /fp:fast /std:c++17\
							/wd4127 /wd4131 /wd4310 /wd4996 /wd4701 /wd4703 /wd26812")

	# /Zc:__cplusplus is required to make __cplusplus accurate
	# /Zc:__cplusplus is available starting with Visual Studio 2017 version 15.7
	# (according to https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	# That version is equivalent to _MSC_VER==1914
	# (according to https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019)
	# CMake's ${MSVC_VERSION} is equivalent to _MSC_VER
	# (according to https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html#variable:MSVC_VERSION)
	if (MSVC_VERSION GREATER_EQUAL 1914)
		SET(CXX_FLAGS_COMMON "${CXX_FLAGS_COMMON} /Zc:__cplusplus")
	endif()

	 # These linker flags shouldn't be applied to Debug release.
	IF (CMAKE_CL_64)	# 64 bits
		SET(ARCH_BITS	64	PARENT_SCOPE)
		SET(LINKER_FLAGS_NODEBUG "/OPT:REF,ICF"			)
	ELSE (CMAKE_CL_64)	# 32 bits
		SET(ARCH_BITS	32	PARENT_SCOPE)
		SET(LINKER_FLAGS_NODEBUG "/OPT:REF,ICF"			)
	ENDIF (CMAKE_CL_64)


	#-- Release compiler and linker flags.
	
	SET (CMAKE_C_FLAGS_RELEASE			"${C_FLAGS_COMMON}   /O2 /EHsc /GL /GA /Gw"			PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_RELEASE		"${CXX_FLAGS_COMMON} /O2 /EHsc /GL /GA /Gw /Gy"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_RELEASE	"${LINKER_FLAGS_NODEBUG} ${EXE_LINKER_EXTRA}\
										 /LTCG" PARENT_SCOPE)

	#-- Nightly compiler and linker flags.

	SET (CMAKE_C_FLAGS_NIGHTLY			"${C_FLAGS_COMMON}   /O2 /EHa /GL /GA /Gw"			PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"${CXX_FLAGS_COMMON} /O2 /EHa /GL /GA /Gw /Gy"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${LINKER_FLAGS_NODEBUG} ${EXE_LINKER_EXTRA}\
										 /LTCG" PARENT_SCOPE)

	#-- Debug compiler and linker flags.

	SET (CMAKE_C_FLAGS_DEBUG			"${C_FLAGS_COMMON}   /Od /EHsc /Oy-"				PARENT_SCOPE)
	SET (CMAKE_CXX_FLAGS_DEBUG			"${CXX_FLAGS_COMMON} /Od /EHsc /Oy- /MDd /ZI /ob0"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_DEBUG	"/DEBUG /SAFESEH:NO ${EXE_LINKER_EXTRA}" PARENT_SCOPE)


	#-- Set mysql .lib directory for the linker.

	IF (CMAKE_CL_64)
		LINK_DIRECTORIES ("lib/bin/x86_64/mariadb/")
	ELSE (CMAKE_CL_64)
		LINK_DIRECTORIES ("lib/bin/x86/mariadb/")
	ENDIF (CMAKE_CL_64)
	
endfunction()


function (toolchain_exe_stuff)
	#-- Windows libraries to link against.
	TARGET_LINK_LIBRARIES ( spheresvr	ws2_32 libmariadb )

	#-- Set define macros.

	 # Architecture defines
	IF (CMAKE_CL_64)
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	PUBLIC _64BITS _WIN64	)
	ELSE (CMAKE_CL_64)
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	PUBLIC _32BITS		)
	ENDIF (CMAKE_CL_64)

	 # Common defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr PUBLIC
		${PREPROCESSOR_DEFS_EXTRA}
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
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin-x86_64/")
	ELSE ()
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin-x86/")
	ENDIF ()
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DIRECTORY	"${OUTDIR}"		)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE	"${OUTDIR}/Release"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY	"${OUTDIR}/Nightly"	)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG		"${OUTDIR}/Debug"	)

	#-- Custom .vcxproj settings (for now, it only affects the debugger working directory).

	SET (SRCDIR ${CMAKE_SOURCE_DIR}) # for the sake of shortness
	CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/spheresvr.vcxproj.user.in" "${CMAKE_BINARY_DIR}/spheresvr.vcxproj.user" @ONLY)
endfunction()
