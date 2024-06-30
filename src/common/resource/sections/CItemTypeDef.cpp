#include "../../../game/CServerConfig.h"
#include "../../../game/CWorld.h"
#include "../../../sphere/threads.h"
#include "../../CException.h"
#include "../../CExpression.h"
#include "CItemTypeDef.h"

int CItemTypeDef::GetItemType() const
{
    ADDTOCALLSTACK("CItemTypeDef::GetItemType");
    return (int)(GetResourceID().GetPrivateUID() & 0xFFFF);
}

bool CItemTypeDef::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CItemTypeDef::r_LoadVal");
    EXC_TRY("LoadVal");
    lpctstr		ptcKey	= s.GetKey();
    lpctstr		pszArgs	= s.GetArgStr();

    if ( !strnicmp( ptcKey, "TERRAIN", 7 ) )
    {
        llong iLo = Exp_GetLLVal( pszArgs );
        if (iLo < 0)
        {
            g_Log.EventError("TERRAIN lower ID has negative value.\n");
            return false;
        }

        GETNONWHITESPACE( pszArgs );
        if ( *pszArgs == ',' )
        {
            ++pszArgs;
            GETNONWHITESPACE( pszArgs );
        }

        llong iHi;
        if ( *pszArgs == '\0' )
            iHi	= iLo;
        else
            iHi	= Exp_GetLLVal( pszArgs );
        if (iHi < 0)
        {
            g_Log.EventError("TERRAIN upper ID has negative value.\n");
            return false;
        }

        if ( iLo > iHi )		// swap
        {
            llong iTmp = iHi;
            iHi	= iLo;
            iLo	= iTmp;
        }

        if (iHi > UINT32_MAX)
        {
            g_Log.EventError("TERRAIN with greatest ID 0x%" PRIx64 " (> 0xFFFF FFFF)?\n", (int64)iHi);
            return false;
        }

        // Get the weak ptr from reshash
        sl::smart_ptr_view<CResourceDef> def_registered = g_Cfg.RegisteredResourceGetDefRef(GetResourceID());
        for (llong i = iLo; i <= iHi; ++i )
        {
            g_World.m_TileTypes.emplace_index_grow((size_t)i, def_registered);
        }
        return true;
    }
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}
