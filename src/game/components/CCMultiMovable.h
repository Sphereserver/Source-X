#ifndef _INC_CCMULTIMOVABLE_H
#define _INC_CCMULTIMOVABLE_H

#include "../CRegion.h"


struct ShipSpeed // speed of a ship
{
    ushort period;	// time between movement in tenths of second
    uchar tiles;	// distance to move in tiles
};

class CCMultiMovable
{
    //CItem *_pLink;
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];

    CTextConsole *_pCaptain;
    bool _fCanTurn;

protected:
    ShipSpeed _shipSpeed;          // Speed of ships (IT_SHIP)
    ShipMovementSpeed _eSpeedMode;  // (0x01 = one tile, 0x02 = rowboat, 0x03 = slow, 0x04 = fast)

public:
    CCMultiMovable(bool fCanTurn);
    virtual ~CCMultiMovable() = default;
    //CItem *GetLink() const;

    bool _OnTick();
    bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc);
    bool r_LoadVal(CScript & s);
    bool r_Verb(CScript & s, CTextConsole * pSrc); // Execute command from script

protected:
    void SetNextMove();
    uint ListObjs(CObjBase ** ppObjList);
    bool CanMoveTo(const CPointMap & pt) const;
    bool MoveDelta(const CPointMap& ptDelta, bool fUpdateViewFull);
    bool MoveToRegion(CRegionWorld *pRegionOld, CRegionWorld* pRegionNew);
    bool OnMoveTick();

    void SetCaptain(CTextConsole *pSrc);
    CTextConsole* GetCaptain();

public:
    int GetFaceOffset() const;
    bool SetMoveDir(DIR_TYPE dir, ShipMovementType eMovementType = SMT_STOP, bool fWheelMove = false);
    bool Face(DIR_TYPE dir);
    bool Move(DIR_TYPE dir, int distance);
    void Stop();
	void SetPilot(CChar *pChar);
};

#endif  //_INC_CCMULTIMOVABLE_H