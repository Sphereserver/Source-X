set (c_compiler_options_common
	-pipe -fexceptions -fnon-call-exceptions
	-O3
)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
set (c_compiler_options_common ${c_compiler_options_common}
	-fno-expensive-optimizations
)
endif ()

#if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
#endif()

#if (MSVC)
#endif
