/**
* @file CUOTerrainInfo.cpp
*
*/

#include "../../common/CUOInstall.h"
#include "CUOTerrainInfo.h"

CUOTerrainInfo::CUOTerrainInfo( TERRAIN_TYPE id )
{
    ASSERT( id < TERRAIN_QTY );

    const CUOTerrainTypeRec_HS* cachedEntry = g_Install.m_tiledata.GetTerrainEntry(id);
    m_flags = cachedEntry->m_flags;
    m_unknown = cachedEntry->m_unknown;
    m_index = cachedEntry->m_index;
    strncpynull(m_name, cachedEntry->m_name, CountOf(m_name));    
}
