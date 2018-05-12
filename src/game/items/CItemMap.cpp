
#include "../../common/CException.h"
#include "CItemMap.h"

/////////////////////////////////////////////////////////////////////////////
// -CItemMap

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
            m_Pins.push_back(pin);
            return true;
        }
        return CItem::r_LoadVal(s);
    EXC_CATCH;

    EXC_DEBUG_START;
            EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CItemMap::r_WriteVal(lpctstr pszKey, CSString &sVal, CTextConsole *pSrc)
{
    ADDTOCALLSTACK("CItemMap::r_WriteVal");
    EXC_TRY("WriteVal");
        if ( !strnicmp(pszKey, "PINS", 4) )
        {
            sVal.FormatSTVal(m_Pins.size());
            return true;
        }
        if ( !strnicmp(pszKey, "PIN.", 4) )
        {
            pszKey += 4;
            size_t i = Exp_GetVal(pszKey) - 1;
            if ( m_Pins.IsValidIndex(i) )
            {
                sVal.Format("%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
                return true;
            }
        }
        return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
    EXC_CATCH;

    EXC_DEBUG_START;
            EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

void CItemMap::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemMap::r_Write");
    CItemVendable::r_Write(s);
    for ( size_t i = 0; i < m_Pins.size(); i++ )
        s.WriteKeyFormat("PIN", "%i,%i", m_Pins[i].m_x, m_Pins[i].m_y);
}

void CItemMap::DupeCopy(const CItem *pItem)
{
    ADDTOCALLSTACK("CItemMap::DupeCopy");
    CItemVendable::DupeCopy(pItem);

    const CItemMap *pMapItem = dynamic_cast<const CItemMap *>(pItem);
    if ( pMapItem == NULL )
        return;
    m_Pins = pMapItem->m_Pins;
}

CItemMap::CItemMap( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef ) {
    m_fPlotMode = false;
}

CItemMap::~CItemMap() {
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
}
