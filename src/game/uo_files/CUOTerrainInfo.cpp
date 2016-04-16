
#include "../../common/CException.h"
#include "../../common/CUOInstall.h"
#include "../CLog.h"
#include "CUOIndexRec.h"
#include "CUOTerrainInfo.h"
#include "CUOTerrainTypeRec.h"
#include "uofiles_enums.h"
#include "uofiles_macros.h"

CSphereTerrainInfo::CSphereTerrainInfo( TERRAIN_TYPE id )
{
    ASSERT( id < TERRAIN_QTY );

    VERFILE_TYPE filedata;
    dword offset;
    CUOIndexRec Index;
    VERFILE_FORMAT format;
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
                offset = (id == 0? 0 : 4) + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOTerrainTypeRec_HS ));
                break;

            case VERFORMAT_ORIGINAL: // original format (CUOTerrainTypeRec)
            default:
                offset = 4 + (( id / UOTILE_BLOCK_QTY ) * 4 ) + ( id * sizeof( CUOTerrainTypeRec ));
                break;
        }
    }

    if ( g_Install.m_File[filedata].Seek( offset, SEEK_SET ) != offset )
    {
        throw CSphereError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Seek");
    }

    switch (format)
    {
        case VERFORMAT_HIGHSEAS: // high seas format (CUOTerrainTypeRec_HS)
            if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec_HS *>(this), sizeof(CUOTerrainTypeRec_HS)) <= 0 )
                throw CSphereError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");
            break;

        case VERFORMAT_ORIGINAL: // old format (CUOTerrainTypeRec)
        default:
        {
            CUOTerrainTypeRec record;
            if ( g_Install.m_File[filedata].Read(static_cast <CUOTerrainTypeRec *>(&record), sizeof(CUOTerrainTypeRec)) <= 0 )
                throw CSphereError(LOGL_CRIT, CGFile::GetLastError(), "CTileTerrainType.ReadInfo: TileData Read");

            m_flags = record.m_flags;
            m_unknown = 0;
            m_index = record.m_index;
            strcpylen(m_name, record.m_name, COUNTOF(m_name));
            break;
        }
    }
}
