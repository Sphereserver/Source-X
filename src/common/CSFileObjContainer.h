/**
* @file CSFileObjContainer.h
*/

#ifndef _INC_CSFILEOBJCONTAINER_H
#define _INC_CSFILEOBJCONTAINER_H

#include "CScriptObj.h"
#include <vector>

class CSFileObj;


class CSFileObjContainer : public CScriptObj
{
private:
    std::vector<CSFileObj *> sFileList;
    int iFilenumber;
    int64 iGlobalTimeout;   // in ticks
    int iCurrentTick;
    // ----------- //
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

private:
    void ResizeContainer( size_t iNewRange );

public:
    static const char *m_sClassName;
    CSFileObjContainer();
    ~CSFileObjContainer();

private:
    CSFileObjContainer(const CSFileObjContainer& copy);
    CSFileObjContainer& operator=(const CSFileObjContainer& other);

public:
    int GetFilenumber(void);
    void SetFilenumber(int);

public:
    bool _OnTick();
    int FixWeirdness();

    virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

    virtual lpctstr GetName() const override
    {
        return "FILE_OBJCONTAINER";
    }
};

#endif // _INC_CSFILEOBJCONTAINER_H
