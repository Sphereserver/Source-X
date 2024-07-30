/**
* @file CException.h
* @brief Management of exceptions.
*/

#ifndef _INC_CEXCEPTION_H
#define _INC_CEXCEPTION_H

#include "../sphere/threads.h"
#include "CLog.h"

// -------------------------------------------------------------------
// -------------------------------------------------------------------

extern "C"
{
	extern void globalstartsymbol();
}

void SetPurecallHandler();
void SetExceptionTranslator();

#ifndef _WIN32
    void SetUnixSignals( bool );
    int IsDebuggerPresent();	// Windows already has this function
#endif

void NotifyDebugger();

// -------------------------------------------------------------------
// -------------------------------------------------------------------

class CSError
{
	// we can throw this structure to produce an error.
	// similar to CFileException and CException
public:
	const LOG_TYPE m_eSeverity;	// const
	const dword m_hError;	    // HRESULT S_OK, "winerror.h" code. 0x20000000 = start of custom codes.
	const lpctstr m_pszDescription;
public:
	CSError( LOG_TYPE eSev, dword hErr, lpctstr pszDescription );
	CSError( const CSError& e );	// copy contstructor needed.
    virtual ~CSError() = default;

    CSError& operator=(const CSError& other) = delete;

public:
#ifdef _WIN32
	static int GetSystemErrorMessage( dword dwError, lptstr lpszError, dword dwErrorBufLength);
#endif
	virtual bool GetErrorMessage( lptstr lpszError, uint uiMaxError ) const;
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
    virtual ~CAssert() = default;

	CAssert& operator=(const CAssert& other) = delete;

public:
	virtual bool GetErrorMessage(lptstr lpszError, uint uiMaxError ) const override;
};

#ifdef _WIN32
	// Catch and get details on the system exceptions.
	class CWinException : public CSError
	{
	public:
		static const char *m_sClassName;
		const size_t m_pAddress;

        CWinException(uint uCode, size_t pAddress);
		virtual ~CWinException();
	private:
        CWinException& operator=(const CWinException& other);

	public:
		virtual bool GetErrorMessage(lptstr lpszError, uint nMaxError) const override;
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
	#define EXC_NOTIFY_DEBUGGER     (void)0
#else
    // we want the debugger to notice of this exception
    #define EXC_NOTIFY_DEBUGGER     NotifyDebugger()
#endif

// _EXC_CAUGHT
#define _EXC_CAUGHT static_cast<AbstractSphereThread *>(ThreadHolder::get().current())->exceptionCaught()

/*--- Main (non SUB) macros ---*/

// EXC_TRY
#define EXC_TRY(a) \
	lpctstr inLocalBlock = ""; \
	lpctstr inLocalArgs = a; \
	uint inLocalBlockCnt = 0; \
	bool fCATCHExcept = false; \
	UnreferencedParameter(fCATCHExcept); \
	try \
	{

// EXC_SET_BLOCK
#define EXC_SET_BLOCK(a)	\
	inLocalBlock = a; \
	++inLocalBlockCnt

// _EXC_CATCH_EXCEPTION_GENERIC (used inside other macros! don't use it manually!)
#define _EXC_CATCH_EXCEPTION_GENERIC(a,excType) \
    fCATCHExcept = true; \
	if ( inLocalBlock != nullptr && inLocalBlockCnt > 0 ) \
		g_Log.CatchEvent(a, "ExcType=%s catched in %s::%s() #%u \"%s\"", excType, m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
	else \
		g_Log.CatchEvent(a, "ExcType=%s catched in %s::%s()", excType, m_sClassName, inLocalArgs); \
	_EXC_CAUGHT

// _EXC_CATCH_EXCEPTION_STD (used inside other macros! don't use it manually!)
#define _EXC_CATCH_EXCEPTION_STD(a) \
    fCATCHExcept = true; \
	if ( inLocalBlock != nullptr && inLocalBlockCnt > 0 ) \
		g_Log.CatchStdException(a, "ExcType=std::exception catched in %s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, inLocalBlockCnt, inLocalBlock); \
	else \
		g_Log.CatchStdException(a, "ExcType=std::exception catched in %s::%s()", m_sClassName, inLocalArgs); \
	_EXC_CAUGHT

// EXC_CATCH
#define EXC_CATCH \
	} \
	catch ( const CAssert& e ) \
	{ \
		_EXC_CATCH_EXCEPTION_GENERIC(&e, "CAssert"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
	catch ( const CSError& e ) \
	{ \
		_EXC_CATCH_EXCEPTION_GENERIC(&e, "CSError"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
    catch ( const std::exception& e ) \
	{ \
		_EXC_CATCH_EXCEPTION_STD(&e); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
	catch (...) \
	{ \
		_EXC_CATCH_EXCEPTION_GENERIC(nullptr, "pure"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	}

// EXC_DEBUG_START
#define EXC_DEBUG_START \
	if ( fCATCHExcept ) \
	{ \
		try \
		{

#define EXC_DEBUG_END \
		} \
		catch ( ... ) \
		{ \
			g_Log.EventError("Exception adding debug message on the exception.\n"); \
			GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		} \
	}

/*--- SUB macros ---*/

// EXC_TRYSUB
#define EXC_TRYSUB(a) \
	lpctstr inLocalSubBlock = ""; \
	lpctstr inLocalSubArgs = a; \
	uint inLocalSubBlockCnt = 0; \
	bool fCATCHExceptSub = false; \
	UnreferencedParameter(fCATCHExceptSub); \
	try \
	{

// EXC_SETSUB_BLOCK
#define EXC_SETSUB_BLOCK(a) \
	inLocalSubBlock = a; \
	++inLocalSubBlockCnt

// _EXC_CATCH_SUB_EXCEPTION_GENERIC(a,b, "ExceptionType") (used inside other macros! don't use it manually!)
#define _EXC_CATCH_SUB_EXCEPTION_GENERIC(a,b,excType) \
    fCATCHExceptSub = true; \
    if ( inLocalSubBlock != nullptr && inLocalSubBlockCnt > 0 ) \
        g_Log.CatchEvent(a, "ExcType=%s catched in SUB: %s::%s() #%u \"%s\" (\"%s\")", excType, m_sClassName, inLocalSubArgs, \
											                inLocalSubBlockCnt, inLocalSubBlock, b); \
    else \
        g_Log.CatchEvent(a, "ExcType=%s catched in SUB: %s::%s() (\"%s\")", excType, m_sClassName, inLocalSubArgs, b); \
    _EXC_CAUGHT

// _EXC_CATCH_SUB_EXCEPTION_STD(a,b) (used inside other macros! don't use it manually!)
#define _EXC_CATCH_SUB_EXCEPTION_STD(a,b) \
    fCATCHExceptSub = true; \
    if ( inLocalSubBlock != nullptr && inLocalSubBlockCnt > 0 ) \
        g_Log.CatchStdException(a, "ExcType=std::exception catched in SUB: %s::%s() #%u \"%s\" (\"%s\")", m_sClassName, inLocalSubArgs, \
											                inLocalSubBlockCnt, inLocalSubBlock, b); \
    else \
        g_Log.CatchStdException(a, "ExcType=std::exception catched in SUB: %s::%s() (\"%s\")", m_sClassName, inLocalSubArgs, b); \
    _EXC_CAUGHT

// EXC_CATCHSUB(a)
#define EXC_CATCHSUB(a)	\
	} \
    catch ( const CAssert& e ) \
	{ \
		_EXC_CATCH_SUB_EXCEPTION_GENERIC(&e, a, "CAssert"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
	} \
	catch ( const CSError& e )	\
	{ \
		_EXC_CATCH_SUB_EXCEPTION_GENERIC(&e, a, "CSError"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
        EXC_NOTIFY_DEBUGGER; \
	} \
    catch ( const std::exception& e ) \
	{ \
		_EXC_CATCH_SUB_EXCEPTION_STD(&e, a); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		EXC_NOTIFY_DEBUGGER; \
	} \
	catch (...) \
	{ \
		_EXC_CATCH_SUB_EXCEPTION_GENERIC(nullptr, a, "pure"); \
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
        EXC_NOTIFY_DEBUGGER; \
	}

// EXC_DEBUGSUB_START
#define EXC_DEBUGSUB_START \
	if ( fCATCHExceptSub ) \
	{ \
		try \
		{

// EXC_DEBUGSUB_END
#define EXC_DEBUGSUB_END \
		} \
		catch ( ... ) \
		{ \
			g_Log.EventError("Exception adding debug message on the exception (sub).\n"); \
			GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1); \
		} \
	}


/*--- Keywords debugging ---*/

#define EXC_ADD_SCRIPT		g_Log.EventDebug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw());
#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", ptcKey, static_cast<lpctstr>(sVal), static_cast<void *>(src));


#else //!_EXCEPTIONS_DEBUG


	#define EXC_TRY(a)          { UnreferencedParameter(a)
	#define EXC_SET_BLOCK(a)      UnreferencedParameter(a)
	#define EXC_SETSUB_BLOCK(a)   UnreferencedParameter(a)
	#define EXC_CATCH           }
	#define EXC_TRYSUB(a)       { UnreferencedParameter(a)
	#define EXC_CATCHSUB(a)       UnreferencedParameter(a); }

	#define EXC_DEBUG_START     if (false) \
	                            {
	#define EXC_DEBUG_END       }

	#define EXC_DEBUGSUB_START  if (false) \
	                            {
	#define EXC_DEBUGSUB_END    }

    #define EXC_NOTIFY_DEBUGGER     (void)0
	#define EXC_ADD_SCRIPT          (void)0
	#define EXC_ADD_SCRIPTSRC       (void)0
	#define EXC_ADD_KEYRET(a)       UnreferencedParameter(a)
	#define EXC_CATCH_EXCEPTION(a)  UnreferencedParameter(a)


#endif //_EXCEPTIONS_DEBUG


/*--- Advanced tooling --- */

//https://rkd.me.uk/posts/2020-04-11-stack-corruption-and-how-to-debug-it.html
#define STACK_RA_CHECK_SETUP void* ra_start = __builtin_return_address(0);
#define STACK_RA_CHECK assert(ra_start == __builtin_return_address(0)); assert(((intptr_t)ra_start & 0xffffffff00000000) == ((intptr_t)main & 0xffffffff00000000));


#endif // _INC_CEXCEPTION_H
