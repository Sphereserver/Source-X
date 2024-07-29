/**
* @file CScriptContexts.cpp
*
*/

#include "../sphere/threads.h"
#include "CLog.h"
#include "CScriptContexts.h"


//***************************************************************************
//	CScriptLineContext

void CScriptLineContext::Init()
{
    m_iOffset = -1;
    m_iLineNum = -1;
}

bool CScriptLineContext::IsValid() const
{
    return (m_iOffset != -1);
}

CScriptLineContext::CScriptLineContext()
{
    Init();
}

//***************************************************************************
//	CScriptFileContext

void CScriptFileContext::_OpenScript( const CScript * pScriptContext )
{
    ADDTOCALLSTACK("CScriptFileContext::_OpenScript");
    _Close();
    m_fOpenScript = true;
    m_pPrvScriptContext = g_Log.SetScriptContext( pScriptContext );
}
void CScriptFileContext::OpenScript( const CScript * pScriptContext )
{
    ADDTOCALLSTACK("CScriptFileContext::OpenScript");
    MT_UNIQUE_LOCK_SET;
    _OpenScript(pScriptContext);
}

void CScriptFileContext::Close()
{
    ADDTOCALLSTACK("CScriptFileContext::Close");
    if ( m_fOpenScript )
    {
        m_fOpenScript = false;
        g_Log.SetScriptContext( m_pPrvScriptContext );
    }
}
void CScriptFileContext::_Close()
{
    ADDTOCALLSTACK("CScriptFileContext::_Close");
    MT_UNIQUE_LOCK_SET;
    Close();
}


//***************************************************************************
//	CScriptObjectContext

void CScriptObjectContext::OpenObject( const CScriptObj * pObjectContext )
{
    ADDTOCALLSTACK("CScriptObjectContext::OpenObject");
    Close();
    m_fOpenObject = true;
    m_pPrvObjectContext = g_Log.SetObjectContext( pObjectContext );
}

void CScriptObjectContext::Close()
{
    ADDTOCALLSTACK("CScriptObjectContext::Close");
    if ( m_fOpenObject )
    {
        m_fOpenObject = false;
        g_Log.SetObjectContext( m_pPrvObjectContext );
    }
}

