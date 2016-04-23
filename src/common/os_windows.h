/**
* @file os_windows.h
* @brief Windows-specific declarations.
*/

#pragma once
#ifndef _INC_OS_WINDOWS_H
#define _INC_OS_WINDOWS_H

//#define SLASH_PATH	"\\"
#define _WIN32_DCOM
#ifndef _WIN32_WINNT
	#define _WIN32_WINNT 0x0501
#endif

#undef FD_SETSIZE
#define FD_SETSIZE 1024		// for max of n users ! default = 64

#define NOMINMAX			// we don't want to have windows min and max macros, we have our minimum and maximum
#include <winsock2.h>
#include <windows.h>
#include <process.h>

/*	thread-specific definitions  */
#define THREAD_ENTRY_RET		void
#define STDFUNC_FILENO			_fileno
#define STDFUNC_GETPID			_getpid
#define STDFUNC_UNLINK			_unlink

// since the only way to make windows not to buffer file is to remove buffer, we
// use this instead flushing
#define	FILE_SETNOCACHE(_x_)	setvbuf(_x_, NULL, _IONBF, 0)
#define FILE_FLUSH(_x_)


#ifndef _MSC_VER	// No Microsoft compiler
	#define _cdecl	__cdecl
	/*
	There is a problem with the UNREFERENCED_PARAMETER macro from mingw and sphereserver.
	operator= is on many clases private and the UNREFERENCED_PARAMETER macro from mingw is (P)=(P),
	so we have a compilation error here.
	*/
	#undef  UNREFERENCED_PARAMETER
	#define UNREFERENCED_PARAMETER(P)	(void)(P)
	// Not defined for mingw.
	#define LSTATUS int
	typedef void (__cdecl *_invalid_parameter_handler)(const wchar_t *,const wchar_t *,const wchar_t *,unsigned int,uintptr_t);
	// Stuctured exception handling windows api not implemented on mingw.
	#define __except(P)		catch(int)
#endif  // _MSC_VER


extern bool NTWindow_Init(HINSTANCE hInstance, LPTSTR lpCmdLinel, int nCmdShow);
extern void NTWindow_Exit();
extern void NTWindow_DeleteIcon();
extern bool NTWindow_OnTick(int iWaitmSec);
extern bool NTWindow_PostMsg(LPCTSTR pszMsg);
extern bool NTWindow_PostMsgColor(COLORREF color);
extern void NTWindow_SetWindowTitle(LPCTSTR pText = NULL);


#endif	// _INC_OS_WINDOWS_H
