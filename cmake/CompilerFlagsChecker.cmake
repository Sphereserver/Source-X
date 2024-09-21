include(CheckCXXCompilerFlag)
message(STATUS "Checking available compiler flags...")


if (NOT MSVC)
    message(STATUS "-- Compilation options:")
    # Compiler option flags. Common to both compilers, but older versions might not support the following.
    #check_cxx_compiler_flag("" COMP_HAS_)

    # Compiler option flags. Expected to work on GCC but not Clang, at the moment.
    check_cxx_compiler_flag("-fno-expensive-optimizations" COMP_HAS_FNO_EXPENSIVE_OPTIMIZATIONS)

    # Compiler option flags. Expected to work on Clang but not GCC, at the moment.
    #check_cxx_compiler_flag("-fforce-emit-vtables" COMP_HAS_F_FORCE_EMIT_VTABLES)

    message(STATUS "-- Compilation options (Address Sanitizer):")
    # Compiler option flags. Sanitizers related (ASAN).
    #check_cxx_compiler_flag("-fsanitize=safe-stack" COMP_HAS_)  # Can't be used with asan!
    check_cxx_compiler_flag("-fsanitize-cfi" COMP_HAS_FSAN_CFI) # cfi: control flow integrity
    check_cxx_compiler_flag("-fsanitize-address-use-after-scope" COMP_HAS_FSAN_ADDRESS_USE_AFTER_SCOPE)
    check_cxx_compiler_flag("-fsanitize=pointer-compare" COMP_HAS_FSAN_PTR_COMP)
    check_cxx_compiler_flag("-fsanitize=pointer-subtract" COMP_HAS_FSAN_PTR_SUB)
    check_cxx_compiler_flag("-fsanitize-trap=all" COMP_HAS_FSAN_TRAP_ALL)

    message(STATUS "-- Compilation options (Sanitizers-related and safety checks):")
    # Compiler option flags. Sanitizers related (other).
    # fsanitize=signed-integer-overflow # -fno-strict-overflow (which implies -fwrapv-pointer and -fwrapv)
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

    # Compiler warning flags.
    message(STATUS "-- Compilation options (warnings):")
    check_cxx_compiler_flag("-Wuseless-cast" COMP_HAS_W_USELESS_CAST)
    check_cxx_compiler_flag("-Wnull-dereference" COMP_HAS_W_NULL_DEREFERENCE)
    check_cxx_compiler_flag("-Wconversion" COMP_HAS_W_CONVERSION) # Temporarily disabled. Implicit type conversions that might change a value, such as narrowing conversions.
    check_cxx_compiler_flag("-Wcast-qual" COMP_HAS_W_CAST_QUAL) # casts that remove a type's const or volatile qualifier.
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
    # -Wshadow
    # -Wfree-nonheap-object (good but false positive warnings on GCC?)
    # -Wcast-align=strict
    # -Wcast-user-defined
    # -Wdate-time

    # Compiler warning flags. To be disabled.
    message(STATUS "-- Compilation options (disable specific warnings):")
    check_cxx_compiler_flag("-Wno-format-nonliteral" COMP_HAS_WNO_FORMAT_NONLITERAL) # Since -Wformat=2 is stricter, you would need to disable this warning.
    check_cxx_compiler_flag("-Wno-nonnull-compare" COMP_HAS_WNO_NONNULL_COMPARE)
    check_cxx_compiler_flag("-Wno-maybe-uninitialized" COMP_HAS_WNO_MAYBE_UNINIT)
    check_cxx_compiler_flag("-Wno-switch" COMP_HAS_WNO_SWITCH)
    check_cxx_compiler_flag("-Wno-implicit-fallthrough" COMP_HAS_WNO_IMPLICIT_FALLTHROUGH)
    check_cxx_compiler_flag("-Wno-parentheses" COMP_HAS_WNO_PARENTHESES)
    check_cxx_compiler_flag("-Wno-misleading-indentation" COMP_HAS_WNO_MISLEADING_INDENTATION)
    check_cxx_compiler_flag("-Wno-unused-result" COMP_HAS_WNO_UNUSED_RESULT)
    check_cxx_compiler_flag("-Wno-nested-anon-types" COMP_HAS_WNO_NESTED_ANON_TYPES)
    check_cxx_compiler_flag("-Wno-format-security" COMP_HAS_WNO_FORMAT_SECURITY) # TODO: remove that when we'll have time to fix every printf format issue
    check_cxx_compiler_flag("-Wno-deprecated-declarations" COMP_HAS_WNO_DEPRECATED_DECLARATIONS)
    #check_cxx_compiler_flag("-Wno-unknown-pragmas" COMP_HAS_)
    #check_cxx_compiler_flag("" COMP_HAS_)

else (NOT MSVC)
    # Compiler warning flags (MSVC).
    message(STATUS "-- Compilation options (MSVC):")
    check_cxx_compiler_flag("/Wd5105" COMP_HAS_DEFINE_MACRO_EXPANSION) # False positive warning, maybe even a MSVC bug.
    #check_cxx_compiler_flag("" COMP_HAS_)
endif()


# ---- Append options into some lists.

# ---- GCC/Clang

if(COMP_HAS_FNO_EXPENSIVE_OPTIMIZATIONS)
    list(APPEND checked_compiler_options "-fno-expensive-optimizations")
endif()

if(COMP_HAS_FSAN_CFI)
    list(APPEND checked_compiler_options_asan "-fsanitize-cfi")
endif()
if(COMP_HAS_FSAN_ADDRESS_USE_AFTER_SCOPE)
    list(APPEND checked_compiler_options_asan "-fsanitize-address-use-after-scope")
endif()
if(COMP_HAS_FSAN_PTR_COMP)
    list(APPEND checked_compiler_options_asan "-fsanitize=pointer-compare" )
endif()
if(COMP_HAS_FSAN_PTR_SUB)
    list(APPEND checked_compiler_options_asan "-fsanitize=pointer-subtract" )
endif()
if(COMP_HAS_FSAN_TRAP_ALL)
    list(APPEND checked_compiler_options_asan "-fsanitize-trap=all" )
endif()

if(COMP_HAS_FCF_PROTECTION_FULL)
    list(APPEND checked_compiler_options_sanitize_other "-fcf-protection=full")
endif()
if(COMP_HAS_F_STACK_CHECK)
    list(APPEND checked_compiler_options_sanitize_other "-fstack-check")
endif()
if(COMP_HAS_F_STACK_PROTECTOR_ALL)
    list(APPEND checked_compiler_options_sanitize_other "-fstack-protector-all")
elseif(COMP_HAS_F_STACK_PROTECTOR_STRONG)
    list(APPEND checked_compiler_options_sanitize_other "-fstack-protector-strong")
elseif(COMP_HAS_F_STACK_PROTECTOR)
    list(APPEND checked_compiler_options_sanitize_other "-fstack-protector")
endif()
if(COMP_HAS_F_STACK_CLASH_PROTECTION)
    list(APPEND checked_compiler_options_sanitize_other "-fstack-clash-protection")
endif()
if(COMP_HAS_F_VTABLE_VERIFY_PREINIT)
    list(APPEND checked_compiler_options_sanitize_other "-fvtable-verify=preinit")
endif()
if(COMP_HAS_F_HARDEN_CFR)
    list(APPEND checked_compiler_options_sanitize_other "-fharden-control-flow-redundancy")
endif()
if(COMP_HAS_F_HARDCFR_CHECK_EXCEPTIONS)
    list(
        APPEND checked_compiler_options_sanitize_other "-fhardcfr-check-exceptions")
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

# ---- MSVC

if(COMP_HAS_DEFINE_MACRO_EXPANSION)
    list(APPEND checked_compiler_flags_msvc "/Wd5105")
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
set(checked_compiler_options_sanitize_other)
set(checked_compiler_warnings)
set(checked_compiler_warnings_disabled)
set(checked_compiler_flags_msvc)
]]

message(STATUS "Adding the following conditional compiler options: ${checked_compiler_options}")
message(STATUS "Adding the following conditional compiler options for ASan: ${checked_compiler_options_asan}")
message(STATUS "Adding the following conditional compiler options for sanitizers: ${checked_compiler_options_sanitize_other}")
message(STATUS "Adding the following conditional compiler warnings: ${checked_compiler_warnings}")
message(STATUS "Adding the following conditional compiler warnings ignore options: ${checked_compiler_warnings_disabled}")

if(MSVC)
    message(STATUS "Adding the following conditional compiler options: ${checked_compiler_flags_msvc}")
endif()
