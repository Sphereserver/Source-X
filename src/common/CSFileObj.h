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
    CSFileText * _pFile;
    bool _fAppend;
    bool _fCreate;
    bool _fRead;
    bool _fWrite;
    tchar * _ptcReadBuffer;
    CSString * _psWriteBuffer;
    // ----------- //
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

private:
    void SetDefaultMode();
    bool FileOpen( lpctstr sPath );
    tchar * GetReadBuffer(bool fDelete = false);
    CSString * GetWriteBuffer();

public:
    static const char *m_sClassName;
    CSFileObj();
    ~CSFileObj();

private:
    CSFileObj(const CSFileObj& copy);
    CSFileObj& operator=(const CSFileObj& other);

public:
    bool _OnTick();
    int FixWeirdness();
    bool IsInUse();
    void FlushAndClose();

    virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

    lpctstr GetName() const
    {
        return "FILE_OBJ";
    }
};


#endif // _INC_CSFILEOBJ_H
