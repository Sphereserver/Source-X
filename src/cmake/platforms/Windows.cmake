# Define macro declarations

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