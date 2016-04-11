SET (TOOLCHAIN 1)
MESSAGE (STATUS "Toolchain: Windows-MSVC.cmake.")

# Here it is setting the Visual Studio warning level to 4 and forcint MultiProccessor compilation
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996")
SET (CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996")

IF (CMAKE_CL_64)
	MESSAGE (STATUS "64 bits compiler detected.")
	SET (ARCH "x86_64")
	LINK_DIRECTORIES ("common/mysql/lib/x86_64")
ELSE (CMAKE_CL_64)
	MESSAGE (STATUS "32 bits compiler detected.")
	SET (ARCH "x86")
	LINK_DIRECTORIES ("common/mysql/lib/x86")
ENDIF (CMAKE_CL_64)

function (toolchain_exe_stuff)
	SET_TARGET_PROPERTIES ( spheresvr
		PROPERTIES
		# Setting the exe to be a windows application and not a console one.
		LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
		# -O3 only works on gcc, using -Ox for 'full optimization' instead.
		COMPILE_FLAGS -Ox
	)
	#SET_TARGET_PROPERTIES ( spheresvrNightly
	#	PROPERTIES
	#	# Setting the exe to be a windows application and not a console one.
	#	LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
	#	# -O3 only works on gcc, using -Ox for 'full optimization' instead.
	#	COMPILE_FLAGS -Ox
	#)

	# Windows libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		libmysql
		ws2_32
		wsock32
	)
	#TARGET_LINK_LIBRARIES ( spheresvrNightly
	#	libmysql
	#	ws2_32
	#	wsock32
	#)

	IF (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 		PUBLIC _64B )
		#TARGET_COMPILE_DEFINITIONS ( spheresvrNightly	PUBLIC _64B )
	ELSE (CMAKE_CL_64)
		# Architecture defines
		TARGET_COMPILE_DEFINITIONS ( spheresvr 		PUBLIC _32B )
		#TARGET_COMPILE_DEFINITIONS ( spheresvrNightly	PUBLIC _32B )
	ENDIF (CMAKE_CL_64)
endfunction()