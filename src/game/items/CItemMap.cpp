
#include "../../common/CException.h"
#include "CItemMap.h"

/////////////////////////////////////////////////////////////////////////////
// -CItemMap

CItemMap::CItemMap( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
    CItemVendable( id, pItemDef )
{
    m_fPlotMode = false;
}

CItemMap::~CItemMap()
{
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
}

bool CItemMap::IsSameType(const CObjBase *pObj) const
{
    const CItemMap *pItemMap = dynamic_cast<const CItemMap *>(pObj);
    if ( pItemMap )
    {
        // Maps can only stack on top of each other if the pins match
        if (m_Pins.size() != pItemMap->m_Pins.size() )
            return false;

        // Check individual pins in the same place
        for ( size_t i = 0; i < m_Pins.size(); i++ )
        {
            if ( m_Pins[i].m_x != pItemMap->m_Pins[i].m_x )
                return false;
            if ( m_Pins[i].m_y != pItemMap->m_Pins[i].m_y )
                return false;
        }
    }

    return CItemVendable::IsSameType(pObj);
}

bool CItemMap::r_LoadVal(CScript & s)	// load an item script
{
    ADDTOCALLSTACK("CItemMap::r_LoadVal");
    EXC_TRY("LoadVal");
        if ( s.IsKeyHead("PIN", 3) )
        {
            CPointMap pntTemp;
            pntTemp.Read(s.GetArgStr());
            CMapPinRec pin(pntTemp.m_x, pntTemp.m_y);
            m_Pins.emplace_back(pin);
            return true;
        }
        return CItem::r_LoadVal(s);
    EXC_CATCH;

    EXC_DEBUG_START;
            EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CItemMap::r_WriteVal(lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CItemMap::r_WriteVal");
    EXC_TRY("WriteVal");
        if ( !strnicmp(ptcKey, "PINS", 4) )
        {
            sVal.FormatSTVal(m_Pins.size());
            return true;
        }
        if ( !strnicmp(ptcKey, "PIN.", 4) )
        {
            ptcKey += 4;
            uint i = Exp_GetUVal(ptcKey) - 1;
            if ( m_Pins.IsValidIndex(i) )
            {
                sVal.Format("%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
                return true;
            }
        }
        return (fNoCallParent ? false : CItemVendable::r_WriteVal(ptcKey, sVal, pSrc));
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

void CItemMap::r_Write(CScript & s)
{
    ADDTOCALLSTACK_DEBUG("CItemMap::r_Write");
    CItemVendable::r_Write(s);
    for ( size_t i = 0; i < m_Pins.size(); i++ )
        s.WriteKeyFormat("PIN", "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
}

void CItemMap::DupeCopy(const CObjBase *pItemObj)
{
    ADDTOCALLSTACK("CItemMap::DupeCopy");
    auto pItem = dynamic_cast<const CItem*>(pItemObj);
    ASSERT(pItem);

    CItemVendable::DupeCopy(pItem);

    const CItemMap *pMapItem = dynamic_cast<const CItemMap *>(pItem);
    if ( pMapItem == nullptr )
        return;
    m_Pins = pMapItem->m_Pins;
}

