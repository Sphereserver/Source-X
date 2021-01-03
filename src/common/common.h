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


#include <memory>   // for smart pointers
#include <cstdlib>
#include "assertion.h"
#include "basic_threading.h"

#ifdef _WIN32
	#include "os_windows.h"
#endif
#include "datatypes.h"
#ifndef _WIN32
	#include "os_unix.h"
#endif


#define CountOf(a)			(sizeof(a)/sizeof((a)[0]))

#if __cplusplus >= 201703L  // is C++17 enabled?
    #define FALLTHROUGH [[fallthrough]]
	#define NODISCARD	[[nodiscard]]
#else
    #define FALLTHROUGH // fall through
    /* yep, the comment appears to silence the warning with GCC, dunno for clang */
	#define NODISCARD
#endif


/* Start of arithmetic code */

// MAKEWORD:  defined in minwindef.h (loaded my windows.h), so it's missing only on Linux.
// MAKEDWORD: undefined even on Windows, it isn't in windows.h.
// MAKELONG:  defined in minwindef.h, we use it only on Windows (CSWindow.h). on Linux is missing, we created a define but is commented.
#define MAKEDWORD(low, high)	((dword)(((word)low) | (((dword)((word)high)) << 16)))

#define IsNegative(c)			(((c) < 0) ? 1 : 0)

//#define IMulDiv(a,b,c)		(((((int)(a)*(int)(b)) + (int)(c / 2)) / (int)(c)) - (IsNegative((int)(a)*(int)(b))))
inline int IMulDiv(int a, int b, int c) noexcept
{
	const int ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

//#define IMulDivLL(a,b,c)		(((((llong)(a)*(llong)(b)) + (llong)(c / 2)) / (llong)(c)) - (IsNegative((llong)(a)*(llong)(b))))
inline llong IMulDivLL(llong a, llong b, llong c) noexcept
{
	const llong ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}
inline realtype IMulDivRT(realtype a, realtype b, realtype c) noexcept
{
	const realtype ab = a*b;
	return ((ab + (c/2)) / c) - IsNegative(ab);
}

//#define IMulDivDown(a,b,c)	(((a)*(b))/(c))
inline int IMulDivDown(int a, int b, int c) noexcept
{
	return (a*b)/c;
}
inline llong IMulDivDownLL(llong a, llong b, llong c) noexcept
{
	return (a*b)/c;
}

//#define sign(n) (((n) < 0) ? -1 : (((n) > 0) ? 1 : 0))
template<typename T> inline T sign(T n) noexcept
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
