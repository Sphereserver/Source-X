/**
* @file CItemShip.h
*
*/

#ifndef _INC_CITEMSHIP_H
#define _INC_CITEMSHIP_H

#include "CItemMulti.h"


class CItemShip : public CItemMulti
{
// IT_SHIP
// A ship
private:
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

    CUID m_uidHold;
    std::vector<CUID> m_uidPlanks;

    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override; // Execute command from script
    virtual int FixWeirdness() override;

    void OnComponentCreate(CItem * pComponent);

public:
    static const char *m_sClassName;
    CItemShip(ITEMID_TYPE id, CItemBase * pItemDef);
    virtual ~CItemShip();

    CItemShip(const CItemShip& copy) = delete;
    CItemShip& operator=(const CItemShip& other) = delete;

public:
    virtual bool _OnTick() override;
    CItemContainer * GetShipHold();
    size_t GetShipPlankCount();
    CItem * GetShipPlank(size_t index);
    //CItemBaseMulti::ShipSpeed GetShipSpeed();
};


#endif // _INC_CITEMSHIP_H
