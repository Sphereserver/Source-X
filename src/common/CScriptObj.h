/**
* @file CScriptObj.h
* @brief A scriptable object
*/

#ifndef _INC_CSCRIPTOBJ_H
#define _INC_CSCRIPTOBJ_H

#include "../common/common.h"

class CScript;
class CTextConsole;
class CSFileText;
class CSString;
class CChar;
class CScriptTriggerArgs;


enum TRIGRUN_TYPE
{
	TRIGRUN_SECTION_EXEC,	// Just execute this. (first line already read!)
	TRIGRUN_SECTION_TRUE,	// execute this section normally.
	TRIGRUN_SECTION_FALSE,	// Just ignore this whole section.
	TRIGRUN_SINGLE_EXEC,	// Execute just this line or blocked segment. (first line already read!)
	TRIGRUN_SINGLE_TRUE,	// Execute just this line or blocked segment.
	TRIGRUN_SINGLE_FALSE	// ignore just this line or blocked segment.
};

enum TRIGRET_TYPE	// trigger script returns.
{
	TRIGRET_RET_FALSE = 0,	// default return. (script might not have been handled)
	TRIGRET_RET_TRUE = 1,
	TRIGRET_RET_DEFAULT,	// we just came to the end of the script.
	TRIGRET_ENDIF,
	TRIGRET_ELSE,
	TRIGRET_ELSEIF,
	TRIGRET_RET_HALFBAKED,
	TRIGRET_BREAK,
	TRIGRET_CONTINUE,
	TRIGRET_QTY
};


class CScriptObj
{
	// This object can be scripted. (but might not be)

	static lpctstr const sm_szScriptKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

private:
	TRIGRET_TYPE OnTriggerForLoop( CScript &s, int iType, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult );

public:
	static const char *m_sClassName;

	virtual TRIGRET_TYPE OnTrigger( lpctstr pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = nullptr );
	bool OnTriggerFind( CScript & s, lpctstr pszTrigName );
	TRIGRET_TYPE OnTriggerScript(CScript& s, lpctstr pszTrigName, CTextConsole* pSrc, CScriptTriggerArgs* pArgs = nullptr);
	TRIGRET_TYPE OnTriggerRun( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pReturn );
	TRIGRET_TYPE OnTriggerRunVal( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

	virtual lpctstr GetName() const = 0;	// ( every object must have at least a type name )

	static bool IsValidRef(const CScriptObj* pRef);

	// Flags = 1 = html
	size_t ParseText( tchar * pszResponse, CTextConsole * pSrc, int iFlags = 0, CScriptTriggerArgs * pArgs = nullptr );

	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

    static size_t r_GetFunctionIndex(lpctstr pszFunction);
    static bool r_CanCall(size_t uiFunctionIndex);
	bool r_Call( lpctstr pszFunction, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * psVal = nullptr, TRIGRET_TYPE * piRet = nullptr ); // Try to execute function
    bool r_Call( size_t uiFunctionIndex, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * psVal = nullptr, TRIGRET_TYPE * piRet = nullptr ); // Try to execute function

	bool r_SetVal( lpctstr ptcKey, lpctstr pszVal );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Load( CScript & s );

public:
	CScriptObj() = default;
	virtual ~CScriptObj() = default;

private:
	CScriptObj(const CScriptObj& copy);
	CScriptObj& operator=(const CScriptObj& other);
};

#endif	// _INC_CSCRIPTOBJ_H
