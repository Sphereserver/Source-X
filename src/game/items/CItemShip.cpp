//
// CItemShip.cpp
//

#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../chars/CChar.h"
#include "../triggers.h"
#include "CItemContainer.h"
#include "CItemShip.h"
#include "CItem.h"

/////////////////////////////////////////////////////////////////////////////


CItemShip::CItemShip(ITEMID_TYPE id, CItemBase * pItemDef) :
    CCTimedObject(PROFILE_SHIPS),
    CItemMulti(id, pItemDef, true)
{
}

CItemShip::~CItemShip()
{
}

bool CItem::Ship_Plank(bool fOpen)
{
    ADDTOCALLSTACK("CItem::Plank");
    // IT_PLANK to IT_SIDE and IT_SIDE_LOCKED
    // This item is the ships plank.

    CItemBase * pItemDef = Item_GetDef();
    ITEMID_TYPE idState = (ITEMID_TYPE)pItemDef->m_ttShipPlank.m_ridState.GetResIndex();
    if (!idState)
        return false;

    if (IsType(IT_SHIP_PLANK))
    {
        if (fOpen)
            return true;
    }
    else
    {
        if (!fOpen)
            return true;
    }

    IT_TYPE oldType = GetType();
    SetID(idState);

    if (IsType(IT_SHIP_PLANK) && (oldType == IT_SHIP_SIDE || oldType == IT_SHIP_SIDE_LOCKED))
    {
        // Save the original Type of the plank if it used to be a ship side
        m_itShipPlank.m_itSideType = (word)oldType;
    }
    else if (oldType == IT_SHIP_PLANK)
    {
        // Restore the type of the ship side
        if (m_itShipPlank.m_itSideType == IT_SHIP_SIDE || m_itShipPlank.m_itSideType == IT_SHIP_SIDE_LOCKED)
            SetType(static_cast<IT_TYPE>(m_itShipPlank.m_itSideType));

        m_itShipPlank.m_itSideType = IT_NORMAL;
    }

    Update();
    return true;
}

bool CItemShip::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CItemShip::r_GetRef");

    if (!strnicmp(pszKey, "HATCH.", 6))
    {
        pszKey += 6;
        pRef = GetShipHold();
        return true;
    }
    else if (!strnicmp(pszKey, "TILLER.", 7))
    {
        pszKey += 7;
        pRef = Multi_GetSign();
        return true;
    }
    else if (!strnicmp(pszKey, "PLANK.", 6))
    {
        pszKey += 6;
        int i = Exp_GetVal(pszKey);
        SKIP_SEPARATORS(pszKey);
        pRef = GetShipPlank(i);
        return true;
    }

    return(CItemMulti::r_GetRef(pszKey, pRef));
}

bool CItemShip::r_Verb(CScript & s, CTextConsole * pSrc) // Execute command from script
{
    return CItemMulti::r_Verb(s, pSrc);

}

void CItemShip::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemShip::r_Write");
    CItemMulti::r_Write(s);
    if (m_uidHold)
        s.WriteKeyHex("HATCH", m_uidHold);
    if (GetShipPlankCount() > 0)
    {
        for (size_t i = 0; i < m_uidPlanks.size(); i++)
            s.WriteKeyHex("PLANK", m_uidPlanks.at(i));
    }
}

enum IMCS_TYPE
{
    IMCS_HATCH,
    IMCS_PLANK,
    IMCS_TILLER,
    IMCS_QTY
};

lpctstr const CItemShip::sm_szLoadKeys[IMCS_QTY + 1] = // static
{
    "HATCH",
    "PLANK",
    "TILLER",
    nullptr
};

bool CItemShip::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CItemShip::r_WriteVal");
    EXC_TRY("WriteVal");

    int index = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);

    switch (index)
    {
        case IMCS_HATCH:
        {
            pszKey += 5;
            CItem * pItemHold = GetShipHold();
            if (pItemHold)
                sVal.FormatHex(pItemHold->GetUID());
            else
                sVal.FormatVal(0);
        } break;

        case IMCS_PLANK:
        {
            pszKey += 6;
            sVal.FormatSTVal(GetShipPlankCount());
        } break;

        case IMCS_TILLER:
        {
            pszKey += 6;
            CItem * pTiller = Multi_GetSign();
            if (pTiller)
                sVal.FormatHex(pTiller->GetUID());
            else
                sVal.FormatVal(0);
        } break;

        default:
            return(CItemMulti::r_WriteVal(pszKey, sVal, pSrc));
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

bool CItemShip::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemShip::r_LoadVal");
    EXC_TRY("LoadVal");
    lpctstr	pszKey = s.GetKey();
    IMCS_TYPE index = (IMCS_TYPE)FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (g_Serv.IsLoading())
    {
        switch (index)
        {
            case IMCS_HATCH:
            {
                m_uidHold = s.GetArgDWVal();
                return true;
            }
            case IMCS_TILLER:
            {
                m_uidLink = s.GetArgDWVal();
                return true;
            }
            case IMCS_PLANK:
            {
                m_uidPlanks.emplace_back(s.GetArgDWVal());
                return true;
            }
            default:
            {
                break;
            }
        }
    }
    return CItemMulti::r_LoadVal(s);

    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

int CItemShip::FixWeirdness()
{
    ADDTOCALLSTACK("CItemShip::FixWeirdness");
    int iResultCode = CItemMulti::FixWeirdness();
    if (iResultCode)
    {
        return iResultCode;
    }

    // CItemShip::GetShipHold() updates/corrects the hold uid
    GetShipHold();

    // CItemShip::GetShipPlank() updates/corrects the list of planks
    GetShipPlank(0);
    return iResultCode;
}

bool CItemShip::OnTick()
{
    CCMultiMovable::OnTick();
    return true;    // Ships always return true, can't 'decay'
}

CItemContainer * CItemShip::GetShipHold()
{
    ADDTOCALLSTACK("CItemShip::GetShipHold");
    CItem * pItem = m_uidHold.ItemFind();
    if (!pItem || pItem->IsDeleted())
    {
        pItem = Multi_FindItemType(IT_SHIP_HOLD);
        if (!pItem || pItem->IsDeleted())
            pItem = Multi_FindItemType(IT_SHIP_HOLD_LOCK);

        if (!pItem || pItem->IsDeleted())
            return nullptr;

        m_uidHold = pItem->GetUID();
    }

    CItemContainer * pItemHold = dynamic_cast<CItemContainer *>(pItem);
    if (!pItemHold)
        return nullptr;

    return pItemHold;
}

size_t CItemShip::GetShipPlankCount()
{
    ADDTOCALLSTACK("CItemShip::GetShipPlankCount");
    // CItemShip::GetShipPlank() updates the list of planks, so
    // calling this first will get an accurate result
    GetShipPlank(0);
    return m_uidPlanks.size();
}

CItem * CItemShip::GetShipPlank(size_t index)
{
    ADDTOCALLSTACK("CItemShip::GetShipPlank");
    // Check the current list of planks is valid
    for (const CUID& curUID : m_uidPlanks)
    {
        const CItem * pItem = curUID.ItemFind();
        if (pItem && Multi_IsPartOf(pItem))
            continue;

        // If an invalid plank uid was found, then wipe the whole list
        // and rebuild it
        m_uidPlanks.clear();
        break;
    }

    // Find plank(s) if the list is empty
    if (m_uidPlanks.empty())
    {
        CWorldSearch Area(GetTopPoint(), Multi_GetMaxDist());
        for (;;)
        {
            const CItem * pItem = Area.GetItem();
            if (pItem == nullptr)
                break;

            if (pItem->IsDeleted())
                continue;

            if (!Multi_IsPartOf(pItem))
                continue;

            if (pItem->IsType(IT_SHIP_PLANK) || pItem->IsType(IT_SHIP_SIDE) || pItem->IsType(IT_SHIP_SIDE_LOCKED))
                m_uidPlanks.emplace_back(pItem->GetUID());
        }
    }

    if (index >= m_uidPlanks.size())
        return nullptr;

    return m_uidPlanks[index].ItemFind();
}

void CItemShip::OnComponentCreate(CItem * pComponent)
{
    ADDTOCALLSTACK("CItemShip::OnComponentCreate");
    switch (pComponent->GetType())
    {
        case IT_SHIP_TILLER:
            // Tillerman is already stored as m_uidLink (Multi_GetSign())
            break;
        case IT_SHIP_HOLD:
        case IT_SHIP_HOLD_LOCK:
            m_uidHold = pComponent->GetUID();
            break;
        case IT_SHIP_PLANK:
        case IT_SHIP_SIDE:
        case IT_SHIP_SIDE_LOCKED:
            m_uidPlanks.emplace_back(pComponent->GetUID());
            break;

        default:
            break;
    }

    CItemMulti::OnComponentCreate(pComponent);
    return;
}

