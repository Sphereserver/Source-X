
#include "CCMultiMovable.h"
#include "../items/CItem.h"
#include "../items/CItemMulti.h"
#include "../items/CItemShip.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../../network/send.h"
#include "../CObjBase.h"
#include "../CWorld.h"
#include "../triggers.h"

enum ShipDelay
{
    ShipDelay_Normal = 500,
    ShipDelay_Fast = 250
};

CCMultiMovable::CCMultiMovable(bool fCanTurn)
{
    _fCanTurn = fCanTurn;
}

void CCMultiMovable::SetCaptain(CTextConsole * pSrc)
{
    _pCaptain = pSrc;
}

CTextConsole * CCMultiMovable::GetCaptain()
{
    return _pCaptain;
}

int CCMultiMovable::GetFaceOffset() const
{
    const CItem* pItem = dynamic_cast<const CItem*>(this);
    ASSERT(pItem);
    return (pItem->GetID() & 3);
}

bool CCMultiMovable::SetMoveDir(DIR_TYPE dir, ShipMovementType eMovementType, bool fWheelMove)
{
    ADDTOCALLSTACK("CCMultiMovable::SetMoveDir");
    // Set the direction we will move next time we get a tick.
    // bMovementType: from the packet above -> (0 = Stop Movement, 1 = One Tile Movement, 2 = Normal Movement) ***These speeds are NOT the same as 0xF6 packet
    // Can be called from Packet 0xBF.0x33 : PacketWheelBoatMove to check if ship can move while setting dir and checking times in the proccess,
    //  otherwise for each click with mouse it will do 1 move.

    // We set new direction regardless of click limitations, so click in another direction means changing dir but makes not more moves until ship's timer moves it.
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    pItemThis->m_itShip.m_DirMove = (byte)dir;
    if (eMovementType == SMT_STOP)
    {
        Stop();
        return true;
    }

    if (fWheelMove && pItemThis->IsTimerSet())
        return false;

    if ((pItemThis->m_itShip.m_DirMove == dir) && (pItemThis->m_itShip._eMovementType != 0))
    {
        if ((pItemThis->m_itShip.m_DirFace == pItemThis->m_itShip.m_DirMove) && (pItemThis->m_itShip._eMovementType == 1))
            eMovementType = SMT_NORMAL;
        /*else
            return false; */ // no need to return, bMovementType different than 1 or 2 is handled below, so it's safe to let it pass.
    }

    if (!pItemThis->IsAttr(ATTR_MAGIC))	// make sound.
    {
        if (!Calc_GetRandVal(10))
            pItemThis->Sound(Calc_GetRandVal(2) ? 0x12 : 0x13);
    }

    pItemThis->m_itShip._eMovementType = (eMovementType >= SMT_NORMAL) ? SMT_NORMAL : eMovementType;	//checking here that packet is legit from client and not modified by 3rd party tools to send speed > 2.
    pItemThis->GetTopSector()->SetSectorWakeStatus();	        // may get here before my client does.

    // SpeedMode, as supported by the 0xF6 packet (sent from server): 0x01 = one tile, 0x02 = rowboat, 0x03 = slow, 0x04 = fast
    // TODO RowBoat's checks.
    _eSpeedMode = (eMovementType == SMT_SLOW) ? SMS_SLOW : SMS_FAST;
    SetNextMove();
    return true;
}

void CCMultiMovable::SetNextMove()
{
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    if (pItemThis->m_itShip._eMovementType == SMT_STOP)
    {
        return;
    }
    int64 iDelay;
    if (IsSetOF(OF_NoSmoothSailing))
    {
        iDelay = (_eSpeedMode == SMS_SLOW) ? (m_shipSpeed.period * MSECS_PER_TENTH) : ((m_shipSpeed.period * MSECS_PER_TENTH) / 2);
    }
    else
    {
        iDelay = (_eSpeedMode == SMS_SLOW) ? (m_shipSpeed.period * MSECS_PER_TENTH) : ((m_shipSpeed.period * MSECS_PER_TENTH) / 2);
    }
    pItemThis->SetTimeout(iDelay);
}

size_t CCMultiMovable::ListObjs(CObjBase ** ppObjList)
{
    ADDTOCALLSTACK("CCMultiMovable::ListObjs");
    // List all the objects in the structure.
    // Move the ship and everything on the deck
    // If too much stuff. then some will fall overboard. hehe.
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    CItemMulti *pMulti = static_cast<CItemMulti*>(pItemThis);
    if (!pItemThis->IsTopLevel())
        return 0;

    int iMaxDist = pMulti->Multi_GetMaxDist();
    int iShipHeight = pItemThis->GetTopZ() + maximum(3, pItemThis->GetHeight());

    // always list myself first. All other items must see my new region !
    size_t iCount = 0;
    ppObjList[iCount++] = pItemThis;

    CWorldSearch AreaChar(pItemThis->GetTopPoint(), iMaxDist);
    AreaChar.SetAllShow(true);
    AreaChar.SetSearchSquare(true);
    while (iCount < MAX_MULTI_LIST_OBJS)
    {
        CChar * pChar = AreaChar.GetChar();
        if (pChar == nullptr)
            break;
        if (!pMulti->GetRegion()->IsInside2d(pChar->GetTopPoint()))
            continue;
        if (pChar->IsDisconnected() && pChar->m_pNPC)
            continue;

        int zdiff = pChar->GetTopZ() - iShipHeight;
        if ((zdiff < -2) || (zdiff > PLAYER_HEIGHT))
            continue;

        ppObjList[iCount++] = pChar;
    }

    CWorldSearch AreaItem(pItemThis->GetTopPoint(), iMaxDist);
    AreaItem.SetSearchSquare(true);
    while (iCount < MAX_MULTI_LIST_OBJS)
    {
        CItem * pItem = AreaItem.GetItem();
        if (pItem == nullptr)
            break;
        if (pItem == pItemThis)	// already listed.
            continue;
        if (!pMulti->Multi_IsPartOf(pItem))
        {
            if (!pMulti->GetRegion()->IsInside2d(pItem->GetTopPoint()))
                continue;

            //I guess we can allow items to be locked on the ships and still move... but disallow attr_static from moving
            //if ( ! pItem->IsMovable() && !pItem->IsType(IT_CORPSE))
            if (pItem->IsAttr(ATTR_STATIC))
                continue;

            int zdiff = pItem->GetTopZ() - iShipHeight;
            if ((zdiff < -2) || (zdiff > PLAYER_HEIGHT))
                continue;
        }
        ppObjList[iCount++] = pItem;
    }
    return iCount;
}

bool CCMultiMovable::MoveDelta(const CPointMap& ptDelta)
{
    ADDTOCALLSTACK("CCMultiMovable::MoveDelta");
    // Move the ship one space in some direction.

    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    CItemMulti *pMulti = static_cast<CItemMulti*>(pItemThis);
    ASSERT(pMulti->GetRegion()->m_iLinkedSectors);

    int znew = pItemThis->GetTopZ() + ptDelta.m_z;
    if (ptDelta.m_z > 0)
    {
        if (znew >= (UO_SIZE_Z - PLAYER_HEIGHT) - 1)
            return false;
    }
    else if (ptDelta.m_z < 0)
    {
        if (znew <= (UO_SIZE_MIN_Z + 3))
            return false;
    }
    CPointMap ptTemp = pItemThis->GetTopPoint();
    CRegionWorld *pRegionOld = dynamic_cast<CRegionWorld*>(ptTemp.GetRegion(REGION_TYPE_AREA));
    ptTemp += ptDelta;
    CRegionWorld *pRegionNew = dynamic_cast<CRegionWorld*>(ptTemp.GetRegion(REGION_TYPE_AREA));
    if (!MoveToRegion(pRegionOld, pRegionNew))
    {
        return false;
    }

    // Move the ship and everything on the deck
    CObjBase * ppObjs[MAX_MULTI_LIST_OBJS + 1];
    size_t iCount = ListObjs(ppObjs);

    for (size_t i = 0; i < iCount; ++i)
    {
        CObjBase * pObj = ppObjs[i];
        if (!pObj)
            continue;
        CPointMap pt = pObj->GetTopPoint();
        pt += ptDelta;

        if (!pt.IsValidPoint())  // boat goes out of bounds !
        {
            DEBUG_ERR(("Ship uid=0%x out of bounds\n", (dword)pItemThis->GetUID()));
            continue;
        }
        pObj->MoveTo(pt);
    }

    ClientIterator it;
    for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
    {
        CChar * tMe = pClient->GetChar();
        if (tMe == nullptr)
            continue;

        byte tViewDist = (uchar)(tMe->GetVisualRange());
        for (size_t i = 0; i < iCount; ++i)
        {
            CObjBase * pObj = ppObjs[i];
            if (!pObj) continue; //no object anymore? skip!
            CPointMap pt = pObj->GetTopPoint();
            CPointMap ptOld(pt);
            ptOld -= ptDelta;

            //Remove objects that just moved out of sight
            if ((tMe->GetTopPoint().GetDistSight(pt) >= tViewDist) && (tMe->GetTopPoint().GetDistSight(ptOld) < tViewDist))
            {
                pClient->addObjectRemove(pObj);
                continue; //no need to keep going. skip!
            }

            if (pClient->CanSee(pObj))
            {
                if (pObj == pItemThis) //This is the ship (usually the first item in the list)
                {
                    if (pClient->GetNetState()->isClientVersion(MINCLIVER_HS) || pClient->GetNetState()->isClientEnhanced())
                    {
                        if (!IsSetOF(OF_NoSmoothSailing))
                        {
                            new PacketMoveShip(pClient, static_cast<CItemShip*>(pMulti), ppObjs, iCount, pItemThis->m_itShip.m_DirMove, pItemThis->m_itShip.m_DirFace, (byte)pMulti->_eSpeedMode);

                            //If client is on Ship
                            if (tMe->GetRegion()->GetResourceID().GetObjUID() == pItemThis->GetUID())
                            {
                                break; //skip to next client
                            }
                        }
                        else if (pClient->GetNetState()->isClientEnhanced())
                            pClient->addObjectRemove(pObj);	//it will be added again in the if clause below
                    }
                }
                const CPointMap& ptMe = tMe->GetTopPoint();
                const NetState* pNetState = pClient->GetNetState();
                if (pObj->IsItem())
                {
                    if ((ptMe.GetDistSight(pt) < tViewDist)
                        && ((ptMe.GetDistSight(ptOld) >= tViewDist) || !(pNetState->isClientVersion(MINCLIVER_HS) || pNetState->isClientEnhanced()) || IsSetOF(OF_NoSmoothSailing)))
                    {
                        CItem *pItem = dynamic_cast <CItem *>(pObj);
                        pClient->addItem(pItem);
                    }

                }
                else
                {
                    CChar *pChar = dynamic_cast <CChar *>(pObj);
                    if (pClient == pChar->GetClient())
                        pClient->addPlayerView(ptOld);
                    else if ((ptMe.GetDistSight(pt) <= tViewDist)
                        && ((ptMe.GetDistSight(ptOld) > tViewDist) || !(pNetState->isClientVersion(MINCLIVER_HS) || pNetState->isClientEnhanced()) || IsSetOF(OF_NoSmoothSailing)))
                    {
                        if ((pt.GetDist(ptOld) > 1) && (pNetState->isClientLessVersion(MINCLIVER_HS)) && (pChar->GetTopPoint().GetDistSight(ptOld) < tViewDist))
                            pClient->addCharMove(pChar);
                        else
                        {
                            pClient->addObjectRemove(pChar);
                            pClient->addChar(pChar);
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool CCMultiMovable::MoveToRegion(CRegionWorld * pRegionOld, CRegionWorld *pRegionNew) const
{

    CItem *pItem = dynamic_cast<CItem*>(const_cast<CCMultiMovable*>(this));
    ASSERT(pItem);
    CItemMulti *pMulti = static_cast<CItemMulti*>(pItem);
    if (pRegionOld == pRegionNew)
    {
        return true;
    }
    if (!pRegionNew)
    {
        return false;
    }
    if (!g_Serv.IsLoading())
    {
        // Leaving region trigger. (may not be allowed to leave ?)
        if (pRegionOld)
        {
            if (IsTrigUsed(TRIGGER_EXIT))
            {
                if (pRegionOld->OnRegionTrigger(pMulti->GetCaptain(), RTRIG_EXIT) == TRIGRET_RET_TRUE)
                {
                    return false;
                }
            }

            if (IsTrigUsed(TRIGGER_REGIONLEAVE))
            {
                CScriptTriggerArgs Args(pRegionOld);
                if (pMulti->OnTrigger(ITRIG_RegionLeave, pMulti->GetCaptain(), &Args) == TRIGRET_RET_TRUE)
                {
                    return false;
                }
            }
        }
        if (pRegionNew)
        {
            if (IsTrigUsed(TRIGGER_ENTER))
            {
                if (pRegionNew->OnRegionTrigger(pMulti->GetCaptain(), RTRIG_ENTER) == TRIGRET_RET_TRUE)
                {
                    return false;
                }
            }

            if (IsTrigUsed(TRIGGER_REGIONENTER))
            {
                CScriptTriggerArgs Args(pRegionNew);
                if (pMulti->OnTrigger(ITRIG_RegionEnter, pMulti->GetCaptain(), &Args) == TRIGRET_RET_TRUE)
                {
                    return false;
                }
            }
        }
    }
    return true;
}

bool CCMultiMovable::CanMoveTo(const CPointMap & pt) const
{
    ADDTOCALLSTACK("CCMultiMovable::CanMoveTo");
    CItem *pItemThis = dynamic_cast<CItem*>(const_cast<CCMultiMovable*>(this));
    ASSERT(pItemThis);
    // Can we move to the new location ? all water type ?
    if (pItemThis->IsAttr(ATTR_MAGIC))
        return true;

    dword dwBlockFlags = CAN_I_WATER;

    g_World.GetHeightPoint2(pt, dwBlockFlags, true);
    if (dwBlockFlags & CAN_I_WATER)
        return true;

    return false;
}

bool CCMultiMovable::Face(DIR_TYPE dir)
{
    ADDTOCALLSTACK("CCMultiMovable::Face");
    // Change the direction of the ship.

    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    CItemMulti *pMulti = static_cast<CItemMulti*>(pItemThis);
    if (!pItemThis->IsTopLevel() || !pMulti->GetRegion())
    {
        return false;
    }

    uint iDirection = 0;
    for (; ; ++iDirection)
    {
        if (iDirection >= CountOf(sm_FaceDir))
            return false;
        if (dir == sm_FaceDir[iDirection])
            break;
    }

    int iFaceOffset = GetFaceOffset();
    ITEMID_TYPE idnew = (ITEMID_TYPE)(pItemThis->GetID() - iFaceOffset + iDirection);
    const CItemBaseMulti * pMultiNew = pMulti->Multi_GetDef(idnew);
    if (pMultiNew == nullptr)
    {
        return false;
    }

    int iTurn = dir - sm_FaceDir[iFaceOffset];

    // ?? Are there blocking items in the way of the turn ?

    // Acquire the CRect for the new direction of the ship
    CRectMap rect(pMultiNew->m_rect);
    rect.m_map = pItemThis->GetTopPoint().m_map;
    rect.OffsetRect(pItemThis->GetTopPoint().m_x, pItemThis->GetTopPoint().m_y);

    // Check that we can fit into this space.
    CPointMap ptTmp;
    ptTmp.m_z = pItemThis->GetTopPoint().m_z;
    ptTmp.m_map = (uchar)(rect.m_map);
    for (ptTmp.m_x = (short)(rect.m_left); ptTmp.m_x < (short)(rect.m_right); ++ptTmp.m_x)
    {
        for (ptTmp.m_y = (short)(rect.m_top); ptTmp.m_y < (short)(rect.m_bottom); ++ptTmp.m_y)
        {
            if (pMulti->GetRegion()->IsInside2d(ptTmp))
                continue;
            // If the ship already overlaps a point then we must
            // already be allowed there.
            if ((!ptTmp.IsValidPoint()) || (!CanMoveTo(ptTmp)))
            {
                CItem *pTiller = pMulti->Multi_GetSign();
                ASSERT(pTiller);
                pTiller->Speak(g_Cfg.GetDefaultMsg(DEFMSG_TILLER_CANT_TURN), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
                return false;
            }
        }
    }

    const CItemBaseMulti * pMultiOld = pMulti->Multi_GetDef(pItemThis->GetID());

    // Reorient everything on the deck
    CObjBase * ppObjs[MAX_MULTI_LIST_OBJS + 1];
    size_t iCount = ListObjs(ppObjs);
    for (size_t i = 0; i < iCount; ++i)
    {
        CObjBase *pObj = ppObjs[i];
        CPointMap pt = pObj->GetTopPoint();

        int xdiff = pt.m_x - pItemThis->GetTopPoint().m_x;
        int ydiff = pt.m_y - pItemThis->GetTopPoint().m_y;
        int xd = xdiff;
        int yd = ydiff;
        switch (iTurn)
        {
            case 2: // right
            case (2 - DIR_QTY):
                xd = -ydiff;
                yd = xdiff;
                break;
            case -2: // left.
            case (DIR_QTY - 2):
                xd = ydiff;
                yd = -xdiff;
                break;
            default: // u turn.
                xd = -xdiff;
                yd = -ydiff;
                break;
        }
        pt.m_x = (short)(pItemThis->GetTopPoint().m_x + xd);
        pt.m_y = (short)(pItemThis->GetTopPoint().m_y + yd);
        if (pObj->IsItem())
        {
            CItem * pItem = static_cast<CItem*>(pObj);
            if (pItem == pItemThis)
            {
                pMulti->GetRegion()->UnRealizeRegion();
                pMulti->SetID(idnew);
                pMulti->MultiRealizeRegion();
            }
            else if (pMulti->Multi_IsPartOf(pItem))
            {
                for (size_t j = 0; j < pMultiOld->m_Components.size(); ++j)
                {
                    const CItemBaseMulti::CMultiComponentItem & component = pMultiOld->m_Components[j];
                    if ((xdiff == component.m_dx) && (ydiff == component.m_dy) && ((pItem->GetTopZ() - pItemThis->GetTopZ()) == component.m_dz))
                    {
                        const CItemBaseMulti::CMultiComponentItem & componentnew = pMultiNew->m_Components[j];
                        pItem->SetID(componentnew.m_id);
                        pt.m_x = pItemThis->GetTopPoint().m_x + componentnew.m_dx;
                        pt.m_y = pItemThis->GetTopPoint().m_y + componentnew.m_dy;
                    }
                }
            }

            if (IsTrigUsed(TRIGGER_SHIPTURN))
            {
                CScriptTriggerArgs Args(dir, sm_FaceDir[iFaceOffset]);
                pItem->OnTrigger(ITRIG_Ship_Turn, &g_Serv, &Args);
            }
        }
        else if (pObj->IsChar())
        {
            CChar *pChar = static_cast<CChar*>(pObj);
            pChar->m_dirFace = GetDirTurn(pChar->m_dirFace, iTurn);
            pChar->RemoveFromView();
        }
        pObj->MoveTo(pt);
    }

    for (size_t i = 0; i < iCount; ++i)
    {
        CObjBase * pObj = ppObjs[i];
        pObj->Update();
    }

    pItemThis->m_itShip.m_DirFace = (uchar)(dir);
    return true;
}

bool CCMultiMovable::Move(DIR_TYPE dir, int distance)
{
    ADDTOCALLSTACK("CCMultiMovable::Move");
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    CItemMulti *pMulti = static_cast<CItemMulti*>(pItemThis);
    if (dir >= DIR_QTY)
        return false;

    const CRegion* pMultiRegion = pMulti->GetRegion();
    if (pMultiRegion == nullptr)
    {
        DEBUG_ERR(("Ship bad region\n"));
        return false;
    }

    CPointMap ptDelta;
    ptDelta.ZeroPoint();

    CPointMap ptFore(pMultiRegion->GetRegionCorner(dir));
    CPointMap ptLeft(pMultiRegion->GetRegionCorner(GetDirTurn(dir, -1 - (dir % 2))));	// acquiring the flat edges requires two 'turns' for diagonal movement
    CPointMap ptRight(pMultiRegion->GetRegionCorner(GetDirTurn(dir, 1 + (dir % 2))));
    CPointMap ptTest(ptLeft.m_x, ptLeft.m_y, pItemThis->GetTopZ(), pItemThis->GetTopMap());

    bool fStopped = false, fTurbulent = false;

    for (int i = 0; i < distance; ++i)
    {
        // Check that we aren't sailing off the edge of the world
        ptFore.Move(dir);
        ptFore.m_z = pItemThis->GetTopZ();
        if (!ptFore.IsValidPoint())
        {
            // Circle the globe
            // Fall off edge of world ?
            fStopped = true;
            fTurbulent = true;
            break;
        }

#ifdef _DEBUG
     // In debug builds, this flashes some spots over tiles as they are checked for valid movement
    #define SPAWNSHIPTRACK(a,b)		pMulti->EffectXYZ(EFFECT_XYZ, ITEMID_FRUIT_APPLE, nullptr, &a, 1, 0, false, b)
#else
    #define SPAWNSHIPTRACK(a,b)
#endif

        // Test along the ship's edge to make sure nothing is blocking movement, this is split into two switch
        // statements (n/s and e/w) since moving diagonally requires the testing of both edges (i.e. 'north+east')
        ptLeft.Move(dir);
        ptRight.Move(dir);

        // test north/south edge (as needed)
        switch (dir)
        {
            case DIR_N:
            case DIR_NE:
            case DIR_NW:
                ptTest.m_y = ptFore.m_y; // align y coordinate
                for (int x = ptLeft.m_x; x <= ptRight.m_x; ++x)
                {
                    ptTest.m_x = (short)(x);
                    SPAWNSHIPTRACK(ptTest, 0x40);
                    if (CanMoveTo(ptTest) == false)
                    {
                        fStopped = true;
                        break;
                    }
                }
                break;

            case DIR_S:
            case DIR_SE:
            case DIR_SW:
                ptTest.m_y = ptFore.m_y;
                for (int x = ptRight.m_x; x <= ptLeft.m_x; x++)
                {
                    ptTest.m_x = (short)(x);
                    SPAWNSHIPTRACK(ptTest, 0x40);
                    if (CanMoveTo(ptTest) == false)
                    {
                        fStopped = true;
                        break;
                    }
                }
                break;

            default:
                break;
        }

        // test east/west edge (as needed)
        switch (dir)
        {
            case DIR_E:
            case DIR_NE:
            case DIR_SE:
                ptTest.m_x = ptFore.m_x; // align x coordinate
                for (int y = ptLeft.m_y; y <= ptRight.m_y; ++y)
                {
                    ptTest.m_y = (short)(y);
                    SPAWNSHIPTRACK(ptTest, 0xe0);
                    if (CanMoveTo(ptTest) == false)
                    {
                        fStopped = true;
                        break;
                    }
                }
                break;

            case DIR_W:
            case DIR_NW:
            case DIR_SW:
                ptTest.m_x = ptFore.m_x;
                for (int y = ptRight.m_y; y <= ptLeft.m_y; ++y)
                {
                    ptTest.m_y = (short)(y);
                    SPAWNSHIPTRACK(ptTest, 0xe0);
                    if (CanMoveTo(ptTest) == false)
                    {
                        fStopped = true;
                        break;
                    }
                }
                break;

            default:
                break;
        }

#undef SHIPSPAWNTRACK // macro no longer needed

        // If the ship has been flagged as stopped, then abort movement here
        if (fStopped == true)
            break;

        // Move delta one space
        ptDelta.Move(dir);
    }

    if (ptDelta.m_x != 0 || ptDelta.m_y != 0 || ptDelta.m_z != 0)
    {
        MoveDelta(ptDelta);

        // Move again
        pItemThis->GetTopSector()->SetSectorWakeStatus();	// may get here b4 my client does.
    }

    if (fStopped == true)
    {
        CItem * pTiller = pMulti->Multi_GetSign();
        ASSERT(pTiller);
        if (fTurbulent == true)
            pTiller->Speak(g_Cfg.GetDefaultMsg(DEFMSG_TILLER_TURB_WATER), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
        else
            pTiller->Speak(g_Cfg.GetDefaultMsg(DEFMSG_TILLER_STOPPED), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
        return false;
    }

    return true;
}

bool CCMultiMovable::OnMoveTick()
{
    ADDTOCALLSTACK("CCMultiMovable::OnMoveTick");
    // We just got a move tick.
    // RETURN: false = delete the multi .
    return true;
}

bool CCMultiMovable::OnTick()
{
    ADDTOCALLSTACK("CCMultiMovable::OnTick");
    // Ships move on their tick.

    if (m_shipSpeed.period == 0 && m_shipSpeed.tiles)    // Multis without movement values can decay as normal items.
    {
        return false;
    }
    const CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    if (pItemThis->m_itShip._eMovementType == SMT_STOP)	// decay the ship instead ???
        return true;

    // Calculate the leading point.
    DIR_TYPE dir = (DIR_TYPE)(pItemThis->m_itShip.m_DirMove);

    if (!Move(dir, m_shipSpeed.tiles))
    {
        Stop();
        return true;
    }
    SetNextMove();
    return true;
}

void CCMultiMovable::Stop()
{
    ADDTOCALLSTACK("CCMultiMovable::Stop");
    // Make sure we have stopped.

    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    pItemThis->m_itShip._eMovementType = SMT_STOP;
    _pCaptain = nullptr;
}

enum
{
    CMV_SHIPANCHORDROP,
    CMV_SHIPANCHORRAISE,
    CMV_SHIPBACK,
    CMV_SHIPBACKLEFT,
    CMV_SHIPBACKRIGHT,
    CMV_SHIPDOWN,
    CMV_SHIPDRIFTLEFT,
    CMV_SHIPDRIFTRIGHT,
    CMV_SHIPFACE,
    CMV_SHIPFORE,
    CMV_SHIPFORELEFT,
    CMV_SHIPFORERIGHT,
    CMV_SHIPGATE,
    CMV_SHIPLAND,
    CMV_SHIPMOVE,
    CMV_SHIPSTOP,
    CMV_SHIPTURN,
    CMV_SHIPTURNLEFT,
    CMV_SHIPTURNRIGHT,
    CMV_SHIPUP,
    CMV_QTY
};

lpctstr const CCMultiMovable::sm_szVerbKeys[CMV_QTY + 1] =
{
    "SHIPANCHORDROP",
    "SHIPANCHORRAISE",
    "SHIPBACK",
    "SHIPBACKLEFT",
    "SHIPBACKRIGHT",
    "SHIPDOWN",		// down one space.
    "SHIPDRIFTLEFT",
    "SHIPDRIFTRIGHT",
    "SHIPFACE",		// set the ships facing direction.
    "SHIPFORE",
    "SHIPFORELEFT",
    "SHIPFORERIGHT",
    "SHIPGATE",		// Moves the whole ship to some new point location.
    "SHIPLAND",
    "SHIPMOVE",		// move in a specified direction.
    "SHIPSTOP",
    "SHIPTURN",
    "SHIPTURNLEFT",
    "SHIPTURNRIGHT",
    "SHIPUP",		// up one space.
    nullptr
};

bool CCMultiMovable::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script
{
    ADDTOCALLSTACK("CItemShip::r_Verb");
    // Speaking in this ships region.
    // return: true = command for the ship.

    //"One (direction*)", " (Direction*), one" Moves ship one tile in desired direction and stops.
    //"Slow (direction*)" Moves ship slowly in desired direction (see below for possible directions).

    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    if (iCmd < 0)
        return false;

    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    CItemMulti *pMultiThis = static_cast<CItemMulti*>(pItemThis);
    if (!pSrc || !pItemThis->IsTopLevel())
        return false;

    CChar * pChar = pSrc->GetChar();

    // Only key holders can command the ship ???
    // if ( pChar && pChar->ContentFindKeyFor( pItem ))

    // Find the tiller man object.
    CItem * pTiller = pMultiThis->Multi_GetSign();
    ASSERT(pTiller);

    // Get current facing dir.
    DIR_TYPE DirFace = sm_FaceDir[GetFaceOffset()];
    int DirMoveChange;
    lpctstr pszSpeak = nullptr;

    switch (iCmd)
    {
        case CMV_SHIPSTOP:
        {
            // "Furl sail"
            // "Stop" Stops current ship movement.
            if (pItemThis->m_itShip._eMovementType == 0)
                break;
            Stop();
            break;
        }

        case CMV_SHIPFACE:
        {
            // Face this direction. do not change the direction of movement.
            if (!s.HasArgs())
                return false;
            return Face(GetDirStr(s.GetArgStr()));
        }

        case CMV_SHIPMOVE:
        {
            // Move one space in this direction.
            // Does NOT protect against exploits !
            if (!s.HasArgs())
                return false;
            pItemThis->m_itShip.m_DirMove = (byte)(GetDirStr(s.GetArgStr()));
            SetCaptain(pSrc);
            return Move((DIR_TYPE)(pItemThis->m_itShip.m_DirMove), m_shipSpeed.tiles);
        }

        case CMV_SHIPGATE:
        {
            // Move the whole ship and contents to another place.
            if (!s.HasArgs())
                return false;

            CPointMap ptdelta = g_Cfg.GetRegionPoint(s.GetArgStr());
            if (!ptdelta.IsValidPoint())
                return false;
            ptdelta -= pItemThis->GetTopPoint();
            return MoveDelta(ptdelta);
        }

        case CMV_SHIPTURNLEFT:
        {
            // "Port",
            // "Turn Left",
            DirMoveChange = -2;
        doturn:
            if (pItemThis->m_itShip.m_fAnchored != 0)
            {
            anchored:
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_ANCHOR_IS_DOWN);
                break;
            }
            DIR_TYPE DirMove = (DIR_TYPE)(pItemThis->m_itShip.m_DirMove);
            pItemThis->m_itShip.m_DirMove = (uchar)(GetDirTurn(DirFace, DirMoveChange));
            if (!Face((DIR_TYPE)(pItemThis->m_itShip.m_DirMove)))
            {
                pItemThis->m_itShip.m_DirMove = (uchar)(DirMove);
                return false;
            }
            break;
        }

        case CMV_SHIPTURNRIGHT:
        {
            // "Starboard",
            // "Turn Right",
            DirMoveChange = 2;
            goto doturn;
        }

        case CMV_SHIPDRIFTLEFT:
        {
            // "Left",
            // "Drift Left",
            DirMoveChange = -2;
        dodirmovechange:
            if (pItemThis->m_itShip.m_fAnchored != 0)
                goto anchored;
            if (!SetMoveDir(GetDirTurn(DirFace, DirMoveChange), SMT_NORMAL))
                return false;
            break;
        }

        case CMV_SHIPDRIFTRIGHT:
        {
            // "Right",
            // "Drift Right",
            DirMoveChange = 2;
            goto dodirmovechange;
        }

        case CMV_SHIPBACK:
        {
            // "Back",			// Move ship backwards
            // "Backward",		// Move ship backwards
            // "Backwards",	// Move ship backwards
            SetCaptain(pSrc);
            DirMoveChange = 4;
            goto dodirmovechange;
        }

        case CMV_SHIPFORE:
        {
            // "Forward"
            // "Foreward",		// Moves ship forward.
            // "Unfurl sail",	// Moves ship forward.
            DirMoveChange = 0;
            SetCaptain(pSrc);
            goto dodirmovechange;
        }

        case CMV_SHIPFORELEFT: // "Forward left",
        {
            DirMoveChange = -1;
            SetCaptain(pSrc);
            goto dodirmovechange;
        }

        case CMV_SHIPFORERIGHT: // "forward right",
        {
            DirMoveChange = 1;
            SetCaptain(pSrc);
            goto dodirmovechange;
        }

        case CMV_SHIPBACKLEFT:
        {
            // "backward left",
            // "back left",
            DirMoveChange = -3;
            SetCaptain(pSrc);
            goto dodirmovechange;
        }

        case CMV_SHIPBACKRIGHT:
        {
            // "backward right",
            // "back right",
            DirMoveChange = 3;
            SetCaptain(pSrc);
            goto dodirmovechange;
        }

        case CMV_SHIPANCHORRAISE: // "Raise Anchor",
        {
            if (pItemThis->m_itShip.m_fAnchored == 0)
            {
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_ANCHOR_IS_ALL_UP);
                break;
            }
            pItemThis->m_itShip.m_fAnchored = 0;
            break;
        }

        case CMV_SHIPANCHORDROP: // "Drop Anchor",
        {
            if (pItemThis->m_itShip.m_fAnchored != 0)
            {
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_ANCHOR_IS_ALL_DOWN);
                break;
            }
            pItemThis->m_itShip.m_fAnchored = 1;
            Stop();
            break;
        }

        case CMV_SHIPTURN:
        {
            //	"Turn around",	// Turns ship around and proceeds.
            // "Come about",	// Turns ship around and proceeds.
            DirMoveChange = 4;
            goto doturn;
        }

        case CMV_SHIPUP: // "Up"
        {
            if (!pItemThis->IsAttr(ATTR_MAGIC))
                return false;

            CPointMap pt;
            pt.m_z = PLAYER_HEIGHT;
            if (MoveDelta(pt))
            {
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_CONFIRM);
            }
            else
            {
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_DENY);
            }
            break;
        }

        case CMV_SHIPDOWN: // "Down"
        {
            if (!pItemThis->IsAttr(ATTR_MAGIC))
                return false;
            CPointMap pt;
            pt.m_z = -PLAYER_HEIGHT;
            if (MoveDelta(pt))
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_CONFIRM);
            else
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_DENY);
            break;
        }

        case CMV_SHIPLAND: // "Land"
        {
            if (!pItemThis->IsAttr(ATTR_MAGIC))
                return false;
            char zold = pItemThis->GetTopZ();
            CPointMap pt = pItemThis->GetTopPoint();
            pt.m_z = zold;
            pItemThis->SetTopZ(-UO_SIZE_Z);	// bottom of the world where i won't get in the way.
            dword dwBlockFlags = CAN_I_WATER;
            char z = g_World.GetHeightPoint2(pt, dwBlockFlags);
            pItemThis->SetTopZ(zold);	// restore z for now.
            pt.InitPoint();
            pt.m_z = z - zold;
            if (pt.m_z)
            {
                MoveDelta(pt);
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_CONFIRM);
            }
            else
            {
                pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_DENY);
            }
            break;
        }

        default:
        {
            return false;
        }
    }

    if (pChar && pTiller)
    {
        if (pszSpeak == nullptr)
        {
            switch (Calc_GetRandVal(3))
            {
                case 1:
                    pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_1);
                    break;
                case 2:
                    pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_2);
                    break;
                default:
                    pszSpeak = g_Cfg.GetDefaultMsg(DEFMSG_TILLER_REPLY_3);
                    break;
            }
        }

        tchar szText[MAX_TALK_BUFFER];
        strcpy(szText, pszSpeak);
        pChar->ParseText(szText, &g_Serv);
        pTiller->Speak(szText, HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL);
    }
    return true;
}

enum CML_TYPE
{

    CML_PILOT,
    CML_SHIPSPEED,
    CML_SPEEDMODE,
    CML_QTY
};

lpctstr const CCMultiMovable::sm_szLoadKeys[CML_QTY + 1] =
{
    "PILOT",
    "SHIPSPEED",
    "SPEEDMODE",
    nullptr
};


bool CCMultiMovable::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemShip::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
    int index = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (index == -1)
    {
        if (!strnicmp(pszKey, "SHIPSPEED.", 10))
            index = CML_SHIPSPEED;
    }
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);
    switch (index)
    {
        case CML_PILOT:
        {
            if (pItemThis->m_itShip.m_Pilot)
                sVal.FormatHex(pItemThis->m_itShip.m_Pilot);
            else
                sVal.FormatVal(0);
        } break;


        case CML_SHIPSPEED:
        {	/*
            * Intervals:
            *       drift forward
            * fast | 0.25|   0.25
            * slow | 0.50|   0.50
            *
            * Speed:
            *       drift forward
            * fast |  0x4|    0x4
            * slow |  0x3|    0x3
            *
            * Tiles (per interval):
            *       drift forward
            * fast |    1|      1
            * slow |    1|      1
            *
            * 'walking' in piloting mode has a 1s interval, speed 0x2
            */
            pszKey += 9;

            if (*pszKey == '.')
            {
                ++pszKey;
                if (!strnicmp(pszKey, "TILES", 5))
                {
                    sVal.FormatVal(m_shipSpeed.tiles);
                    break;
                }
                else if (!strnicmp(pszKey, "PERIOD", 6))
                {
                    sVal.FormatVal(m_shipSpeed.period);
                    break;
                }
                return false;
            }

            sVal.Format("%d,%d", m_shipSpeed.period, m_shipSpeed.tiles);
        } break;

        case CML_SPEEDMODE:
        {
            sVal.FormatVal(_eSpeedMode);
        }	break;
        default:
        {
            return false;
        }
    }
    return true;
}


bool CCMultiMovable::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemShip::r_LoadVal");
    lpctstr	pszKey = s.GetKey();
    CML_TYPE index = (CML_TYPE)FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    CItem *pItemThis = dynamic_cast<CItem*>(this);
    ASSERT(pItemThis);

    switch (index)
    {
        case CML_SPEEDMODE:
        {
            ShipMovementSpeed speed = (ShipMovementSpeed)(s.GetArgVal());
            if (speed > SMS_FAST)
                speed = SMS_FAST;
            else if (speed < SMS_NORMAL)
                speed = SMS_NORMAL;	//Max = 4, Min = 1.
            _eSpeedMode = speed;
        }
        break;

        case CML_SHIPSPEED:
        {
            pszKey += 9;
            if (*pszKey == '.')
            {
                ++pszKey;
                if (!strnicmp(pszKey, "TILES", 5))
                {
                    m_shipSpeed.tiles = s.GetArgUCVal();
                    return true;
                }
                else if (!strnicmp(pszKey, "PERIOD", 6))
                {
                    m_shipSpeed.period = (s.GetArgUSVal() * (IsSetOF(OF_NoSmoothSailing) ? MSECS_PER_TENTH : 1)); // get tenths from script, convert to msecs.
                    return true;
                }
                int64 piVal[2];
                size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, CountOf(piVal));
                if (iQty == 2)
                {
                    m_shipSpeed.period = (ushort)(piVal[0] * (IsSetOF(OF_NoSmoothSailing) ? MSECS_PER_TENTH : 1));
                    m_shipSpeed.tiles = (uchar)(piVal[1]);
                    return true;
                }
                else
                {
                    return false;
                }
            }
        } 
        break;
        case CML_PILOT:
        {
            CChar * pChar = CUID(s.GetArgDWVal()).CharFind();
            if (pChar)
                pItemThis->m_itShip.m_Pilot = pChar->GetUID();
            else
            {
                pItemThis->m_itShip.m_Pilot.InitUID();
                Stop();
            }
            return 1;
        } 
        break;
        default:
        {
            return false;
        }
    }
    return true;
}