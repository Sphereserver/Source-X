/**
* @file CWindow.h
* @brief Base window class for controls.
*/

#pragma once
#ifndef CSCRIPTOBJ_H
#define CSCRIPTOBJ_H

#include "CArray.h"
#include "CScript.h"
#include "../sphere/threads.h"
#include "CVarDefMap.h"
#include "CVarFloat.h"
#include "CTextConsole.h"


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

#define SKIP_SEPARATORS(p)	while ( *(p)=='.' ) { (p)++; }	// || ISWHITESPACE(*(p))
#define SKIP_ARGSEP(p)	while ( *(p)== ',' || IsSpace(*p) ){ (p)++; }
#define SKIP_IDENTIFIERSTRING(p) while ( _ISCSYM(*p) ){ (p)++; }

	static LPCTSTR const sm_szScriptKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

private:
	TRIGRET_TYPE OnTriggerForLoop( CScript &s, int iType, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult );
public:
	static const char *m_sClassName;
	TRIGRET_TYPE OnTriggerScript( CScript &s, LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL );
	virtual TRIGRET_TYPE OnTrigger( LPCTSTR pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL );
	bool OnTriggerFind( CScript & s, LPCTSTR pszTrigName );
	TRIGRET_TYPE OnTriggerRun( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pReturn );
	TRIGRET_TYPE OnTriggerRunVal( CScript &s, TRIGRUN_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

	virtual LPCTSTR GetName() const = 0;	// ( every object must have at least a type name )

	// Flags = 1 = html
	size_t ParseText( TCHAR * pszResponse, CTextConsole * pSrc, int iFlags = 0, CScriptTriggerArgs * pArgs = NULL );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	bool r_Call( LPCTSTR pszFunction, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * psVal = NULL, TRIGRET_TYPE * piRet = NULL );

	bool r_SetVal( LPCTSTR pszKey, LPCTSTR pszVal );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Load( CScript & s );

public:
	CScriptObj() { };
	virtual ~CScriptObj() { };

private:
	CScriptObj(const CScriptObj& copy);
	CScriptObj& operator=(const CScriptObj& other);
};

class CScriptTriggerArgs : public CScriptObj
{
	// All the args an event will need.
	static LPCTSTR const sm_szLoadKeys[];
public:
	static const char *m_sClassName;
	INT64							m_iN1;		// "ARGN" or "ARGN1" = a modifying numeric arg to the current trigger.
	INT64							m_iN2;		// "ARGN2" = a modifying numeric arg to the current trigger.
	INT64							m_iN3;		// "ARGN3" = a modifying numeric arg to the current trigger.

	CScriptObj *				m_pO1;		// "ARGO" or "ARGO1" = object 1
											// these can go out of date ! get deleted etc.

	CGString					m_s1;		// ""ARGS" or "ARGS1" = string 1
	CGString					m_s1_raw;	// RAW, used to build argv in runtime

	CGPtrTypeArray	<LPCTSTR>	m_v;

	CVarDefMap 					m_VarsLocal;// "LOCAL.x" = local variable x
	CVarFloat					m_VarsFloat;// "FLOAT.x" = float local variable x
	CLocalObjMap				m_VarObjs;	// "REFx" = local object x

public:

	CScriptTriggerArgs() :
		m_iN1(0),  m_iN2(0), m_iN3(0)
	{
		m_pO1 = NULL;
	}

	explicit CScriptTriggerArgs( LPCTSTR pszStr );

	explicit CScriptTriggerArgs( CScriptObj * pObj ) :
		m_iN1(0),  m_iN2(0), m_iN3(0), m_pO1(pObj)
	{
	}

	explicit CScriptTriggerArgs( INT64 iVal1 ) :
		m_iN1(iVal1),  m_iN2(0), m_iN3(0)
	{
		m_pO1 = NULL;
	}
	CScriptTriggerArgs( INT64 iVal1, INT64 iVal2, INT64 iVal3 = 0 ) :
		m_iN1(iVal1), m_iN2(iVal2), m_iN3(iVal3)
	{
		m_pO1 = NULL;
	}

	CScriptTriggerArgs( INT64 iVal1, INT64 iVal2, CScriptObj * pObj ) :
		m_iN1(iVal1), m_iN2(iVal2), m_iN3(0), m_pO1(pObj)
	{
	}

	virtual ~CScriptTriggerArgs()
	{
	};

private:
	CScriptTriggerArgs(const CScriptTriggerArgs& copy);
	CScriptTriggerArgs& operator=(const CScriptTriggerArgs& other);

public:
	void getArgNs( INT64 *iVar1 = NULL, INT64 *iVar2 = NULL, INT64 *iVar3 = NULL) //Puts the ARGN's into the specified variables
	{
		if (iVar1)
			*iVar1 = this->m_iN1;

		if (iVar2)
			*iVar2 = this->m_iN2;

		if (iVar3)
			*iVar3 = this->m_iN3;
	}
		
	void Init( LPCTSTR pszStr );
	bool r_Verb( CScript & s, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s );
	bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
	bool r_Copy( CTextConsole * pSrc );
	LPCTSTR GetName() const
	{
		return "ARG";
	}
};

class CFileObj : public CScriptObj
{
	private:
			CFileText * sWrite;
			bool bAppend;
			bool bCreate;
			bool bRead;
			bool bWrite;
			TCHAR * tBuffer;
			CGString * cgWriteBuffer;
			// ----------- //
			static LPCTSTR const sm_szLoadKeys[];
			static LPCTSTR const sm_szVerbKeys[];

	private:
			void SetDefaultMode(void);
			bool FileOpen( LPCTSTR sPath );
			TCHAR * GetReadBuffer(bool);
			CGString * GetWriteBuffer(void);

	public:
			static const char *m_sClassName;
			CFileObj();
			~CFileObj();

	private:
			CFileObj(const CFileObj& copy);
			CFileObj& operator=(const CFileObj& other);

	public:
			bool OnTick();
			int FixWeirdness();
			bool IsInUse();
			void FlushAndClose();

			virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
			virtual bool r_LoadVal( CScript & s );
			virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
			virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

			LPCTSTR GetName() const
			{
				return "FILE_OBJ";
			}
};

class CFileObjContainer : public CScriptObj
{
	private:
		std::vector<CFileObj *> sFileList;
		int iFilenumber;
		int iGlobalTimeout;
		int iCurrentTick;
		// ----------- //
		static LPCTSTR const sm_szLoadKeys[];
		static LPCTSTR const sm_szVerbKeys[];

	private:
		void ResizeContainer( size_t iNewRange );

	public:
		static const char *m_sClassName;
		CFileObjContainer();
		~CFileObjContainer();

	private:
		CFileObjContainer(const CFileObjContainer& copy);
		CFileObjContainer& operator=(const CFileObjContainer& other);

	public:
		int GetFilenumber(void);
		void SetFilenumber(int);

	public:
		bool OnTick();
		int FixWeirdness();

		virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
		virtual bool r_LoadVal( CScript & s );
		virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
		virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

		LPCTSTR GetName() const
		{
			return "FILE_OBJCONTAINER";
		}
};

#endif	// CSCRIPTOBJ_H
