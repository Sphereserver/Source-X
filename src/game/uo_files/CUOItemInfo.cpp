/**
* @file CUOItemInfo.cpp
*
*/

#include "../../common/CUOInstall.h"
#include "../../game/items/CItemBase.h"
#include "CUOItemInfo.h"


CUOItemInfo::CUOItemInfo( ITEMID_TYPE id )
{
    if ( id >= ITEMID_MULTI )
    {
        // composite/multi type objects.
        m_flags = 0;
        m_weight = 0xFF;
        m_layer = LAYER_NONE;
        m_dwAnim = 0;
        m_height = 0;
        strcpy( m_name, CItemBase::IsID_Ship(id) ? "ship" : "structure" );
        return;
    }

    const CUOItemTypeRec_HS* cachedEntry = g_Install.m_tiledata.GetItemEntry(id);
    m_flags = cachedEntry->m_flags;
    m_dwUnk5 = cachedEntry->m_dwUnk5;
    m_weight = cachedEntry->m_weight;
    m_layer = cachedEntry->m_layer;
    m_dwUnk11 = cachedEntry->m_dwUnk11;
    m_dwAnim = cachedEntry->m_dwAnim;
    m_wUnk19 = cachedEntry->m_wUnk19;
    m_height = cachedEntry->m_height;
    Str_CopyLimitNull(m_name, cachedEntry->m_name, CountOf(m_name));
}