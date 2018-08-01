/**
* @file CSFileObj.h
* @brief A scriptable object
*/

#ifndef _INC_CSFILEOBJ_H
#define _INC_CSFILEOBJ_H

#include "CScriptObj.h"

class CSFileText;


class CSFileObj : public CScriptObj
{
private:
    CSFileText * sWrite;
    bool bAppend;
    bool bCreate;
    bool bRead;
    bool bWrite;
    tchar * tBuffer;
    CSString * cgWriteBuffer;
    // ----------- //
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

private:
    void SetDefaultMode(void);
    bool FileOpen( lpctstr sPath );
    tchar * GetReadBuffer(bool);
    CSString * GetWriteBuffer(void);

public:
    static const char *m_sClassName;
    CSFileObj();
    ~CSFileObj();

private:
    CSFileObj(const CSFileObj& copy);
    CSFileObj& operator=(const CSFileObj& other);

public:
    bool OnTick();
    int FixWeirdness();
    bool IsInUse();
    void FlushAndClose();

    virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
    virtual bool r_LoadVal( CScript & s );
    virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc );
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

    lpctstr GetName() const
    {
        return "FILE_OBJ";
    }
};


#endif // _INC_CSFILEOBJ_H
