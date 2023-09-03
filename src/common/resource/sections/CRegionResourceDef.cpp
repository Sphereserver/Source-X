#include "../../../sphere/threads.h"
#include "../../../game/items/CItemBase.h"
#include "../../../game/CServerConfig.h"
#include "../../CException.h"
#include "../CResourceLock.h"
#include "CRegionResourceDef.h"

enum RMC_TYPE
{
    RMC_AMOUNT,
    RMC_DEFNAME,
    RMC_REAP,
    RMC_REAPAMOUNT,
    RMC_REGEN,
    RMC_SKILL,
    RMC_QTY
};

lpctstr const CRegionResourceDef::sm_szLoadKeys[RMC_QTY+1] =
{
    "AMOUNT",
    "DEFNAME",
    "REAP",
    "REAPAMOUNT",
    "REGEN",
    "SKILL",
    nullptr
};

lpctstr const CRegionResourceDef::sm_szTrigName[RRTRIG_QTY+1] =	// static
{
    "@AAAUNUSED",
    "@RESOURCEFOUND",
    "@RESOURCEGATHER",
    "@RESOURCETEST",
    nullptr
};


TRIGRET_TYPE CRegionResourceDef::OnTrigger( lpctstr pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
    ADDTOCALLSTACK("CRegionResourceDef::OnTrigger");
    // Attach some trigger to the cchar. (PC or NPC)
    // RETURN: true = block further action.

    CResourceLock s;
    if ( ResourceLock( s ))
    {
        TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pSrc, pArgs );
        return iRet;
    }
    return TRIGRET_RET_DEFAULT;
}

bool CRegionResourceDef::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CRegionResourceDef::r_LoadVal");
    EXC_TRY("LoadVal");
    // RES_REGIONRESOURCE
    switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
    {
        case RMC_AMOUNT: // AMOUNT
            m_vcAmount.Load( s.GetArgRaw() );
            break;
        case RMC_DEFNAME: // "DEFNAME",
            return SetResourceName( s.GetArgStr());
        case RMC_REAP: // "REAP",
            m_ReapItem = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
            break;
        case RMC_REAPAMOUNT:
            m_vcReapAmount.Load( s.GetArgRaw() );
            break;
        case RMC_REGEN:	// Tenths of second once found how long to regen this type.
            m_vcRegenerateTime.Load( s.GetArgRaw() );
            break;
        case RMC_SKILL:
            m_vcSkill.Load( s.GetArgRaw() );
            break;
        default:
            return( CResourceDef::r_LoadVal( s ));
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CRegionResourceDef::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CRegionResourceDef::r_WriteVal");
    EXC_TRY("r_WriteVal");
    // RES_REGIONRESOURCE
    switch ( FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
    {
        case RMC_AMOUNT:
            sVal = m_vcAmount.Write();
            break;
        case RMC_REAP: // "REAP",
        {
            CItemBase * pItemDef = CItemBase::FindItemBase(m_ReapItem);
            if ( !pItemDef )
            {
                return false;
            }

            sVal = pItemDef->GetResourceName();
        } break;
        case RMC_REAPAMOUNT:
            sVal = m_vcReapAmount.Write();
            break;
        case RMC_REGEN:
            sVal = m_vcRegenerateTime.Write();
            break;
        case RMC_SKILL:
            sVal = m_vcSkill.Write();
            break;
        default:
            return ( fNoCallParent ? false : CResourceDef::r_WriteVal( ptcKey, sVal, pSrc ) );
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

CRegionResourceDef::CRegionResourceDef( CResourceID rid ) :
    CResourceLink( rid )
{
    // set defaults first.
    m_ReapItem = ITEMID_NOTHING;
}

CRegionResourceDef::~CRegionResourceDef()
{
}
