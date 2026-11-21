#include "../../../sphere/threads.h"
#include "../../../game/items/CItemBase.h"
#include "../../../game/CServerConfig.h"
//#include "../../CException.h" // included in the precompiled header
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


TRIGRET_TYPE CRegionResourceDef::OnTrigger(lpctstr pszTrigName, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CRegionResourceDef::OnTrigger");
    // Attach some trigger to the cchar. (PC or NPC)
    // RETURN: true = block further action.

    CResourceLock s;
    if ( ResourceLock( s ))
    {
        TRIGRET_TYPE iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pScriptArgs, pSrc);
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
        {
            // A random choice ({item 1 item 2 ...}) would be evaluated only here, once per script load (startup or resync),
            //  giving always the same item until the next load: keep the expression to pick again on every gather.
            lpctstr ptcReap = s.GetArgStr();
            m_sReapExpr = (strchr(ptcReap, '{') != nullptr) ? ptcReap : "";
            m_ReapItem = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ptcReap));
            break;
        }
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

ITEMID_TYPE CRegionResourceDef::GetReapItem() const
{
    ADDTOCALLSTACK("CRegionResourceDef::GetReapItem");
    if (m_sReapExpr.IsEmpty())
        return m_ReapItem;

    const int iIndex = g_Cfg.ResourceGetIndexType(RES_ITEMDEF, m_sReapExpr.GetBuffer());
    if (iIndex < 0)
        return m_ReapItem;
    return (ITEMID_TYPE)iIndex;
}
