# Generate config.h file for libev (if we are using it)

FUNCTION (contains_compile_definition  definition ret)
SET (result FALSE PARENT_SCOPE)

FOREACH (TARG ${TARGETS})
	GET_TARGET_PROPERTY (DEFS ${TARG} COMPILE_DEFINITIONS)
	FOREACH (DEF ${DEFS})
		IF ("${DEF}" STREQUAL "${definition}")
			SET (ret TRUE PARENT_SCOPE)
			RETURN ()
		ENDIF ()
	ENDFOREACH ()
ENDFOREACH ()

GET_DIRECTORY_PROPERTY( DEFS COMPILE_DEFINITIONS )
FOREACH (DEF ${DEFS})
	IF ("${DEF}" STREQUAL "${definition}")
		SET (ret TRUE PARENT_SCOPE)
		RETURN ()
	ENDIF ()
ENDFOREACH ()

ENDFUNCTION ()


SET (ret FALSE)
contains_compile_definition("_LIBEV" ret)

IF (ret)
	INCLUDE ("lib/libev/cmake/configure.cmake")
ENDIF (ret)
