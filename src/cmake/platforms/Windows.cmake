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
