/**
* @file CException.h
* @brief Management of exceptions.
*/

#ifndef _INC_CEXCEPTION_H
#define _INC_CEXCEPTION_H

#include "../common/CLog.h"
#include <csignal>
#ifndef _WIN32
	#include <sys/wait.h>
#endif

// -------------------------------------------------------------------
// -------------------------------------------------------------------

extern "C"
{
	extern void globalstartsymbol();
	extern void globalendsymbol();
	extern const int globalstartdata;
	extern const int globalenddata;
}

void SetPurecallHandler();
void SetExceptionTranslator();
void SetUnixSignals( bool );

#if !defined(_WIN32) && defined(_DEBUG)
	int IsDebuggerPresent();	// Windows already has this function
#endif

// -------------------------------------------------------------------
// -------------------------------------------------------------------

class CSError
{
	// we can throw this structure to produce an error.
	// similar to CFileException and CException
public:
	LOG_TYPE m_eSeverity;	// const
	dword m_hError;	// HRESULT S_OK, "winerror.h" code. 0x20000000 = start of custom codes.
	lpctstr m_pszDescription;
public:
	CSError( LOG_TYPE eSev, dword hErr, lpctstr pszDescription );
	CSError( const CSError& e );	// copy contstructor needed.
	virtual ~CSError() {}
public:
	CSError& operator=(const CSError& other);
public:
#ifdef _WIN32
	static int GetSystemErrorMessage( dword dwError, lptstr lpszError, uint nMaxError );
#endif
	virtual bool GetErrorMessage( lptstr lpszError, uint nMaxError ) const;
};

class CAssert : public CSError
{
protected:
	lpctstr const m_pExp;
	lpctstr const m_pFile;
	const long long m_llLine;
public:
	static const char *m_sClassName;
	CAssert(LOG_TYPE eSeverity, lpctstr pExp, lpctstr pFile, long long llLine) :
		CSError(eSeverity, 0, "Assert"), m_pExp(pExp), m_pFile(pFile), m_llLine(llLine)
	{
	}
	virtual ~CAssert()
	{
	}

private:
	CAssert& operator=(const CAssert& other);

public:
	virtual bool GetErrorMessage(lptstr lpszError, uint uiMaxError ) const;
};

#ifdef _WIN32
	// Catch and get details on the system exceptions.
	class CException : public CSError
	{
	public:
		static const char *m_sClassName;
		const size_t m_pAddress;

		CException(uint uCode, size_t pAddress);
		virtual ~CException();
	private:
		CException& operator=(const CException& other);

	public:
		virtual bool GetErrorMessage(lptstr lpszError, uint nMaxError, uint * pnHelpContext = NULL ) const;
	};
#endif


// -------------------------------------------------------------------
// -------------------------------------------------------------------

//	Exceptions debugging routine.
#ifdef _EXCEPTIONS_DEBUG


#include "../sphere/ProfileTask.h"

/*--- Common macros ---*/

// EXC_NOTIFY_DEBUGGER
#ifndef _DEBUG
	#define EXC_NOTIFY_DEBUGGER (void)0
#else	// we want the debugger to notice of this exception
	#ifdef _WIN32
		#ifdef _MSC_VER
			#define EXC_NOTIFY_DEBUGGER if (IsDebuggerPresent()) __debugbreak()
		#else
			#define EXC_NOTIFY_DEBUGGER if (IsDebuggerPresent()) abort()
		#endif
	#else
		#define EXC_NOTIFY_DEBUGGER if (IsDebuggerPresent()) std::raise(SIGINT)
	#endif
#endif

// EXC_PRINT_STACK_TRACE
#ifdef THREAD_TRACK_CALLSTACK
    #define EXC_PRINT_STACK_TRACE StackDebugInformation::printStackTrace()
#else
    #define EXC_PRINT_STACK_TRACE (void)0
#endif

/*--- Main (non SUB) macros ---*/

// EXC_TRY
#define EXC_TRY(a) \
	lpctstr inLocalBlock = ""; \
	lpctstr inLocalArgs = a; \
	uint inLocalBlockCnt = 0; \
	bool bCATCHExcept = false; \
	UNREFERENCED_PARAMETER(bCATCHExcept); \
	try \
	{

// EXC_SET_BLOCK
#define EXC_SET_BLOCK(a)	\
	inLocalBlock = a; \
	++inLocalBlockCnt

// EXC_CATCH_EXCEPTION_SPHERE (used inside other macros! don't use it manually!)
#define EXC_CATCH_EXCEPTION_SPHERE(a) \
    bCATCHExcept = true; \
	if ( inLocalBlock != nullptr && inLocalBlockCnt > 0 ) \
		g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
	else \
		g_Log.CatchEvent(a, "%s::%s()", m_sClassName, inLocalArgs); \
	EXC_PRINT_STACK_TRACE

// EXC_CATCH_EXCEPTION_STD (used inside other macros! don't use it manually!)
#define EXC_CATCH_EXCEPTION_STD(a) \
    bCATCHExcept = true; \
	if ( inLocalBlock != nullptr && inLocalBlockCnt > 0 ) \
		g_Log.CatchStdException(a, "std::exception in %s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
	else \
		g_Log.CatchStdException(a, "std::exception in %s::%s()", m_sClassName, inLocalArgs); \
	EXC_PRINT_STACK_TRACE

// EXC_CATCH
#define EXC_CATCH \
	} \
	catch ( const CAssert& e ) \
	{ \
		EXC_CATCH_EXCEPTION_SPHERE(&e); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
	} \
	catch ( const CSError& e ) \
	{ \
		EXC_CATCH_EXCEPTION_SPHERE(&e); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
    catch ( const std::exception& e ) \
	{ \
		EXC_CATCH_EXCEPTION_STD(&e); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
	catch (...) \
	{ \
		EXC_CATCH_EXCEPTION_SPHERE(nullptr); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	}

// EXC_DEBUG_START
#define EXC_DEBUG_START \
	if ( bCATCHExcept ) \
	{ \
		try \
		{

#define EXC_DEBUG_END \
			/*EXC_PRINT_STACK_TRACE;*/ \
		} \
		catch ( ... ) \
		{ \
			g_Log.EventError("Exception adding debug message on the exception.\n"); \
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		} \
	}

/*--- SUB macros ---*/

// EXC_TRYSUB
#define EXC_TRYSUB(a) \
	lpctstr inLocalSubBlock = ""; \
	lpctstr inLocalSubArgs = a; \
	uint inLocalSubBlockCnt = 0; \
	bool bCATCHExceptSub = false; \
	UNREFERENCED_PARAMETER(bCATCHExceptSub); \
	try \
	{

// EXC_SETSUB_BLOCK
#define EXC_SETSUB_BLOCK(a) \
	inLocalSubBlock = a; \
	++inLocalSubBlockCnt

// EXC_CATCH_SUB_EXCEPTION_SPHERE(a,b) (used inside other macros! don't use it manually!)
#define EXC_CATCH_SUB_EXCEPTION_SPHERE(a,b) \
    bCATCHExceptSub = true; \
    EXC_PRINT_STACK_TRACE; \
    if ( inLocalSubBlock != nullptr && inLocalSubBlockCnt > 0 ) \
        g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
											                inLocalSubBlockCnt, inLocalSubBlock); \
    else \
        g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
        //g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)

// EXC_CATCH_SUB_EXCEPTION_STD(a,b) (used inside other macros! don't use it manually!)
#define EXC_CATCH_SUB_EXCEPTION_STD(a,b) \
    bCATCHExceptSub = true; \
    EXC_PRINT_STACK_TRACE; \
    if ( inLocalSubBlock != nullptr && inLocalSubBlockCnt > 0 ) \
        g_Log.CatchStdException(a, "std::exception in SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
														                            inLocalSubBlockCnt, inLocalSubBlock); \
    else \
        g_Log.CatchStdException(a, "std::exception in SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
        //g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)

// EXC_CATCHSUB(a)
#define EXC_CATCHSUB(a)	\
	} \
    catch ( const CAssert& e ) \
	{ \
		EXC_CATCH_SUB_EXCEPTION_SPHERE(&e, a); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
	} \
	catch ( const CSError& e )	\
	{ \
		EXC_CATCH_SUB_EXCEPTION_SPHERE(&e, a); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
        EXC_NOTIFY_DEBUGGER; \
	} \
    catch ( const std::exception& e ) \
	{ \
		EXC_CATCH_SUB_EXCEPTION_STD(&e, a); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
	catch (...) \
	{ \
		EXC_CATCH_SUB_EXCEPTION_SPHERE(nullptr, a); \
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
        EXC_NOTIFY_DEBUGGER; \
	}

// EXC_DEBUGSUB_START
#define EXC_DEBUGSUB_START \
	if ( bCATCHExceptSub ) \
	{ \
		try \
		{

// EXC_DEBUGSUB_END
#define EXC_DEBUGSUB_END \
			/*EXC_PRINT_STACK_TRACE;*/ \
		} \
		catch ( ... ) \
		{ \
			g_Log.EventError("Exception adding debug message on the exception.\n"); \
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
		} \
	}

/*--- Keywords debugging ---*/

#define EXC_ADD_SCRIPT		g_Log.EventDebug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw());
#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<lpctstr>(sVal), static_cast<void *>(src));


#else //!_EXCEPTIONS_DEBUG


	#define EXC_TRY(a) {
	#define EXC_SET_BLOCK(a)
	#define EXC_SETSUB_BLOCK(a)
	#define EXC_CATCH }
	#define EXC_TRYSUB(a) {
	#define EXC_CATCHSUB(a) }

	#define EXC_DEBUG_START if (false) \
	{
	#define EXC_DEBUG_END }

	#define EXC_DEBUGSUB_START	if (false) \
	{
	#define EXC_DEBUGSUB_END }

    #define EXC_NOTIFY_DEBUGGER (void)0
	#define EXC_ADD_SCRIPT		(void)0
	#define EXC_ADD_SCRIPTSRC	(void)0
	#define EXC_ADD_KEYRET(a)	(void)0
	#define EXC_CATCH_EXCEPTION(a)


#endif //_EXCEPTIONS_DEBUG


#endif // _INC_CEXCEPTION_H
