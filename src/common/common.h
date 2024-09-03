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
#include "assertion.h"
#include "basic_threading.h"

#ifdef _WIN32
	#include "os_windows.h"
#endif
#include "datatypes.h"
#ifndef _WIN32
	#include "os_unix.h"
#endif

#include "sphere_library/stypecast.h"


/* Coding helpers */

// Strings
#define STRINGIFY_IMPL_(x)	#x
#define STRINGIFY(x)		STRINGIFY_IMPL_(x)

// Sizes
#define ARRAY_COUNT(a)			        (sizeof(a)/sizeof((a)[0]))

// Flags and bitmasks. Those macros works with 1 or multiple ORed flags together.
#define HAS_FLAGS_STRICT(var, flag)     (((var) & (flag)) == flag)          // Every one of the passed flags has to be set
#define HAS_FLAGS_ANY(var, flag)        (static_cast<bool>((var) & (flag))) // True if even only one of the passed flags are set


/* Start of arithmetic code */

//#define IsNegative(c)			(((c) < 0) ? 1 : 0)
template <typename T>
constexpr bool IsNegative(T val) noexcept {
    return (val < 0);
}

//-- Bitwise magic: combine numbers.

// MAKEWORD:  defined in minwindef.h (loaded by windows.h), so it's missing only on Linux.
// MAKEDWORD: undefined even on Windows, it isn't in windows.h.
// MAKELONG:  defined in minwindef.h, we use it only on Windows (CSWindow.h). on Linux is missing, we created a define but is commented.
#define MAKEDWORD(low, high)	((dword)(((word)low) | (((dword)((word)high)) << 16)))


//#define IMulDiv(a,b,c)		(((((int)(a)*(int)(b)) + (int)(c / 2)) / (int)(c)) - (IsNegative((int)(a)*(int)(b))))
constexpr int IMulDiv(const int a, const int b, const int c) noexcept
{
	const int ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

constexpr uint UIMulDiv(const uint a, const uint b, const uint c) noexcept
{
	const int ab = a * b;
	return ((ab + (c / 2)) / c) - IsNegative(ab);
}

//#define IMulDivLL(a,b,c)		(((((llong)(a)*(llong)(b)) + (llong)(c / 2)) / (llong)(c)) - (IsNegative((llong)(a)*(llong)(b))))
constexpr llong IMulDivLL(const llong a, const llong b, const llong c) noexcept
{
	const llong ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}
constexpr realtype IMulDivRT(const realtype a, const realtype b, const realtype c) noexcept
{
	const realtype ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

//#define IMulDivDown(a,b,c)	(((a)*(b))/(c))
constexpr int IMulDivDown(const int a, const int b, const int c) noexcept
{
	return (a*b)/c;
}
constexpr llong IMulDivDownLL(const llong a, const llong b, const llong c) noexcept
{
	return (a*b)/c;
}

//#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
template<typename T>
constexpr T sign(const T n) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Invalid data type.");
	return ( (n < 0) ? -1 : ((n > 0) ? 1 : 0) );
}

#define minimum(x,y)		((x)<(y)?(x):(y))		// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!
#define maximum(x,y)		((x)>(y)?(x):(y))		// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!
#define medium(x,y,z)	((x)>(y)?(x):((z)<(y)?(z):(y)))	// NOT to be used with functions! Store the result of the function in a variable first, otherwise the function will be executed twice!


/* End of arithmetic code */


/* Compiler/c++ language helpers */

// Ensure that a constexpr value or a generic expression is evaluated at compile time.
template <typename T>
consteval T ensure_comptime(T&& val_) noexcept {
    return val_;
}

/*
	There is a problem with the UnreferencedParameter macro from mingw and sphereserver.
	operator= is on many clases private and the UnreferencedParameter macro from mingw is (P)=(P),
	so we have a compilation error here.
*/
#undef UNREFERENCED_PARAMETER
template <typename T>
constexpr void UnreferencedParameter(T const&) noexcept {
    ;
}

//#include <type_traits> // already included by stypecast.h

// Arguments: Class, arguments...
#define STATIC_ASSERT_NOEXCEPT_CONSTRUCTOR(_ClassType, ...) \
    static_assert( std::is_nothrow_constructible_v<_ClassType __VA_OPT__(,) __VA_ARGS__>, #_ClassType  " constructor should be noexcept!")
#define STATIC_ASSERT_THROWING_CONSTRUCTOR(_ClassType, ...) \
    static_assert(!std::is_nothrow_constructible_v<_ClassType __VA_OPT__(,) __VA_ARGS__>, #_ClassType " constructor should *not* be noexcept!")

// Arguments: function_name, arguments...
#define STATIC_ASSERT_NOEXCEPT_FREE_FUNCTION(_func, ...) \
    static_assert( std::is_nothrow_invocable_v<decltype(&_func) __VA_OPT__(,) __VA_ARGS__>, #_func  " function should be noexcept!")
#define STATIC_ASSERT_THROWING_FREE_FUNCTION(_func, ...) \
    static_assert(!std::is_nothrow_invocable_v<decltype(&_func) __VA_OPT__(,) __VA_ARGS__>, #_func  " function should be noexcept!")
// static_assert( noexcept(std::declval<decltype(&_func)>()()), #_func " should be noexcept");

// Arguments: Class, function_name, arguments...
#define STATIC_ASSERT_NOEXCEPT_MEMBER_FUNCTION(_ClassType, _func, ...) \
    static_assert( std::is_nothrow_invocable_v<decltype(&_ClassType::_func), _ClassType __VA_OPT__(,) __VA_ARGS__>, #_func  " function should be noexcept!")
#define STATIC_ASSERT_THROWING_MEMBER_FUNCTION(_ClassType, _func, ...) \
    static_assert(!std::is_nothrow_invocable_v<decltype(&_ClassType::_func), _ClassType __VA_OPT__(,) __VA_ARGS__>, #_func  " function should be noexcept!")


// Function specifier, like noexcept. Use this to make us know that the function code was checked and we know it can throw an exception.
#define CANTHROW    noexcept(false)

// Cpp attributes
#define FALLTHROUGH [[fallthrough]]
#define NODISCARD	[[nodiscard]]

#ifdef _DEBUG
    #define NOEXCEPT_NODEBUG
#else
    #define NOEXCEPT_NODEBUG noexcept
#endif


// For unrecoverable/unloggable errors. Should be used almost *never*.
#define STDERR_LOG(...)     fprintf(stderr, __VA_ARGS__); fflush(stderr)

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


/* Sanitizers utilities */

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


#endif	// _INC_COMMON_H
