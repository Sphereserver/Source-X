#include "../../../sphere/threads.h"
#include "../../game/chars/CChar.h"
#include "../../game/CServerConfig.h"
#include "../../game/triggers.h"
#include "../CException.h"
#include "CRegionResourceDef.h"
#include "CRandGroupDef.h"

enum RGC_TYPE
{
    RGC_CALCMEMBERINDEX,
    RGC_CATEGORY,
    RGC_CONTAINER,
    RGC_DEFNAME,
    RGC_DESCRIPTION,
    RGC_ID,
    RGC_RESOURCES,
    RGC_SUBSECTION,
    RGC_WEIGHT,
    RGC_QTY
};

lpctstr const CRandGroupDef::sm_szLoadKeys[RGC_QTY+1] =
{
    "CALCMEMBERINDEX",
    "CATEGORY",
    "CONTAINER",
    "DEFNAME",
    "DESCRIPTION",
    "ID",
    "RESOURCES",
    "SUBSECTION",
    "WEIGHT",
    nullptr
};

int CRandGroupDef::CalcTotalWeight()
{
    ADDTOCALLSTACK("CRandGroupDef::CalcTotalWeight");
    int iTotal = 0;
    size_t iQty = m_Members.size();
    for ( size_t i = 0; i < iQty; ++i )
    {
        iTotal += (int)(m_Members[i].GetResQty());
    }
    return( m_iTotalWeight = iTotal );
}

bool CRandGroupDef::r_LoadVal( CScript &s )
{
    ADDTOCALLSTACK("CRandGroupDef::r_LoadVal");
    EXC_TRY("LoadVal");
    // RES_SPAWN or RES_REGIONTYPE
    switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
    {
        case RGC_CATEGORY:
            m_sCategory = s.GetArgStr();
            break;
        case RGC_SUBSECTION:
            m_sSubsection = s.GetArgStr();
            break;
        case RGC_DESCRIPTION:
        {
            m_sDescription = s.GetArgStr();
            if ( !strcmpi(m_sDescription, "@") )
                m_sDescription = m_sSubsection;
        }
        break;
        case RGC_DEFNAME: // "DEFNAME"
            return SetResourceName( s.GetArgStr());
        case RGC_ID:	// "ID"
        case RGC_CONTAINER:
        {
            tchar	*ppCmd[2];
            size_t iArgs = Str_ParseCmds(s.GetArgStr(), ppCmd, CountOf(ppCmd));
            CResourceQty rec;

            rec.SetResourceID(
                g_Cfg.ResourceGetID(RES_CHARDEF, ppCmd[0]),
                ( iArgs > 1 && ppCmd[1][0] ) ? Exp_GetVal(ppCmd[1]) : 1 );
            m_iTotalWeight += (int)(rec.GetResQty());
            m_Members.emplace_back(rec);
        }
        break;

        case RGC_RESOURCES:
            m_Members.Load(s.GetArgStr());
            CalcTotalWeight();
            break;

        case RGC_WEIGHT: // Modify the weight of the last item.
            if (!m_Members.empty() )
            {
                int iWeight = s.GetArgVal();
                m_Members[m_Members.size() - 1].SetResQty(iWeight);
                CalcTotalWeight();
            }
            break;

            //default:
            //Ignore the rest
            //return( CResourceDef::r_LoadVal( s ));
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CRandGroupDef::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    ADDTOCALLSTACK("CRandGroupDef::r_WriteVal");
    EXC_TRY("WriteVal");

    switch ( FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
    {
        case RGC_CATEGORY:
            sVal = m_sCategory;
            break;
        case RGC_SUBSECTION:
            sVal = m_sSubsection;
            break;
        case RGC_DESCRIPTION:
            sVal = m_sDescription;
            break;
        case RGC_ID:
        case RGC_CONTAINER:
        {
            size_t i = GetRandMemberIndex();
            if ( i != SCONT_BADINDEX )
                sVal.FormatHex(GetMemberID(i).GetResIndex());
            break;
        }
        case RGC_CALCMEMBERINDEX:
        {
            ptcKey += 15;
            GETNONWHITESPACE( ptcKey );

            if ( ptcKey[0] == '\0' )
                sVal.FormatSTVal( GetRandMemberIndex(nullptr, false) );
            else
            {
                CChar * pSend = CUID::CharFindFromUID(Exp_GetDWVal(ptcKey));

                if ( pSend )
                    sVal.FormatSTVal( GetRandMemberIndex(pSend, false) );
                else
                    return false;
            }

        } break;

        case RGC_DEFNAME: // "DEFNAME"
            sVal = GetResourceName();
            break;

        case RGC_RESOURCES:
        {
            ptcKey	+= 9;
            if ( *ptcKey == '.' )
            {
                SKIP_SEPARATORS( ptcKey );

                if ( !strnicmp( ptcKey, "COUNT", 5 ))
                {
                    sVal.FormatSTVal(m_Members.size());
                }
                else
                {
                    bool fQtyOnly = false;
                    bool fKeyOnly = false;
                    int index = Exp_GetVal( ptcKey );
                    SKIP_SEPARATORS( ptcKey );

                    if ( !strnicmp( ptcKey, "KEY", 3 ))
                        fKeyOnly = true;
                    else if ( !strnicmp( ptcKey, "VAL", 3 ))
                        fQtyOnly = true;

                    tchar *pszTmp = Str_GetTemp();
                    m_Members.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
                    if ( fQtyOnly && pszTmp[0] == '\0' )
                        strcpy( pszTmp, "0" );

                    sVal = pszTmp;
                }
            }
            else
            {
                tchar *pszTmp = Str_GetTemp();
                m_Members.WriteKeys( pszTmp );
                sVal = pszTmp;
            }
        } break;

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

size_t CRandGroupDef::GetRandMemberIndex( CChar * pCharSrc, bool fTrigger ) const
{
    ADDTOCALLSTACK("CRandGroupDef::GetRandMemberIndex");
    int rid;
    size_t iCount = m_Members.size();
    if ( iCount <= 0 )
        return SCONT_BADINDEX;

    int iWeight = 0;
    size_t i;
    if ( pCharSrc == nullptr )
    {
        iWeight	= Calc_GetRandVal( m_iTotalWeight ) + 1;

        for ( i = 0; iWeight > 0 && i < iCount; ++i )
        {
            iWeight -= (int)(m_Members[i].GetResQty());
        }
        if ( i >= iCount && iWeight > 0 )
            return SCONT_BADINDEX;

        ASSERT(i > 0);
        return( i - 1 );
    }

    std::vector<size_t> members;

    // calculate weight only of items pCharSrc can get
    int iTotalWeight = 0;
    for ( i = 0; i < iCount; ++i )
    {
        CRegionResourceDef * pOreDef = dynamic_cast <CRegionResourceDef *>( g_Cfg.ResourceGetDef( m_Members[i].GetResourceID() ) );
        // If no regionresource, return just some random entry!
        if (pOreDef != nullptr)
        {
            rid = pOreDef->m_ReapItem;
            if (rid != 0)
            {
                if (!pCharSrc->Skill_MakeItem((ITEMID_TYPE)(rid), CUID(UID_CLEAR), SKTRIG_SELECT))
                    continue;

                if (IsTrigUsed(TRIGGER_RESOURCETEST))
                {
                    if (fTrigger && pOreDef->OnTrigger("@ResourceTest", pCharSrc, nullptr) == TRIGRET_RET_TRUE)
                        continue;
                }
            }
        }
        members.emplace_back(i);
        iTotalWeight += (int)(m_Members[i].GetResQty());
    }
    iWeight = Calc_GetRandVal( iTotalWeight ) + 1;
    iCount = members.size();

    for ( i = 0; iWeight > 0 && i < iCount; ++i )
    {
        iWeight -= (int)(m_Members[members[i]].GetResQty());
    }
    if ( i >= iCount && iWeight > 0 )
        return SCONT_BADINDEX;
    ASSERT(i > 0);
    return members[i - 1];
}
