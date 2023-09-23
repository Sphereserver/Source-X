#include "../../../sphere/threads.h"
#include "../../../game/CServerConfig.h"
#include "../../CException.h"
#include "CSkillClassDef.h"

enum SCC_TYPE
{
    SCC_DEFNAME,
    SCC_NAME,
    SCC_SKILLSUM,
    SCC_STATSUM,
    SCC_QTY
};

lpctstr const CSkillClassDef::sm_szLoadKeys[SCC_QTY+1] =
{
    "DEFNAME",
    "NAME",
    "SKILLSUM",
    "STATSUM",
    nullptr
};

void CSkillClassDef::Init()
{
    ADDTOCALLSTACK("CSkillClassDef::Init");
    m_SkillSumMax = 10*1000;
    m_StatSumMax = 300;
    size_t i;
    for ( i = 0; i < ARRAY_COUNT(m_SkillLevelMax); ++i )
    {
        m_SkillLevelMax[i] = 1000;
    }
    for ( i = 0; i < ARRAY_COUNT(m_StatMax); ++i )
    {
        m_StatMax[i] = 100;
    }
}

bool CSkillClassDef::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CSkillClassDef::r_WriteVal");
    EXC_TRY("WriteVal");
    switch ( FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
    {
        case SCC_NAME: // "NAME"
            sVal = m_sName;
            break;
        case SCC_SKILLSUM:
            sVal.FormatUVal( m_SkillSumMax );
            break;
        case SCC_STATSUM:
            sVal.FormatUVal( m_StatSumMax );
            break;
        default:
        {
            int i = g_Cfg.FindSkillKey(ptcKey);
            if ( i != SKILL_NONE )
            {
                ASSERT( (i >= 0) && (uint(i) < ARRAY_COUNT(m_SkillLevelMax)) );
                sVal.FormatUSVal( m_SkillLevelMax[i] );
                break;
            }
            i = g_Cfg.GetStatKey( ptcKey);
            if ( i >= 0 )
            {
                ASSERT( uint(i) < ARRAY_COUNT(m_StatMax));
                sVal.FormatUSVal( m_StatMax[i] );
                break;
            }
        }
        return ( fNoCallParent ? false : CResourceDef::r_WriteVal( ptcKey, sVal, pSrc ) );
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}


bool CSkillClassDef::r_LoadVal( CScript &s )
{
    ADDTOCALLSTACK("CSkillClassDef::r_LoadVal");
    EXC_TRY("LoadVal");
    switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
    {
        case SCC_DEFNAME:
            return SetResourceName( s.GetArgStr());
        case SCC_NAME:
            m_sName = s.GetArgStr();
            break;
        case SCC_SKILLSUM:
            m_SkillSumMax = s.GetArgUVal();
            break;
        case SCC_STATSUM:
            m_StatSumMax = s.GetArgUVal();
            break;
        default:
        {
            lpctstr ptcKey = s.GetKey();
            int i = g_Cfg.FindSkillKey(ptcKey);
            if ( i != SKILL_NONE )
            {
                ASSERT( (i >= 0) && (uint(i) < ARRAY_COUNT(m_SkillLevelMax)) );
                m_SkillLevelMax[i] = s.GetArgUSVal();
                break;
            }
            i = g_Cfg.GetStatKey(ptcKey);
            if ( i >= 0 )
            {
                ASSERT( (uint)i < ARRAY_COUNT(m_StatMax));
                m_StatMax[i] = s.GetArgUSVal();
                break;
            }
        }
        return CResourceDef::r_LoadVal( s );
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}
