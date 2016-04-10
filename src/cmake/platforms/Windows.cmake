TARGET_COMPILE_DEFINITIONS (spheresvr
# Extra defs
        PUBLIC _MTNETWORK
# GIT defs
        PUBLIC _GITVERSION
# Defines
        PUBLIC _WINDOWS
# Temporary setting _CRT_SECURE_NO_WARNINGS to do not spamm so much in the buil proccess while we get rid of -W4 warnings and, after it, -Wall.
        PUBLIC _CRT_SECURE_NO_WARNINGS
# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
# on a running environment. Also it makes sphere more stable since exceptions are local.
	PUBLIC _EXCEPTIONS_DEBUG
# Removing WINSOCK warnings until the code gets updated or reviewed.		
	PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
)


IF ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )

	# Linux libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		pthread
		libmysql
		ws2_32
		wsock32
	)
	#[[
	TARGET_LINK_LIBRARIES ( spheresvrNightly
		pthread
		libmysql
		ws2_32
		wsock32
	) ]]

	#[[
	TARGET_COMPILE_DEFINITIONS (spheresvrNightly
	# Nighly defs
		PUBLIC _NIGHTLYBUILD
		PUBLIC THREAD_TRACK_CALLSTACK
	# Debug defs
		PUBLIC _DEBUG
		PUBLIC _PACKETDUMP
		PUBLIC _TESTEXCEPTION
		PUBLIC DEBUG_CRYPT_MSGS
	# Extra defs
		PUBLIC _MTNETWORK
	# GIT defs
		PUBLIC _GITVERSION
	# Defines
		PUBLIC _WINDOWS
		PUBLIC _CRT_SECURE_NO_WARNINGS
		PUBLIC _EXCEPTIONS_DEBUG
		PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
	) ]]

ELSE ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )		# assuming it's MSVC

	# Windows libs.
	TARGET_LINK_LIBRARIES ( spheresvr
		libmysql
		ws2_32
		wsock32
	)
	#[[
	TARGET_LINK_LIBRARIES ( spheresvrNightly
		libmysql
		ws2_32
		wsock32
	) ]]

	SET_TARGET_PROPERTIES ( spheresvr
		PROPERTIES
	# Setting the exe to be a windows application and not a console one.
		LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
	# -O3 only works on gcc, using -Ox for 'full optimization' instead.
		COMPILE_FLAGS -Ox
	)
	#[[
	SET_TARGET_PROPERTIES ( spheresvrNightly
		PROPERTIES
	# Setting the exe to be a windows application and not a console one.
		LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
	# -O3 only works on gcc, using -Ox for 'full optimization' instead.
		COMPILE_FLAGS -Ox
	) #]]

	#[[
	TARGET_COMPILE_DEFINITIONS (spheresvrNightly
	# Extra defs
		PUBLIC _MTNETWORK
	# GIT defs
		PUBLIC _GITVERSION
	# Defines
		PUBLIC _NIGHTLYBUILD
		PUBLIC _WINDOWS
		PUBLIC _CRT_SECURE_NO_WARNINGS	
		PUBLIC _EXCEPTIONS_DEBUG
		PUBLIC _WINSOCK_DEPRECATED_NO_WARNINGS
	) ]]

ENDIF ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
