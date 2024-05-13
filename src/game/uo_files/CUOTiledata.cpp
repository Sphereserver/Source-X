/**
* @file CUOTiledata.cpp
*
*/

#include "../../sphere/threads.h"
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../../common/CUOInstall.h"
#include "CUOTiledata.h"


static ITEMID_TYPE GetMaxTiledataItem()
{
    ADDTOCALLSTACK("CUOTiledata::GetMaxTileDataItem");

    CSFile* pTileData = g_Install.GetMulFile(VERFILE_TILEDATA);
    ASSERT(pTileData != nullptr);

    dword dwLength = (dword)pTileData->GetLength();	// length of file
    dword dwEntrySize = 0;							// size of tiledata entry
    dword dwOffset = 0;								// offset to tiledata items

    VERFILE_FORMAT format = g_Install.GetMulFormat(VERFILE_TILEDATA);
    switch (format)
    {
        case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec_HS)
            dwEntrySize = sizeof(CUOItemTypeRec_HS);
            dwOffset = UOTILE_TERRAIN_SIZE2 + 4;
            break;

        case VERFORMAT_ORIGINAL: // original format (CUOItemTypeRec)
        default:
            dwEntrySize = sizeof(CUOItemTypeRec);
            dwOffset = UOTILE_TERRAIN_SIZE + 4;
            break;
    }

    ASSERT(dwEntrySize > 0);
    ASSERT(dwOffset < dwLength);

    // items are sorted in blocks of 32 with 4 byte padding between, so determine how
    // many blocks will fit in the file to find how many items there could be
    dwLength -= dwOffset;
    dword dwBlocks = (dwLength / ((UOTILE_BLOCK_QTY * dwEntrySize) + 4)) + 1;
    return (ITEMID_TYPE)(dwBlocks * UOTILE_BLOCK_QTY);
}


void CUOTiledata::Load()
{
    ADDTOCALLSTACK("CUOTiledata::Load");
    g_Log.Event(LOGM_INIT, "Caching tiledata.mul...\n");

    VERFILE_TYPE filedata;
    dword offset;
    CUOIndexRec Index;
    VERFILE_FORMAT format;

    // Cache the Tiledata Terrain entries

    uint idMax = (uint)TERRAIN_QTY;
    _tiledataTerrainEntries.clear();
    _tiledataTerrainEntries.resize(idMax);
    for (uint id = 0; id < idMax; ++id)
    {
        if ( g_VerData.FindVerDataBlock( VERFILE_TILEDATA, id/UOTILE_BLOCK_QTY, Index ))
        {
            filedata = VERFILE_VERDATA;
            format = VERFORMAT_ORIGINAL;
            offset = Index.GetFileOffset() + 4 + (sizeof(CUOTerrainTypeRec)*(id%UOTILE_BLOCK_QTY));
            ASSERT( Index.GetBlockLength() >= sizeof( CUOTerrainTypeRec ));
        }
        else
        {
            filedata = VERFILE_TILEDATA;
            format = g_Install.GetMulFormat(filedata);

            switch (format)
            {
                case VERFORMAT_HIGHSEAS: // high seas format (CUOTerrainTypeRec_HS)
                    offset = 4u + (( id / UOTILE_BLOCK_QTY ) * 4u ) + ( id * sizeof(CUOTerrainTypeRec_HS) );
                    break;

                case VERFORMAT_ORIGINAL: // original format (CUOTerrainTypeRec)
                default:
                    offset = 4u + (( id / UOTILE_BLOCK_QTY ) * 4u ) + ( id * sizeof(CUOTerrainTypeRec) );
                    break;
            }
        }

        if ( (uint)g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
        {
            throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.ReadInfo: TileData Seek");
        }

        switch (format)
        {
            case VERFORMAT_HIGHSEAS: // high seas format (CUOTerrainTypeRec_HS)
                if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec_HS *>(&(_tiledataTerrainEntries[id])), sizeof(CUOTerrainTypeRec_HS)) <= 0 )
                    throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.ReadInfo: TileData Read");
                break;

            case VERFORMAT_ORIGINAL: // old format (CUOTerrainTypeRec)
            default:
            {
                CUOTerrainTypeRec record;
                if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec *>(&record), sizeof(CUOTerrainTypeRec)) <= 0 )
                    throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.ReadInfo: TileData Read");

                CUOTerrainTypeRec_HS* cachedEntry = &_tiledataTerrainEntries[id];
                cachedEntry->m_flags = record.m_flags;
                cachedEntry->m_unknown = 0;
                cachedEntry->m_index = record.m_index;
                Str_CopyLimitNull(cachedEntry->m_name, record.m_name, ARRAY_COUNT(cachedEntry->m_name));
                break;
            }
        }
    }


    // Cache the Tiledata Item entries

    idMax = (uint)GetMaxTiledataItem();
    _tiledataItemEntries.clear();
    _tiledataItemEntries.resize(idMax);
    for (uint id = 0; id < idMax; ++id)
    {
        if ( g_VerData.FindVerDataBlock( VERFILE_TILEDATA, (id + TERRAIN_QTY) / UOTILE_BLOCK_QTY, Index ) )
        {
            filedata = VERFILE_VERDATA;
            format = VERFORMAT_ORIGINAL;
            offset = Index.GetFileOffset() + 4 + (sizeof(CUOItemTypeRec) * (id % UOTILE_BLOCK_QTY));
            ASSERT( Index.GetBlockLength() >= sizeof(CUOItemTypeRec) );
        }
        else
        {
            filedata = VERFILE_TILEDATA;
            format = g_Install.GetMulFormat(filedata);
            switch (format)
            {
                case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec_HS)
                    offset = UOTILE_TERRAIN_SIZE2 + 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof(CUOItemTypeRec_HS));
                    break;

                case VERFORMAT_ORIGINAL: // original format (CUOItemTypeRec)
                default:
                    offset = UOTILE_TERRAIN_SIZE + 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof(CUOItemTypeRec));
                    break;
            }
        }

        if ( (uint)g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
        {
            throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.Load: TileData Seek");
        }

        switch (format)
        {
            case VERFORMAT_HIGHSEAS: // high seas format (CUOItemTypeRec_HS)
                if ( g_Install.m_File[filedata].Read( static_cast<CUOItemTypeRec_HS*>(&(_tiledataItemEntries[id])), sizeof(CUOItemTypeRec_HS)) <= 0 )
                    throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.Load: TileData Read");
                break;

            case VERFORMAT_ORIGINAL: // old format (CUOItemTypeRec)
            default:
            {
                CUOItemTypeRec record;
                if ( g_Install.m_File[filedata].Read( static_cast <CUOItemTypeRec *>(&record), sizeof(CUOItemTypeRec)) <= 0 )
                    throw CSError(LOGL_CRIT, CSFile::GetLastError(), "CUOTiledata.Item.Load: TileData Read");

                CUOItemTypeRec_HS* cachedEntry = &_tiledataItemEntries[id];
                cachedEntry->m_flags = record.m_flags;
                cachedEntry->m_weight = record.m_weight;
                cachedEntry->m_layer = record.m_layer;
                cachedEntry->m_dwUnk11 = record.m_dwUnk6;
                cachedEntry->m_wAnim = record.m_wAnim;
                cachedEntry->m_wHue = record.m_wHue;
                cachedEntry->m_wLightIndex = record.m_wLightIndex;
                cachedEntry->m_height = record.m_height;
                Str_CopyLimitNull(cachedEntry->m_name, record.m_name, ARRAY_COUNT(cachedEntry->m_name));
                break;
            }
        }
    }
}

