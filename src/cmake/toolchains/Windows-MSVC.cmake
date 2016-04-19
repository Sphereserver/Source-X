SET (TOOLCHAIN 1)

function (toolchain_after_project)
	MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")
	SET(CMAKE_SYSTEM_NAME Windows)

	# Here it is setting the Visual Studio warning level to 4 and forcint MultiProccessor compilation
	SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996" PARENT_SCOPE)
	SET (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996" PARENT_SCOPE)

	# Nightly compiler and linker flags
	SET (CMAKE_CXX_FLAGS_NIGHTLY		"${CMAKE_CXX_FLAGS}"		PARENT_SCOPE)
	SET (CMAKE_EXE_LINKER_FLAGS_NIGHTLY	"${CMAKE_EXE_LINKER_FLAGS}"	PARENT_SCOPE)

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
	SET_TARGET_PROPERTIES ( spheresvr
		PROPERTIES
		# Setting the exe to be a windows application and not a console one.
		LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
		# -O3 only works on gcc, using -Ox for 'full optimization' instead.
		COMPILE_FLAGS -Ox
	)

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