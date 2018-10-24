#if defined(_WIN32) && !defined(_DEBUG) && !defined(_NO_CRASHDUMP)

#include "crashdump.h"

bool CrashDump::m_bEnabled = false;
HMODULE CrashDump::m_hDll = nullptr;
MINIDUMPWRITEDUMP CrashDump::m_tDumpFunction = nullptr;

bool CrashDump::IsEnabled()
{
	return m_bEnabled;
}

void CrashDump::Enable()
{
	m_hDll = LoadLibrary("dbghelp.dll");
	if (!m_hDll)
	{
		m_bEnabled = false;
		return;
	}

	m_tDumpFunction = reinterpret_cast<MINIDUMPWRITEDUMP>(GetProcAddress(m_hDll, "MiniDumpWriteDump"));
	if (!m_tDumpFunction)
	{
		m_bEnabled = false;
		FreeLibrary(m_hDll);
		return;
	}

	m_bEnabled = true;
}

void CrashDump::Disable()
{
	m_bEnabled = false;

	if (m_tDumpFunction != nullptr)
		m_tDumpFunction = nullptr;

	if (m_hDll != nullptr)
	{
		FreeLibrary(m_hDll);
		m_hDll = nullptr;
	}
}

void CrashDump::StartCrashDump( DWORD processID, DWORD threadID, struct _EXCEPTION_POINTERS* pExceptionData ) 
{
	HANDLE process = nullptr, dumpFile = nullptr;
	
	if (!processID || !threadID || !pExceptionData)
		return;

	process = GetCurrentProcess();
	if (process == nullptr)
		return;
	
	char buf[128] = "";
	SYSTEMTIME st;
	GetSystemTime(&st);
	sprintf(buf, "sphereCrash_%02i%02i-%02i%02i%02i%04i.dmp", st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	dumpFile = CreateFile(buf, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, 0, 0);
	if (dumpFile == INVALID_HANDLE_VALUE)
		return;
	
	MINIDUMP_EXCEPTION_INFORMATION eInfo;
	eInfo.ThreadId = threadID;
	eInfo.ExceptionPointers = pExceptionData;
	eInfo.ClientPointers = true;
	
	m_tDumpFunction(process, processID, dumpFile, MiniDumpWithDataSegs, &eInfo, nullptr, nullptr);
	
	CloseHandle(dumpFile);
	return;
}

#endif