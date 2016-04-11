TARGET_COMPILE_DEFINITIONS ( spheresvr
	# Extra defs
        PUBLIC _MTNETWORK
	# GIT defs
        PUBLIC _GITVERSION
	# Defines
        PUBLIC _CONSOLE
        PUBLIC _LINUX
        PUBLIC _REENTRANT
)

#TARGET_COMPILE_DEFINITIONS ( spheresvrNightly
#	# Nighly defs
#	PUBLIC _NIGHTLYBUILD
#	PUBLIC THREAD_TRACK_CALLSTACK
#	# Debug defs
#       PUBLIC _DEBUG
#       PUBLIC _EXCEPTIONS_DEBUG
#       PUBLIC _PACKETDUMP
#       PUBLIC _TESTEXCEPTION
#       PUBLIC DEBUG_CRYPT_MSGS
#	# Extra defs
#       PUBLIC _MTNETWORK
#	# GIT defs
#       PUBLIC _GITVERSION
#	# Defines
#       PUBLIC _CONSOLE
#       PUBLIC _LINUX
#       PUBLIC _REENTRANT
# )

# Linux libs.
TARGET_LINK_LIBRARIES ( spheresvr
        pthread
        mysqlclient
        rt
        dl
)
#TARGET_LINK_LIBRARIES ( spheresvrNightly
#       pthread
#       mysqlclient
#       rt
#       dl
#)