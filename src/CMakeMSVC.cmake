# Here it is setting the Visual Studio warning level to 4 and forcint MultiProccessor compilation
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996")
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W4 -MP /wd4127 /wd4131 /wd4310 /wd4996")
TARGET_COMPILE_DEFINITIONS (spheresvr
# Extra defs
        PUBLIC _MTNETWORK
# SVN defs
        PUBLIC _SUBVERSION
# Defines
        PUBLIC _WIN32
# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spamm so much in the buil proccess while we get rid of -W4 warnings and, after it, -Wall.
        PUBLIC _CRT_SECURE_NO_WARNINGS
# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
# on a running environment. Also it makes sphere more stable since exceptions are local.
		PUBLIC _EXCEPTIONS_DEBUG
# Removing WINSOCK warnings until the code gets updated or reviewed.		
		PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
)
TARGET_COMPILE_DEFINITIONS (spheresvrNightly
# Extra defs
        PUBLIC _MTNETWORK
# SVN defs
        PUBLIC _SUBVERSION
# Defines
        PUBLIC _NIGHTLYBUILD
        PUBLIC _WIN32
        PUBLIC _CRT_SECURE_NO_WARNINGS	
		PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
)
SET_TARGET_PROPERTIES (spheresvr
        PROPERTIES
# Setting the exe to be a windows application and not a console one.
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
# -O3 only works on gcc, using -Ox for 'full optimization' instead.
        COMPILE_FLAGS -Ox
)
SET_TARGET_PROPERTIES (spheresvrNightly
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
TARGET_LINK_LIBRARIES ( spheresvrNightly
        libmysql
        ws2_32
        wsock32
)