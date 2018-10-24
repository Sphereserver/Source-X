/**
* @file CCItemDamageable.h
*
*/

#ifndef _INC_CCItemDamageable_H
#define _INC_CCItemDamageable_H

#include "../CComponent.h"


class CCItemDamageable : public CComponent
{
    word _iCurHits;
    word _iMaxHits;
    int64 _iTimeLastUpdate;
    bool _fNeedUpdate;
    static lpctstr const sm_szLoadKeys[];
public:
    CCItemDamageable(CObjBase *pLink);
    virtual ~CCItemDamageable();

    void SetCurHits(word iCurHits);
    void SetMaxHits(word iMaxHits);
    word GetCurHits() const;
    word GetMaxHits() const;
    void OnTickStatsUpdate();

    virtual void Delete(bool fForced = false) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_WriteVal(lpctstr pszKey, CSString & s, CTextConsole * pSrc = nullptr) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_GetRef(lpctstr & pszKey, CScriptObj * & pRef) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;
    virtual void Copy(const CComponent *target) override;
    virtual CCRET_TYPE OnTick() override;
};
#endif //_INC_CCItemDamageable_H