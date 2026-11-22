
//#include "CException.h" // included in the precompiled header

#ifdef WINDOWS_SHOULD_EMIT_CRASH_DUMP
#include "crashdump/crashdump.h"
#endif

#ifndef _WIN32
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>

#include "../game/CServer.h"

#include <fcntl.h>
int IsDebuggerPresent(void)
{
	char buf[1024];
	int debugger_present = 0;

	int status_fd = open("/proc/self/status", O_RDONLY);
	if (status_fd == -1)
		return 0;

	ssize_t num_read = read(status_fd, buf, sizeof(buf)-1);
    close(status_fd);

	if (num_read > 0)
	{
		static const char TracerPid[] = "TracerPid:";
		char *tracer_pid;

		buf[num_read] = 0;
		tracer_pid    = strstr(buf, TracerPid);
		if (tracer_pid)
			debugger_present = !!atoi(tracer_pid + sizeof(TracerPid) - 1);
	}

	return debugger_present;
}

#endif // !_WIN32


void NotifyDebugger()
{
    if (IsDebuggerPresent())
    {

#ifdef _WIN32
    #ifdef MSVC_COMPILER
        __debugbreak();
    #else
        std::abort();
    #endif
#else
        raise(SIGINT);
#endif

    }
}


[[noreturn]]
void RaiseImmediateAbort(int iErrCode)
{
    // Have iErrCode to help with the debugging process. Use a different number for each invocation,
    UnreferencedParameter(iErrCode);

#ifdef _DEBUG
    reserved_stderr_log("RaiseImmediateAbort with code %d.\n", iErrCode);
#endif

    EXC_NOTIFY_DEBUGGER;

#if !defined(_WIN32)
    // The trap raises SIGILL: dump the core right here, without going through Signal_Fatal.
    signal(SIGILL, SIG_DFL);
#endif
#if defined(__GNUC__) || defined(__clang__)
    __builtin_trap();
#else
    std::abort();
#endif
}

#ifdef _WIN32
int CSError::GetSystemErrorMessage(dword dwError, lptstr lpszError, dword dwErrorBufLength) // static
{
    //	PURPOSE:  copies error message text to a string
    //
    //	PARAMETERS:
    //		lpszBuf - destination buffer
    //		dwSize - size of buffer
    //
    //	RETURN VALUE:
    //		destination buffer

    LPCVOID lpSource = nullptr;
    va_list* Arguments = nullptr;
    DWORD nChars = ::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        lpSource,
        dwError, LANG_NEUTRAL,
        lpszError, dwErrorBufLength, Arguments);

    if (nChars > (DWORD)0)
    {     // successful translation -- trim any trailing junk
        DWORD index = nChars - 1;      // index of last character
        while ((lpszError[index] == '\n') || (lpszError[index] == '\r'))
            lpszError[index--] = '\0';
        nChars = index + 1;
    }

    return nChars;
}
#endif

bool CSError::GetErrorMessage( lptstr lpszError, uint uiMaxError ) const
{
#ifdef _WIN32
	// Compatible with CException and CSFileException
	if ( m_hError )
	{
		// return the message defined by the system for the error code
		tchar szCode[ 1024 ];
		int nChars = GetSystemErrorMessage( m_hError, szCode, sizeof(szCode) );
		if ( nChars )
		{
			if ( m_hError & 0x80000000 )
				snprintf( lpszError, uiMaxError, "Error Pri=%d, Code=0x%x(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription );
			else
				snprintf( lpszError, uiMaxError, "Error Pri=%d, Code=%u(%s), Desc='%s'", m_eSeverity, m_hError, szCode, m_pszDescription );
			return true;
		}
	}
#endif

	if ( m_hError & 0x80000000 )
		snprintf( lpszError, uiMaxError, "Error Pri=%d, Code=0x%x, Desc='%s'", m_eSeverity, m_hError, m_pszDescription );
	else
		snprintf( lpszError, uiMaxError, "Error Pri=%d, Code=%u, Desc='%s'", m_eSeverity, m_hError, m_pszDescription );
	return true;
}

CSError::CSError( const CSError &e ) :
	m_eSeverity( e.m_eSeverity ),
	m_hError( e.m_hError ),
	m_pszDescription( e.m_pszDescription )
{
}

CSError::CSError( LOG_TYPE eSev, dword hErr, lpctstr pszDescription ) :
	m_eSeverity( eSev ),
	m_hError( hErr ),
	m_pszDescription( pszDescription )
{
}


// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------


bool CAssert::GetErrorMessage(lptstr lpszError, uint uiMaxError) const
{
	snprintf(lpszError, uiMaxError, "Assert severity=%d: '%s' file '%s', line %lld", m_eSeverity, m_pExp, m_pFile, m_llLine);
	return true;
}

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

#ifdef WINDOWS_SPHERE_SHOULD_HANDLE_STRUCTURED_EXCEPTIONS

CWinStructuredException::CWinStructuredException(uint uCode, size_t pAddress) :
	CSError(LOGL_CRIT, uCode, "Exception"), m_pAddress(pAddress)
{
}

CWinStructuredException::~CWinStructuredException()
{
}

bool CWinStructuredException::GetErrorMessage(lptstr lpszError, uint uiMaxError) const
{
	lpctstr zMsg;
	switch ( m_hError )
	{
		case STATUS_BREAKPOINT:				zMsg = "Breakpoint";				break;
		case STATUS_ACCESS_VIOLATION:		zMsg = "Access Violation";			break;
		case STATUS_FLOAT_DIVIDE_BY_ZERO:	zMsg = "Float: Divide by Zero";		break;
		case STATUS_INTEGER_DIVIDE_BY_ZERO:	zMsg = "Integer: Divide by Zero";	break;
		case STATUS_STACK_OVERFLOW:			zMsg = "Stack Overflow";			break;
		default:
			snprintf(lpszError, uiMaxError, "code=0x%x, (0x%" PRIxSIZE_T ")", m_hError, m_pAddress);
			return true;
	}

	snprintf(lpszError, uiMaxError, "\"%s\" (0x%" PRIxSIZE_T ")", zMsg, m_pAddress);
	return true;
}

#endif

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

[[noreturn]]
void Assert_Fail( lpctstr pExp, lpctstr pFile, long long llLine )
{
	EXC_NOTIFY_DEBUGGER;
	throw CAssert(LOGL_CRIT, pExp, pFile, llLine);
}

static void SPHERE_CDECL Sphere_Purecall_Handler()
{
	// catch this special type of C++ exception as well.
	Assert_Fail("purecall", "unknown", 1);
}

void SetPurecallHandler()
{
	// We don't want sphere to immediately exit if something calls a pure virtual method.
    // Those functions set the behavior process-wide, so there's no need to call this on each thread.
#ifdef MSVC_COMPILER
	_set_purecall_handler(Sphere_Purecall_Handler);
#else
	// GCC handler for pure calls is __cxxabiv1::__cxa_pure_virtual.
	// Its code calls std::terminate(), which then calls abort(), so we set the terminate handler to redefine this behaviour.
	std::set_terminate(Sphere_Purecall_Handler);
#endif
}

#ifdef WINDOWS_SPHERE_SHOULD_HANDLE_STRUCTURED_EXCEPTIONS
static void SPHERE_CDECL Sphere_Structured_Exception_Windows( unsigned int id, struct _EXCEPTION_POINTERS* pData )
{
#   ifdef WINDOWS_SHOULD_EMIT_CRASH_DUMP
	if ( CrashDump::IsEnabled() )
		CrashDump::StartCrashDump(GetCurrentProcessId(), GetCurrentThreadId(), pData);
#   endif

	// WIN32 gets an exception.
	size_t pCodeStart = (size_t)(byte *) &globalstartsymbol;	// sync up to my MAP file.

	size_t pAddr = (size_t)pData->ExceptionRecord->ExceptionAddress;
	pAddr -= pCodeStart;

    throw CWinStructuredException(id, pAddr);
}

void SetWindowsStructuredExceptionTranslator()
{
    // Process-wide, no need to call this on each thread.
    _set_se_translator( Sphere_Structured_Exception_Windows );
}
#endif

#ifndef _WIN32
// Signal handlers may run on any thread, interrupting any code, also while it holds a lock (e.g. the log one, which isn't
//  recursive). They do only async-signal-safe operations: set atomic flags (lock-free), write(), signal(), raise().
// The work requested by a signal is done by the main thread (CServer::_OnTick) or by the normal shutdown sequence.

static void Signal_WriteStderr(const char* ptcMsg) noexcept
{
    const ssize_t iRet = write(STDERR_FILENO, ptcMsg, strlen(ptcMsg));
    UnreferencedParameter(iRet);
}

static void Signal_Hangup(int sig = 0) noexcept // Save and stop (SIGHUP, e.g. the terminal was closed)
{
    UnreferencedParameter(sig);
    // The main thread saves the world, then sets the exit flag.
    g_Serv.RequestSaveAndExit();
}

static void Signal_Stop(int sig = 0) noexcept // Stop request (SIGTERM, e.g. by a service manager)
{
    // Orderly shutdown, like the SHUTDOWN command or a Windows service stop: the threads end on their own when they see
    //  the exit flag, then Sphere_ExitServer runs (f_onserver_exit, sockets and world closing). The reason is logged at the exit.
    g_Serv.SetExitFlag(sig ? sig : SIGTERM);
}

static void Signal_Fatal(int sig = 0) noexcept // SIGQUIT, SIGABRT, SIGILL
{
    // The process can't go on: end it with the default action of the signal, which dumps the core with the stack of the
    //  thread that got the signal.
    // Don't log through g_Log or end the threads from here: this thread could be holding a lock they need, and it would hang
    //  instead of ending. A thread can't be ended safely from a signal handler either (see AbstractThread::terminate).
    switch (sig)
    {
        case SIGQUIT:   Signal_WriteStderr("FATAL: SIGQUIT received, ending the server with a core dump.\n");        break;
        case SIGABRT:   Signal_WriteStderr("FATAL: SIGABRT received (abort), ending the server with a core dump.\n"); break;
        case SIGILL:    Signal_WriteStderr("FATAL: SIGILL received (illegal instruction), ending the server with a core dump.\n"); break;
        default:        Signal_WriteStderr("FATAL: fatal signal received, ending the server with a core dump.\n");   break;
    }

    // The signal is blocked while its handler runs: the raised one is delivered, with its default action, when this returns.
    // A SIGILL caused by an instruction gets raised again by the instruction itself, now with the default action.
    signal(sig, SIG_DFL);
    raise(sig);
}

static void Signal_Break(int sig = 0) noexcept		// signal handler attached when using secure mode
{
    UnreferencedParameter(sig);
    Signal_WriteStderr("NOTE: Secure mode prevents CTRL+C.\n");
}

static void Signal_Illegal_Instruction(int sig = 0)
{
#ifdef THREAD_TRACK_CALLSTACK
    StackDebugInformation::freezeCallStack(true);
#endif

    g_Log.Event(LOGL_FATAL, "%s\n", strsignal(sig));
#ifdef THREAD_TRACK_CALLSTACK
    StackDebugInformation::printStackTrace();
#endif

    if (sig)
    {
        sigset_t set;
        signal(sig, &Signal_Illegal_Instruction);
        sigemptyset(&set);
        sigaddset(&set, sig);
        //sigprocmask(SIG_UNBLOCK, &set, nullptr);
        pthread_sigmask(SIG_UNBLOCK, &set, nullptr);
    }

#ifdef THREAD_TRACK_CALLSTACK
    StackDebugInformation::freezeCallStack(false);
#endif
    throw CSError(LOGL_FATAL, sig, strsignal(sig));
}

static void Signal_Children(int sig = 0)
{
    UnreferencedParameter(sig);
    while (waitpid((pid_t)(-1), nullptr, WNOHANG) > 0) {}
}
#endif

#ifndef _WIN32
void SetUnixSignals( bool fSet )    // Signal handlers are installed only in secure mode
{

#ifdef _SANITIZERS
    constexpr bool fSan = true;
#else
    constexpr bool fSan = false;
#endif

#ifdef _DEBUG
    const bool fDebugger = IsDebuggerPresent();
#else
    constexpr bool fDebugger = false;
#endif

    if (!fDebugger && !fSan)
    {
        signal( SIGHUP,  fSet ? &Signal_Hangup : SIG_DFL );
        signal( SIGTERM, fSet ? &Signal_Stop : SIG_DFL );
        signal( SIGQUIT, fSet ? &Signal_Fatal : SIG_DFL );
        signal( SIGABRT, fSet ? &Signal_Fatal : SIG_DFL );
        signal( SIGILL,  fSet ? &Signal_Fatal : SIG_DFL );
        signal( SIGINT,  fSet ? &Signal_Break : SIG_DFL );
        signal( SIGSEGV, fSet ? &Signal_Illegal_Instruction : SIG_DFL );
        signal( SIGFPE,  fSet ? &Signal_Illegal_Instruction : SIG_DFL );
    }

	signal( SIGPIPE,	fSet ? SIG_IGN : SIG_DFL );
	signal( SIGCHLD,	fSet ? &Signal_Children : SIG_DFL );

}
#endif
