/**
* @file common.h
* @brief Header that should be included by every file.
*/

#ifndef _INC_COMMON_H
#define _INC_COMMON_H


#define SPHERE_DEF_PORT			2593

#define SPHERE_FILE				"sphere"	// file name prefix
#define SPHERE_TITLE			"SphereServer"
#define SPHERE_SCRIPT			".scp"

#define SCRIPT_MAX_LINE_LEN		4096		// default size.
#define SCRIPT_MAX_SECTION_LEN	128


// C abs function has different in/out types the std:: ones in cmath. It's defined in stdlib.h.
#include <stdlib.h>
#include <memory>   // for smart pointers
#include <type_traits>
#include "assertion.h"
#include "basic_threading.h"

#ifdef _WIN32
	#include "os_windows.h"
#endif
#include "datatypes.h"
#ifndef _WIN32
	#include "os_unix.h"
#endif


// Strings
#define _STRINGIFY_AUX(x)	#x
#define STRINGIFY(x)		_STRINGIFY_AUX(x)

// Sizes
#define ARRAY_COUNT(a)			        (sizeof(a)/sizeof((a)[0]))

// Flags and bitmasks. Those macros works with 1 or multiple ORed flags together.
#define HAS_FLAGS_STRICT(var, flag)     (((var) & (flag)) == flag)          // Every one of the passed flags has to be set
#define HAS_FLAGS_ANY(var, flag)        (static_cast<bool>((var) & (flag))) // True if even only one of the passed flags are set

// Cpp attributes
#define FALLTHROUGH [[fallthrough]]
#define NODISCARD	[[nodiscard]]

#ifdef _DEBUG
    #define NOEXCEPT_NODEBUG
#else
    #define NOEXCEPT_NODEBUG noexcept
#endif

/*
	There is a problem with the UnreferencedParameter macro from mingw and sphereserver.
	operator= is on many clases private and the UnreferencedParameter macro from mingw is (P)=(P),
	so we have a compilation error here.
*/
#undef UNREFERENCED_PARAMETER
template <typename T>
inline void UnreferencedParameter(T const&) noexcept {
    ;
}


/* Sanitizers utility */

#if defined(_MSC_VER)

    #if defined(__SANITIZE_ADDRESS__) || defined(ADDRESS_SANITIZER)
        #define NO_SANITIZE_ADDRESS __declspec(no_sanitize_address)
    #else
        #define NO_SANITIZE_ADDRESS
    #endif

    // Not yet implemented on MSVC, as of 2022 ver.
    #define NO_SANITIZE_UNDEFINED

#elif defined(__clang__)

    #if defined(__SANITIZE_ADDRESS__) || defined(ADDRESS_SANITIZER)
        #define NO_SANITIZE_ADDRESS __attribute__((no_sanitize_address))
    #else
        #define NO_SANITIZE_ADDRESS
    #endif

    //#if __has_feature(undefined_behavior_sanitizer) || defined(UNDEFINED_BEHAVIOR_SANITIZER)
        // Clang doesn't have an attribute nor a pragma for this (yet?)
    //    #define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize_undefined))
    //#else
        #define NO_SANITIZE_UNDEFINED
    //#endif

#elif defined(__GNUC__)

    #if defined(__SANITIZE_ADDRESS__) || defined(ADDRESS_SANITIZER)
        #define NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
    #else
        #define NO_SANITIZE_ADDRESS
    #endif

    // GCC still hasn't __SANITIZE_UNDEFINED__ ?
    //#if defined(__SANITIZE_UNDEFINED__) || defined(UNDEFINED_BEHAVIOR_SANITIZER)
    //    #define NO_SANITIZE_UNDEFINED __attribute__((no_sanitize("undefined")))
    //#else
        #define NO_SANITIZE_UNDEFINED
    //#endif

#else

    #define NO_SANITIZE_ADDRESS
    #define NO_SANITIZE_UNDEFINED

#endif


/* Start of arithmetic code */

#define IsNegative(c)			(((c) < 0) ? 1 : 0)


//-- Bitwise magic: combine numbers.

// MAKEWORD:  defined in minwindef.h (loaded by windows.h), so it's missing only on Linux.
// MAKEDWORD: undefined even on Windows, it isn't in windows.h.
// MAKELONG:  defined in minwindef.h, we use it only on Windows (CSWindow.h). on Linux is missing, we created a define but is commented.
#define MAKEDWORD(low, high)	((dword)(((word)low) | (((dword)((word)high)) << 16)))


//-- Explicitly promote to a larger type or narrow to a smaller type, instead of inattentive casts.

// Promote to the corresponding 32 bits numeric type a smaller numeric variable.
template <typename T>
auto i_promote32(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) < 4, "Input variable is not smaller than a 32 bit number.");
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype32>(a);
		return static_cast<int32>(a);
	}
	else
		return static_cast<uint32>(a);
}

// Promote to the corresponding 64 bits numeric type a smaller numeric variable.
template <typename T>
auto i_promote64(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) < 8, "Input variable is not smaller than a 64 bit number.");
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype64>(a);
		return static_cast<int64>(a);
	}
	else
		return static_cast<uint64>(a);
}

template <typename T>
auto i_narrow32(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) >= 4, "Input variable is smaller than a 32 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
	constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype32>(static_cast<uint64>(a) & umask);
		return static_cast<int32>(static_cast<uint64>(a) & umask);
	}
	else
		return static_cast<uint32>(static_cast<uint64>(a) & umask);
}

template <typename T>
auto i_narrow16(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) >= 2, "Input variable is smaller than a 16 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
	constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype16>(static_cast<uint64>(a) & umask);
		return static_cast<int16>(static_cast<uint64>(a) & umask);
	}
	else
		return static_cast<uint16>(static_cast<uint64>(a) & umask);
}

template <typename T>
auto i64_narrow32(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
	constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype32>(a & umask);
		return static_cast<int32>(a & umask);
	}
	else
		return static_cast<uint32>(a & umask);
}

template <typename T>
auto i64_narrow16(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
	constexpr uint64 umask = 0x0000'0000'0000'FFFF;
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype16>(a & umask);
		return static_cast<int16>(a & umask);
	}
	else
		return static_cast<uint16>(a & umask);
}

template <typename T>
auto i32_narrow16(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
	static_assert(sizeof(T) == 4, "Input variable is not a 32 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
	constexpr uint32 umask = 0x0000'FFFF;
	if constexpr (std::is_signed_v<T>)
	{
		if constexpr (std::is_floating_point_v<T>)
			return static_cast<realtype16>(a & umask);
		return static_cast<int16>(a & umask);
	}
	else
		return static_cast<uint16>(a & umask);
}

template <typename T>
auto i16_narrow8(const T a) noexcept
{
	static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
	static_assert(std::is_integral_v<T>, "Only integral types are supported by this function.");
	static_assert(sizeof(T) == 2, "Input variable is not a 16 bit number.");

	// Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
	constexpr uint16 umask = 0x00FF;
	if constexpr (std::is_signed_v<T>)
	{
		return static_cast<int8>(a & umask);
	}
	else
		return static_cast<uint8>(a & umask);
}


//#define IMulDiv(a,b,c)		(((((int)(a)*(int)(b)) + (int)(c / 2)) / (int)(c)) - (IsNegative((int)(a)*(int)(b))))
inline int IMulDiv(const int a, const int b, const int c) noexcept
{
	const int ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

inline uint UIMulDiv(const uint a, const uint b, const uint c) noexcept
{
	const int ab = a * b;
	return ((ab + (c / 2)) / c) - IsNegative(ab);
}

//#define IMulDivLL(a,b,c)		(((((llong)(a)*(llong)(b)) + (llong)(c / 2)) / (llong)(c)) - (IsNegative((llong)(a)*(llong)(b))))
inline llong IMulDivLL(const llong a, const llong b, const llong c) noexcept
{
	const llong ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}
inline realtype IMulDivRT(const realtype a, const realtype b, const realtype c) noexcept
{
	const realtype ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

//#define IMulDivDown(a,b,c)	(((a)*(b))/(c))
inline int IMulDivDown(const int a, const int b, const int c) noexcept
{
	return (a*b)/c;
}
inline llong IMulDivDownLL(const llong a, const llong b, const llong c) noexcept
{
	return (a*b)/c;
}

//#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
template<typename T> inline T sign(const T n) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Invalid data type.");
	return ( (n < 0) ? -1 : ((n > 0) ? 1 : 0) );
}

#define minimum(x,y)		((x)<(y)?(x):(y))		// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!
#define maximum(x,y)		((x)>(y)?(x):(y))		// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!
#define medium(x,y,z)		((x)>(y)?(x):((z)<(y)?(z):(y)))	// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!


/* End of arithmetic code */


// use to indicate that a function uses printf-style arguments, allowing GCC
// to validate the format string and arguments:
// a = 1-based index of format string
// b = 1-based index of arguments
// (note: add 1 to index for non-static class methods because 'this' argument
// is inserted in position 1)
#ifdef _MSC_VER
	#define __printfargs(a,b)
#else
	#ifdef _WIN32
		#define __printfargs(a,b) __attribute__ ((format(gnu_printf, a, b)))
	#else
		#define __printfargs(a,b) __attribute__ ((format(printf, a, b)))
	#endif
#endif


#endif	// _INC_COMMON_H
