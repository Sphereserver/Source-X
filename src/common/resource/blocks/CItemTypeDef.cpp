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
        size_t iLo = Exp_GetVal( pszArgs );
        GETNONWHITESPACE( pszArgs );

        if ( *pszArgs == ',' )
        {
            pszArgs++;
            GETNONWHITESPACE( pszArgs );
        }

        size_t iHi;
        if ( *pszArgs == '\0' )
            iHi	= iLo;
        else
            iHi	= Exp_GetVal( pszArgs );

        if ( iLo > iHi )		// swap
        {
            size_t iTmp = iHi;
            iHi	= iLo;
            iLo	= iTmp;
        }

        for ( size_t i = iLo; i <= iHi; i++ )
        {
            g_World.m_TileTypes.assign_at_grow(i, this);
        }
        return true;
    }
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}
