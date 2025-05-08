/**
* @file CResourceLock.h
*
*/

#ifndef _INC_CRESOURCELOCK_H
#define _INC_CRESOURCELOCK_H

#include "../CScript.h"
#include "../CScriptContexts.h"

class CResourceScript;


class CResourceLock : public CScript
{
    // Open a copy of a script that is already open
    // NOTE: This should ONLY be stack based !
    // preserve the previous openers offset in the script.
private:
    CResourceScript * m_pLock;
    CScriptLineContext m_PrvLockContext;		// i must return the locked file back here.

    CScriptFileContext m_PrvScriptContext;		// where was i before (context wise) opening this. (for error tracking)
    CScriptObjectContext m_PrvObjectContext;	// object context (for error tracking)
private:
    void _Init()
    {
        m_pLock = nullptr;
        m_PrvLockContext.Init();	// means the script was NOT open when we started.
    }

protected:
    virtual bool _Open(lpctstr = nullptr, uint = 0) override;
    virtual bool Open(lpctstr = nullptr, uint = 0) override;
    virtual void _Close() override;
    virtual void Close() override;
    virtual bool _ReadTextLine( bool fRemoveBlanks ) override;
    virtual bool ReadTextLine( bool fRemoveBlanks ) override;

public:
    static const char *m_sClassName;
    CResourceLock()
    {
        _Init();
    }
    ~CResourceLock()
    {
        Close();
    }

    CResourceLock(const CResourceLock& copy) = delete;
    CResourceLock& operator=(const CResourceLock& other) = delete;

public:
    int OpenLock( CResourceScript * pLock, CScriptLineContext context );
    void AttachObj( const CScriptObj * pObj );
};


#endif // _INC_CRESOURCELOCK_H
