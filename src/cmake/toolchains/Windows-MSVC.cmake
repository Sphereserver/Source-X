SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	SET(CMAKE_SYSTEM_NAME Windows)

	# Base compiler and linker flags are the same for every build type
	SET (CMAKE_C_FLAGS		"/DWIN32 /D_WINDOWS /Ox /W4 /MP 
					/wd4127 /wd4131 /wd4310 /wd4996"			PARENT_SCOPE)
		# Setting the exe to be a windows application and not a console one.
	SET (CMAKE_EXE_LINKER_FLAGS	"${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS"		PARENT_SCOPE)

	# Release compiler and linker flags
		# Setting the Visual Studio warning level to 4 and forcing MultiProccessor compilation
	SET (CMAKE_CXX_FLAGS_RELEASE		"/DWIN32 /D_WINDOWS /Ox /GR /W4 /MP 
						/wd4127 /wd4131 /wd4310 /wd4996"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_RELEASE	"${CMAKE_EXE_LINKER_FLAGS}"			PARENT_SCOPE)

	# Debug compiler and linker flags
	SET (CMAKE_CXX_FLAGS_DEBUG		"/DWIN32 /D_WINDOWS /D_DEBUG /MDd /Zi /ob0 /Od 
						/GR /W4 /MP /wd4127 /wd4131 /wd4310 /wd4996"	PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_DEBUG	"${CMAKE_EXE_LINKER_FLAGS} /DEBUG /INCREMENTAL"	PARENT_SCOPE)

	# Nightly compiler and linker flags
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"/DWIN32 /D_WINDOWS /Ox /GR /W4 /MP 
						/wd4127 /wd4131 /wd4310 /wd4996"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${CMAKE_EXE_LINKER_FLAGS}"			PARENT_SCOPE)

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
		wsock32
	)

	IF (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 	
			PUBLIC _64B
			PUBLIC _WIN64	)
	ELSE (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 		PUBLIC _32B )
	ENDIF (CMAKE_CL_64)
endfunction()