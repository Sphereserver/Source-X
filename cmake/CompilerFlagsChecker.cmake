include(CheckCXXCompilerFlag)
message(STATUS "Checking available compiler flags...")

if(NOT MSVC)
    message(STATUS "-- Compilation options:")

    # Compiler option flags.
    list(
        APPEND
        compiler_options_base
        -pthread
        -fexceptions
        -fnon-call-exceptions
        -pipe
        -ffast-math
    )
    list(APPEND base_compiler_options_warning -Werror;-Wall;-Wextra;-Wpedantic)

    # Linker flags (warnings)
    #check_cxx_compiler_flag("-Wl,--fatal-warnings" COMP_LINKER_HAS_FATAL_WARNINGS_V1)
    #check_cxx_compiler_flag("-Wl,-fatal_warnings" COMP_LINKER_HAS_FATAL_WARNINGS_V2)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        set(COMP_LINKER_HAS_FATAL_WARNINGS_V2 TRUE)
    else()
        set(COMP_LINKER_HAS_FATAL_WARNINGS_V1 TRUE)
    endif()

    # Compiler option flags. Common to both compilers, but older versions might not support the following.
    #check_cxx_compiler_flag("" COMP_HAS_)

    # Compiler option flags. Expected to work on GCC but not Clang, at the moment.
    check_cxx_compiler_flag("-fno-expensive-optimizations" COMP_HAS_FNO_EXPENSIVE_OPTIMIZATIONS)

    # Compiler option flags. Expected to work on Clang but not GCC, at the moment.
    # check_cxx_compiler_flag("-fforce-emit-vtables" COMP_HAS_F_FORCE_EMIT_VTABLES)
    # -fwhole-program-vtables
    # -fstrict-vtable-pointers

    if(${USE_ASAN})
        # Compiler option flags. Sanitizers related (ASAN).
        message(STATUS "-- Compilation options (Address Sanitizer):")

        set(CMAKE_REQUIRED_LIBRARIES -fsanitize=address) # link against sanitizer lib, being required by the following compilation test
        check_cxx_compiler_flag("-fsanitize=address" COMP_HAS_ASAN)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(NOT COMP_HAS_ASAN)
            message(FATAL_ERROR "This compiler does not support Address Sanitizer. Disable it.")
        endif()

        check_cxx_compiler_flag("-fsanitize-cfi" COMP_HAS_FSAN_CFI) # cfi: control flow integrity
        check_cxx_compiler_flag("-fsanitize-address-use-after-scope" COMP_HAS_FSAN_ADDRESS_USE_AFTER_SCOPE)
        check_cxx_compiler_flag("-fsanitize=pointer-compare" COMP_HAS_FSAN_PTR_COMP)
        check_cxx_compiler_flag("-fsanitize=pointer-subtract" COMP_HAS_FSAN_PTR_SUB)
        check_cxx_compiler_flag("-fsanitize-trap=all" COMP_HAS_FSAN_TRAP_ALL)
    endif()

    if(${USE_UBSAN})
        # Compiler option flags. Sanitizers related (UBSAN).
        message(STATUS "-- Compilation options (Undefined Behavior Sanitizer):")

        set(CMAKE_REQUIRED_LIBRARIES -fsanitize=undefined) # link against sanitizer lib, being required by the following compilation test
        check_cxx_compiler_flag("-fsanitize=undefined" COMP_HAS_UBSAN)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(NOT COMP_HAS_UBSAN)
            message(FATAL_ERROR "This compiler does not support Undefined Behavior Sanitizer. Disable it.")
        endif()

        check_cxx_compiler_flag("-fsanitize=float-divide-by-zero" COMP_HAS_FSAN_FLOAT_DIVIDE_BY_ZERO)
        check_cxx_compiler_flag("-fsanitize=unsigned-integer-overflow" COMP_HAS_FSAN_UNSIGNED_INTEGER_OVERFLOW) #Unlike signed integer overflow, this is not undefined behavior, but it is often unintentional.
        #check_cxx_compiler_flag("-fsanitize=implicit-conversion" COMP_HAS_FSAN_IMPLICIT_CONVERSION)
        #check_cxx_compiler_flag("-fsanitize=local-bounds" COMP_HAS_FSAN_LOCAL_BOUNDS)
        check_cxx_compiler_flag("-fno-sanitize=enum" COMP_HAS_FSAN_NO_ENUM)
    endif()

    if(${USE_LSAN})
        # Compiler option flags. Sanitizers related (LSAN).
        message(STATUS "-- Compilation options (Leak Sanitizer):")

        set(CMAKE_REQUIRED_LIBRARIES -fsanitize=leak) # link against sanitizer lib, being required by the following compilation test
        check_cxx_compiler_flag("-fsanitize=leak" COMP_HAS_LSAN)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(NOT COMP_HAS_LSAN)
            message(FATAL_ERROR "This compiler does not support Leak Sanitizer. Disable it.")
        endif()
    endif()

    if(${USE_MSAN})
        # Compiler option flags. Sanitizers related (UBSAN).
        message(STATUS "-- Compilation options (Memory Behavior Sanitizer):")

        set(CMAKE_REQUIRED_LIBRARIES -fsanitize=memory) # link against sanitizer lib, being required by the following compilation test
        check_cxx_compiler_flag("-fsanitize=memory" COMP_HAS_MSAN)
        unset(CMAKE_REQUIRED_LIBRARIES)
        if(NOT COMP_HAS_MSAN)
            message(FATAL_ERROR "This compiler does not support Memory Sanitizer. Disable it.")
        endif()

        # MemorySanitizer: it doesn't work out of the box. It needs to be linked to an MSAN-instrumented build of libc++ and libc++abi.
        #  This means: one should build them from LLVM source...
        #  https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo
        #IF (${USE_MSAN})
        #    SET (CMAKE_CXX_FLAGS    "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        #ENDIF()
        # Use "-stdlib=libstdc++" to link against GCC c/c++ libs (this is done by default)
        # To use LLVM libc++ use "-stdlib=libc++", but you need to install it separately

        message(
            WARNING
            "You have enabled MSAN. Make sure you do know what you are doing. It doesn't work out of the box. \
See comments in the toolchain and: https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo"
        )

        check_cxx_compiler_flag("-fsanitize=memory-track-origins" COMP_HAS_F_SANITIZE_MEMORY_TRACK_ORIGINS)
        check_cxx_compiler_flag("-fPIE" COMP_HAS_F_PIE)
    endif()

    if(ENABLED_SANITIZER)
        if(RUNTIME_STATIC_LINK)
            check_cxx_compiler_flag("-fstatic-libasan" COMP_HAS_F_STATIC_LIBASAN)
            if(NOT COMP_HAS_F_STATIC_LIBASAN)
                message(FATAL_ERROR "This compiler doesn't support statically linking libasan. Turn off RUNTIME_STATIC_LINK?")
            endif()
        endif()
    endif()

    if(USE_COMPILER_HARDENING_OPTIONS)
        message(STATUS "-- Compilation options (code hardening):")
        # Compiler option flags. Other sanitizers or code hardening.
        if(NOT USE_ASAN)
            check_cxx_compiler_flag("-fsanitize=safe-stack" COMP_HAS_F_SANITIZE_SAFE_STACK) # Can't be used with asan!
        endif()
        check_cxx_compiler_flag("-fcf-protection=full" COMP_HAS_FCF_PROTECTION_FULL)
        check_cxx_compiler_flag("-fstack-check" COMP_HAS_F_STACK_CHECK)
        check_cxx_compiler_flag("-fstack-protector-all" COMP_HAS_F_STACK_PROTECTOR_ALL)
        check_cxx_compiler_flag("-fstack-protector-strong" COMP_HAS_F_STACK_PROTECTOR_STRONG)
        check_cxx_compiler_flag("-fstack-protector" COMP_HAS_F_STACK_PROTECTOR)
        check_cxx_compiler_flag("-fstack-clash-protection" COMP_HAS_F_STACK_CLASH_PROTECTION)
        #Flags GCC specific?
        check_cxx_compiler_flag("-fvtable-verify=preinit" COMP_HAS_F_VTABLE_VERIFY_PREINIT)
        check_cxx_compiler_flag("-fharden-control-flow-redundancy" COMP_HAS_F_HARDEN_CFR)
        check_cxx_compiler_flag("-fhardcfr-check-exceptions" COMP_HAS_F_HARDCFR_CHECK_EXCEPTIONS)
        #check_cxx_compiler_flag("" COMP_HAS_)
    endif()

    # Compiler warning flags.
    message(STATUS "-- Compilation options (warnings):")
    #check_cxx_compiler_flag("-Wuseless-cast" COMP_HAS_W_USELESS_CAST)
    check_cxx_compiler_flag("-Wnull-dereference" COMP_HAS_W_NULL_DEREFERENCE)
    #check_cxx_compiler_flag("-Wconversion" COMP_HAS_W_CONVERSION) # Temporarily disabled. Implicit type conversions that might change a value, such as narrowing conversions.
    #check_cxx_compiler_flag("-Wcast-qual" COMP_HAS_W_CAST_QUAL) # casts that remove a type's const or volatile qualifier.
    check_cxx_compiler_flag("-Wzero-as-null-pointer-constant" COMP_HAS_W_ZERO_NULLPTR)
    check_cxx_compiler_flag("-Wdisabled-optimization" COMP_HAS_W_DISABLE_OPT)
    check_cxx_compiler_flag("-Winvalid-pch" COMP_HAS_W_INVALID_PCH)
    check_cxx_compiler_flag("-Wshadow" COMP_HAS_W_SHADOW)
    #check_cxx_compiler_flag("" COMP_HAS_)

    # Compiler warning flags. Expected to work con GCC but not Clang, at the moment.
    check_cxx_compiler_flag("-Wlto-type-mismatch" COMP_HAS_W_LTO_TYPE_MISMATCH)
    check_cxx_compiler_flag("-Wshift-overflow=2" COMP_HAS_W_SHIFT_OVERFLOW)
    check_cxx_compiler_flag("-Wduplicated-cond" COMP_HAS_W_DUPLICATED_COND)
    check_cxx_compiler_flag("-Wsized-deallocation" COMP_HAS_W_SIZED_DEALLOC)
    check_cxx_compiler_flag("-Wvector-operation-performance" COMP_HAS_W_VECTOR_OP_PERF)
    check_cxx_compiler_flag("-Wtrampolines" COMP_HAS_W_TRAMPOLINES) # we don't want trampolines for security concerns (it's a technique to implement nested functions).
    #check_cxx_compiler_flag("" COMP_HAS_)

    # Compiler warning flags. Expected to work con Clang but not GCC, at the moment.
    check_cxx_compiler_flag("-Wweak-vtables" COMP_HAS_W_WEAK_VTABLES)
    check_cxx_compiler_flag("-Wmissing-prototypes" COMP_HAS_W_MISSING_PROTOTYPES)
    check_cxx_compiler_flag("-Wmissing-variable-declarations" COMP_HAS_W_MISSING_VARIABLE_DECL)
    #check_cxx_compiler_flag("" COMP_HAS_)

    # Compiler warning flags. TODO: To be evaluated...
    # -Wfree-nonheap-object (good but false positive warnings on GCC?)
    # -Wcast-align=strict
    # -Wcast-user-defined
    # -Wdate-time

    # Compiler warning flags. To be disabled.
    message(STATUS "-- Compilation options (disable specific warnings):")
    check_cxx_compiler_flag("-Wunused-function" COMP_HAS_WNO_UNUSED_FUNCTION)
    check_cxx_compiler_flag("-Wformat-nonliteral" COMP_HAS_WNO_FORMAT_NONLITERAL) # Since -Wformat=2 is stricter, you would need to disable this warning.
    check_cxx_compiler_flag("-Wnonnull-compare" COMP_HAS_WNO_NONNULL_COMPARE)
    check_cxx_compiler_flag("-Wmaybe-uninitialized" COMP_HAS_WNO_MAYBE_UNINIT)
    check_cxx_compiler_flag("-Wswitch" COMP_HAS_WNO_SWITCH)
    check_cxx_compiler_flag("-Wimplicit-fallthrough" COMP_HAS_WNO_IMPLICIT_FALLTHROUGH)
    check_cxx_compiler_flag("-Wparentheses" COMP_HAS_WNO_PARENTHESES)
    check_cxx_compiler_flag("-Wmisleading-indentation" COMP_HAS_WNO_MISLEADING_INDENTATION)
    check_cxx_compiler_flag("-Wunused-result" COMP_HAS_WNO_UNUSED_RESULT)
    check_cxx_compiler_flag("-Wnested-anon-types" COMP_HAS_WNO_NESTED_ANON_TYPES)
    check_cxx_compiler_flag("-Wformat-security" COMP_HAS_WNO_FORMAT_SECURITY) # TODO: remove that when we'll have time to fix every printf format issue
    check_cxx_compiler_flag("-Wdeprecated-declarations" COMP_HAS_WNO_DEPRECATED_DECLARATIONS)
    check_cxx_compiler_flag("-Wlanguage-extension-token" COMP_HAS_WNO_LANGUAGE_EXTENSION_TOKEN)
    #check_cxx_compiler_flag("-Wunknown-pragmas" COMP_HAS_)
    #check_cxx_compiler_flag("" COMP_HAS_)

elseif(MSVC)
    # Compiler warning flags (MSVC).
    message(STATUS "-- Compilation options (MSVC):")
    check_cxx_compiler_flag("/W5105" COMP_HAS_W_DEFINE_MACRO_EXPANSION) # False positive warning, maybe even a MSVC bug.
    #check_cxx_compiler_flag("" COMP_HAS_)

endif()

# ---- Append options into some lists.

# ---- GCC/Clang

if(NOT MSVC)
    if(COMP_LINKER_HAS_FATAL_WARNINGS_V1)
        list(APPEND checked_linker_options_all "-Wl,--fatal-warnings")
    endif()
    if(COMP_LINKER_HAS_FATAL_WARNINGS_V2)
        list(APPEND checked_linker_options_all "-Wl,-fatal_warnings")
    endif()

    if(COMP_HAS_FNO_EXPENSIVE_OPTIMIZATIONS)
        list(APPEND checked_compiler_options "-fno-expensive-optimizations")
    endif()

    if(COMP_HAS_ASAN)
        list(APPEND checked_compiler_options_asan "-fsanitize=address")
        list(APPEND checked_linker_options_all "-fsanitize=address")
        if(COMP_HAS_F_STATIC_LIBASAN)
            list(APPEND checked_linker_options_all "-static-libasan")
        endif()
        if(COMP_HAS_FSAN_CFI)
            list(APPEND checked_compiler_options_asan "-fsanitize-cfi")
        endif()
        if(COMP_HAS_FSAN_ADDRESS_USE_AFTER_SCOPE)
            list(APPEND checked_compiler_options_asan "-fsanitize-address-use-after-scope")
        endif()
        if(COMP_HAS_FSAN_PTR_COMP)
            list(APPEND checked_compiler_options_asan "-fsanitize=pointer-compare")
        endif()
        if(COMP_HAS_FSAN_PTR_SUB)
            list(APPEND checked_compiler_options_asan "-fsanitize=pointer-subtract")
        endif()
        if(COMP_HAS_FSAN_TRAP_ALL)
            list(APPEND checked_compiler_options_asan "-fsanitize-trap=all")
        endif()
    endif()

    if(COMP_HAS_UBSAN)
        list(APPEND checked_compiler_options_ubsan "-fsanitize=undefined")
        list(APPEND checked_linker_options_all "-fsanitize=undefined")
        if(RUNTIME_STATIC_LINK)
            list(APPEND checked_linker_options_all "-static-libubsan")
        endif()
        if(COMP_HAS_FSAN_FLOAT_DIVIDE_BY_ZERO)
            list(APPEND checked_compiler_options_ubsan "-fsanitize=float-divide-by-zero")
        endif()
        if(COMP_HAS_FSAN_UNSIGNED_INTEGER_OVERFLOW)
            list(APPEND checked_compiler_options_ubsan "-fsanitize=unsigned-integer-overflow")
        endif()
        if(COMP_HAS_FSAN_IMPLICIT_CONVERSION)
            list(APPEND checked_compiler_options_ubsan "-fsanitize=implicit-conversion")
        endif()
        if(COMP_HAS_FSAN_NO_ENUM)
            list(APPEND checked_compiler_options_ubsan "-fno-sanitize=enum")
        endif()
        if(COMP_HAS_FSAN_LOCAL_BOUNDS)
            list(APPEND checked_compiler_options_ubsan "-fsanitize=local-bounds")
        endif()
    endif()

    if(COMP_HAS_LSAN)
        list(APPEND checked_compiler_options_lsan "-fsanitize=leak")
        list(APPEND checked_linker_options_all "-fsanitize=leak")
        if(RUNTIME_STATIC_LINK)
            list(APPEND checked_linker_options_all "-static-liblsan")
        endif()
    endif()

    if(COMP_HAS_MSAN)
        list(APPEND checked_compiler_options_msan "-fsanitize=memory")
        list(APPEND checked_linker_options_all "-fsanitize=memory")
        if(RUNTIME_STATIC_LINK)
            list(APPEND checked_linker_options_all "-static-libmsan")
        endif()
        if(COMP_HAS_F_SANITIZE_MEMORY_TRACK_ORIGINS)
            list(APPEND checked_compiler_options_msan "-fsanitize=memory-track-origins")
        endif()
        if(COMP_HAS_F_PIE)
            list(APPEND checked_compiler_options_msan "-fPIE")
            list(APPEND checked_linker_options_all "-pie")
        endif()
    endif()

    if(COMP_HAS_F_SANITIZE_SAFE_STACK)
        list(APPEND checked_compiler_options_sanitize_harden "-fsanitize=safe-stack")
    endif()
    if(COMP_HAS_FCF_PROTECTION_FULL)
        list(APPEND checked_compiler_options_sanitize_harden "-fcf-protection=full")
    endif()
    if(COMP_HAS_F_STACK_CHECK)
        list(APPEND checked_compiler_options_sanitize_harden "-fstack-check")
    endif()
    if(COMP_HAS_F_STACK_PROTECTOR_ALL)
        list(APPEND checked_compiler_options_sanitize_harden "-fstack-protector-all")
    elseif(COMP_HAS_F_STACK_PROTECTOR_STRONG)
        list(APPEND checked_compiler_options_sanitize_harden "-fstack-protector-strong")
    elseif(COMP_HAS_F_STACK_PROTECTOR)
        list(APPEND checked_compiler_options_sanitize_harden "-fstack-protector")
    endif()
    if(COMP_HAS_F_STACK_CLASH_PROTECTION)
        list(APPEND checked_compiler_options_sanitize_harden "-fstack-clash-protection")
    endif()
    if(COMP_HAS_F_VTABLE_VERIFY_PREINIT)
        list(APPEND checked_compiler_options_sanitize_harden "-fvtable-verify=preinit")
    endif()
    if(COMP_HAS_F_HARDEN_CFR)
        list(APPEND checked_compiler_options_sanitize_harden "-fharden-control-flow-redundancy")
    endif()
    if(COMP_HAS_F_HARDCFR_CHECK_EXCEPTIONS)
        list(APPEND checked_compiler_options_sanitize_harden "-fhardcfr-check-exceptions")
    endif()

    if(COMP_HAS_W_USELESS_CAST)
        list(APPEND checked_compiler_warnings "-Wuseless-cast")
    endif()
    if(COMP_HAS_NULL_DEREFERENCE)
        list(APPEND checked_compiler_warnings "-Wnull-dereference")
    endif()
    if(COMP_HAS_W_CONVERSION)
        list(APPEND checked_compiler_warnings "-Wconversion")
    endif()
    if(COMP_HAS_W_CAST_QUAL)
        list(APPEND checked_compiler_warnings "-Wcast-qual")
    endif()
    if(COMP_HAS_W_ZERO_NULLPTR)
        list(APPEND checked_compiler_warnings "-Wzero-as-null-pointer-constant")
    endif()
    if(COMP_HAS_W_DISABLE_OPT)
        list(APPEND checked_compiler_warnings "-Wdisabled-optimization")
    endif()
    if(COMP_HAS_W_INVALID_PCH)
        list(APPEND checked_compiler_warnings "-Winvalid-pch")
    endif()
    if(COMP_HAS_W_SHADOW)
        list(APPEND checked_compiler_warnings "-Wshadow")
    endif()

    if(COMP_HAS_W_LTO_TYPE_MISMATCH)
        list(APPEND checked_compiler_warnings "-Wlto-type-mismatch")
    endif()
    if(COMP_HAS_W_SHIFT_OVERFLOW)
        list(APPEND checked_compiler_warnings "-Wshift-overflow=2")
    endif()
    if(COMP_HAS_W_DUPLICATED_COND)
        list(APPEND checked_compiler_warnings "-Wduplicated-cond")
    endif()
    if(COMP_HAS_W_SIZED_DEALLOC)
        list(APPEND checked_compiler_warnings "-Wsized-deallocation")
    endif()
    if(COMP_HAS_W_VECTOR_OP_PERF)
        list(APPEND checked_compiler_warnings "-Wvector-operation-performance")
    endif()
    if(COMP_HAS_W_TRAMPOLINES)
        list(APPEND checked_compiler_warnings "-Wtrampolines")
    endif()
    if(COMP_HAS_W_WEAK_VTABLES)
        list(APPEND checked_compiler_warnings "-Wweak-vtables")
    endif()
    if(COMP_HAS_W_MISSING_PROTOTYPES)
        list(APPEND checked_compiler_warnings "-Wmissing-prototypes")
    endif()
    if(COMP_HAS_W_MISSING_VARIABLE_DECL)
        list(APPEND checked_compiler_warnings "-Wmissing-variable-declarations")
    endif()

    if(COMP_HAS_WNO_UNUSED_FUNCTION)
        list(APPEND checked_compiler_warnings_disabled "-Wno-unused-function")
    endif()
    if(COMP_HAS_WNO_FORMAT_NONLITERAL)
        list(APPEND checked_compiler_warnings_disabled "-Wno-format-nonliteral")
    endif()
    if(COMP_HAS_WNO_NONNULL_COMPARE)
        list(APPEND checked_compiler_warnings_disabled "-Wno-nonnull-compare")
    endif()
    if(COMP_HAS_WNO_MAYBE_UNINIT)
        list(APPEND checked_compiler_warnings_disabled "-Wno-maybe-uninitialized")
    endif()
    if(COMP_HAS_WNO_SWITCH)
        list(APPEND checked_compiler_warnings_disabled "-Wno-switch")
    endif()
    if(COMP_HAS_WNO_IMPLICIT_FALLTHROUGH)
        list(APPEND checked_compiler_warnings_disabled "-Wno-implicit-fallthrough")
    endif()
    if(COMP_HAS_WNO_PARENTHESES)
        list(APPEND checked_compiler_warnings_disabled "-Wno-parentheses")
    endif()
    if(COMP_HAS_WNO_MISLEADING_INDENTATION)
        list(APPEND checked_compiler_warnings_disabled "-Wno-misleading-indentation")
    endif()
    if(COMP_HAS_WNO_UNUSED_RESULT)
        list(APPEND checked_compiler_warnings_disabled "-Wno-unused-result")
    endif()
    if(COMP_HAS_WNO_NESTED_ANON_TYPES)
        list(APPEND checked_compiler_warnings_disabled "-Wno-nested-anon-types")
    endif()
    if(COMP_HAS_WNO_FORMAT_SECURITY)
        list(APPEND checked_compiler_warnings_disabled "-Wno-format-security")
    endif()
    if(COMP_HAS_WNO_DEPRECATED_DECLARATIONS)
        list(APPEND checked_compiler_warnings_disabled "-Wno-deprecated-declarations")
    endif()
    if(COMP_HAS_WNO_LANGUAGE_EXTENSION_TOKEN)
        list(APPEND checked_compiler_warnings_disabled "-Wno-language-extension-token")
    endif()
endif()

# ---- MSVC

if(MSVC)
    #list(
    #    APPEND
    #    base_compiler_options_msvc
    #    "/wd5105"
    #)

    if(COMP_HAS_W_DEFINE_MACRO_EXPANSION)
        list(APPEND checked_compiler_options_msvc "/wd5105")
    endif()

endif()

# ---- Double checks

if(
    CMAKE_COMPILER_IS_GNUCC
    AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 12.1
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 13.0
)
    # False positive warning, disable this flag.
    unset(COMPILER_SUPPORTS_NULL_DEREFERENCE CACHE)
endif()

# ----

#[[
set(checked_compiler_options)
set(checked_compiler_options_asan)
#set(checked_compiler_options_ubsan)
#set(checked_compiler_options_lsan)
set(checked_compiler_options_sanitize_harden)
set(checked_compiler_warnings)
set(checked_compiler_warnings_disabled)
set(checked_compiler_options_msvc)
]]

## ----

# Now sum up, report and encapsulate all the options.

if(NOT MSVC)
    list(
        APPEND
        list_explicit_compiler_options_all
        ${compiler_options_base}
        ${base_compiler_options_warning}
        ${checked_compiler_options}
        ${checked_compiler_options_asan}
        ${checked_compiler_options_ubsan}
        ${checked_compiler_options_lsan}
        ${checked_compiler_options_msan}
        ${checked_compiler_options_sanitize_harden}
        ${checked_compiler_warnings}
        ${checked_compiler_warnings_disabled}
    )

    list(APPEND list_explicit_linker_options_all ${checked_linker_options_all};-pthread;-dynamic)

    #string(JOIN " " string_checked_compiler_options_all ${list_checked_compiler_options_all})
    #set(string_checked_compiler_options_all CACHE INTERNAL STRING)
    set(list_explicit_compiler_options_all CACHE INTERNAL STRING)
    set(list_explicit_linker_options_all CACHE INTERNAL STRING)

    # --

    message(STATUS "Adding the following base compiler options: ${compiler_options_base}")
    message(STATUS "Adding the following base compiler warning options: ${base_compiler_options_warning}")

    message(STATUS "Adding the following conditional compiler options: ${checked_compiler_options}.")
    if(COMP_HAS_ASAN)
        message(STATUS "Adding the following conditional compiler options for ASan: ${checked_compiler_options_asan}.")
    endif()
    if(COMP_HAS_UBSAN)
        message(
            STATUS
            "Adding the following conditional compiler options for UBSan: ${checked_compiler_options_ubsan}."
        )
    endif()
    if(COMP_HAS_LSAN)
        message(STATUS "Adding the following conditional compiler options for LSan: ${checked_compiler_options_lsan}.")
    endif()

    if(COMP_HAS_MSAN)
        message(STATUS "Adding the following conditional compiler options for MSan: ${checked_compiler_options_msan}.")
    endif()

    message(
        STATUS
        "Adding the following conditional compiler options for hardening: ${checked_compiler_options_sanitize_harden}."
    )
    message(STATUS "Adding the following conditional compiler warnings: ${checked_compiler_warnings}.")
    message(
        STATUS
        "Adding the following conditional compiler warnings ignore options: ${checked_compiler_warnings_disabled}."
    )
    message(STATUS "Adding the following linker options: ${list_explicit_linker_options_all}.")

elseif(MSVC)
    list(
        APPEND
        list_explicit_compiler_options_all
        ${base_compiler_options_msvc}
        ${checked_compiler_options_msvc}
    )

    message(STATUS "Adding the following conditional compiler options: ${list_explicit_compiler_options_all}.")

endif()
