//
// CItemMultiCustom.cpp
//

#include "../../common/resource/blocks/CDialogDef.h"
#include "../../common/CLog.h"
#include "../../common/CException.h"
#include "../../common/CUOInstall.h"
#include "../../network/network.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CWorld.h"
#include "../triggers.h"
#include "CItemMultiCustom.h"

/////////////////////////////////////////////////////////////////////////////

CItemMultiCustom::CItemMultiCustom(ITEMID_TYPE id, CItemBase * pItemDef) : 
    CTimedObject(PROFILE_MULTIS), CItemMulti(id, pItemDef, true)
{
    m_designMain.m_iRevision = 0;
    m_designMain.m_pData = nullptr;
    m_designMain.m_iDataRevision = 0;
    m_designWorking.m_iRevision = 0;
    m_designWorking.m_pData = nullptr;
    m_designWorking.m_iDataRevision = 0;
    m_designBackup.m_iRevision = 0;
    m_designBackup.m_pData = nullptr;
    m_designBackup.m_iDataRevision = 0;
    m_designRevert.m_iRevision = 0;
    m_designRevert.m_pData = nullptr;
    m_designRevert.m_iDataRevision = 0;
    m_pArchitect = nullptr;
    m_pSphereMulti = nullptr;
    m_rectDesignArea.SetRectEmpty();
    _iMaxPlane = -1;

    if (!g_Serv.IsLoading())
    {
        ResetStructure();
        CommitChanges();
    }
}

CItemMultiCustom::~CItemMultiCustom()
{
    if (m_pArchitect != nullptr)
    {
        m_pArchitect->m_pHouseDesign = nullptr;
        m_pArchitect = nullptr;
    }

    if (m_pSphereMulti != nullptr)
    {
        delete m_pSphereMulti;
        m_pSphereMulti = nullptr;
    }

    ComponentsContainer::iterator it;
    for (it = m_designMain.m_vectorComponents.begin(); it != m_designMain.m_vectorComponents.end(); it = m_designMain.m_vectorComponents.erase(it))				delete *it;
    for (it = m_designWorking.m_vectorComponents.begin(); it != m_designWorking.m_vectorComponents.end(); it = m_designWorking.m_vectorComponents.erase(it))	delete *it;
    for (it = m_designBackup.m_vectorComponents.begin(); it != m_designBackup.m_vectorComponents.end(); it = m_designBackup.m_vectorComponents.erase(it))		delete *it;
    for (it = m_designRevert.m_vectorComponents.begin(); it != m_designRevert.m_vectorComponents.end(); it = m_designRevert.m_vectorComponents.erase(it))		delete *it;

    m_designMain.m_vectorComponents.clear();
    if (m_designMain.m_pData != nullptr)
    {
        delete[] m_designMain.m_pData;
        m_designMain.m_pData = nullptr;
        m_designMain.m_iDataRevision = 0;
    }

    m_designWorking.m_vectorComponents.clear();
    if (m_designWorking.m_pData != nullptr)
    {
        delete[] m_designWorking.m_pData;
        m_designWorking.m_pData = nullptr;
        m_designWorking.m_iDataRevision = 0;
    }

    m_designBackup.m_vectorComponents.clear();
    if (m_designBackup.m_pData != nullptr)
    {
        delete[] m_designBackup.m_pData;
        m_designBackup.m_pData = nullptr;
        m_designBackup.m_iDataRevision = 0;
    }

    m_designRevert.m_vectorComponents.clear();
    if (m_designRevert.m_pData != nullptr)
    {
        delete[] m_designRevert.m_pData;
        m_designRevert.m_pData = nullptr;
        m_designRevert.m_iDataRevision = 0;
    }
}

void CItemMultiCustom::BeginCustomize(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::BeginCustomize");
    // enter the given client into design mode for this building
    if (pClientSrc == nullptr || pClientSrc->GetChar() == nullptr)
        return;

    if (m_pArchitect != nullptr)
        EndCustomize(true);

    if (PacketHouseBeginCustomise::CanSendTo(pClientSrc->GetNetState()) == false)
        return;

    // copy the main design to working, ready for editing
    CopyDesign(&m_designMain, &m_designWorking);
    ++m_designWorking.m_iRevision;

    // client will silently close all open dialogs and let the server think they're still open, so we need to update opened gump counts here
    CDialogDef* pDlg = nullptr;
    for (CClient::OpenedGumpsMap_t::iterator it = pClientSrc->m_mapOpenedGumps.begin(), end = pClientSrc->m_mapOpenedGumps.end(); it != end; ++it)
    {
        // the client leaves 'nodispose' dialogs open
        pDlg = dynamic_cast<CDialogDef*>(g_Cfg.ResourceGetDef(CResourceID(RES_DIALOG, it->first)));
        if (pDlg != nullptr && pDlg->m_fNoDispose == true)
            continue;

        it->second = 0;
    }
    CChar * pChar = pClientSrc->GetChar();

    if (IsTrigUsed(TRIGGER_HOUSEDESIGNBEGIN))
    {
        CScriptTriggerArgs args;
        args.m_pO1 = this;
        args.m_iN1 = 1; // Redeed AddOns
        args.m_iN2 = 0; // Transfer Lockdowns and Secured containers to Moving Crate.
        args.m_iN3 = 2; // Eject everyone from house.
        if (pChar->OnTrigger(CTRIG_HouseDesignBegin, pChar, &args) == TRIGRET_RET_TRUE)
        {
            EndCustomize(true);
            return;
        }

        if (args.m_iN1 == 1)
        {
            RedeedAddons();
        }
        if (args.m_iN2 == 1)
        {
            TransferSecuredToMovingCrate();
            TransferLockdownsToMovingCrate();
        }
        if (args.m_iN3 == 1)
        {
            EjectAll(pChar->GetUID());
        }
        else if (args.m_iN3 == 2)
        {
            EjectAll();
        }
    }

    // copy the working design to revert
    CopyDesign(&m_designWorking, &m_designRevert);

    // register client in design mode
    m_pArchitect = pClientSrc;
    m_pArchitect->m_pHouseDesign = this;

    new PacketHouseBeginCustomise(pClientSrc, this);

    // make sure they know the building exists
    pClientSrc->addItem(this);

    // send the latest building design
    SendStructureTo(pClientSrc);

    // move client to building and hide it
    pChar->StatFlag_Set(STATF_HIDDEN);

    CPointMap ptOld = pChar->GetTopPoint();
    CPointMap ptNew(GetTopPoint());
    ptNew.m_z += 7;

    pChar->MoveToChar(ptNew);
    pChar->UpdateMove(ptOld);

    // hide all dynamic items inside the house
    CWorldSearch Area(GetTopPoint(), GetDesignArea().GetWidth() / 2);
    Area.SetSearchSquare(true);
    for (;;)
    {
        CItem * pItem = Area.GetItem();
        if (pItem == nullptr)
            break;
        if (pItem != this)
            pClientSrc->addObjectRemove(pItem);
    }
}

void CItemMultiCustom::EndCustomize(bool bForced)
{
    ADDTOCALLSTACK("CItemMultiCustom::EndCustomize");
    // end customization, exiting the client from design mode
    if (m_pArchitect == nullptr)
        return;

    // exit the 'architect' from customise mode
    new PacketHouseEndCustomise(m_pArchitect, this);
    m_pArchitect->m_pHouseDesign = nullptr;

    CClient * pClient = m_pArchitect;
    m_pArchitect = nullptr;

    CChar * pChar = pClient->GetChar();
    if (pChar != nullptr)
    {
        if (IsTrigUsed(TRIGGER_HOUSEDESIGNEXIT))
        {
            CScriptTriggerArgs Args(this);
            Args.m_iN1 = bForced;
            if (pChar->OnTrigger(CTRIG_HouseDesignExit, pChar, &Args) == TRIGRET_RET_TRUE && !bForced)
            {
                BeginCustomize(pClient);
                return;
            }
        }

        // make sure scripts don't try and force the client back into design mode when
        // they're trying to log out
        if (m_pArchitect && bForced)
        {
            m_pArchitect->m_pHouseDesign = nullptr;
            m_pArchitect = nullptr;
        }

        // reveal character
        pChar->StatFlag_Clear(STATF_HIDDEN);

        // move character to signpost (unless they're already outside of the building)
        if (Multi_GetSign() && m_pRegion->IsInside2d(pChar->GetTopPoint()))
        {
            CPointMap ptOld = pChar->GetTopPoint();
            CPointMap ptDest = Multi_GetSign()->GetTopPoint();

            // find ground height, since the signpost is usually raised
            dword dwBlockFlags = 0;
            ptDest.m_z = g_World.GetHeightPoint2(ptDest, dwBlockFlags, true);

            pChar->MoveToChar(ptDest);
            pChar->UpdateMove(ptOld);
            pChar->Update();
        }

        SendStructureTo(pClient);
    }
}

void CItemMultiCustom::SwitchToLevel(CClient * pClientSrc, uchar iLevel)
{
    ADDTOCALLSTACK("CItemMultiCustom::SwitchToLevel");
    // switch the client to the given level of the building

    CChar * pChar = pClientSrc->GetChar();
    if (pChar == nullptr)
        return;

    uchar iMaxLevel = GetLevelCount();
    if (iLevel > iMaxLevel)
        iLevel = iMaxLevel;

    CPointMap pt = GetTopPoint();
    pt.m_z += GetPlaneZ(iLevel);

    pChar->SetTopZ(pt.m_z);
    pChar->UpdateMove(GetTopPoint());
    pClientSrc->addItem(this);
}

void CItemMultiCustom::CommitChanges(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::CommitChanges");
    // commit all the changes that have been made to the
    // working copy so that everyone can see them
    if (m_designWorking.m_iRevision == m_designMain.m_iRevision)
        return;

    int iOldPlane = _iMaxPlane;
    if ((pClientSrc != nullptr && pClientSrc->GetChar() != nullptr))
    {
        short iMaxZ = 0;
        bool fSendFullTrigger = IsTrigUsed(TRIGGER_HOUSEDESIGNCOMMITITEM);

        for (ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); ++i)
        {
            if (fSendFullTrigger)
            {
                CScriptTriggerArgs Args;
                Args.m_VarsLocal.SetNum("ID", (*i)->m_item.m_wTileID);
                Args.m_VarsLocal.SetNum("P.X", (*i)->m_item.m_dx);
                Args.m_VarsLocal.SetNum("P.Y", (*i)->m_item.m_dy);
                Args.m_VarsLocal.SetNum("P.Z", (*i)->m_item.m_dz);
                Args.m_VarsLocal.SetNum("VISIBLE", (*i)->m_item.m_visible);

                TRIGRET_TYPE iRet = pClientSrc->GetChar()->OnTrigger(CTRIG_HouseDesignCommitItem, pClientSrc->GetChar(), &Args);
                if (iRet == TRIGRET_RET_FALSE)
                {
                    m_designMain.m_vectorComponents.erase(i);
                    continue;
                }
            }
            if ((*i)->m_item.m_dz > iMaxZ)
            {
                iMaxZ = (*i)->m_item.m_dz;
                _iMaxPlane = GetPlane((uchar)(*i)->m_item.m_dz);
            }
        }
        if (IsTrigUsed(TRIGGER_HOUSEDESIGNCOMMIT))
        {
            CScriptTriggerArgs Args;
            Args.m_iN1 = m_designMain.m_vectorComponents.size();
            Args.m_iN2 = m_designWorking.m_vectorComponents.size();
            Args.m_iN3 = m_designWorking.m_iRevision;
            Args.m_pO1 = this;
            Args.m_VarsLocal.SetNum("FIXTURES.OLD", GetFixtureCount(&m_designMain));
            Args.m_VarsLocal.SetNum("FIXTURES.NEW", GetFixtureCount(&m_designWorking));
            Args.m_VarsLocal.SetNum("MAXZ", iMaxZ);

            if (pClientSrc->GetChar()->OnTrigger(CTRIG_HouseDesignCommit, pClientSrc->GetChar(), &Args) == TRIGRET_RET_TRUE)
                return;
        }
    }
    while (_iMaxPlane < iOldPlane)
    {
        ClearFloor((char)iOldPlane);
        --iOldPlane;
    }

    // replace the main design with the working design
    CopyDesign(&m_designWorking, &m_designMain);

    if (g_Serv.IsLoading() || !GetTopPoint().IsValidPoint())
        return;

    // remove all existing dynamic item fixtures
    CWorldSearch Area(GetTopPoint(), GetDesignArea().GetWidth());
    Area.SetSearchSquare(true);
    CItem * pItem;
    for (;;)
    {
        pItem = Area.GetItem();
        if (pItem == nullptr)
            break;

        if ((dword)pItem->m_TagDefs.GetKeyNum("FIXTURE") != (dword)GetUID())
            continue;

        pItem->Delete();
    }

    CRectMap rectNew;
    rectNew.SetRectEmpty();
    rectNew.m_map = GetTopMap();

    for (ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i)
    {
        rectNew.UnionPoint((*i)->m_item.m_dx, (*i)->m_item.m_dy);
        if ((*i)->m_item.m_visible)
            continue;

        // replace the doors and teleporters with real items
        pItem = CItem::CreateScript((*i)->m_item.GetDispID());
        if (pItem == nullptr)
            continue;

        CPointMap pt(GetTopPoint());
        pt.m_x += (*i)->m_item.m_dx;
        pt.m_y += (*i)->m_item.m_dy;
        pt.m_z += (char)((*i)->m_item.m_dz);

        pItem->m_uidLink = GetUID();
        pItem->ClrAttr(ATTR_DECAY | ATTR_CAN_DECAY);
        pItem->SetAttr(ATTR_MOVE_NEVER);
        pItem->m_TagDefs.SetNum("FIXTURE", (int64)(GetUID()));

        if (pItem->IsType(IT_TELEPAD))
        {
            // link telepads
            for (ComponentsContainer::iterator j = i + 1; j != i; ++j)
            {
                if (j == m_designMain.m_vectorComponents.end())
                    j = m_designMain.m_vectorComponents.begin();

                if ((*j)->m_item.GetDispID() != (*i)->m_item.GetDispID())
                    continue;

                pItem->m_itTelepad.m_ptMark = GetComponentPoint(*j);
                break;
            }
        }
        pItem->MoveToUpdate(pt);
        OnComponentCreate(pItem);
        AddComp(pItem->GetUID());
    }

    rectNew.OffsetRect(GetTopPoint().m_x, GetTopPoint().m_y);
    if (m_pRegion != nullptr && !m_pRegion->IsInside(rectNew))
    {
        // items outside the region won't be noticed in los/movement checks,
        // so the region boundaries need to be stretched to fit all the components
        g_Log.EventWarn("Building design for 0%x does not fit inside the MULTIREGION boundaries (design boundaries: %s). Attempting to resize region...\n", (dword)GetUID(), rectNew.Write());

        CRect rect = m_pRegion->GetRegionRect(0);
        rectNew.UnionRect(rect);

        m_pRegion->SetRegionRect(rectNew);
        m_pRegion->UnRealizeRegion();
        m_pRegion->RealizeRegion();
    }

    m_designMain.m_iRevision++;
    m_designWorking.m_iRevision = m_designMain.m_iRevision;

    if (m_pSphereMulti != nullptr)
    {
        // multi object needs to be recreated
        delete m_pSphereMulti;
        m_pSphereMulti = nullptr;
    }

    // update to all
    Update();
}

void CItemMultiCustom::AddItem(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z, short iStairID)
{
    ADDTOCALLSTACK("CItemMultiCustom::AddItem");
    // add an item to the building design at the given location
    const CChar * pCharSrc = nullptr;
    if (pClientSrc != nullptr)
    {
        pCharSrc = pClientSrc->GetChar();
        if (pCharSrc == nullptr)
            return;
    }

    CItemBase * pItemBase = CItemBase::FindItemBase(id);
    if (pItemBase == nullptr)
    {
        g_Log.EventWarn("Unscripted item 0%x being added to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    // floor tiles have no height and can be placed under other objects (walls, doors, etc)
    bool bFloor = (pItemBase->GetHeight() == 0);
    // fixtures are items that should be replaced by dynamic items
    bool bFixture = (pItemBase->IsType(IT_DOOR) || pItemBase->IsType(IT_DOOR_LOCKED) || pItemBase->IsID_Door(id) || pItemBase->IsType(IT_TELEPAD));

    if (!g_Serv.IsLoading())
    {
        if (!IsValidItem(id, pClientSrc, false))
        {
            g_Log.EventWarn("Invalid item 0%x being added to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
            SendStructureTo(pClientSrc);
            return;
        }

        CPointMap pt(GetTopPoint());
        pt.m_x += x;
        pt.m_y += y;

        if (pClientSrc != nullptr)
        {
            // if a client is placing the item, make sure that
            // it is within the design area
            CRect rectDesign = GetDesignArea();
            if (!rectDesign.IsInside2d(pt))
            {
                if (!rectDesign.IsInsideX(pt.m_x) || !rectDesign.IsInsideY(pt.m_y - 1))
                {
                    g_Log.EventWarn("Item 0%x being added to building 0%x outside of boundaries by 0%x (%s).\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0, pt.WriteUsed());
                    SendStructureTo(pClientSrc);
                    return;
                }

                // items can be placed 1 tile south of the edit area, but
                // only at ground level
                z = 0;
            }
        }

        if (z == INT8_MIN)
        {
            // determine z level based on player's position
            if (pCharSrc != nullptr)
                z = GetPlaneZ(GetPlane(pCharSrc->GetTopZ() - GetTopZ()));
            else
                z = 0;
        }

        Component * pPrevComponents[INT8_MAX];
        size_t iCount = GetComponentsAt(x, y, z, pPrevComponents, &m_designWorking);
        if (iCount > 0)
        {
            // remove previous item(s) in this location
            for (size_t i = 0; i < iCount; i++)
            {
                if (bFloor != pPrevComponents[i]->m_isFloor)
                    continue;

                RemoveItem(nullptr, pPrevComponents[i]->m_item.GetDispID(), pPrevComponents[i]->m_item.m_dx, pPrevComponents[i]->m_item.m_dy, (char)(pPrevComponents[i]->m_item.m_dz));
            }
        }
    }

    Component * pComponent = new Component;
    pComponent->m_item.m_wTileID = (word)(id);
    pComponent->m_item.m_dx = x;
    pComponent->m_item.m_dy = y;
    pComponent->m_item.m_dz = z;
    pComponent->m_item.m_visible = !bFixture;
    pComponent->m_isStair = iStairID;
    pComponent->m_isFloor = bFloor;

    m_designWorking.m_vectorComponents.emplace_back(pComponent);
    ++m_designWorking.m_iRevision;

    if (!g_Serv.IsLoading()) // quick fix, change it to execute only on customize mode

    {
        CItemContainer *pMovingCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
        std::vector<CUID> vListLocks;
        GetLockdownsAt(x, y, z, vListLocks);
        if (!vListLocks.empty())
        {
            for (std::vector<CUID>::iterator it = vListLocks.begin(); it != vListLocks.end(); ++it)
            {
                CItem *pItem = static_cast<CItem*>((*it).ItemFind());
                if (pItem)
                {
                    UnlockItem((*it));
                    pMovingCrate->ContentAdd(pItem);
                    pItem->RemoveFromView();
                }
            }
        }
        vListLocks.clear();

        std::vector<CUID> vListConts;
        GetSecuredAt(x, y, z, vListConts);
        if (!vListConts.empty())
        {
            for (std::vector<CUID>::iterator it = vListConts.begin(); it != vListConts.end(); ++it)
            {
                CItemContainer *pCont = static_cast<CItemContainer*>((*it).ItemFind());
                if (pCont)
                {
                    Release((*it));
                    pMovingCrate->ContentAdd(pCont);
                    pCont->RemoveFromView();
                }
            }
        }
        vListConts.clear();
    }
}

void CItemMultiCustom::AddStairs(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z, short iStairID)
{
    ADDTOCALLSTACK("CItemMultiCustom::AddStairs");
    // add a staircase to the building, the given ID must
    // be the ID of a multi to extract the items from
    const CChar * pCharSrc = nullptr;
    if (pClientSrc != nullptr)
    {
        pCharSrc = pClientSrc->GetChar();
        if (pCharSrc == nullptr)
            return;
    }

    const CSphereMulti * pMulti = g_Cfg.GetMultiItemDefs(id);
    if (pMulti == nullptr)
    {
        g_Log.EventWarn("Unscripted multi 0%x being added to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    if (!IsValidItem(id, pClientSrc, true))
    {
        g_Log.EventWarn("Invalid multi 0%x being added to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    if (z == INT8_MIN)
    {
        if (pCharSrc != nullptr)
            z = GetPlaneZ(GetPlane(pCharSrc->GetTopZ() - GetTopZ()));
        else
            z = 0;
    }

    if (iStairID == -1)
        iStairID = GetStairCount() + 1;

    size_t iQty = pMulti->GetItemCount();
    for (size_t i = 0; i < iQty; i++)
    {
        const CUOMultiItemRec_HS * pMultiItem = pMulti->GetItem(i);
        if (pMultiItem == nullptr)
            continue;

        if (!pMultiItem->m_visible)
            continue;

        AddItem(nullptr, pMultiItem->GetDispID(), x + pMultiItem->m_dx, y + pMultiItem->m_dy, z + (char)(pMultiItem->m_dz), iStairID);
    }
    SendStructureTo(pClientSrc);
}

void CItemMultiCustom::AddRoof(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z)
{
    ADDTOCALLSTACK("CItemMultiCustom::AddRoof");
    // add a roof piece to the building
    const CChar * pCharSrc = nullptr;
    if (pClientSrc != nullptr)
    {
        pCharSrc = pClientSrc->GetChar();
        if (pCharSrc == nullptr)
            return;
    }

    CItemBase * pItemBase = CItemBase::FindItemBase(id);
    if (pItemBase == nullptr)
    {
        g_Log.EventWarn("Unscripted roof tile 0%x being added to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    if ((pItemBase->GetTFlags() & UFLAG4_ROOF) == 0)
    {
        g_Log.EventWarn("Non-roof tile 0%x being added as a roof to building 0%x by 0%x.\n", id, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    if (z < -3 || z > 12 || (z % 3 != 0))
    {
        g_Log.EventWarn("Roof tile 0%x being added at invalid height %d to building 0%x by 0%x.\n", id, z, (dword)GetUID(), pCharSrc != nullptr ? (dword)pCharSrc->GetUID() : 0);
        SendStructureTo(pClientSrc);
        return;
    }

    if (pCharSrc != nullptr)
        z += GetPlaneZ(GetPlane(pCharSrc->GetTopZ() - GetTopZ()));

    AddItem(pClientSrc, id, x, y, z);
}

void CItemMultiCustom::RemoveItem(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z)
{
    ADDTOCALLSTACK("CItemMultiCustom::RemoveItem");
    // remove the item that's found at given location
    // ITEMID_NOTHING means we should remove any items found
    CRect rectDesign = GetDesignArea();
    CPointMap pt(GetTopPoint());
    pt.m_x += x;
    pt.m_y += y;

    if (pClientSrc != nullptr)
    {
        bool allowRemove = true;
        switch (GetPlane(z))
        {
            case 1:
                // at first level, clients cannot remove dirt tiles
                if (id == ITEMID_DIRT_TILE)
                    allowRemove = false;
                break;

            case 0:
                // at ground level, clients can only remove components along the bottom edge (stairs)
                if (pt.m_y != rectDesign.m_bottom)
                    allowRemove = false;
        }

        if (allowRemove == false)
        {
            SendStructureTo(pClientSrc);
            return;
        }
    }

    Component * pComponents[INT8_MAX];
    size_t iCount = GetComponentsAt(x, y, z, pComponents, &m_designWorking);
    if (iCount <= 0)
        return;

    bool bReplaceDirt = false;
    for (size_t i = 0; i < iCount; i++)
    {
        for (ComponentsContainer::iterator j = m_designWorking.m_vectorComponents.begin(); j != m_designWorking.m_vectorComponents.end(); ++j)
        {
            if (*j != pComponents[i])
                continue;

            if (id != ITEMID_NOTHING && ((*j)->m_item.GetDispID() != id))
                break;

            if (pClientSrc != nullptr && RemoveStairs(*j))
                break;

            // floor tiles the ground floor are replaced with dirt tiles
            if (((*j)->m_item.m_wTileID != ITEMID_DIRT_TILE) && (*j)->m_isFloor && (GetPlane(*j) == 1) && (GetPlaneZ(GetPlane(*j)) == (char)((*j)->m_item.m_dz)))
                bReplaceDirt = true;

            m_designWorking.m_vectorComponents.erase(j);
            m_designWorking.m_iRevision++;
            break;
        }
    }

    if (pClientSrc != nullptr && bReplaceDirt)
    {
        // make sure that the location is within the proper boundaries
        if (rectDesign.IsInsideX(pt.m_x) && rectDesign.IsInsideX(pt.m_x - 1) && rectDesign.IsInsideY(pt.m_y) && rectDesign.IsInsideY(pt.m_y - 1))
            AddItem(nullptr, ITEMID_DIRT_TILE, x, y, 7);
    }
    SendStructureTo(pClientSrc);
}

bool CItemMultiCustom::RemoveStairs(Component * pStairComponent)
{
    ADDTOCALLSTACK("CItemMultiCustom::RemoveStairs");
    // attempt to remove the given component as a stair piece,
    // removing all 'linked' stair pieces along with it
    // return false means that the passed component was not a
    // stair piece and should be removed normally
    if (pStairComponent == nullptr)
        return false;

    if (pStairComponent->m_isStair == 0)
        return false;

    short iStairID = pStairComponent->m_isStair;

    for (ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); )
    {
        if ((*i)->m_isStair == iStairID)
        {
            bool bReplaceDirt = false;
            if ((*i)->m_isFloor && (GetPlane(*i) == 1) && (GetPlaneZ(GetPlane(*i)) == (*i)->m_item.m_dz))
                bReplaceDirt = true;

            short x = (*i)->m_item.m_dx;
            short y = (*i)->m_item.m_dy;
            char z = (char)((*i)->m_item.m_dz);

            i = m_designWorking.m_vectorComponents.erase(i);
            m_designWorking.m_iRevision++;

            if (bReplaceDirt)
                AddItem(nullptr, ITEMID_DIRT_TILE, x, y, z);
        }
        else
        {
            ++i;
        }
    }

    return true;
}

void CItemMultiCustom::RemoveRoof(CClient * pClientSrc, ITEMID_TYPE id, short x, short y, char z)
{
    ADDTOCALLSTACK("CItemMultiCustom::RemoveRoof");

    CItemBase * pItemBase = CItemBase::FindItemBase(id);
    if (pItemBase == nullptr)
        return;

    if ((pItemBase->GetTFlags() & UFLAG4_ROOF) == 0)
        return;

    RemoveItem(pClientSrc, id, x, y, z);
}

void CItemMultiCustom::SendVersionTo(CClient * pClientSrc) const
{
    ADDTOCALLSTACK("CItemMultiCustom::SendVersionTo");
    // send the revision number of this building to the given
    // client
    if (pClientSrc == nullptr || pClientSrc->IsPriv(PRIV_DEBUG))
        return;

    // send multi version
    new PacketHouseDesignVersion(pClientSrc, this);
}

void CItemMultiCustom::SendStructureTo(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::SendStructureTo");
    // send the design details of this building to the given
    // client
    if (pClientSrc == nullptr || pClientSrc->GetChar() == nullptr || pClientSrc->IsPriv(PRIV_DEBUG))
        return;

    if (PacketHouseDesign::CanSendTo(pClientSrc->GetNetState()) == false)
        return;

    DesignDetails * pDesign = nullptr;
    if (m_pArchitect == pClientSrc)
        pDesign = &m_designWorking;	// send the working copy to the designer
    else
        pDesign = &m_designMain;	// other clients will only see final designs

    // check if a packet is already saved
    if (pDesign->m_pData != nullptr)
    {
        // check the saved packet matches the design revision
        if (pDesign->m_iDataRevision == pDesign->m_iRevision)
        {
            pDesign->m_pData->send(pClientSrc);
            return;
        }

        // remove the saved packet and continue to build a new one
        delete pDesign->m_pData;
        pDesign->m_pData = nullptr;
        pDesign->m_iDataRevision = 0;
    }

    PacketHouseDesign* cmd = new PacketHouseDesign(this, pDesign->m_iRevision);

    if (pDesign->m_vectorComponents.size())
    {
        // determine the dimensions of the building
        const CRect rectDesign = GetDesignArea();
        int iMinX = rectDesign.m_left, iMinY = rectDesign.m_top;
        int iWidth = rectDesign.GetWidth();
        int iHeight = rectDesign.GetHeight();

        ComponentsContainer vectorStairs;
        Component * pComp;
        CItemBase * pItemBase;

        if (_iMaxPlane < 0)
        {

            // find the highest plane/floor
            for (ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i)
            {
                if (GetPlane(*i) <= _iMaxPlane)
                    continue;

                _iMaxPlane = GetPlane(*i);
            }
        }

        nword wPlaneBuffer[PLANEDATA_BUFFER];

        for (int iCurrentPlane = 0; iCurrentPlane <= _iMaxPlane; iCurrentPlane++)
        {
            // for each plane, generate a list of items
            bool bFoundItems = false;
            int iItemCount = 0;
            int iMaxIndex = 0;

            memset(wPlaneBuffer, 0, sizeof(wPlaneBuffer));
            for (ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i)
            {
                if (GetPlane(*i) != iCurrentPlane)
                    continue;

                pComp = *i;
                if (!pComp->m_item.m_visible && pDesign != &m_designWorking)
                    continue;

                pItemBase = CItemBase::FindItemBase(pComp->m_item.GetDispID());
                if (pItemBase == nullptr)
                    continue;

                // calculate the x,y position as an offset from the topleft corner
                CPointMap ptComp = GetComponentPoint(pComp);
                int x = (ptComp.m_x - 1) - iMinX;
                int y = (ptComp.m_y - 1) - iMinY;

                // index is (x*height)+y
                int index; // = (x * iHeight) + y;
                if (iCurrentPlane == 0)
                    index = ((x + 1) * (iHeight + 1)) + (y + 1);
                else
                    index = (x * (iHeight - 1)) + y;

                if ((GetPlaneZ(GetPlane(pComp)) != (char)(pComp->m_item.m_dz)) ||
                    (pItemBase->GetHeight() == 0 || pComp->m_isFloor) ||
                    (x < 0 || y < 0 || x >= iWidth || y >= iHeight) ||
                    (index < 0 || index >= PLANEDATA_BUFFER))
                {
                    // items that are:
                    //  - not level with plane height
                    //  - heightless / is a floor
                    //  - outside of building area
                    //  - outside of buffer bounds
                    // are placed in the stairs list
                    vectorStairs.push_back(pComp);
                    continue;
                }

                wPlaneBuffer[index] = (word)(pComp->m_item.GetDispID());
                bFoundItems = true;
                iItemCount++;
                iMaxIndex = maximum(iMaxIndex, index);
            }

            if (bFoundItems == false)
                continue;

            int iPlaneSize = (iMaxIndex + 1) * sizeof(nword);
            cmd->writePlaneData(iCurrentPlane, iItemCount, (byte*)wPlaneBuffer, iPlaneSize);
        }

        for (ComponentsContainer::iterator i = vectorStairs.begin(); i != vectorStairs.end(); ++i)
        {
            pComp = *i;
            if (!pComp->m_item.m_visible && pDesign != &m_designWorking)
                continue;

            // stair items can be sent in any order
            cmd->writeStairData(pComp->m_item.GetDispID(), pComp->m_item.m_dx, pComp->m_item.m_dy, (char)(pComp->m_item.m_dz));
        }
    }

    cmd->finalise();

    // save the packet in the design data
    pDesign->m_pData = cmd;
    pDesign->m_iDataRevision = pDesign->m_iRevision;

    // send the packet
    pDesign->m_pData->send(pClientSrc);
}

void CItemMultiCustom::BackupStructure()
{
    ADDTOCALLSTACK("CItemMultiCustom::BackupStructure");
    // create a backup of the working copy
    if (m_designWorking.m_iRevision == m_designBackup.m_iRevision)
        return;

    CopyDesign(&m_designWorking, &m_designBackup);
}

void CItemMultiCustom::RestoreStructure(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::RestoreStructure");
    // restore the working copy using the details stored in the
    // backup design
    if (m_designWorking.m_iRevision == m_designBackup.m_iRevision || m_designBackup.m_iRevision == 0)
        return;

    CopyDesign(&m_designBackup, &m_designWorking);

    if (pClientSrc != nullptr)
        pClientSrc->addItem(this);
}

void CItemMultiCustom::RevertChanges(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::RevertChanges");
    // restore the working copy using the details stored in the
    // revert design
    if (m_designWorking.m_iRevision == m_designRevert.m_iRevision || m_designRevert.m_iRevision == 0)
        return;

    CopyDesign(&m_designRevert, &m_designWorking);

    if (pClientSrc != nullptr)
        pClientSrc->addItem(this);
}

void CItemMultiCustom::ResetStructure(CClient * pClientSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::ResetStructure");
    // return the building design to it's original state, which
    // is simply the 'foundation' design from the multi.mul file

    m_designWorking.m_vectorComponents.clear();
    m_designWorking.m_iRevision++;
    const CSphereMulti * pMulti = g_Cfg.GetMultiItemDefs(GetID());
    if (pMulti != nullptr)
    {
        size_t iQty = pMulti->GetItemCount();
        for (size_t i = 0; i < iQty; i++)
        {
            const CUOMultiItemRec_HS * pMultiItem = pMulti->GetItem(i);
            if (pMultiItem == nullptr)
                continue;

            if (!pMultiItem->m_visible)
                continue;

            AddItem(nullptr, pMultiItem->GetDispID(), pMultiItem->m_dx, pMultiItem->m_dy, (char)(pMultiItem->m_dz));
        }
    }

    if (pClientSrc != nullptr)
        pClientSrc->addItem(this);
}

int CItemMultiCustom::GetRevision(const CClient * pClientSrc) const
{
    ADDTOCALLSTACK("CItemMultiCustom::GetRevision");
    // return the revision number, making sure to return
    // the working revision if the client is the designer
    if (pClientSrc && pClientSrc == m_pArchitect)
        return m_designWorking.m_iRevision;

    return m_designMain.m_iRevision;
}

uchar CItemMultiCustom::GetLevelCount()
{
    ADDTOCALLSTACK("CItemMultiCustom::GetLevelCount");
    // return how many levels (including the roof) there are
    // client decides this based on the size of the foundation
    const CRect rectDesign = GetDesignArea();
    if (rectDesign.GetWidth() >= 14 || rectDesign.GetHeight() >= 14)
        return 4;

    return 3;
}

short CItemMultiCustom::GetStairCount()
{
    ADDTOCALLSTACK("CItemMultiCustom::GetStairCount");
    // find and return the highest stair id
    short iCount = 0;
    for (ComponentsContainer::iterator i = m_designWorking.m_vectorComponents.begin(); i != m_designWorking.m_vectorComponents.end(); ++i)
    {
        if ((*i)->m_isStair == 0)
            continue;

        iCount = maximum(iCount, (*i)->m_isStair);
    }

    return iCount;
}

size_t CItemMultiCustom::GetFixtureCount(DesignDetails * pDesign)
{
    ADDTOCALLSTACK("CItemMultiCustom::GetFixtureCount");
    if (pDesign == nullptr)
        pDesign = &m_designMain;

    size_t count = 0;
    for (ComponentsContainer::iterator i = pDesign->m_vectorComponents.begin(); i != pDesign->m_vectorComponents.end(); ++i)
    {
        if ((*i)->m_item.m_visible)
            continue;

        count++;
    }

    return count;
}

size_t CItemMultiCustom::GetComponentsAt(short x, short y, char z, Component ** pComponents, DesignDetails * pDesign)
{
    ADDTOCALLSTACK("CItemMultiCustom::GetComponentsAt");
    // find a list of components that are located at the given
    // position, and store them in the given Component* array,
    // returning the number of components found

    if (pDesign == nullptr)
        pDesign = &m_designMain;

    size_t count = 0;
    Component * pComponent;
    for (size_t i = 0; i < pDesign->m_vectorComponents.size(); i++)
    {
        pComponent = pDesign->m_vectorComponents.at(i);

        if (pComponent->m_item.m_dx != x || pComponent->m_item.m_dy != y)
            continue;

        if (z != INT8_MIN && GetPlane((char)(pComponent->m_item.m_dz)) != GetPlane(z))
            continue;

        pComponents[count++] = pComponent;
    }

    return count;
}

const CPointMap CItemMultiCustom::GetComponentPoint(Component * pComp) const
{
    ADDTOCALLSTACK("CItemMultiCustom::GetComponentPoint");
    return GetComponentPoint(pComp->m_item.m_dx, pComp->m_item.m_dy, (char)(pComp->m_item.m_dz));
}

const CPointMap CItemMultiCustom::GetComponentPoint(short dx, short dy, char dz) const
{
    ADDTOCALLSTACK("CItemMultiCustom::GetComponentPoint");
    // return the real world location from the given offset
    CPointMap ptBase(GetTopPoint());
    ptBase.m_x += dx;
    ptBase.m_y += dy;
    ptBase.m_z += dz;

    return ptBase;
}

const CItemMultiCustom::CSphereMultiCustom * CItemMultiCustom::GetMultiItemDefs()
{
    ADDTOCALLSTACK("CItemMultiCustom::GetMultiItemDefs");
    // return a CSphereMultiCustom object that represents the components
    // in the main design
    if (m_pSphereMulti == nullptr)
    {
        m_pSphereMulti = new CSphereMultiCustom;
        m_pSphereMulti->LoadFrom(&m_designMain);
    }

    return m_pSphereMulti;
}

const CRect CItemMultiCustom::GetDesignArea()
{
    ADDTOCALLSTACK("CItemMultiCustom::GetDesignArea");
    // return the foundation dimensions, which is the design
    // area editable by clients

    if (m_rectDesignArea.IsRectEmpty())
    {
        m_rectDesignArea.SetRect(0, 0, 1, 1, GetTopMap());

        const CSphereMulti * pMulti = g_Cfg.GetMultiItemDefs(GetID());
        if (pMulti != nullptr)
        {
            // the client uses the multi items to determine the area
            // that's editable
            size_t iQty = pMulti->GetItemCount();
            for (size_t i = 0; i < iQty; i++)
            {
                const CUOMultiItemRec_HS * pMultiItem = pMulti->GetItem(i);
                if (pMultiItem == nullptr)
                    continue;

                if (!pMultiItem->m_visible)
                    continue;

                m_rectDesignArea.UnionPoint(pMultiItem->m_dx, pMultiItem->m_dy);
            }
        }
        else
        {
            // multi data is not available, so assume the region boundaries
            // are correct
            CRect rectRegion = m_pRegion->GetRegionRect(0);
            m_rectDesignArea.SetRect(rectRegion.m_left, rectRegion.m_top, rectRegion.m_right, rectRegion.m_top, rectRegion.m_map);

            const CPointMap pt = GetTopPoint();
            m_rectDesignArea.OffsetRect(-pt.m_x, -pt.m_y);

            DEBUG_WARN(("Multi data is not available for customizable building 0%x.", GetID()));
        }
    }

    CRect rect;
    const CPointMap pt = GetTopPoint();

    rect.SetRect(m_rectDesignArea.m_left, m_rectDesignArea.m_top, m_rectDesignArea.m_right, m_rectDesignArea.m_bottom, GetTopMap());
    rect.OffsetRect(pt.m_x, pt.m_y);

    return rect;
}

void CItemMultiCustom::DelComp(CUID uidComponent)
{
    /* The code bellow should remove the item from the m_mainDesign, however it's not being deleted from there even if the world item is
        So, in the next customize, the item will appear again.
        TODO: Make the whole customizing system more flexible to allow 'enter' and 'exit' customize mode and tweak the items without involving 
        any heavy load, trigger, or player iteraction/notifications.
    */
    /*CItem *pComp = uidComponent.ItemFind();
    if (pComp->m_TagDefs.GetKeyNum("FIXTURE", true) > 0)
    {
        CPointMap pt = pComp->GetTopPoint();
        pt.m_x -= GetTopPoint().m_x;
        pt.m_y -= GetTopPoint().m_y;
        pt.m_z -= GetTopPoint().m_z;
        RemoveItem(GetOwner().CharFind()->GetClient(), pComp->GetDispID(), pt.m_x, pt.m_y, pt.m_z);
    }*/
    CItemMulti::DelComp(uidComponent);
}

void CItemMultiCustom::CopyDesign(DesignDetails * designFrom, DesignDetails * designTo)
{
    ADDTOCALLSTACK("CItemMultiCustom::CopyComponents");
    // overwrite the details of one design with the details
    // of another
    Component * pComponent;

    // copy components
    designTo->m_vectorComponents.clear();
    for (ComponentsContainer::iterator i = designFrom->m_vectorComponents.begin(); i != designFrom->m_vectorComponents.end(); ++i)
    {
        pComponent = new Component;
        *pComponent = **i;

        designTo->m_vectorComponents.push_back(pComponent);
    }

    // copy revision
    designTo->m_iRevision = designFrom->m_iRevision;

    // copy saved packet
    if (designTo->m_pData != nullptr)
        delete designTo->m_pData;

    if (designFrom->m_pData != nullptr)
    {
        designTo->m_pData = new PacketHouseDesign(designFrom->m_pData);
        designTo->m_iDataRevision = designFrom->m_iDataRevision;
    }
    else
    {
        designTo->m_pData = nullptr;
        designTo->m_iDataRevision = 0;
    }
}

void CItemMultiCustom::GetLockdownsAt(short dx, short dy, char dz, std::vector<CUID> &vList)
{
    if (_lLockDowns.empty())
    {
        return;
    }
    short iFixedX = GetTopPoint().m_x + dx;
    short iFixedY = GetTopPoint().m_y + dy;
    char iFloor = CalculateLevel(GetTopPoint().m_z + dz);  // get the Diff Z from the Multi's Z
    for (std::vector<CUID>::iterator it = _lLockDowns.begin(); it != _lLockDowns.end(); ++it)
    {
        CItem *pItem = (*it).ItemFind();
        if ((pItem->GetTopPoint().m_x == iFixedX)
            && pItem->GetTopPoint().m_y == iFixedY
            && (CalculateLevel(pItem->GetTopPoint().m_z) == iFloor))
        {
            vList.push_back((*it));
        }
    }
    return;
}

void CItemMultiCustom::GetSecuredAt(short dx, short dy, char dz, std::vector<CUID> &vList)
{
    if (_lSecureContainers.empty())
    {
        return;
    }
    short iFixedX = GetTopPoint().m_x + dx;
    short iFixedY = GetTopPoint().m_y + dy;
    char iFloor = CalculateLevel(GetTopPoint().m_z + dz);  // get the Diff Z from the Multi's Z
    for (std::vector<CUID>::iterator it = _lSecureContainers.begin(); it != _lSecureContainers.end(); ++it)
    {
        CItemContainer *pCont = static_cast<CItemContainer*>((*it).ItemFind());
        if ((pCont->GetTopPoint().m_x == iFixedX)
            && pCont->GetTopPoint().m_y == iFixedY
            && (CalculateLevel(pCont->GetTopPoint().m_z) == iFloor))
        {
            vList.push_back(*it);
        }
    }
    return;
}

char CItemMultiCustom::CalculateLevel(char z)
{
    z -= GetTopPoint().m_z; // Take out the Multi Z level.
    z -= 6; // Customizable's Houses have a +6 level from the foundation.
    z /= 20;    // Each floor 
    return z;
}

void CItemMultiCustom::ClearFloor(char iFloor)
{
    char iBaseZ = GetTopPoint().m_z + (iFloor * 20) + 6;
    short iMaxZ = iBaseZ + 19;
    short iMinZ = iBaseZ;
    CItemContainer *pCrate = static_cast<CItemContainer*>(GetMovingCrate(true).ItemFind());
    int i = 0;
    // Removing Secured Containers.
    int max = (int)_lSecureContainers.size();
    if (max > 0)
    {
        for (i = 0; i < max; ++i)
        {
            CUID uid = _lSecureContainers[i];
            CItemContainer *pCont = static_cast<CItemContainer*>(uid.ItemFind());
            if ((pCont->GetTopPoint().m_z >= iMinZ) && (pCont->GetTopPoint().m_z <= iMaxZ))
            {
                Release(uid);
                pCont->RemoveFromView();
                pCrate->ContentAdd(pCont);
            }
        }
    }
    // Removing Lockdowns
    max = (int)_lLockDowns.size();
    if (max > 0)
    {
        for (i = 0; i < max; ++i)
        {
            CUID uid = _lLockDowns[i];
            CItem *pItem = uid.ItemFind();
            if ((pItem->GetTopPoint().m_z >= iMinZ) && (pItem->GetTopPoint().m_z <= iMaxZ))
            {
                UnlockItem(uid);
                pItem->RemoveFromView();
                pCrate->ContentAdd(pItem);
            }
        }
    }
    // Redeeding Addons.
    max = (int)_lAddons.size();
    if (max > 0)
    {
        for (i = 0; i < max; ++i)
        {
            CUID uid = _lLockDowns[i];
            CItemMulti *pAddon = static_cast<CItemMulti*>(uid.ItemFind());
            if ((pAddon->GetTopPoint().m_z >= iMinZ) && (pAddon->GetTopPoint().m_z <= iMaxZ))
            {
                pAddon->Redeed(false, false);
            }
        }
    }

    CWorldSearch Area(m_pRegion->m_pt, Multi_GetMaxDist());	// largest area.
    Area.SetSearchSquare(true);
    for (;;)
    {
        CItem * pItem = Area.GetItem();
        if (pItem == nullptr)
        {
            break;
        }
        if (pItem->GetUID() == GetUID() || pItem->GetUID() == pCrate->GetUID()) // Multi itself or the Moving Crate, neither is not handled here.
        {
            continue;
        }
        if (GetCompPos(pItem->GetUID()) != -1) // Components are not moved, Custom Multis only have sign and post as components and we are not going to remove them
        {
            continue;
        }
        if (pItem->GetTopPoint().m_z < iMinZ || pItem->GetTopPoint().m_z > iMaxZ || GetPlane(pItem->GetTopPoint().m_z) <= iFloor)
        {
            continue;
        }
        if (pItem->GetTopPoint().m_y == GetDesignArea().m_bottom)   // Do not mess with items outside the exterior of the multi (the extra +1 Y on the entrance).
        {
            continue;
        }
        if (pItem->IsType(IT_MULTI_ADDON) || pItem->IsType(IT_MULTI))  // If the item is a house Addon, redeed it.
        {
            static_cast<CItemMulti*>(pItem)->Redeed(false, false);
            Area.RestartSearch();	// we removed an item and this will mess the search loop, so restart to fix it.
            continue;
        }
        pItem->RemoveFromView();
        pCrate->ContentAdd(pItem);
    }
    
    Component *comp;

    // Reset Main Design
    i = (int)m_designMain.m_vectorComponents.size()-1; 

    for (;i >= 0;--i)   //decreasing iteration.
    {
        comp = m_designMain.m_vectorComponents[i];
        if (comp->m_item.m_wTileID == ITEMID_DIRT_TILE)
        {
            continue;
        }
        if (comp->m_item.m_dz >= iMinZ && comp->m_item.m_dz <= iMaxZ)
        {
            m_designMain.m_vectorComponents.erase(m_designMain.m_vectorComponents.begin()+i);
            m_designMain.m_iRevision++;
            continue;
        }
    }
    if (m_pSphereMulti != nullptr)
    {
        // multi object needs to be recreated
        delete m_pSphereMulti;
        m_pSphereMulti = nullptr;
    }

    // update to all
    Update();
}

enum
{
    IMCV_ADDITEM,
    IMCV_ADDMULTI,
    IMCV_CLEAR,
    IMCV_CLEARFLOOR,
    IMCV_COMMIT,
    IMCV_CUSTOMIZE,
    IMCV_ENDCUSTOMIZE,
    IMCV_REMOVEITEM,
    IMCV_RESET,
    IMCV_RESYNC,
    IMCV_REVERT,
    IMCV_QTY
};

lpctstr const CItemMultiCustom::sm_szVerbKeys[IMCV_QTY + 1] =
{
    "ADDITEM",
    "ADDMULTI",
    "CLEAR",
    "CLEARFLOOR",
    "COMMIT",
    "CUSTOMIZE",
    "ENDCUSTOMIZE",
    "REMOVEITEM",
    "RESET",
    "RESYNC",
    "REVERT",
    nullptr
};

bool CItemMultiCustom::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CItemMultiCustom::r_GetRef");
    if (!strnicmp("DESIGNER.", pszKey, 9))
    {
        pszKey += 9;
        pRef = (m_pArchitect ? m_pArchitect->GetChar() : nullptr);
        return true;
    }

    return CItemMulti::r_GetRef(pszKey, pRef);
}

bool CItemMultiCustom::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script
{
    ADDTOCALLSTACK("CItemMultiCustom::r_Verb");
    EXC_TRY("Verb");
    // Speaking in this multis region.
    // return: true = command for the multi.

    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1);
    if (iCmd < 0)
    {
        return CItemMulti::r_Verb(s, pSrc);
    }

    CChar * pChar = (pSrc != nullptr ? pSrc->GetChar() : nullptr);

    switch (iCmd)
    {
        case IMCV_ADDITEM:
        {
            tchar * ppArgs[4];
            size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs), ",");
            if (iQty != 4)
            {
                return false;
            }

            AddItem(nullptr,
                (ITEMID_TYPE)(Exp_GetVal(ppArgs[0])),
                (Exp_GetSVal(ppArgs[1])),
                (Exp_GetSVal(ppArgs[2])),
                (Exp_GetCVal(ppArgs[3])));
        }
        break;

        case IMCV_ADDMULTI:
        {
            tchar * ppArgs[4];
            size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs), ",");
            if (iQty != 4)
                return false;

            ITEMID_TYPE id = (ITEMID_TYPE)(Exp_GetVal(ppArgs[0]));
            if (id <= 0)
                return false;

            // if a multi is added that is not normally allowed through
            // design mode, the pieces need to be added individually
            AddStairs(nullptr,
                id,
                (Exp_GetSVal(ppArgs[1])),
                (Exp_GetSVal(ppArgs[2])),
                (Exp_GetCVal(ppArgs[3])),
                (sm_mapValidItems.find(id) == sm_mapValidItems.end() ? 0 : -1));
        }
        break;

        case IMCV_CLEAR:
        {
            m_designWorking.m_vectorComponents.clear();
            m_designWorking.m_iRevision++;
        }
        break;

        case IMCV_CLEARFLOOR:
        {
            char iFloor = s.GetArgCVal();
            if (iFloor == -1)
            {
                for (int i = 0; i < _iMaxPlane; ++i)
                {
                    ClearFloor((char)i);
                }
            }
            else
            {
                ClearFloor(s.GetArgCVal());
            }
        }
        break;

        case IMCV_COMMIT:
        {
            CommitChanges();
        }
        break;

        case IMCV_CUSTOMIZE:
        {
            if (s.HasArgs())
                pChar = CUID(s.GetArgVal()).CharFind();
            else if (pSrc)
                pChar = pSrc->GetChar();

            if (pChar == nullptr || !pChar->IsClient())
                return false;

            BeginCustomize(pChar->GetClient());
        }
        break;

        case IMCV_ENDCUSTOMIZE:
        {
            EndCustomize(true);
        }
        break;

        case IMCV_REMOVEITEM:
        {
            tchar * ppArgs[4];
            size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs), ",");
            if (iQty != 4)
                return false;

            RemoveItem(nullptr,
                (ITEMID_TYPE)(Exp_GetVal(ppArgs[0])),
                (Exp_GetSVal(ppArgs[1])),
                (Exp_GetSVal(ppArgs[2])),
                (Exp_GetCVal(ppArgs[3])));
        }
        break;

        case IMCV_RESET:
        {
            ResetStructure();
        }
        break;

        case IMCV_RESYNC:
        {
            if (s.HasArgs())
                pChar = CUID(s.GetArgVal()).CharFind();

            if (pChar == nullptr || !pChar->IsClient())
                return false;

            SendStructureTo(pChar->GetClient());
        }
        break;

        case IMCV_REVERT:
        {
            CopyDesign(&m_designMain, &m_designWorking);
            m_designWorking.m_iRevision++;
        }
        break;

        default:
        {
            return CItemMulti::r_Verb(s, pSrc);
        }
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return true;
}

void CItemMultiCustom::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemMultiCustom::r_Write");
    CItemMulti::r_Write(s);

    Component * comp;
    for (ComponentsContainer::iterator i = m_designMain.m_vectorComponents.begin(); i != m_designMain.m_vectorComponents.end(); ++i)
    {
        comp = *i;
        s.WriteKeyFormat("COMP", "%d,%d,%d,%d,%d", comp->m_item.GetDispID(), comp->m_item.m_dx, comp->m_item.m_dy, (char)(comp->m_item.m_dz), comp->m_isStair);
    }

    if (m_designMain.m_iRevision)
        s.WriteKeyVal("REVISION", m_designMain.m_iRevision);
}

enum IMCC_TYPE
{
    IMCC_COMPONENTS,
    IMCC_DESIGN,
    IMCC_DESIGNER,
    IMCC_EDITAREA,
    IMCC_FIXTURES,
    IMCC_REVISION,
    IMCC_QTY
};

lpctstr const CItemMultiCustom::sm_szLoadKeys[IMCC_QTY + 1] = // static
{
    "COMPONENTS",
    "DESIGN",
    "DESIGNER",
    "EDITAREA",
    "FIXTURES",
    "REVISION",
    nullptr
};

bool CItemMultiCustom::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemMultiCustom::r_WriteVal");
    EXC_TRY("WriteVal");

    int index = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (index == -1)
    {
        if (!strnicmp(pszKey, "DESIGN.", 5))
            index = IMCC_DESIGN;
    }

    switch (index)
    {
        case IMCC_COMPONENTS:
            pszKey += 10;
            sVal.FormatSTVal(m_designMain.m_vectorComponents.size());
            break;

        case IMCC_DESIGN:
        {
            pszKey += 6;
            if (!*pszKey)
                sVal.FormatSTVal(m_designMain.m_vectorComponents.size());
            else if (*pszKey == '.')
            {
                SKIP_SEPARATORS(pszKey);
                size_t iQty = Exp_GetSTVal(pszKey);
                if (iQty >= m_designMain.m_vectorComponents.size())
                    return false;

                SKIP_SEPARATORS(pszKey);
                CUOMultiItemRec_HS item = m_designMain.m_vectorComponents.at(iQty)->m_item;

                if (!strnicmp(pszKey, "ID", 2))		sVal.FormatVal(item.GetDispID());
                else if (!strnicmp(pszKey, "DX", 2))	sVal.FormatVal(item.m_dx);
                else if (!strnicmp(pszKey, "DY", 2))	sVal.FormatVal(item.m_dy);
                else if (!strnicmp(pszKey, "DZ", 2))	sVal.FormatVal(item.m_dz);
                else if (!strnicmp(pszKey, "D", 1))	sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
                else if (!strnicmp(pszKey, "FIXTURE", 7))		sVal.FormatVal(item.m_visible ? 0 : 1);
                else if (!*pszKey)				sVal.Format("%u,%i,%i,%i", item.GetDispID(), item.m_dx, item.m_dy, item.m_dz);
                else								return false;
            }
            else
                return false;

        } break;

        case IMCC_DESIGNER:
        {
            pszKey += 8;
            CChar * pDesigner = m_pArchitect ? m_pArchitect->GetChar() : nullptr;
            if (pDesigner != nullptr)
                sVal.FormatHex(pDesigner->GetUID());
            else
                sVal.FormatHex(0);

        } break;

        case IMCC_EDITAREA:
        {
            pszKey += 8;
            CRect rectDesign = GetDesignArea();
            sVal.Format("%d,%d,%d,%d", rectDesign.m_left, rectDesign.m_top, rectDesign.m_right, rectDesign.m_bottom);
        } break;

        case IMCC_FIXTURES:
            pszKey += 8;
            sVal.FormatSTVal(GetFixtureCount());
            break;

        case IMCC_REVISION:
            pszKey += 8;
            sVal.FormatVal(m_designMain.m_iRevision);
            break;

        default:
            return CItemMulti::r_WriteVal(pszKey, sVal, pSrc);
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

bool CItemMultiCustom::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemMultiCustom::r_LoadVal");
    EXC_TRY("LoadVal");

    if (g_Serv.IsLoading())
    {
        if (s.IsKey("COMP"))
        {
            tchar * ppArgs[5];
            size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs), ",");
            if (iQty != 5)
                return false;

            AddItem(nullptr,
                (ITEMID_TYPE)(ATOI(ppArgs[0])),
                (short)(ATOI(ppArgs[1])),
                (short)(ATOI(ppArgs[2])),
                (char)(ATOI(ppArgs[3])),
                (short)(ATOI(ppArgs[4])));
            return true;
        }
        else if (s.IsKey("REVISION"))
        {
            m_designWorking.m_iRevision = s.GetArgVal();
            CommitChanges();
            return true;
        }
    }
    return CItemMulti::r_LoadVal(s);
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

uchar CItemMultiCustom::GetPlane(char z)
{
    if (z >= 67)
        return 4;
    else if (z >= 47)
        return 3;
    else if (z >= 27)
        return 2;
    else if (z >= 7)
        return 1;
    else
        return 0;
}

uchar CItemMultiCustom::GetPlane(Component * pComponent)
{
    return GetPlane((char)(pComponent->m_item.m_dz));
}

char CItemMultiCustom::GetPlaneZ(uchar plane)
{
    return 7 + ((plane - 1) * 20);
}

bool CItemMultiCustom::IsValidItem(ITEMID_TYPE id, CClient * pClientSrc, bool bMulti)
{
    ADDTOCALLSTACK("CItemMultiCustom::IsValidItem");
    if (!bMulti && (id <= 0 || id >= ITEMID_MULTI))
        return false;
    if (bMulti && (id < ITEMID_MULTI || id > ITEMID_MULTI_MAX))
        return false;

    // GMs and scripts can place any item
    if (!pClientSrc || pClientSrc->IsPriv(PRIV_GM))
        return true;

    // load item lists
    if (!LoadValidItems())
        return false;

    // check the item exists in the database
    ValidItemsContainer::iterator it = sm_mapValidItems.find(id);
    if (it == sm_mapValidItems.end())
        return false;

    // check if client enabled features contains the item FeatureMask
    uint iFeatureFlag = g_Cfg.GetPacketFlag(false, (RESDISPLAY_VERSION)(pClientSrc->GetResDisp()));
    if ((iFeatureFlag & it->second) != it->second)
        return false;

    return true;
}

CItemMultiCustom::ValidItemsContainer CItemMultiCustom::sm_mapValidItems;

bool CItemMultiCustom::LoadValidItems()
{
    ADDTOCALLSTACK("CItemMultiCustom::LoadValidItems");
    if (!sm_mapValidItems.empty())	// already loaded?
        return true;

    static const char * sm_szItemFiles[][32] =
    {
        // list of files containing valid items
        { "doors.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", nullptr },
        { "misc.txt", "Piece1", "Piece2", "Piece3", "Piece4", "Piece5", "Piece6", "Piece7", "Piece8", nullptr },
        { "floors.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", nullptr },
        { "teleprts.txt", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "F13", "F14", "F15", "F16", nullptr },
        { "roof.txt", "North", "East", "South", "West", "NSCrosspiece", "EWCrosspiece", "NDent", "EDent", "SDent", "WDent", "NTPiece", "ETPiece", "STPiece", "WTPiece", "XPiece", "Extra Piece", nullptr },
        { "walls.txt", "South1", "South2", "South3", "Corner", "East1", "East2", "East3", "Post", "WindowS", "AltWindowS", "WindowE", "AltWindowE", "SecondAltWindowS", "SecondAltWindowE", nullptr },
        { "stairs.txt", "Block", "North", "East", "South", "West", "Squared1", "Squared2", "Rounded1", "Rounded2", nullptr },
        { nullptr },
        // list of files containing valid multis
        { "stairs.txt", "MultiNorth", "MultiEast", "MultiSouth", "MultiWest", nullptr },
        { nullptr }
    };

    CSVRowData csvDataRow;
    bool fMultiFile = false;
    int iFileIndex = 0;
    int i = 0;

    EXC_TRY("LoadCSVFiles");

    for (i = 0; sm_szItemFiles[i][0] != nullptr || !fMultiFile; ++i, ++iFileIndex)
    {
        if (sm_szItemFiles[i][0] == nullptr)
        {
            fMultiFile = true;
            --iFileIndex;
            continue;
        }

        CSVFile& curCSV = g_Install.m_CsvFiles[iFileIndex];
        if (!curCSV.IsFileOpen() && !g_Install.OpenFile(curCSV, sm_szItemFiles[i][0], OF_READ | OF_SHARE_DENY_WRITE))
            continue;

        while (curCSV.ReadNextRowContent(csvDataRow))
        {
            for (int ii = 1; sm_szItemFiles[i][ii] != nullptr; ++ii)
            {
                const std::string& strCurRow = csvDataRow[sm_szItemFiles[i][ii]];
                if (strCurRow.empty() || !IsDigit(strCurRow[0]))
                    continue;
                ITEMID_TYPE itemid = (ITEMID_TYPE)std::stoul(strCurRow, nullptr, 10);
                if (itemid <= 0 || itemid >= ITEMID_MULTI)
                    continue;

                if (fMultiFile)
                {
                    itemid = (ITEMID_TYPE)(itemid + ITEMID_MULTI);
                    if (itemid <= ITEMID_MULTI || itemid > ITEMID_MULTI_MAX)
                        continue;
                }

                const std::string& strFeatureMask = csvDataRow["FeatureMask"];
                if (strFeatureMask.empty())
                {
                    sm_mapValidItems[itemid] = 0;
                    DEBUG_MSG(("No FeatureMask in file '%s', row=%d.\n", sm_szItemFiles[i][0], curCSV.GetCurrentRow()));
                    continue;
                }
                sm_mapValidItems[itemid] = (uint)std::stoul(strFeatureMask, nullptr, 10);
            }
        }

        g_Install.m_CsvFiles[iFileIndex].Close();
    }

    // make sure we have at least 1 item
    sm_mapValidItems[ITEMID_NOTHING] = 0xFFFFFFFF;
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    g_Log.EventDebug("file index=%d\n", iFileIndex);
    g_Log.EventDebug("file name '%s'\n", sm_szItemFiles[i][0]);

    tchar* pszRowFull = Str_GetTemp();
    tchar* pszHeaderFull = Str_GetTemp();
    for (CSVRowData::iterator itCsv = csvDataRow.begin(), end = csvDataRow.end(); itCsv != end; ++itCsv)
    {
        strcat(pszHeaderFull, "\t");
        strcat(pszHeaderFull, itCsv->first.c_str());

        strcat(pszRowFull, "\t");
        strcat(pszRowFull, itCsv->second.c_str());
    }

    g_Log.EventDebug("header count '%" PRIuSIZE_T "', header text '%s'\n", csvDataRow.size(), pszRowFull);
    g_Log.EventDebug("column count '%" PRIuSIZE_T "', row text '%s'\n", csvDataRow.size(), pszHeaderFull);
    EXC_DEBUG_END;
    return false;
}

void CItemMultiCustom::CSphereMultiCustom::LoadFrom(CItemMultiCustom::DesignDetails * pDesign)
{
    m_iItemQty = (uint)pDesign->m_vectorComponents.size();

    m_pItems = new CUOMultiItemRec_HS[m_iItemQty];
    for (uint i = 0; i < m_iItemQty; ++i)
        memcpy(&m_pItems[i], &pDesign->m_vectorComponents[i]->m_item, sizeof(CUOMultiItemRec_HS));
}
