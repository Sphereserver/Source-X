/**
* @file CScriptContexts.h
*
*/

#ifndef _INC_CSCRIPTCONTEXTS_H
#define _INC_CSCRIPTCONTEXTS_H

#include "basic_threading.h"

class CResourceLock;
class CScript;
class CScriptObj;


struct CScriptLineContext
{
    int m_iOffset;
    int m_iLineNum;		// for debug purposes if there is an error.

    void Init();
    bool IsValid() const;
    CScriptLineContext();
};

class CScriptFileContext
{
    MT_CMUTEX_DEF;
    friend class CResourceLock;
    // Track a temporary context into a script.
    // NOTE: This should ONLY be stack based !

private:
    bool m_fOpenScript;	// nullptr context may be legit.
    const CScript * m_pPrvScriptContext;	// previous general context before this was opened.

private:
    void _Init()
    {
        m_fOpenScript = false;
    }

public:
    static const char *m_sClassName;

private:    void _OpenScript( const CScript * pScriptContext );
public:     void OpenScript( const CScript * pScriptContext );
private:	void _Close();
public:     void Close();
public:
    CScriptFileContext(): m_pPrvScriptContext(nullptr)
    {
        _Init();
    }
    explicit CScriptFileContext(const CScript * pScriptContext)
    {
        _Init();
        _OpenScript(pScriptContext);
    }
    ~CScriptFileContext()
    {
        _Close();
    }

private:
    CScriptFileContext(const CScriptFileContext& copy);
    CScriptFileContext& operator=(const CScriptFileContext& other);
};

class CScriptObjectContext
{
    // Track a temporary context of an object.
    // NOTE: This should ONLY be stack based !

private:
    bool m_fOpenObject;	// nullptr context may be legit.
    const CScriptObj * m_pPrvObjectContext;	// previous general context before this was opened.

private:
    void Init()
    {
        m_fOpenObject = false;
    }

public:
    static const char *m_sClassName;
    void OpenObject( const CScriptObj * pObjectContext );
    void Close();
    CScriptObjectContext() : m_pPrvObjectContext(nullptr)
    {
        Init();
    }
    explicit CScriptObjectContext(const CScriptObj * pObjectContext)
    {
        Init();
        OpenObject(pObjectContext);
    }
    ~CScriptObjectContext()
    {
        Close();
    }

private:
    CScriptObjectContext(const CScriptObjectContext& copy);
    CScriptObjectContext& operator=(const CScriptObjectContext& other);
};


#endif // _INC_CSCRIPTCONTEXTS_H
