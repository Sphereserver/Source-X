#include "../game/CServerTime.h"
#include "../sphere/threads.h"
#include "CException.h"
#include "CExpression.h"
#include "CScript.h"
#include "CSFileObj.h"
#include "CSFileObjContainer.h"


enum CFO_TYPE
{
#define ADD(a,b) CFO_##a,
#include "../tables/CSFileObjContainer_props.tbl"
#undef ADD
    CFO_QTY
};

lpctstr const CSFileObjContainer::sm_szLoadKeys[CFO_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CSFileObjContainer_props.tbl"
#undef ADD
    nullptr
};

enum CFOV_TYPE
{
#define ADD(a,b) CFOV_##a,
#include "../tables/CSFileObjContainer_functions.tbl"
#undef ADD
    CFOV_QTY
};

lpctstr const CSFileObjContainer::sm_szVerbKeys[CFOV_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CSFileObjContainer_functions.tbl"
#undef ADD
    nullptr
};

void CSFileObjContainer::ResizeContainer( size_t iNewRange )
{
    ADDTOCALLSTACK("CSFileObjContainer::ResizeContainer");
    if ( iNewRange == sFileList.size() )
    {
        return;
    }

    bool bDeleting = ( iNewRange < sFileList.size() );
    int howMuch = (int)(iNewRange - sFileList.size());
    if ( howMuch < 0 )
    {
        howMuch = (-howMuch);
    }

    if ( bDeleting )
    {
        if ( sFileList.empty() )
        {
            return;
        }

        CSFileObj * pObjHolder = nullptr;

        for ( size_t i = (sFileList.size() - 1); howMuch > 0; --howMuch, --i )
        {
            pObjHolder = sFileList.at(i);
            sFileList.pop_back();

            if ( pObjHolder )
            {
                delete pObjHolder;
            }
        }
    }
    else
    {
        for ( int i = 0; i < howMuch; ++i )
        {
            sFileList.emplace_back();
        }
    }
}

CSFileObjContainer::CSFileObjContainer()
{
    iGlobalTimeout = iCurrentTick = 0;
    SetFilenumber(0);
}

CSFileObjContainer::~CSFileObjContainer()
{
    ResizeContainer(0);
    sFileList.clear();
}

int CSFileObjContainer::GetFilenumber()
{
    ADDTOCALLSTACK("CSFileObjContainer::GetFilenumber");
    return iFilenumber;
}

void CSFileObjContainer::SetFilenumber( int iHowMuch )
{
    ADDTOCALLSTACK("CSFileObjContainer::SetFilenumber");
    ResizeContainer(iHowMuch);
    iFilenumber = iHowMuch;
}

bool CSFileObjContainer::_OnTick()
{
    ADDTOCALLSTACK("CSFileObjContainer::_OnTick");
    EXC_TRY("Tick");

    if ( !iGlobalTimeout )
        return true;

    if ( ++iCurrentTick >= iGlobalTimeout )
    {
        iCurrentTick = 0;

        for ( std::vector<CSFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
        {
            if ( !(*i)->_OnTick() )
            {
                // Error and fixweirdness
            }
        }
    }

    return true;
    EXC_CATCH;

    //EXC_DEBUG_START;
    //EXC_DEBUG_END;
    return false;
}

int CSFileObjContainer::FixWeirdness()
{
    ADDTOCALLSTACK("CSFileObjContainer::FixWeirdness");
    return 0;
}

bool CSFileObjContainer::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
    ADDTOCALLSTACK("CSFileObjContainer::r_GetRef");
    if ( !strnicmp("FIRSTUSED.",ptcKey,10) )
    {
        ptcKey += 10;

        CSFileObj * pFirstUsed = nullptr;
        for ( std::vector<CSFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
        {
            if ( (*i)->IsInUse() )
            {
                pFirstUsed = (*i);
                break;
            }
        }

        if ( pFirstUsed != nullptr )
        {
            pRef = pFirstUsed;
            return true;
        }
    }
    else
    {
        size_t nNumber = (size_t)( Exp_GetVal(ptcKey) );
        SKIP_SEPARATORS(ptcKey);

        if ( nNumber >= sFileList.size() )
            return false;

        CSFileObj * pFile = sFileList.at(nNumber);

        if ( pFile != nullptr )
        {
            pRef = pFile;
            return true;
        }
    }

    return false;
}

bool CSFileObjContainer::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CSFileObjContainer::r_LoadVal");
    EXC_TRY("LoadVal");
    lpctstr ptcKey = s.GetKey();

    int index = FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

    switch ( index )
    {
        case CFO_OBJECTPOOL:
            SetFilenumber(s.GetArgVal());
            break;

        case CFO_GLOBALTIMEOUT:
            iGlobalTimeout = (int64)llabs(s.GetArgLLVal()*MSECS_PER_SEC);
            break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CSFileObjContainer::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CSFileObjContainer::r_WriteVal");
    EXC_TRY("WriteVal");

    if ( !strnicmp("FIRSTUSED.",ptcKey,10) )
    {
        ptcKey += 10;

        CSFileObj * pFirstUsed = nullptr;
        for ( std::vector<CSFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
        {
            if ( (*i)->IsInUse() )
            {
                pFirstUsed = (*i);
                break;
            }
        }

        if ( pFirstUsed != nullptr )
        {
            return static_cast<CScriptObj *>(pFirstUsed)->r_WriteVal(ptcKey, sVal, pSrc);
        }

        return false;
    }

    int iIndex = FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

    if ( iIndex < 0 )
    {
        if (!fNoCallParent)
        {
            uint nNumber = Exp_GetUVal(ptcKey);
            SKIP_SEPARATORS(ptcKey);

            if ( nNumber >= sFileList.size() )
                return false;

            CSFileObj * pFile = sFileList[nNumber];
            if ( pFile != nullptr )
            {
                CScriptObj * pObj = dynamic_cast<CScriptObj*>( pFile );
                if (pObj != nullptr)
                    return pObj->r_WriteVal(ptcKey, sVal, pSrc);
            }
        }
        
        return false;
    }

    switch ( iIndex )
    {
        case CFO_OBJECTPOOL:
            sVal.FormatVal( GetFilenumber() );
            break;

        case CFO_GLOBALTIMEOUT:
            sVal.FormatLLVal(iGlobalTimeout/MSECS_PER_SEC);
            break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

bool CSFileObjContainer::r_Verb( CScript & s, CTextConsole * pSrc )
{
    ADDTOCALLSTACK("CSFileObjContainer::r_Verb");
    EXC_TRY("Verb");
    ASSERT(pSrc);

    lpctstr ptcKey = s.GetKey();

    if ( !strnicmp("FIRSTUSED.",ptcKey,10) )
    {
        ptcKey += 10;

        CSFileObj * pFirstUsed = nullptr;
        for ( std::vector<CSFileObj *>::iterator i = sFileList.begin(); i != sFileList.end(); ++i )
        {
            if ( (*i)->IsInUse() )
            {
                pFirstUsed = (*i);
                break;
            }
        }

        if ( pFirstUsed != nullptr )
            return static_cast<CScriptObj *>(pFirstUsed)->r_Verb(s,pSrc);

        return false;
    }

    int index = FindTableSorted( ptcKey, sm_szVerbKeys, ARRAY_COUNT( sm_szVerbKeys )-1 );

    if ( index < 0 )
    {
        if ( strchr( ptcKey, '.') ) // 0.blah format
        {
            size_t nNumber = Exp_GetSTVal(ptcKey);
            if ( nNumber < sFileList.size() )
            {
                CSFileObj * pFile = sFileList.at(nNumber);
                if ( pFile != nullptr )
                {
                    CScriptObj* pObj = dynamic_cast<CScriptObj*>(pFile);
                    if (pObj != nullptr)
                    {
                        SKIP_SEPARATORS(ptcKey);
                        CScript script(ptcKey, s.GetArgStr());
                        script.CopyParseState(s);
                        return pObj->r_Verb(script, pSrc);
                    }
                }

                return false;
            }
        }

        return ( this->r_LoadVal( s ) );
    }

    switch ( index )
    {
        case CFOV_CLOSEOBJECT:
        case CFOV_RESETOBJECT:
        {
            bool bResetObject = ( index == CFOV_RESETOBJECT );
            if ( s.HasArgs() )
            {
                size_t nNumber = (size_t)( s.GetArgVal() );
                if ( nNumber >= sFileList.size() )
                    return false;

                CSFileObj * pObjVerb = sFileList.at(nNumber);
                if ( bResetObject )
                {
                    delete pObjVerb;
                    sFileList.at(nNumber) = new CSFileObj();
                }
                else
                {
                    pObjVerb->FlushAndClose();
                }
            }
        } break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}
