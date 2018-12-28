#ifndef _INC_CCMULTIMOVABLE_H
#define _INC_CCMULTIMOVABLE_H

#include "CCTimedObject.h"
#include "../CPointBase.h"
#include "../CRegion.h"


static const DIR_TYPE sm_FaceDir[] =
{
    DIR_N,
    DIR_E,
    DIR_S,
    DIR_W
};

struct ShipSpeed // speed of a ship
{
    ushort period;	// time between movement in tenths of second
    uchar tiles;	// distance to move in tiles
};

class CCMultiMovable
{
    CItem *_pLink;
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

    CTextConsole *_pCaptain;
    bool _fCanTurn;

public:
    CCMultiMovable(bool fCanTurn);
    virtual ~CCMultiMovable() = default;
    CItem *GetLink() const;

    bool OnTick();
    bool r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc);
    bool r_LoadVal(CScript & s);
    bool r_Verb(CScript & s, CTextConsole * pSrc); // Execute command from script

protected:
    ShipSpeed m_shipSpeed;  // Speed of ships (IT_SHIP)
    ShipMovementSpeed _eSpeedMode;       // (0x01 = one tile, 0x02 = rowboat, 0x03 = slow, 0x04 = fast)

    void SetNextMove();
    size_t ListObjs(CObjBase ** ppObjList);
    bool CanMoveTo(const CPointMap & pt) const;
    bool MoveDelta(CPointBase pdelta);
    bool MoveToRegion(CRegionWorld *pRegionOld, CRegionWorld* pRegionNew) const;
    bool OnMoveTick();

    void SetCaptain(CTextConsole *pSrc);
    CTextConsole* GetCaptain();
public:
    int GetFaceOffset() const;
    bool SetMoveDir(DIR_TYPE dir, ShipMovementType eMovementType = SMT_STOP, bool fWheelMove = false);
    bool Face(DIR_TYPE dir);
    bool Move(DIR_TYPE dir, int distance);
    void Stop();
};

#endif  //_INC_CCMULTIMOVABLE_H