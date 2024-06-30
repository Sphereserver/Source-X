/**
* @file CUOTerrainInfo.cpp
*
*/

#include "../../common/CUOInstall.h"
#include "../../common/CLog.h"
#include "CUOTerrainInfo.h"

CUOTerrainInfo::CUOTerrainInfo( TERRAIN_TYPE id, bool fNameNeeded )
{
    if (id >= TERRAIN_QTY)
    {
        g_Log.EventError("Trying to read data for a terrain with invalid id 0x%" PRIx16 ".\n", id);
        return;
    }

    const CUOTerrainTypeRec_HS* cachedEntry = g_Install.m_tiledata.GetTerrainEntry(id);
    m_flags = cachedEntry->m_flags;
    m_unknown = cachedEntry->m_unknown;
    m_index = cachedEntry->m_index;

    if (fNameNeeded)
    {
        // Gotta go fast
        memcpy(m_name, cachedEntry->m_name, 20);
        //m_name[19] = '\0';    // This is not necessary because the cached entry already is zero terminated.
        //Str_CopyLimitNull(m_name, cachedEntry->m_name, ARRAY_COUNT(m_name));
    }
    else
    {
        m_name[0] = '\0';
    }
}
