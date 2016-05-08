SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	SET(CMAKE_SYSTEM_NAME	"Windows"	PARENT_SCOPE)

	# Base compiler and linker flags are the same for every build type
	SET (CMAKE_C_FLAGS		"/W4 /MP /Ox
					/wd4127 /wd4131 /wd4310 /wd4996"			PARENT_SCOPE)
		# Setting the exe to be a windows application and not a console one.
	SET (CMAKE_EXE_LINKER_FLAGS	"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS"		PARENT_SCOPE)

	# Release compiler and linker flags
		# Setting the Visual Studio warning level to 4 and forcing MultiProccessor compilation
	SET (CMAKE_CXX_FLAGS_RELEASE		"/W4 /MP /GR /Ox
						/wd4127 /wd4131 /wd4310 /wd4996"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO"	PARENT_SCOPE)

	# Debug compiler and linker flags
	SET (CMAKE_CXX_FLAGS_DEBUG		"/W4 /MP /GR /Od
						 /wd4127 /wd4131 /wd4310 /wd4996
						 /D_DEBUG /MDd /Zi /ob0"			PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:YES
						/DEBUG"						PARENT_SCOPE)

	# Nightly compiler and linker flags
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"/W4 /MP /GR /Ox /EHa
						/wd4127 /wd4131 /wd4310 /wd4996"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${CMAKE_EXE_LINKER_FLAGS} /INCREMENTAL:NO"	PARENT_SCOPE)

	IF (CMAKE_CL_64)
		MESSAGE (STATUS "64 bits compiler detected.")
		SET (ARCH "x86_64" PARENT_SCOPE)
		LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/common/mysql/lib/x86_64")
	ELSE (CMAKE_CL_64)
		MESSAGE (STATUS "32 bits compiler detected.")
		SET (ARCH "x86" PARENT_SCOPE)
		LINK_DIRECTORIES ("${CMAKE_SOURCE_DIR}/common/mysql/lib/x86")
	ENDIF (CMAKE_CL_64)
endfunction()

function (toolchain_exe_stuff)
	# Windows libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		libmysql
		ws2_32
	)

	IF (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	
			PUBLIC _64BITS
			PUBLIC _WIN64	)
	ELSE (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 		PUBLIC _32BITS )
	ENDIF (CMAKE_CL_64)


	TARGET_COMPILE_DEFINITIONS ( spheresvr

		PUBLIC	_MTNETWORK
	# GIT defs
		PUBLIC	_GITVERSION
	# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spamm so much in the build proccess while we get rid of -W4 warnings and, after it, -Wall.
		PUBLIC	_CRT_SECURE_NO_WARNINGS
	# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
	#  on a running environment. Also it makes sphere more stable since exceptions are local.
		PUBLIC	_EXCEPTIONS_DEBUG
	# _WIN32 is always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
		PUBLIC	_WIN32
	# Removing WINSOCK warnings until the code gets updated or reviewed.
		PUBLIC	_WINSOCK_DEPRECATED_NO_WARNINGS

	# Others
		PUBLIC $<$<OR:$<CONFIG:Release>,$<CONFIG:Nightly>>:	THREAD_TRACK_CALLSTACK>
		PUBLIC $<$<OR:$<CONFIG:Release>,$<CONFIG:Nightly>>:	NDEBUG>

		PUBLIC $<$<CONFIG:Debug>:	_DEBUG>
		PUBLIC $<$<CONFIG:Debug>:	_PACKETDUMP>
		PUBLIC $<$<CONFIG:Debug>:	_TESTEXCEPTION>
		PUBLIC $<$<CONFIG:Debug>:	DEBUG_CRYPT_MSGS>

		PUBLIC $<$<CONFIG:Nightly>:	_NIGHTLYBUILD>
	)

	# Custom output directory
	IF (CMAKE_CL_64)
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin64/")
	ELSE (CMAKE_CL_64)
		SET(OUTDIR "${CMAKE_BINARY_DIR}/bin/")
	ENDIF (CMAKE_CL_64)
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTDIR})
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_RELEASE "${OUTDIR}/Release")
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_DEBUG "${OUTDIR}/Debug")
	SET_TARGET_PROPERTIES(spheresvr PROPERTIES RUNTIME_OUTPUT_NIGHTLY "${OUTDIR}/Nightly")

	# Custom .vcxproj settings
	CONFIGURE_FILE("cmake/spheresvr.vcxproj.user.in" "${CMAKE_BINARY_DIR}/spheresvr.vcxproj.user" @ONLY)
endfunction()