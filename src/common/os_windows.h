/**
* @file os_windows.h
* @brief Windows-specific declarations.
*/

#ifndef _INC_OS_WINDOWS_H
#define _INC_OS_WINDOWS_H


// _WIN32_WINNT version constants
/*
#define _WIN32_WINNT_NT4                    0x0400 // Windows NT 4.0
#define _WIN32_WINNT_WIN2K                  0x0500 // Windows 2000
#define _WIN32_WINNT_WINXP                  0x0501 // Windows XP
#define _WIN32_WINNT_WS03                   0x0502 // Windows Server 2003
#define _WIN32_WINNT_WIN6                   0x0600 // Windows Vista
#define _WIN32_WINNT_VISTA                  0x0600 // Windows Vista
#define _WIN32_WINNT_WS08                   0x0600 // Windows Server 2008
#define _WIN32_WINNT_LONGHORN               0x0600 // Windows Vista
#define _WIN32_WINNT_WIN7                   0x0601 // Windows 7
#define _WIN32_WINNT_WIN8                   0x0602 // Windows 8
#define _WIN32_WINNT_WINBLUE                0x0603 // Windows 8.1
#define _WIN32_WINNT_WINTHRESHOLD           0x0A00 // Windows 10
#define _WIN32_WINNT_WIN10                  0x0A00 // Windows 10
*/
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501	// By default we target Windows XP
#endif


#undef FD_SETSIZE
#define FD_SETSIZE 1024		// for max of n users ! default = 64	(FD: file descriptor)

// disable useless windows.h features
#define WIN32_LEAN_AND_MEAN	// include just windows.h without the other winapi headers, we'll add them manually when needed
#undef NOMINMAX
#define NOMINMAX			// we don't want to have windows min and max macros, we have our minimum and maximum
#define NOATOM
#define NOCRYPT
#define NOMCX
#define NOMETAFILE
#define NOKANJI
#define NOKERNEL
#define NOOPENFILE
#define NORASTEROPS
#define NOSOUND
#define NOSYSMETRICS
#define NOTEXTMETRIC
#define NOWH

#   ifdef _MSC_VER
// Workaround to a possible VS compiler bug: instead of complaining if a macro expands to a "defined" macro,
//  it complains if a define macro contains the words "defined" in its name...
#       pragma warning(push)
#       pragma warning(disable: 5105)
#   endif
#   include <windows.h>
#   ifdef _MSC_VER
#       pragma warning(pop)
#   endif


/*	file handling definitions  */
#define STDFUNC_FILENO(_x_)		_get_osfhandle(_fileno(_x_))
#define STDFUNC_UNLINK			_unlink

/*	threading definitions  */
#define STDFUNC_GETPID			_getpid

// since the only way to make windows not to buffer file is to remove buffer, we
// use this instead flushing
#define	FILE_SETNOCACHE(_x_)	setvbuf(_x_, nullptr, _IONBF, 0)
#define FILE_FLUSH(_x_)

#ifndef STRICT
	#define STRICT			// strict conversion of handles and pointers.
#endif

#ifndef _MSC_VER	// No Microsoft compiler
	#define _cdecl	__cdecl

	// Not defined for mingw.
	#define LSTATUS int
	typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *,const wchar_t *,const wchar_t *,unsigned int,uintptr_t);
	// Stuctured exception handling windows api not implemented on mingw.
	#define __except(P)		catch(int)
#endif  // _MSC_VER

const OSVERSIONINFO * Sphere_GetOSInfo() noexcept;


#endif	// _INC_OS_WINDOWS_H
