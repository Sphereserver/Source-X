#include "../../../game/CServerConfig.h"
#include "../../../game/game_macros.h"
#include "../../../game/uo_files/uofiles_enums.h"
#include "../../CException.h"
#include "../../CExpression.h"
#include "CSpellDef.h"


lpctstr const CSpellDef::sm_szTrigName[SPTRIG_QTY+1] =
{
    "@AAAUNUSED",
    "@EFFECT",
    "@EFFECTADD",
    "@EFFECTREMOVE",
    "@EFFECTTICK",
    "@FAIL",
    "@SELECT",
    "@START",
    "@SUCCESS",
    "@TARGETCANCEL",
    nullptr
};

enum SPC_TYPE
{
    SPC_CAST_TIME,
    SPC_DEFNAME,
    SPC_DURATION,
    SPC_EFFECT,
    SPC_EFFECT_ID,
    SPC_FLAGS,
    SPC_GROUP,
    SPC_INTERRUPT,
    SPC_LAYER,
    SPC_MANAUSE,
    SPC_NAME,
    SPC_PROMPT_MSG,
    SPC_RESOURCES,
    SPC_RUNE_ITEM,
    SPC_RUNES,
    SPC_SCROLL_ITEM,
    SPC_SKILLREQ,
    SPC_SOUND,
    SPC_TITHINGUSE,
    SPC_QTY
};

lpctstr const CSpellDef::sm_szLoadKeys[SPC_QTY+1] =
{
    "CAST_TIME",
    "DEFNAME",
    "DURATION",
    "EFFECT",
    "EFFECT_ID",
    "FLAGS",
    "GROUP",
    "INTERRUPT",
    "LAYER",
    "MANAUSE",
    "NAME",
    "PROMPT_MSG",
    "RESOURCES",
    "RUNE_ITEM",
    "RUNES",
    "SCROLL_ITEM",
    "SKILLREQ",
    "SOUND",
    "TITHINGUSE",
    nullptr
};

CSpellDef::CSpellDef( SPELL_TYPE id ) :
    CResourceLink( CResourceID( RES_SPELL, id ))
{
    m_dwFlags = SPELLFLAG_DISABLED;
    m_dwGroup = 0;
    m_sound = 0;
    m_idSpell = ITEMID_NOTHING;
    m_idScroll = ITEMID_NOTHING;
    m_idEffect = ITEMID_NOTHING;
    m_idLayer = LAYER_NONE;
    m_wManaUse = 0;
    m_wTithingUse = 0;
    m_CastTime.Init();
    m_Interrupt.Init();
    m_Interrupt.m_aiValues.resize(1);
    m_Interrupt.m_aiValues[0] = 1000;
}

bool CSpellDef::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    ADDTOCALLSTACK("CSpellDef::r_WriteVal");
    EXC_TRY("WriteVal");
    int index = FindTableSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
    if (index < 0)
    {
        if (!strnicmp( "RESOURCES.", ptcKey, 10 ))
            index = SPC_RESOURCES;
    }
    switch (index)
    {
        case SPC_CAST_TIME:
            sVal = m_CastTime.Write();
            break;
        case SPC_DEFNAME:
            sVal = GetResourceName();
            break;
        case SPC_DURATION:
            sVal = m_Duration.Write();
            break;
        case SPC_EFFECT:
            sVal = m_vcEffect.Write();
            break;
        case SPC_EFFECT_ID:
            sVal.FormatVal( m_idEffect );
            break;
        case SPC_FLAGS:
            sVal.FormatVal( m_dwFlags );
            break;
        case SPC_GROUP:
            sVal.FormatVal( m_dwGroup );
            break;
        case SPC_INTERRUPT:
            sVal = m_Interrupt.Write();
            break;
        case SPC_LAYER:
            sVal.FormatVal(m_idLayer);
            break;
        case SPC_MANAUSE:
            sVal.FormatVal( m_wManaUse );
            break;
        case SPC_NAME:
            sVal = m_sName;
            break;
        case SPC_PROMPT_MSG:
            sVal = m_sTargetPrompt;
            break;
        case SPC_RESOURCES:
        {
            ptcKey	+= 9;
            // Check for "RESOURCES.*"
            if ( *ptcKey == '.' )
            {
                bool fQtyOnly = false;
                bool fKeyOnly = false;
                SKIP_SEPARATORS( ptcKey );
                // Determine the index of the resource
                // we wish to find
                index = Exp_GetVal( ptcKey );
                SKIP_SEPARATORS( ptcKey );

                // Check for "RESOURCES.x.KEY"
                if ( !strnicmp( ptcKey, "KEY", 3 ))
                    fKeyOnly = true;
                // Check for "RESORUCES.x.VAL"
                else if ( !strnicmp( ptcKey, "VAL", 3 ))
                    fQtyOnly = true;

                tchar *pszTmp = Str_GetTemp();
                m_Reags.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
                if ( fQtyOnly && pszTmp[0] == '\0' )
                    strcpy( pszTmp, "0" ); // Return 0 for empty quantity
                sVal = pszTmp;
            }
            else
            {
                tchar *pszTmp = Str_GetTemp();
                m_Reags.WriteKeys( pszTmp );
                sVal = pszTmp;
            }
            break;
        }
        case SPC_RUNE_ITEM:
            sVal.FormatVal( m_idSpell );
            break;
        case SPC_RUNES:
            // This may only be basic chars !
            sVal = m_sRunes;
            break;
        case SPC_SCROLL_ITEM:
            sVal.FormatVal( m_idScroll );
            break;
        case SPC_SKILLREQ:
        {
            tchar *pszTmp = Str_GetTemp();
            m_SkillReq.WriteKeys( pszTmp );
            sVal = pszTmp;
        }
        break;
        case SPC_SOUND:
            sVal.FormatVal(m_sound);
            break;
        case SPC_TITHINGUSE:
            sVal.FormatVal(m_wTithingUse);
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

bool CSpellDef::r_LoadVal( CScript &s )
{
    ADDTOCALLSTACK("CSpellDef::r_LoadVal");
    EXC_TRY("LoadVal");
    // RES_SPELL
    switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
    {
        case SPC_CAST_TIME:
            m_CastTime.Load( s.GetArgRaw());
            break;
        case SPC_DEFNAME:
            return SetResourceName( s.GetArgStr());
        case SPC_DURATION:
            m_Duration.Load( s.GetArgRaw());
            break;
        case SPC_EFFECT:
            m_vcEffect.Load( s.GetArgRaw());
            break;
        case SPC_EFFECT_ID:
            m_idEffect = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
            break;
        case SPC_FLAGS:
            m_dwFlags = s.GetArgVal();
            break;
        case SPC_GROUP:
            m_dwGroup = s.GetArgVal();
            break;
        case SPC_INTERRUPT:
            m_Interrupt.Load( s.GetArgRaw());
            break;
        case SPC_LAYER:
            m_idLayer = (LAYER_TYPE)(s.GetArgVal());
            break;
        case SPC_MANAUSE:
            m_wManaUse = (word)(s.GetArgVal());
            break;
        case SPC_NAME:
            m_sName = s.GetArgStr();
            break;
        case SPC_PROMPT_MSG:
            m_sTargetPrompt	= s.GetArgStr();
            break;
        case SPC_RESOURCES:
            m_Reags.Load( s.GetArgStr());
            break;
        case SPC_RUNE_ITEM:
            m_idSpell = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
            break;
        case SPC_RUNES:
            // This may only be basic chars !
            m_sRunes = s.GetArgStr();
            break;
        case SPC_SCROLL_ITEM:
            m_idScroll = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
            break;
        case SPC_SKILLREQ:
            m_SkillReq.Load( s.GetArgStr());
            break;
        case SPC_SOUND:
            m_sound = (SOUND_TYPE)(s.GetArgVal());
            break;
        case SPC_TITHINGUSE:
            m_wTithingUse = (word)(s.GetArgVal());
            break;
        default:
            return CResourceDef::r_LoadVal( s );
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CSpellDef::GetPrimarySkill( int * piSkill, int * piQty ) const
{
    ADDTOCALLSTACK("CSpellDef::GetPrimarySkill");
    size_t i = m_SkillReq.FindResourceType( RES_SKILL );
    if ( i == SCONT_BADINDEX )
        return false;

    if ( piQty != nullptr )
        *piQty = (int)(m_SkillReq[i].GetResQty());
    if ( piSkill != nullptr )
        *piSkill = m_SkillReq[i].GetResIndex();
    return (g_Cfg.GetSkillDef((SKILL_TYPE)(m_SkillReq[i].GetResIndex())) != nullptr);
}
