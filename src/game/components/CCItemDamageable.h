/**
* @file CCItemDamageable.h
*
*/

#ifndef _INC_CCITEMDAMAGEABLE_H
#define _INC_CCITEMDAMAGEABLE_H

#include "../CComponent.h"

class CItem;


class CCItemDamageable : public CComponent
{
    CItem *_pLink;
    static lpctstr const sm_szLoadKeys[];

    word _iCurHits;
    word _iMaxHits;
    int64 _iTimeLastUpdate;
    bool _fNeedUpdate;
    
public:
    CCItemDamageable(CItem *pLink);
    virtual ~CCItemDamageable() override = default;
    static bool CanSubscribe(const CItem* pItem);

    CItem *GetLink() const;

    void SetCurHits(word iCurHits);
    void SetMaxHits(word iMaxHits);
    word GetCurHits() const;
    word GetMaxHits() const;
    void OnTickStatsUpdate();

    virtual void Delete(bool fForced = false) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & s, CTextConsole * pSrc = nullptr) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;
    virtual void Copy(const CComponent *target) override;
    virtual CCRET_TYPE OnTickComponent() override;
};
#endif //_INC_CCITEMDAMAGEABLE_H