# TODO: CITE ANTLR

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Pedantic error flags
    set(PEDANTIC_COMPILE_FLAGS
        -pedantic-errors
        -Wall
        -Wextra
        -pedantic
        -Wundef
        -Wmissing-include-dirs
        -Wredundant-decls
        -Wwrite-strings
        -Wpointer-arith
        -Wformat=2
        -Wno-format-nonliteral
        -Wcast-qual
        -Wcast-align
        -Wconversion
        -Wctor-dtor-privacy
        -Wdisabled-optimization
        -Winvalid-pch
        -Woverloaded-virtual
        -Wno-ctor-dtor-privacy
    )
    #if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6)
    set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wno-dangling-else -Wno-unused-local-typedefs)
    #endif ()
    #if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    set(PEDANTIC_COMPILE_FLAGS
        ${PEDANTIC_COMPILE_FLAGS}
        -Wdouble-promotion
        -Wtrampolines
        -Wzero-as-null-pointer-constant
        -Wuseless-cast
        -Wvector-operation-performance
        -Wsized-deallocation
        -Wshadow
    )
    #endif ()
    #if (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.0)
    set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wshift-overflow=2 -Wnull-dereference -Wduplicated-cond)
    #endif ()
    set(WERROR_FLAG -Werror)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Pedantic error flags
    set(PEDANTIC_COMPILE_FLAGS
        -Wall
        -Wextra
        -pedantic
        -Wconversion
        -Wundef
        -Wdeprecated
        -Wweak-vtables
        -Wshadow
        -Wno-gnu-zero-variadic-macro-arguments
    )
    check_cxx_compiler_flag(-Wzero-as-null-pointer-constant HAS_NULLPTR_WARNING)
    if(HAS_NULLPTR_WARNING)
        set(PEDANTIC_COMPILE_FLAGS ${PEDANTIC_COMPILE_FLAGS} -Wzero-as-null-pointer-constant)
    endif()
    set(WERROR_FLAG -Werror)
endif()

if(MSVC)
    # Pedantic error flags
    set(PEDANTIC_COMPILE_FLAGS /W3)
    set(WERROR_FLAG /WX)
endif()
