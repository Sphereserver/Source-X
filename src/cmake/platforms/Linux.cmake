# Define macro declarations

TARGET_COMPILE_DEFINITIONS ( spheresvr

	PUBLIC	_MTNETWORK
# GIT defs
	PUBLIC	_GITVERSION
# Enable advanced exceptions catching. Consumes some more resources, but is very useful for debug
#  on a running environment. Also it makes sphere more stable since exceptions are local.
	PUBLIC	_EXCEPTIONS_DEBUG
# Others
	PUBLIC	_LINUX
	PUBLIC	_CONSOLE
	PUBLIC	_REENTRANT

	PUBLIC $<$<OR:$<CONFIG:Release>,$<CONFIG:Nightly>>:	THREAD_TRACK_CALLSTACK>

	PUBLIC $<$<CONFIG:Debug>:	_DEBUG>
	PUBLIC $<$<CONFIG:Debug>:	_PACKETDUMP>
	PUBLIC $<$<CONFIG:Debug>:	_TESTEXCEPTION>
	PUBLIC $<$<CONFIG:Debug>:	DEBUG_CRYPT_MSGS>

	PUBLIC $<$<CONFIG:Nightly>:	_NIGHTLYBUILD>
)

# Linux libs.
TARGET_LINK_LIBRARIES ( spheresvr
        pthread
        mysqlclient
        rt
        dl
)
