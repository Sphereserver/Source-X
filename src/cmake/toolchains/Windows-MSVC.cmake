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