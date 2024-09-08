# Git revision checker
set(GITBRANCH_VAL "N/A")
set(GITHASH_VAL "N/A")
set(GITREV_VAL 0)

if(CMAKE_NO_GIT_REVISION)
    message(STATUS "As per CMAKE_NO_GIT_REVISION, Git revision number and hash will not be available.")
else()
    find_package(Git)
    if(NOT GIT_FOUND)
        message(WARNING "Git not found! Revision number and hash will not be available.")
    else()
        set(GIT_CMD git)
        set(GIT_ARGS_VALID_REPO rev-parse)
        set(GIT_ARGS_REV_HASH rev-parse HEAD)
        set(GIT_ARGS_REV_COUNT rev-list --count HEAD)
        set(GIT_ARGS_BRANCH branch --show-current)

        message(STATUS "Checking if the folder is a valid git repo...")

        execute_process(
            COMMAND ${GIT_CMD} ${GIT_ARGS_VALID_REPO}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE RES_VALID_REPO
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(NOT "${RES_VALID_REPO}" STREQUAL "0")
            message(WARNING "Invalid Git repo! Revision number and hash will not be available.")
        else()
            message(STATUS "Checking git revision...")

            execute_process(
                COMMAND ${GIT_CMD} ${GIT_ARGS_REV_COUNT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE OUT_REV_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            if("${OUT_REV_COUNT}" STREQUAL "")
                message(
                    WARNING
                    "Git repo has no commits (not a clone from remote?). Revision number and hash will not be available."
                )
            else()
                math(EXPR RET_REV_COUNT "${OUT_REV_COUNT}")

                if("${OUT_REV_COUNT}" STREQUAL "")
                    message(WARNING "Git revision not available!")
                else()
                    message(STATUS "Git revision number: ${OUT_REV_COUNT}")
                    set(GITREV_VAL ${OUT_REV_COUNT})

                    message(STATUS "Checking git revision hash...")
                    execute_process(
                        COMMAND ${GIT_CMD} ${GIT_ARGS_REV_HASH}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        OUTPUT_VARIABLE OUT_REV_HASH
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                    )
                    message(STATUS "Git revision hash: ${OUT_REV_HASH}")
                    set(GITHASH_VAL ${OUT_REV_HASH})
                endif()
            endif()

            message(STATUS "Checking git branch...")

            execute_process(
                COMMAND ${GIT_CMD} ${GIT_ARGS_BRANCH}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE OUT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            if("${OUT_BRANCH}" STREQUAL "")
                message(WARNING "Git branch not available!")
            else()
                message(STATUS "Git branch: ${OUT_BRANCH}")
                set(GITBRANCH_VAL ${OUT_BRANCH})
            endif()
        endif()
    endif()
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/src/common/version/GitRevision.h.in"
    "${CMAKE_SOURCE_DIR}/src/common/version/GitRevision.h"
)
