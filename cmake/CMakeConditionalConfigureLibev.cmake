# Generate config.h file for libev (if we are using it)
SET (USE_EV 0)
FOREACH (TARG ${TARGETS})
	GET_TARGET_PROPERTY (DEFS ${TARG} COMPILE_DEFINITIONS)
	FOREACH (DEF ${DEFS})
		IF ("${DEF}" STREQUAL "_LIBEV")
			SET (USE_EV 1)
		ENDIF ("${DEF}" STREQUAL "_LIBEV")
	ENDFOREACH (DEF ${DEFS})
ENDFOREACH (TARG ${TARGETS})

IF (USE_EV)
	INCLUDE ("lib/libev/cmake/configure.cmake")
ENDIF (USE_EV)