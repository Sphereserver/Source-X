if (MSVC)
	set (c_compiler_options_common
		/O2 /EHsc /GL /GA /Gw /Gy /GF /GR- /GS-
	)
	set (c_linker_options_common
		/OPT:REF,ICF /LTCG
	)

else (MSVC)
	set (c_compiler_options_common
		-pipe -fexceptions -fnon-call-exceptions
		-O3
	)
	set (c_linker_options_common
		-s
	)

	if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
		set (c_compiler_options_common ${c_compiler_options_common}
			-fno-expensive-optimizations
		)
	endif ()

	#if (${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
#endif()

endif()
