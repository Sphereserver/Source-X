#ifdef WINDOWS_SHOULD_EMIT_CRASH_DUMP
#define _INC_CRASHDUMP_H

#include <cstdio>

// TODO: enable creation of the crash dump also for mingw, since it can manage windows structured exceptions.

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN	// include just windows.h without the other winapi headers, we'll add them manually when needed
#endif
#include <windows.h>

#ifdef __MINGW32__
	#include "mingwdbghelp.h"
#else  // !__MINGW32__
	#pragma warning(disable:4091)
	#include <dbghelp.h>
	#pragma warning(default:4091)
#endif  // __MINGW32__

typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess,
										 DWORD dwPid,
										 HANDLE hFile,
										 MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
class CrashDump
{
	private:
		static bool m_fEnabled;
		static HMODULE m_hDll;
		static MINIDUMPWRITEDUMP m_tDumpFunction;

	public:
		static void StartCrashDump( DWORD , DWORD , struct _EXCEPTION_POINTERS* );
		static bool IsEnabled();
		static void Enable();
		static void Disable();

	private:
		CrashDump(void);
		CrashDump(const CrashDump& copy);
		CrashDump& operator=(const CrashDump& other);
};


#endif
