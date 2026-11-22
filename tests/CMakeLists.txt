#set(FETCHCONTENT_BASE_DIR
#    "${CMAKE_SOURCE_DIR}/third_party"
#)

message(STATUS "")
message(STATUS "Unit Tests requested")

include("cmake/FetchGithubDep.cmake")
set(EXTERNAL_CACHE_DIR "${CMAKE_SOURCE_DIR}/external")

# Fetch Doctest
fetch_github_library(doctest "https://github.com/doctest/doctest.git" "${EXTERNAL_CACHE_DIR}")

# Enable testing
enable_testing()

# Clang 21+ with -Wpedantic: the doctest macros use __COUNTER__, a C2y extension.
include(CheckCXXCompilerFlag)
if(NOT MSVC)
    check_cxx_compiler_flag("-Wc2y-extensions" COMP_HAS_WC2Y_EXTENSIONS)
endif()

function(setup_tests)
    foreach(tgt ${TARGETS})
        target_link_libraries(${tgt} PRIVATE doctest::doctest)
        target_compile_definitions(${tgt} PRIVATE UNIT_TESTING)
        # Doctest for some reason uses NaN, which is disabled by -ffast-math
        if("${CXX_COMPILER_ID}" STREQUAL "MSVC")
            target_compile_options(${tgt} PRIVATE /fp:precise)
        else()
            target_compile_options(${tgt} PRIVATE -fno-finite-math-only)
        endif()
        if(COMP_HAS_WC2Y_EXTENSIONS)
            target_compile_options(${tgt} PRIVATE -Wno-c2y-extensions)
        endif()

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

