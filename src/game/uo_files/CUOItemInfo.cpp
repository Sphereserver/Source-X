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
        m_wAnim = 0;
        m_wHue = 0;
        m_wLightIndex = 0;
        m_height = 0;
        strcpy( m_name, CItemBase::IsID_Ship(id) ? "ship" : "structure" );
        return;
    }

    const CUOItemTypeRec_HS* cachedEntry = g_Install.m_tiledata.GetItemEntry(id);
    m_flags = cachedEntry->m_flags;
    m_weight = cachedEntry->m_weight;
    m_layer = cachedEntry->m_layer;
    m_dwUnk11 = cachedEntry->m_dwUnk11;
    m_wAnim = cachedEntry->m_wAnim;
    m_wHue = cachedEntry->m_wHue;
    m_wLightIndex = cachedEntry->m_wLightIndex;
    m_height = cachedEntry->m_height;
    Str_CopyLimitNull(m_name, cachedEntry->m_name, ARRAY_COUNT(m_name));
}