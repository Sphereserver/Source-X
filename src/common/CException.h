/*
* @file CException.h
* @brief Management of exceptions.
*/

#pragma once
#ifndef _INC_CEXCEPTION_H
#define _INC_CEXCEPTION_H

#include <stack>
#include "../game/CLog.h"
#include "../sphere/threads.h"


// -------------------------------------------------------------------
// -------------------------------------------------------------------

extern "C"
{
	extern void globalstartsymbol();
	extern void globalendsymbol();
	extern const int globalstartdata;
	extern const int globalenddata;
}

void SetExceptionTranslator();
void SetUnixSignals( bool );

// -------------------------------------------------------------------
// -------------------------------------------------------------------

class CSphereError
{
	// we can throw this structure to produce an error.
	// similar to CFileException and CException
public:
	LOGL_TYPE m_eSeverity;	// const
	dword m_hError;	// HRESULT S_OK, "winerror.h" code. 0x20000000 = start of custom codes.
	lpctstr m_pszDescription;
public:
	CSphereError( LOGL_TYPE eSev, dword hErr, lpctstr pszDescription );
	CSphereError( const CSphereError& e );	// copy contstructor needed.
	virtual ~CSphereError();
public:
	CSphereError& operator=(const CSphereError& other);
public:
#ifdef _WINDOWS
	static int GetSystemErrorMessage( dword dwError, lptstr lpszError, uint nMaxError );
#endif
	virtual bool GetErrorMessage( lptstr lpszError, uint nMaxError,	uint * pnHelpContext = NULL ) const;
};

class CAssert : public CSphereError
{
protected:
	lpctstr const m_pExp;
	lpctstr const m_pFile;
	const long m_lLine;
public:
	/*
	lpctstr const GetAssertFile();
	const unsigned GetAssertLine();
	*/
	static const char *m_sClassName;
	CAssert(LOGL_TYPE eSeverity, lpctstr pExp, lpctstr pFile, long lLine);
	virtual ~CAssert();
private:
	CAssert& operator=(const CAssert& other);

public:
	virtual bool GetErrorMessage(lptstr lpszError, uint nMaxError, uint * pnHelpContext = NULL ) const;
};

#ifdef _WINDOWS
	// Catch and get details on the system exceptions.
	class CException : public CSphereError
	{
	public:
		static const char *m_sClassName;
		const dword m_dwAddress;

		CException(uint uCode, dword dwAddress);
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

#include "../game/CLog.h"
#include "../sphere/ProfileTask.h"
	#define EXC_TRY(a) \
		lpctstr inLocalBlock = ""; \
		lpctstr inLocalArgs = a; \
		uint inLocalBlockCnt(0); \
		bool bCATCHExcept = false; \
		UNREFERENCED_PARAMETER(bCATCHExcept); \
		try \
		{

	#define EXC_SET(a) inLocalBlock = a; inLocalBlockCnt++

	#ifdef THREAD_TRACK_CALLSTACK
		#define EXC_CATCH_EXCEPTION(a) \
			bCATCHExcept = true; \
			StackDebugInformation::printStackTrace(); \
			if ( inLocalBlock != NULL && inLocalBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, \
															inLocalBlockCnt, inLocalBlock); \
			else \
				g_Log.CatchEvent(a, "%s::%s()", m_sClassName, inLocalArgs)
	#else
		#define EXC_CATCH_EXCEPTION(a) \
			bCATCHExcept = true; \
			if ( inLocalBlock != NULL && inLocalBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "%s::%s() #%u \"%s\"", m_sClassName, inLocalArgs, \
															inLocalBlockCnt, inLocalBlock); \
			else \
				g_Log.CatchEvent(a, "%s::%s()", m_sClassName, inLocalArgs)
	#endif

	#define EXC_CATCH	}	\
		catch ( const CSphereError& e )	{ EXC_CATCH_EXCEPTION(&e); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); } \
		catch (...) { EXC_CATCH_EXCEPTION(NULL); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); }

	#define EXC_DEBUG_START if ( bCATCHExcept ) { try {

	#define EXC_DEBUG_END \
		/*StackDebugInformation::printStackTrace();*/ \
	} catch ( ... ) { g_Log.EventError("Exception adding debug message on the exception.\n"); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); }}

	#define EXC_TRYSUB(a) \
		lpctstr inLocalSubBlock = ""; \
		lpctstr inLocalSubArgs = a; \
		uint inLocalSubBlockCnt(0); \
		bool bCATCHExceptSub = false; \
		UNREFERENCED_PARAMETER(bCATCHExceptSub); \
		try \
		{

	#define EXC_SETSUB(a) inLocalSubBlock = a; inLocalSubBlockCnt++

	#ifdef THREAD_TRACK_CALLSTACK
		#define EXC_CATCH_SUB(a,b) \
			bCATCHExceptSub = true; \
			StackDebugInformation::printStackTrace(); \
			if ( inLocalSubBlock != NULL && inLocalSubBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
															inLocalSubBlockCnt, inLocalSubBlock); \
			else \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
			//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
	#else
		#define EXC_CATCH_SUB(a,b) \
			bCATCHExceptSub = true; \
			if ( inLocalSubBlock != NULL && inLocalSubBlockCnt > 0 ) \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s() #%u \"%s\"", m_sClassName, b, inLocalSubArgs, \
															inLocalSubBlockCnt, inLocalSubBlock); \
			else \
				g_Log.CatchEvent(a, "SUB: %s::%s::%s()", m_sClassName, b, inLocalSubArgs)
			//g_Log.CatchEvent(a, "%s::%s", b, inLocalSubBlock)
	#endif

	#define EXC_CATCHSUB(a)	}	\
		catch ( const CSphereError& e )	\
						{ \
						EXC_CATCH_SUB(&e, a); \
						CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
						} \
						catch (...) \
						{ \
						EXC_CATCH_SUB(NULL, a); \
						CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); \
						}

	#define EXC_DEBUGSUB_START if ( bCATCHExceptSub ) { try {
	
	#define EXC_DEBUGSUB_END \
		/*StackDebugInformation::printStackTrace();*/ \
	} catch ( ... ) { g_Log.EventError("Exception adding debug message on the exception.\n"); CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1); }}

	#define EXC_ADD_SCRIPT		g_Log.EventDebug("command '%s' args '%s'\n", s.GetKey(), s.GetArgRaw());
	#define EXC_ADD_SCRIPTSRC	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	#define EXC_ADD_KEYRET(src)	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, (lpctstr)sVal, static_cast<void *>(src));

#else //!_EXCEPTIONS_DEBUG

	#define EXC_TRY(a) {
	#define EXC_SET(a)
	#define EXC_SETSUB(a)
	#define EXC_CATCH }
	#define EXC_TRYSUB(a) {
	#define EXC_CATCHSUB(a) }
	
	#define EXC_DEBUG_START goto ENDEXCEPTIONSDEBUGBLOCK; \
	{
	#define EXC_DEBUG_END } \
	ENDEXCEPTIONSDEBUGBLOCK:
	#define EXC_DEBUGSUB_START	goto ENDEXCEPTIONSDEBUGSUBBLOCK; \
	{
	#define EXC_DEBUGSUB_END } \
	ENDEXCEPTIONSDEBUGSUBBLOCK:
	#define EXC_ADD_SCRIPT
	#define EXC_ADD_SCRIPTSRC
	#define EXC_ADD_KEYRET(a)
	#define EXC_CATCH_EXCEPTION(a)

#endif //_EXCEPTIONS_DEBUG

#endif // _INC_CEXCEPTION_H
