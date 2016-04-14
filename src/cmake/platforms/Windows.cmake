TARGET_COMPILE_DEFINITIONS ( spheresvr
	# Extra defs
        PUBLIC _MTNETWORK
	# GIT defs
        PUBLIC _GITVERSION
	# Defines
        PUBLIC _WIN32	# _WIN32 is always defined, even on 64 bits. Keeping it for compatibility with external code and libraries.
	# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spamm so much in the buil proccess while we get rid of -W4 warnings and, after it, -Wall.
        PUBLIC _CRT_SECURE_NO_WARNINGS
	# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
	# on a running environment. Also it makes sphere more stable since exceptions are local.
	PUBLIC _EXCEPTIONS_DEBUG
	# Removing WINSOCK warnings until the code gets updated or reviewed.		
	PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
)


IF ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )

	#TARGET_COMPILE_DEFINITIONS ( spheresvrNightly
	#	# Nighly defs
	#	PUBLIC _NIGHTLYBUILD
	#	PUBLIC THREAD_TRACK_CALLSTACK
	#	# Debug defs
	#	PUBLIC _DEBUG
	#	PUBLIC _PACKETDUMP
	#	PUBLIC _TESTEXCEPTION
	#	PUBLIC DEBUG_CRYPT_MSGS
	#	# Extra defs
	#	PUBLIC _MTNETWORK
	#	# GIT defs
	#	PUBLIC _GITVERSION
	#	# Defines
	#	PUBLIC _WINDOWS
	#	PUBLIC _CRT_SECURE_NO_WARNINGS
	#	PUBLIC _EXCEPTIONS_DEBUG
	#	PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
	#)

ELSE ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )	# assuming it's MSVC

	#TARGET_COMPILE_DEFINITIONS (spheresvrNightly
	#	# Extra defs
	#	PUBLIC _MTNETWORK
	#	# GIT defs
	#	PUBLIC _GITVERSION
	#	# Defines
	#	PUBLIC _NIGHTLYBUILD
	#	PUBLIC _WINDOWS
	#	PUBLIC _CRT_SECURE_NO_WARNINGS	
	#	PUBLIC _EXCEPTIONS_DEBUG
	#	PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
	#)

ENDIF ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
