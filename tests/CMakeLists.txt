#set(FETCHCONTENT_BASE_DIR
#    "${CMAKE_SOURCE_DIR}/third_party"
#)
include(FetchContent)

message(STATUS "")
message(STATUS "Unit Tests requested")
message(STATUS "Fetching DocTest")

# Download the JSON for the latest release
set(DOCTEST_RELEASE_API
    "https://api.github.com/repos/doctest/doctest/releases/latest"
)

file(DOWNLOAD
    ${DOCTEST_RELEASE_API}
    ${CMAKE_BINARY_DIR}/doctest_latest.json
    SHOW_PROGRESS
)

# Extract "tag_name" via regex
file(READ "${CMAKE_BINARY_DIR}/doctest_latest.json" _json)
string(REGEX MATCH "\"tag_name\"[ \t]*:[ \t]*\"([^\"]+)\"" _
       "${_json}")
set(DOCTEST_TAG "${CMAKE_MATCH_1}")

message(STATUS "Using doctest tag: ${DOCTEST_TAG}")

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        ${DOCTEST_TAG}
)
FetchContent_MakeAvailable(doctest)

# Enable testing
enable_testing()

function(setup_tests)
    foreach(tgt ${TARGETS})
        target_link_libraries(${tgt} PRIVATE doctest::doctest)
        target_compile_definitions(${tgt} PRIVATE UNIT_TESTING)

        #[[
        add_custom_target(run_tests
            COMMAND spheresvr --reporters=console
            DEPENDS spheresvr
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
        ]]

        # Register with CTest
        add_test(NAME all_tests_${tgt}
                COMMAND ${tgt})
    endforeach()
endfunction()
