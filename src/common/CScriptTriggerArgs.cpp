#include "../game/CObjBase.h"
#include "CLog.h"
#include "CException.h"
#include "CExpression.h"
#include "CScriptTriggerArgs.h"


void CScriptTriggerArgs::Init( lpctstr pszStr )
{
    ADDTOCALLSTACK_INTENSIVE("CScriptTriggerArgs::Init");
    m_pO1 = nullptr;

    if ( pszStr == nullptr )
        pszStr	= "";
    // raw is left untouched for now - it'll be split the 1st time argv is accessed
    m_s1_raw = pszStr;
    bool fQuote = false;
    if ( *pszStr == '"' )
    {
        fQuote = true;
        ++pszStr;
    }

    m_s1 = pszStr;

    // take quote if present.
    if (fQuote)
    {
        tchar * str = strchr(const_cast<tchar*>(m_s1.GetPtr()), '"');
        if ( str != nullptr )
            *str = '\0';
    }

    m_iN1 = 0;
    m_iN2 = 0;
    m_iN3 = 0;

    // attempt to parse this.
    if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
    {
        m_iN1 = Exp_GetSingle(pszStr);
        SKIP_ARGSEP( pszStr );
        if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
        {
            m_iN2 = Exp_GetSingle(pszStr);
            SKIP_ARGSEP( pszStr );
            if ( IsDigit(*pszStr) || ((*pszStr == '-') && IsDigit(*(pszStr+1))) )
            {
                m_iN3 = Exp_GetSingle(pszStr);
            }
        }
    }

    // ensure argv will be recalculated next time it is accessed
    m_v.clear();
}

CScriptTriggerArgs::CScriptTriggerArgs( lpctstr pszStr )
{
    Init( pszStr );
}

bool CScriptTriggerArgs::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
    ADDTOCALLSTACK("CScriptTriggerArgs::r_GetRef");

    if ( !strnicmp(ptcKey, "ARGO.", 5) )		// ARGO.NAME
    {
        ptcKey += 5;
        if ( *ptcKey == '1' )
            ++ptcKey;
        pRef = m_pO1;
        return true;
    }
    else if ( !strnicmp(ptcKey, "REF", 3) )		// REF[1-65535].NAME
    {
        lpctstr pszTemp = ptcKey;
        pszTemp += 3;
        if (*pszTemp && IsDigit( *pszTemp ))
        {
            char * pEnd;
            ushort number = (ushort)strtol( pszTemp, &pEnd, 10 );
            if ( number > 0 ) // Can only use 1 to 65535 as REFs
            {
                pszTemp = pEnd;
                // Make sure REFx or REFx.KEY is being used
                if (( !*pszTemp ) || ( *pszTemp == '.' ))
                {
                    if ( *pszTemp == '.' )
                        ++pszTemp;

                    pRef = m_VarObjs.Get( number );
                    ptcKey = pszTemp;
                    return true;
                }
            }
        }
    }
    return false;
}


enum AGC_TYPE
{
    AGC_N,
    AGC_N1,
    AGC_N2,
    AGC_N3,
    AGC_O,
    AGC_S,
    AGC_V,
    AGC_LVAR,
    AGC_TRY,
    AGC_TRYSRV,
    AGC_QTY
};

lpctstr const CScriptTriggerArgs::sm_szLoadKeys[AGC_QTY+1] =
{
    "ARGN",
    "ARGN1",
    "ARGN2",
    "ARGN3",
    "ARGO",
    "ARGS",
    "ARGV",
    "LOCAL",
    "TRY",
    "TRYSRV",
    nullptr
};


bool CScriptTriggerArgs::r_Verb( CScript & s, CTextConsole * pSrc )
{
    ADDTOCALLSTACK("CScriptTriggerArgs::r_Verb");
    EXC_TRY("Verb");
    int	index = -1;
    lpctstr ptcKey = s.GetKey();

    if ( !strnicmp( "FLOAT.", ptcKey, 6 ) )
    {
        return( m_VarsFloat.Insert( (ptcKey+6), s.GetArgStr(), true ) );
    }
    else if ( !strnicmp( "LOCAL.", ptcKey, 6 ) )
    {
        bool fQuoted = false;
        m_VarsLocal.SetStr( s.GetKey()+6, fQuoted, s.GetArgStr( &fQuoted ), false );
        return true;
    }
    else if ( !strnicmp( "REF", ptcKey, 3 ) )
    {
        lpctstr pszTemp = ptcKey;
        pszTemp += 3;
        if (*pszTemp && IsDigit( *pszTemp ))
        {
            char * pEnd;
            ushort number = (ushort)(strtol( pszTemp, &pEnd, 10 ));
            if ( number > 0 ) // Can only use 1 to 65535 as REFs
            {
                pszTemp = pEnd;
                if ( !*pszTemp ) // setting REFx to a new object
                {
                    CUID uid = s.GetArgVal();
                    CObjBase * pObj = uid.ObjFind();
                    m_VarObjs.Insert( number, pObj, true );
                    ptcKey = pszTemp;
                    return true;
                }
                else if ( *pszTemp == '.' ) // accessing REFx object
                {
                    ptcKey = ++pszTemp;
                    CObjBase * pObj = m_VarObjs.Get( number );
                    if ( !pObj )
                        return false;

                    CScript script( ptcKey, s.GetArgStr());
                    script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// If s is a CResourceFile, it should have valid m_iResourceFileIndex
                    script.m_iLineNum = s.m_iLineNum;						// Line where Key/Arg were read
                    return pObj->r_Verb( script, pSrc );
                }
            }
        }
    }
    else if ( !strnicmp(ptcKey, "ARGO", 4) )
    {
        ptcKey += 4;
        if ( *ptcKey == '.' )
            index = AGC_O;
        else
        {
            ptcKey ++;
            CObjBase * pObj = static_cast<CObjBase*>(static_cast<CUID>(Exp_GetSingle(ptcKey)).ObjFind());
            if (!pObj)
                m_pO1 = nullptr;	// no pObj = cleaning argo
            else
                m_pO1 = pObj;
            return true;
        }
    }
    else
        index = FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 );

    switch (index)
    {
        case AGC_N:
        case AGC_N1:
            m_iN1	= s.GetArgVal();
            return true;
        case AGC_N2:
            m_iN2	= s.GetArgVal();
            return true;
        case AGC_N3:
            m_iN3	= s.GetArgVal();
            return true;
        case AGC_S:
            Init( s.GetArgStr() );
            return true;
        case AGC_O:
        {
            lpctstr pszTemp = s.GetKey() + strlen(sm_szLoadKeys[AGC_O]);
            if ( *pszTemp == '.' )
            {
                ++pszTemp;
                if ( !m_pO1 )
                    return false;

                CScript script( pszTemp, s.GetArgStr() );
                script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// If s is a CResourceFile, it should have valid m_iResourceFileIndex
                script.m_iLineNum = s.m_iLineNum;						// Line where Key/Arg were read
                return m_pO1->r_Verb( script, pSrc );
            }
        } return false;
        case AGC_TRY:
        case AGC_TRYSRV:
        {
            CScript script(s.GetArgStr());
            script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// If s is a CResourceFile, it should have valid m_iResourceFileIndex
            script.m_iLineNum = s.m_iLineNum;						// Line where Key/Arg were read
            if (r_Verb(script, pSrc))
                return true;
        }
        default:
            return false;
    }
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}

bool CScriptTriggerArgs::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CScriptTriggerArgs::r_LoadVal");
    UNREFERENCED_PARAMETER(s);
    return false;
}

bool CScriptTriggerArgs::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    ADDTOCALLSTACK("CScriptTriggerArgs::r_WriteVal");
    EXC_TRY("WriteVal");
    if ( IsSetEF( EF_Intrinsic_Locals ) )
    {
        EXC_SET_BLOCK("intrinsic");
        CVarDefCont * pVar = m_VarsLocal.GetKey( ptcKey );
        if ( pVar )
        {
            sVal = pVar->GetValStr();
            return true;
        }
    }
    else if ( !strnicmp("LOCAL.", ptcKey, 6) )
    {
        EXC_SET_BLOCK("local");
        ptcKey	+= 6;
        sVal = m_VarsLocal.GetKeyStr(ptcKey, true);
        return true;
    }

    if ( !strnicmp( "FLOAT.", ptcKey, 6 ) )
    {
        EXC_SET_BLOCK("float");
        ptcKey += 6;
        sVal = m_VarsFloat.Get( ptcKey );
        return true;
    }
    else if ( !strnicmp(ptcKey, "ARGV", 4) )
    {
        EXC_SET_BLOCK("argv");
        ptcKey += 4;
        SKIP_SEPARATORS(ptcKey);

        size_t uiQty = m_v.size();
        if ( uiQty <= 0 )
        {
            // PARSE IT HERE
            tchar * ptcArg = const_cast<tchar *>(m_s1_raw.GetPtr());
            tchar * s = ptcArg;
            bool fQuotes = false;
            bool fInerQuotes = false;
            while ( *s )
            {
                // ignore leading spaces
                if ( IsSpace(*s ) )
                {
                    ++s;
                    continue;
                }

                // add empty arguments if they are provided
                if ( (*s == ',') && !fQuotes)
                {
                    m_v.emplace_back( TSTRING_NULL );
                    ++s;
                    continue;
                }

                // check to see if the argument is quoted (incase it contains commas)
                if ( *s == '"' )
                {
                    ++s;
                    fQuotes = true;
                    fInerQuotes = false;
                }

                ptcArg = s;	// arg starts here
                ++s;

                while (*s)
                {
                    if ( (*s == '"' ) && fQuotes )
                    {
                        *s	= '\0';
                        fQuotes = false;
                    }
                    else if ( *s == '"' )
                    {
                        fInerQuotes = !fInerQuotes;
                    }
                    /*if ( *s == '"' )
                    {
                    if ( fQuotes )
                    {
                    *s	= '\0';
                    fQuotes = false;
                    break;
                    }
                    *s = '\0';
                    ++s;
                    fQuotes	= true;	// maintain
                    break;
                    }*/
                    if ( !fQuotes && !fInerQuotes && (*s == ',') )
                    {
                        *s = '\0';
                        ++s;
                        break;
                    }
                    ++s;
                }
                m_v.emplace_back( ptcArg );
            }
            uiQty = m_v.size();
        }

        if ( *ptcKey == '\0' )
        {
            sVal.FormatUVal((uint)uiQty);
            return true;
        }

        SKIP_SEPARATORS(ptcKey);
        uint uiNum = Exp_GetUSingle(ptcKey);
        if ( uiNum >= m_v.size() )
        {
            sVal = "";
            return true;
        }
        sVal = m_v[uiNum];
        return true;
    }

    EXC_SET_BLOCK("generic");
    int index = FindTableSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
    switch (index)
    {
        case AGC_N:
        case AGC_N1:
            sVal.FormatLLVal(m_iN1);
            break;
        case AGC_N2:
            sVal.FormatLLVal(m_iN2);
            break;
        case AGC_N3:
            sVal.FormatLLVal(m_iN3);
            break;
        case AGC_O:
        {
            CObjBase *pObj = dynamic_cast <CObjBase*> (m_pO1);
            if ( pObj )
                sVal.FormatHex(pObj->GetUID());
            else
                sVal.FormatVal(0);
        }
        break;
        case AGC_S:
            sVal = m_s1;
            break;
        default:
            return (fNoCallParent ? false : CScriptObj::r_WriteVal(ptcKey, sVal, pSrc));
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}
