#ifndef _INC_TARGET_INFO_H
#define _INC_TARGET_INFO_H


[[maybe_unused]]
constexpr const char* get_target_os_str()
{
#if defined(_WIN32)
    return "Windows";
#elif defined(_BSD)
    return "FreeBSD";
#elif defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "MacOS";
#else
    return "Unknown";
#endif
}

[[maybe_unused]]
constexpr const char* get_target_word_size_str()
{
    return (sizeof(void*) == 8) ? "64" : "32";
}

// __ARM_ARCH
[[maybe_unused]]
constexpr const char* get_target_arch_str()
{
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
    return "x86";
#elif defined(__ARM_ARCH_2__)
    return "ARMv2";
#elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
    return "ARMv3";
#elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
    return "ARMv4T";
#elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
    return "ARMv5"
#elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
    return "ARMv6T2";
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
    return "ARMv6";
#elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "ARMv7";
#elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "ARMv7A";
#elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
    return "ARMv7R";
#elif defined(__ARM_ARCH_7M__)
    return "ARMv7M";
#elif defined(__ARM_ARCH_7S__)
    return "ARMv7S";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "ARMv64";
#elif defined(__riscv)
    return "RISC-V"
#elif defined(mips) || defined(__mips__) || defined(__mips)
    return "MIPS";
#elif defined(__sh__)
    return "SUPERH";
#elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
    return "POWERPC";
#elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
    return "POWERPC64";
#elif defined(__sparc__) || defined(__sparc)
    return "SPARC";
#elif defined(__m68k__)
    return "M68K";
#else
    return "UNKNOWN";
#endif
}


#endif // _INC_TARGET_INFO_H
