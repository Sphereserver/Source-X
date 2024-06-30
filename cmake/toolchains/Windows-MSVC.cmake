SET (TOOLCHAIN_LOADED 1)

function (toolchain_force_compiler)
	# Already managed by the generator.
	#SET (CMAKE_C_COMPILER 	"...cl.exe" 	CACHE STRING "C compiler" 	FORCE)
	#SET (CMAKE_CXX_COMPILER "...cl.exe" 	CACHE STRING "C++ compiler" FORCE)

	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	#SET(CMAKE_SYSTEM_NAME	"Windows"						PARENT_SCOPE)
endfunction ()


function (toolchain_after_project)
#[[
	IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
		MESSAGE (STATUS "Detected 64 bits architecture")
		SET(ARCH_BITS	64	CACHE INTERNAL "" FORCE) # override
	ELSE ()
		MESSAGE (STATUS "Detected 32 bits architecture")
		SET(ARCH_BITS	32	CACHE INTERNAL "" FORCE) # override
	ENDIF ()
]]
	if (NOT CMAKE_VS_PLATFORM_NAME)
		set (CMAKE_VS_PLATFORM_NAME "${CMAKE_VS_PLATFORM_NAME_DEFAULT}")
	endif ()
	if (${CMAKE_VS_PLATFORM_NAME} STREQUAL "Win32")
	set (CMAKE_SYSTEM_PROCESSOR "x86" CACHE INTERNAL "" FORCE)
	endif ()
	set (CMAKE_SYSTEM_PROCESSOR "${CMAKE_VS_PLATFORM_NAME}" CACHE INTERNAL "" FORCE)

	include ("${CMAKE_SOURCE_DIR}/cmake/CMakeDetectArch.cmake")

	MESSAGE (STATUS "Target Arch: ${ARCH}")
	MESSAGE (STATUS "Generating for MSVC platform ${CMAKE_VS_PLATFORM_NAME}.")
endfunction()


function (toolchain_exe_stuff)

	#-- Set mariadb .lib directory for the linker.

	IF (CMAKE_CL_64)
		TARGET_LINK_DIRECTORIES (spheresvr PRIVATE "lib/_bin/x86_64/mariadb/")
	ELSE ()
		TARGET_LINK_DIRECTORIES (spheresvr PRIVATE "lib/_bin/x86/mariadb/")
	ENDIF ()


	#-- Configure the Windows application type and add global linker flags.

	IF (${WIN32_SPAWN_CONSOLE})
		add_link_options ("LINKER:/ENTRY:WinMainCRTStartup")	# Handled by is_win32_app_linker -> "LINKER:/SUBSYSTEM:CONSOLE"
		SET (PREPROCESSOR_DEFS_EXTRA	_WINDOWS_CONSOLE)
	#ELSE ()
	#	add_link_options ("LINKER:/ENTRY:WinMainCRTStartup") 	# Handled by is_win32_app_linker -> "LINKER: /SUBSYSTEM:WINDOWS"
	ENDIF ()


	#-- Validate sanitizers options and store them between the common compiler flags.

	SET (ENABLED_SANITIZER false)
	IF (${USE_ASAN})
		IF (${MSVC_TOOLSET_VERSION} LESS_EQUAL 141) # VS 2017
			MESSAGE (FATAL_ERROR "This MSVC version doesn't yet support ASAN. Use a newer one instead.")
		ENDIF()
		SET (CXX_FLAGS_EXTRA 	${CXX_FLAGS_EXTRA} /fsanitize=address)
		SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_MSAN})
		MESSAGE (FATAL_ERROR "MSVC doesn't yet support MSAN.")
		SET (USE_MSAN false)
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_LSAN})
		MESSAGE (FATAL_ERROR "MSVC doesn't yet support LSAN.")
		SET (USE_LSAN false)
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${USE_UBSAN})
		MESSAGE (FATAL_ERROR "MSVC doesn't yet support UBSAN.")
		SET (USE_UBSAN false)
		#SET (ENABLED_SANITIZER true)
	ENDIF ()
	IF (${ENABLED_SANITIZER})
		SET (PREPROCESSOR_DEFS_EXTRA	${PREPROCESSOR_DEFS_EXTRA} _SANITIZERS)
	ENDIF ()


	#-- Apply compiler flags.

	SET (cxx_compiler_flags_common
		${CXX_FLAGS_EXTRA} /W4 /MP /GR /fp:fast /std:c++20
		/wd4310	# cast truncates constant value
		/wd4996 # use of deprecated stuff
		/wd4701 # Potentially uninitialized local variable 'name' used
		/wd4702 # Unreachable code
		/wd4703 # Potentially uninitialized local pointer variable 'name' used
		/wd4127 # The controlling expression of an if statement or while loop evaluates to a constant.
		/wd26812# The enum type 'type-name' is unscoped. Prefer 'enum class' over 'enum'
	)

	# /Zc:__cplusplus is required to make __cplusplus accurate
	# /Zc:__cplusplus is available starting with Visual Studio 2017 version 15.7
	# (according to https://learn.microsoft.com/en-us/cpp/build/reference/zc-cplusplus)
	# That version is equivalent to _MSC_VER==1914
	# (according to https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros?view=vs-2019)
	# CMake's ${MSVC_VERSION} is equivalent to _MSC_VER
	# (according to https://cmake.org/cmake/help/latest/variable/MSVC_VERSION.html#variable:MSVC_VERSION)
	if (MSVC_VERSION GREATER_EQUAL 1914)
		SET(cxx_compiler_flags_common	${cxx_compiler_flags_common} /Zc:__cplusplus)
	endif()

	# Needed, otherwise throws a error... Regression?
	set(CMAKE_C_FLAGS_DEBUG_INIT "" INTERNAL)
	set(CMAKE_CXX_FLAGS_DEBUG_INIT "" INTERNAL)

	target_compile_options(spheresvr PRIVATE
		${cxx_compiler_flags_common}
		$<$<CONFIG:Release>: $<IF:$<BOOL:${RUNTIME_STATIC_LINK}>,/MT,/MD>	/EHsc /Oy /GL /GA /Gw /Gy /GF /GR-  $<IF:$<BOOL:${ENABLED_SANITIZER}>,/O1 /Zi,/O2>>
		$<$<CONFIG:Nightly>: $<IF:$<BOOL:${RUNTIME_STATIC_LINK}>,/MT,/MD>	/EHa  /Oy /GL /GA /Gw /Gy /GF		$<IF:$<BOOL:${ENABLED_SANITIZER}>,/O1 /Zi,/O2>>
		$<$<CONFIG:Debug>:	 $<IF:$<BOOL:${RUNTIME_STATIC_LINK}>,/MTd,/MDd> /EHsc /Oy- /MDd /ob1 /Od 		$<IF:$<BOOL:${ENABLED_SANITIZER}>,/Zi,/ZI>> #/Gs
		# ASan (and compilation for ARM arch) doesn't support edit and continue option (ZI)
	)

	if ("${ARCH}" STREQUAL "x86_64")
		target_compile_options(spheresvr PRIVATE	/arch:SSE2)
	endif ()

	#-- Apply linker flags.

	# For some reason only THIS one isn't created, and CMake complains with an error...
	set(CMAKE_EXE_LINKER_FLAGS_NIGHTLY CACHE INTERNAL ${CMAKE_EXE_LINKER_FLAGS_RELEASE} "")

	target_link_options(spheresvr PRIVATE
		$<$<CONFIG:Release>: ${EXE_LINKER_EXTRA} /OPT:REF,ICF /LTCG /NODEFAULTLIB:libcmtd>
		$<$<CONFIG:Nightly>: ${EXE_LINKER_EXTRA} /OPT:REF,ICF /LTCG /NODEFAULTLIB:libcmtd>
		$<$<CONFIG:Debug>:	 ${EXE_LINKER_EXTRA} /DEBUG /LTCG:OFF /SAFESEH:NO /NODEFAULTLIB:libcmt
			$<$<NOT:$<BOOL:${ENABLED_SANITIZER}>>:/INCREMENTAL /EDITANDCONTINUE> >
	)
	# MSVC doesn't yet have an option to statically link against *san sanitizer libraries.
	# /INCREMENTAL and /EDITANDCONTINUE not compatible with a MSVC Asan build.


	#-- Windows libraries to link against.
	TARGET_LINK_LIBRARIES ( spheresvr	PRIVATE ws2_32 libmariadb )


	#-- Set define macros.

	# Common defines
	TARGET_COMPILE_DEFINITIONS ( spheresvr PRIVATE
		${PREPROCESSOR_DEFS_EXTRA}
		$<$<NOT:$<BOOL:${CMAKE_NO_GIT_REVISION}>>:_GITVERSION>
		_CRT_SECURE_NO_WARNINGS
		# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spam so much in the build proccess.
		_EXCEPTIONS_DEBUG
		# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
	  	#  on a running environment. Also it makes sphere more stable since exceptions are local.
		_WINSOCK_DEPRECATED_NO_WARNINGS
		# Removing WINSOCK warnings until the code gets updated or reviewed.

	 # Per-build defines
	 	$<$<NOT:$<CONFIG:Debug>>:	NDEBUG>
		$<$<CONFIG:Nightly>:		_NIGHTLYBUILD THREAD_TRACK_CALLSTACK>
		$<$<CONFIG:Debug>:			_DEBUG THREAD_TRACK_CALLSTACK _PACKETDUMP>
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
	CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/cmake/toolchains/include/spheresvr.vcxproj.user.in" "${CMAKE_BINARY_DIR}/spheresvr.vcxproj.user" @ONLY)


	#-- Solution settings.
	# Set spheresvr as startup project.
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT spheresvr)
endfunction()
