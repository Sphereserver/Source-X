# Git revision checker
SET (GITHASH_VAL "N/A")
SET (GITREV_VAL  0)

IF (CMAKE_NO_GIT_REVISION)
	MESSAGE (STATUS "As per CMAKE_NO_GIT_REVISION, Git revision number and hash will not be available.")

ELSE ()
	find_package(Git)
	IF (NOT GIT_FOUND)
		MESSAGE (WARNING "Git not found! Revision number and hash will not be available.")

	ELSE ()
		SET (GIT_CMD git)
		SET (GIT_ARGS_VALID_REPO  rev-parse)
		SET (GIT_ARGS_REV_HASH    rev-parse HEAD)
		SET (GIT_ARGS_REV_COUNT   rev-list --count HEAD)
		SET (GIT_ARGS_BRANCH  branch --show-current)

		MESSAGE (STATUS "Checking if the folder is a valid git repo...")
		
		EXECUTE_PROCESS (COMMAND ${GIT_CMD} ${GIT_ARGS_VALID_REPO}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			RESULT_VARIABLE RES_VALID_REPO
			OUTPUT_STRIP_TRAILING_WHITESPACE
		)

		IF (NOT "${RES_VALID_REPO}" STREQUAL "0")
			MESSAGE (WARNING "Invalid Git repo! Revision number and hash will not be available.")
		
		ELSE ()
			MESSAGE (STATUS "Checking git revision...")

			EXECUTE_PROCESS (COMMAND ${GIT_CMD} ${GIT_ARGS_REV_COUNT}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				OUTPUT_VARIABLE OUT_REV_COUNT
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)

			IF ("${OUT_REV_COUNT}" STREQUAL "")
				MESSAGE (WARNING "Git repo has no commits (not a clone from remote?). Revision number and hash will not be available.")

			ELSE ()
				MATH (EXPR RET_REV_COUNT "${OUT_REV_COUNT}")

				IF ("${OUT_REV_COUNT}" STREQUAL "")
					MESSAGE (WARNING "Git revision not available!")
					
				ELSE ()
					MESSAGE (STATUS "Git revision number: ${OUT_REV_COUNT}")
					SET (GITREV_VAL ${OUT_REV_COUNT})
					
					MESSAGE (STATUS "Checking git revision hash...")
					EXECUTE_PROCESS (COMMAND ${GIT_CMD} ${GIT_ARGS_REV_HASH}
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						OUTPUT_VARIABLE OUT_REV_HASH
						OUTPUT_STRIP_TRAILING_WHITESPACE
					)
					MESSAGE (STATUS "Git revision hash: ${OUT_REV_HASH}")
					SET (GITHASH_VAL ${OUT_REV_HASH})
					
				ENDIF ()

			ENDIF ()

			MESSAGE (STATUS "Checking git branch...")

			EXECUTE_PROCESS (COMMAND ${GIT_CMD} ${GIT_ARGS_BRANCH}
				WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
				OUTPUT_VARIABLE OUT_BRANCH
				OUTPUT_STRIP_TRAILING_WHITESPACE
			)

			IF ("${OUT_BRANCH}" STREQUAL "")
					MESSAGE (WARNING "Git branch not available!")
					
				ELSE ()
					MESSAGE (STATUS "Git branch: ${OUT_BRANCH}")
					SET (GITBRANCH_VAL ${OUT_BRANCH})

			ENDIF ()

		ENDIF ()
			
	ENDIF()

ENDIF()

CONFIGURE_FILE (
 "${CMAKE_SOURCE_DIR}/src/common/version/GitRevision.h.in"
 "${CMAKE_SOURCE_DIR}/src/common/version/GitRevision.h"
)