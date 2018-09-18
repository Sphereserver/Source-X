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
    int64 m_NextMove;
    CTextConsole *m_pCaptain;


    inline int Ship_GetFaceOffset() const
    {
        return (GetID() & 3);
    }
    void Ship_SetNextMove();
    size_t Ship_ListObjs(CObjBase ** ppObjList);
    bool Ship_CanMoveTo(const CPointMap & pt) const;
    bool Ship_MoveDelta(CPointBase pdelta);
    bool Ship_MoveToRegion(CRegionWorld *pRegionOld, CRegionWorld* pRegionNew) const;
    bool Ship_OnMoveTick();

    virtual bool r_GetRef(lpctstr & pszKey, CScriptObj * & pRef);
    virtual void r_Write(CScript & s);
    virtual bool r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc);
    virtual bool r_LoadVal(CScript & s);
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc); // Execute command from script
    virtual int FixWeirdness();
    virtual void OnComponentCreate(CItem * pComponent);

public:
    bool Ship_SetMoveDir(DIR_TYPE dir, byte bMovementType = 0, bool fWheelMove = false);
    bool Ship_Face(DIR_TYPE dir);
    bool Ship_Move(DIR_TYPE dir, int distance);
    static const char *m_sClassName;
    CItemShip(ITEMID_TYPE id, CItemBase * pItemDef);
    virtual ~CItemShip();

private:
    CItemShip(const CItemShip& copy);
    CItemShip& operator=(const CItemShip& other);

public:
    virtual bool OnTick();
    void Ship_Stop();
    CItemContainer * GetShipHold();
    size_t GetShipPlankCount();
    CItem * GetShipPlank(size_t index);
    //CItemBaseMulti::ShipSpeed GetShipSpeed();
};


#endif // _INC_CITEMSHIP_H
