find_program(CMAKE_CXX_CPPCHECK NAMES cppcheck)
if(CMAKE_CXX_CPPCHECK)
    include(CheckTypeSize)
    check_type_size("void*" SIZEOF_VOID_P)
    #message(STATUS "Pointer size = ${SIZEOF_VOID_P}")

    list(
        APPEND CMAKE_CXX_CPPCHECK
        "--verbose"
        #"--debug"
        "--report-progress"
        "--enable=all"
        "--safety"
        "--inconclusive"
        #"--force"
        #"--inline-suppr"
        "--check-level=exhaustive"
        "--template='{file}({line}): {severity} ({id}): {message}'"
        #"--checkers-report=${CMAKE_SOURCE_DIR}/cppcheck_out.txt"
        "--output-file=${CMAKE_BINARY_DIR}/cppcheck.log"
        "--suppressions-list=${CMAKE_SOURCE_DIR}/static_analysis/cppcheck-suppressions.txt"
        "--config-exclude=lib/"
        "-i lib/"
        "-D__SIZEOF_POINTER__=${SIZEOF_VOID_P}"
        "-U_TESTEXCEPTION"
        "-U_EXCEPTIONS_DEBUG"
    )
endif()
