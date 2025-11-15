
#include "../game/chars/CChar.h"
#include "../game/chars/CStoneMember.h"
#include "../game/items/CItem.h"
#include "../game/clients/CAccount.h"
#include "../game/clients/CClient.h"
#include "../game/CScriptProfiler.h"
#include "../game/CSector.h"
#include "../game/CServer.h"
#include "../game/CWorld.h"
#include "../game/CWorldMap.h"
#include "../game/CWorldSearch.h"
#include "../game/CWorldTimedFunctions.h"
#include "../sphere/ProfileTask.h"
#include "crypto/CBCrypt.h"
#include "crypto/CMD5.h"
#include "sphere_library/CSRand.h"
#include "resource/sections/CResourceNamedDef.h"
#include "resource/CResourceLock.h"
//#include "CExpression.h" // included in the precompiled header
//#include "CScriptParserBufs.h" // included in the precompiled header via CExpression.h
#include "CSFileObjContainer.h"
#include "CFloatMath.h"

#ifdef _WIN32
#   include <process.h>
#else
#   include <sys/wait.h>
#   include <errno.h>	// errno
#   include <spawn.h> // for posix_spawn()
#endif

#if !defined(_WIN32) && !defined(__GLIBC__)
extern char **environ;
#endif

class CStoneMember;

enum SREF_TYPE
{
    SREF_DB,
    SREF_FILE,
    SREF_I,
    SREF_LDB,
    SREF_MDB,
    SREF_NEW,
    SREF_OBJ,
    SREF_SERV,
    SREF_UID,
    SREF_QTY
};

static lpctstr const _ptcSRefKeys[SREF_QTY+1] =
{
    "DB",
    "FILE",
    "I",
    "LDB",
    "MDB",
    "NEW",
    "OBJ",
    "SERV",
    "UID",
    nullptr
};


bool CScriptObj::ParseError_UndefinedKeyword(lpctstr ptcKey) // static
{
	g_Log.EventError("Undefined keyword '%s'.\n", ptcKey);
	return false;
}

bool CScriptObj::IsValidRef(const CScriptObj* pRef) noexcept // static
{
	bool fValid = false;
	if (pRef)
	{
		const CObjBase* pRefObj = dynamic_cast<const CObjBase*>(pRef);
		fValid = (pRefObj == nullptr) ? true : pRefObj->IsValidUID();
	}
	return fValid;
}

bool CScriptObj::IsValidRef(const CUID& uidRef) noexcept // static
{
	return (nullptr != uidRef.ObjFind(true));
}

bool CScriptObj::r_GetRefFull(lpctstr& ptcKey, CScriptObj*& pRef)
{
	ADDTOCALLSTACK("CScriptObj::r_GetRefFull");
	bool fRef = false;

	// Special refs
	CClient* pThisClient = nullptr;
	if (CChar* pThisChar = dynamic_cast<CChar*>(this))
	{
		// r_GetRef is a virtual method, but if the Client is attached to a Char, its r_GetRef won't be called,
		//	since CClient doesn't inherit from CChar and the pointer to the attached Client is stored in the Char's Class.
		// For Webpages or other stuff CClient::r_GetRef will be called, since it's a parent class.
		pThisClient = pThisChar->GetClientActive();
		if (pThisClient)
		{
			fRef = pThisClient->r_GetRef(ptcKey, pRef);

			if (fRef && (pRef == pThisClient))
				pRef = this;	// Special handling of "I" ref.
		}
	}

	// Standard Refs
	if (!fRef)
	{
		fRef = r_GetRef(ptcKey, pRef);
	}

	return fRef;
}

bool CScriptObj::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CScriptObj::r_GetRef");
	// A key name that just links to another object.

    int index = FindTableHeadSorted(ptcKey, _ptcSRefKeys, ARRAY_COUNT(_ptcSRefKeys)-1);
    switch (index)
    {
        case SREF_SERV:
            if (ptcKey[4] != '.')
                return false;
            ptcKey += 5;
            pRef = &g_Serv;
            return true;
        case SREF_UID:
            if (ptcKey[3] != '.')
                return false;
            ptcKey += 4;
            pRef = CUID::ObjFindFromUID(Exp_GetDWVal(ptcKey));
            SKIP_SEPARATORS(ptcKey);
            return true;
        case SREF_OBJ:
            if (ptcKey[3] != '.')
                return false;
            ptcKey += 4;
            pRef = ( g_World.m_uidObj.IsValidUID() ? g_World.m_uidObj.ObjFind() : nullptr);
            return true;
        case SREF_NEW:
            if (ptcKey[3] != '.')
                return false;
            ptcKey += 4;
            pRef = ( g_World.m_uidNew.IsValidUID() ? g_World.m_uidNew.ObjFind() : nullptr);
            return true;
        case SREF_I:
			if (ptcKey[1] != '.')
				return false;
			ptcKey += 2;
            pRef = this;
            return true;
        case SREF_FILE:
            if ( !IsSetOF(OF_FileCommands) )
                return false;
            if (ptcKey[4] != '.')
                return false;
            ptcKey += 5;
            pRef = &(g_Serv._hFile);
            return true;
        case SREF_DB:
            if (ptcKey[2] != '.')
                return false;
            ptcKey += 3;
            pRef = &(g_Serv._hDb);
            return true;
        case SREF_LDB:
            if (ptcKey[3] != '.')
                return false;
            ptcKey += 4;
            pRef = &(g_Serv._hLdb);
            return true;
        case SREF_MDB:
            if (ptcKey[3] != '.')
                return false;
            ptcKey += 4;
            pRef = &(g_Serv._hMdb);
            return true;
        default:
            return false;
    }
}

enum SSC_TYPE
{
	#define ADD(a,b) SSC_##a,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	SSC_QTY
};

lpctstr const CScriptObj::sm_szLoadKeys[SSC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CScriptObj_functions.tbl"
	#undef ADD
	nullptr
};

size_t CScriptObj::r_GetFunctionIndex(lpctstr pszFunction) // static
{
    ADDTOCALLSTACK_DEBUG("CScriptObj::r_GetFunctionIndex");
    GETNONWHITESPACE( pszFunction );
    return g_Cfg.m_Functions.find_sorted(pszFunction);
}

bool CScriptObj::r_CanCall(size_t uiFunctionIndex) // static
{
    ADDTOCALLSTACK_DEBUG("CScriptObj::r_CanCall");
    if (uiFunctionIndex == sl::scont_bad_index())
        return false;
    ASSERT(uiFunctionIndex < g_Cfg.m_Functions.size());
    return true;
}

bool CScriptObj::r_Call( lpctstr pszFunction, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc, CSString * psVal, TRIGRET_TYPE * piRet )
{
    ADDTOCALLSTACK("CScriptObj::r_Call (FunctionName)");

    size_t index = r_GetFunctionIndex(pszFunction);
    if ( !r_CanCall(index) )
        return false;

    return r_Call(index, pScriptArgs, pSrc, psVal, piRet);
}

bool CScriptObj::r_Call( size_t uiFunctionIndex, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc, CSString * psVal, TRIGRET_TYPE * piRet )
{
    ADDTOCALLSTACK("CScriptObj::r_Call (FunctionIndex)");
	EXC_TRY("Call by index");
    ASSERT(r_CanCall(uiFunctionIndex));

    CResourceNamedDef * pFunction = g_Cfg.m_Functions[uiFunctionIndex].get();
    ASSERT(pFunction);
    CResourceLock sFunction;
    if ( pFunction->ResourceLock(sFunction) )
    {
        CScriptProfiler::CScriptProfilerFunction *pFun = nullptr;
        TIME_PROFILE_INIT;

        //	If functions profiler is on, search this function record and get pointer to it
        //	if not, create the corresponding record
        if ( IsSetEF(EF_Script_Profiler) )
        {
            char *pName = Str_GetTemp();
            char *pSpace;

            //	lowercase for speed, and strip arguments
            Str_CopyLimitNull(pName, pFunction->GetName(), Str_TempLength());
            if ( (pSpace = strchr(pName, ' ')) != nullptr )
                *pSpace = 0;
            _strlwr(pName);

            if ( g_profiler.initstate != 0xf1 )	// it is not initalised
            {
                memset(&g_profiler, 0, sizeof(g_profiler));
                g_profiler.initstate = (uchar)(0xf1); // ''
            }
            for ( pFun = g_profiler.FunctionsHead; pFun != nullptr; pFun = pFun->next )
            {
                if ( !strcmp(pFun->name, pName) )
                    break;
            }

            // first time function called. so create a record for it
            if ( pFun == nullptr )
            {
                pFun = new CScriptProfiler::CScriptProfilerFunction;
                memset(pFun, 0, sizeof(CScriptProfiler::CScriptProfilerFunction));
                Str_CopyLimitNull(pFun->name, pName, sizeof(CScriptProfiler::CScriptProfilerFunction::name));
                if ( g_profiler.FunctionsTail )
                    g_profiler.FunctionsTail->next = pFun;
                else
                    g_profiler.FunctionsHead = pFun;
                g_profiler.FunctionsTail = pFun;
            }

            //	prepare the informational block
            ++pFun->called;
            ++g_profiler.called;
            TIME_PROFILE_START;
        }

        TRIGRET_TYPE iRet = OnTriggerRun(sFunction, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, psVal);

        if ( IsSetEF(EF_Script_Profiler) )
        {
            //	update the time call information
            TIME_PROFILE_END;
            llTicksStart = llTicksEnd - llTicksStart;
            pFun->total += llTicksStart;
            pFun->average = (pFun->total / pFun->called);
            if ( pFun->max < llTicksStart )
                pFun->max = llTicksStart;
            if (( pFun->min > llTicksStart ) || ( !pFun->min ))
                pFun->min = llTicksStart;
            g_profiler.total += llTicksStart;
        }

        if ( piRet )
            *piRet	= iRet;

        if (iRet == TRIGRET_RET_ABORTED)
            return false;
    }
	EXC_CATCH;
    return true;
}

bool CScriptObj::r_ExecSingle(lpctstr ptcLine)
{
	ADDTOCALLSTACK("CScriptObj::r_ExecSingle");
	ASSERT(ptcLine);

	CScript scriptLine(ptcLine);
	return r_Verb(scriptLine, &g_Serv);
}

bool CScriptObj::r_SetVal( lpctstr ptcKey, lpctstr ptcVal )
{
    ADDTOCALLSTACK("CScriptObj::r_SetVal");
	ASSERT(ptcKey);
	ASSERT(ptcVal);

	CScript s( ptcKey, ptcVal );
	return r_LoadVal( s );
}

bool CScriptObj::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CScriptObj::r_LoadVal");
	EXC_TRY("LoadVal");
	lpctstr ptcKey = s.GetKey();

	// ignore these.
	int index = FindTableHeadSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys)-1);
	if ( index < 0 )
	{
		return ParseError_UndefinedKeyword(s.GetKey());
	}

	switch ( index )
	{
        case SSC_VAR0:
        case SSC_VAR:
        {
            const bool fZero = (index == SSC_VAR0);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            g_ExprGlobals.mtEngineLockedWriter()->m_VarGlobals.SetStr(ptcKey + (fZero ? 5 : 4), fQuoted, ptcArg, fZero);
            return true;
        }

        case SSC_LIST:
            if ( ! g_ExprGlobals.mtEngineLockedWriter()->m_ListGlobals.r_LoadVal(ptcKey + 5, s) )
				DEBUG_ERR(("Unable to process command '%s %s'\n", ptcKey, s.GetArgRaw()));
			return true;

		case SSC_DEFMSG:
        {
			ptcKey += 7;
            // TODO: we really should be using a sorted vector or an hash map...
            auto rw = g_ExprGlobals.mtEngineLockedWriter();
            for ( int l = 0; l < DEFMSG_QTY; ++l )
			{
                if ( !strcmpi(ptcKey, rw->sm_szDefMsgNames[l]) )
				{
					bool fQuoted = false;
                    tchar * args = s.GetArgStr(&fQuoted);
                    Str_CopyLimitNull(rw->sm_szDefMessages[l], args, CExprGlobals::m_kiDefmsgMaxLen);
					return true;
				}
			}
			g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", ptcKey);
			return false;
        }
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

static void StringFunction( int iFunc, lpctstr ptcKey, CSString &sVal )
{
	GETNONWHITESPACE(ptcKey);
	if ( *ptcKey == '(' )
		++ptcKey;

	tchar * ppCmd[4];
	int iCount = Str_ParseCmds( const_cast<tchar *>(ptcKey), ppCmd, ARRAY_COUNT(ppCmd), ")" );
	if ( iCount <= 0 )
	{
		DEBUG_ERR(( "Bad string function usage. missing )\n" ));
		return;
	}

	switch ( iFunc )
	{
		case SSC_CHR:
			sVal.Format( "%c", Exp_GetSingle( ppCmd[0] ) );
			return;
		case SSC_StrReverse:
			sVal = ppCmd[0];	// strreverse(str) = reverse the string
			sVal.Reverse();
			return;
		case SSC_StrToLower:	// strlower(str) = lower case the string
			sVal = ppCmd[0];
			sVal.MakeLower();
			return;
		case SSC_StrToUpper:	// strupper(str) = upper case the string
			sVal = ppCmd[0];
			sVal.MakeUpper();
			return;
	}
}

bool CScriptObj::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallParent);
	ADDTOCALLSTACK("CScriptObj::r_WriteVal");

	EXC_TRY("WriteVal-Ref");

	CScriptObj* pRef = nullptr;
	bool fRef = r_GetRefFull(ptcKey, pRef);

	if ( !strnicmp(ptcKey, "GetRefType", 10) )
	{
		CScriptObj * pTmpRef;
		if ( pRef )
			pTmpRef = pRef;
		else
			pTmpRef = pSrc->GetChar();

		if ( pTmpRef == &g_Serv )
			sVal.FormatHex( 0x01 );
		else if ( pTmpRef == &(g_Serv._hFile) )
			sVal.FormatHex( 0x02 );
		else if (( pTmpRef == &(g_Serv._hDb) ) || dynamic_cast<CDataBase*>(pTmpRef) )
			sVal.FormatHex( 0x00008 );
		else if ( dynamic_cast<CResourceDef*>(pTmpRef) )
			sVal.FormatHex( 0x00010 );
		else if ( dynamic_cast<CResourceHolder*>(pTmpRef) )
			sVal.FormatHex( 0x00020 );
		else if ( dynamic_cast<CScriptTriggerArgs*>(pTmpRef) )
			sVal.FormatHex( 0x00040 );
		else if ( dynamic_cast<CSFileObj*>(pTmpRef) )
			sVal.FormatHex( 0x00080 );
		else if ( dynamic_cast<CSFileObjContainer*>(pTmpRef) )
			sVal.FormatHex( 0x00100 );
		else if ( dynamic_cast<CAccount*>(pTmpRef) )
			sVal.FormatHex( 0x00200 );
		//else if ( dynamic_cast<CPartyDef*>(pTmpRef) )
		//	sVal.FormatHex( 0x00400 );
		else if ( pTmpRef == &(g_Serv._hLdb) )
			sVal.FormatHex( 0x00400 );
		else if ( dynamic_cast<CStoneMember*>(pTmpRef) )
			sVal.FormatHex( 0x00800 );
		else if ( dynamic_cast<CServerDef*>(pTmpRef) )
			sVal.FormatHex( 0x01000 );
		else if ( dynamic_cast<CSector*>(pTmpRef) )
			sVal.FormatHex( 0x02000 );
		else if ( dynamic_cast<CWorld*>(pTmpRef) )
			sVal.FormatHex( 0x04000 );
		else if ( dynamic_cast<CGMPage*>(pTmpRef) )
			sVal.FormatHex( 0x08000 );
		else if ( dynamic_cast<CClient*>(pTmpRef) )
			sVal.FormatHex( 0x10000 );
		else if ( CObjBase *pObj = dynamic_cast<CObjBase*>(pTmpRef) )
		{
			if ( dynamic_cast<CChar*>(pObj) )
				sVal.FormatHex( 0x40000 );
			else if ( dynamic_cast<CItem*>(pObj) )
				sVal.FormatHex( 0x80000 );
			else
				sVal.FormatHex( 0x20000 );
		}
        else if ( pTmpRef == &(g_Serv._hMdb) )
            sVal.FormatHex( 0x100000 );
        else if (dynamic_cast<CSQLite*>(pTmpRef))
            sVal.FormatHex( 0x200000 );
		else
			sVal.FormatHex( 0x01 );
		return true;
	}

	if (fRef)
	{
		if ( ptcKey[0] == '\0' )	// we were just testing the ref.
		{
			if (pRef == nullptr)
			{
				// Invalid ref: just return 0, it isn't an error.
				sVal.SetValFalse();
				return true;
			}

            const CObjBase * pRefObj = dynamic_cast<const CObjBase*>(pRef);
			if (pRefObj)
				sVal.FormatHex( (dword)pRefObj->GetUID() );
			else
				sVal.FormatVal( 1 );
			return true;
		}

		if (!strnicmp("ISVALID", ptcKey, 7))
		{
			sVal.FormatVal(IsValidRef(pRef));
			return true;
		}

		if (pRef == nullptr)	// good command but bad reference.
		{
			sVal.SetValFalse();
			return false;
		}
		return pRef->r_WriteVal( ptcKey, sVal, pSrc );
	}

    if (fNoCallChildren)
    {
        // Special case. We pass fNoCallChildren = true if we are called from CClient::r_WriteVal and we want to check only for the ref.
        //  Executing the code below is only a waste of resources.
        return false;
    }

	EXC_SET_BLOCK("Writeval-Statement");
	int index = FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		// <dSOMEVAL> same as <eval <SOMEVAL>> to get dec from the val
		if (( *ptcKey == 'd' ) || ( *ptcKey == 'D' ))
		{
            lpctstr ptcArg = ptcKey + 1;
            if ( r_WriteVal(ptcArg, sVal, pSrc) )
			{
                if ( *sVal != '-' )
                    sVal.FormatLLVal(Str_ToLL(sVal.GetBuffer()).value_or(0));
				return true;
			}
		}
        // <hSOMEVAL> same as <HVAL <SOMEVAL>> to get hex from the val
        else if ((*ptcKey == 'h') || (*ptcKey == 'H'))
        {
            lpctstr ptcArg = ptcKey + 1;
            if (r_WriteVal(ptcArg, sVal, pSrc))
            {
                if (*sVal != '-')
                    sVal.FormatLLHex(Str_ToLL(sVal.GetBuffer()).value_or(0));
                return true;
            }
        }
		// <r>, <r15>, <r3,15> are shortcuts to rand(), rand(15) and rand(3,15)
		else if (( *ptcKey == 'r' ) || ( *ptcKey == 'R' ))
		{
			ptcKey += 1;
			if ( *ptcKey && (( *ptcKey < '0' ) || ( *ptcKey > '9' )) && *ptcKey != '-' )
				goto badcmd;

			int64 min = 1000, max = INT64_MIN;

			if ( *ptcKey )
			{
				min = Exp_Get64Val(ptcKey);
				SKIP_ARGSEP(ptcKey);
			}
			if ( *ptcKey )
			{
				max = Exp_Get64Val(ptcKey);
				SKIP_ARGSEP(ptcKey);
			}

			if ( max == INT64_MIN )
			{
				max = min - 1;
				min = 0;
			}

			if ( min >= max )
				sVal.FormatLLVal(min);
			else
				sVal.FormatLLVal(g_Rand.GetLLVal2(min, max));

			return true;
		}
badcmd:
		return false;	// Bad command.
	}

	ptcKey += strlen( sm_szLoadKeys[index] );
	SKIP_SEPARATORS(ptcKey);
	bool fZero = false;

	switch ( index )
	{
        case SSC_RESOURCEINDEX:
            sVal.FormatHex(ResGetIndex(Exp_GetVal(ptcKey)));
            break;
        case SSC_RESOURCETYPE:
            sVal.FormatHex(ResGetType(Exp_GetVal(ptcKey)));
            break;

		case SSC_LISTCOL:
			// Set the alternating color.
			sVal = (CWebPageDef::sm_iListIndex & 1) ? "bgcolor=\"#E8E8E8\"" : "";
			return true;
		case SSC_OBJ:
			if ( !g_World.m_uidObj.ObjFind() )
				g_World.m_uidObj.ClearUID();
			sVal.FormatHex((dword)g_World.m_uidObj);
			return true;
		case SSC_NEW:
			if ( !g_World.m_uidNew.ObjFind() )
				g_World.m_uidNew.ClearUID();
			sVal.FormatHex((dword)g_World.m_uidNew);
			return true;
		case SSC_SRC:
			if ( pSrc == nullptr )
				pRef = nullptr;
			else
			{
				pRef = pSrc->GetChar();	// if it can be converted .
				if ( ! pRef )
					pRef = dynamic_cast <CScriptObj*> (pSrc);	// if it can be converted .
			}
			if ( ! pRef )
			{
				sVal.FormatVal( 0 );
				return true;
			}
			if ( !*ptcKey )
			{
                CObjBase *pObj = dynamic_cast <CObjBase*> (pRef);	// if it can be converted .
				sVal.FormatHex( pObj ? (dword) pObj->GetUID() : 0 );
				return true;
			}
			return pRef->r_WriteVal( ptcKey, sVal, pSrc );
		case SSC_VAR0:
			fZero = true;
			FALLTHROUGH;
		case SSC_VAR:
			// "VAR." = get/set a system wide variable.
			{
                auto gReader = g_ExprGlobals.mtEngineLockedReader();
                const CVarDefCont * pVar = gReader->m_VarGlobals.GetKey(ptcKey);
                sVal = CVarDefCont::GetValStrZeroed(pVar, fZero);
			}
			return true;
        case SSC_DEFLIST:
            g_ExprGlobals.mtEngineLockedReader()->m_ListInternals.r_Write(pSrc, ptcKey, sVal);
            return true;
        case SSC_LIST:
        {
            auto gReader = g_ExprGlobals.mtEngineLockedReader();
            if (! gReader->m_ListGlobals.r_Write(pSrc, ptcKey, sVal))
                sVal = "-1";
        }
            return true;
		case SSC_DEF0:
			fZero = true;
			FALLTHROUGH;
		case SSC_DEF:
        {
            auto gReader = g_ExprGlobals.mtEngineLockedReader();
            const CVarDefCont * pVar = gReader->m_VarDefs.GetKey(ptcKey);
            if ( pVar )
                sVal = pVar->GetValStr();
            else if ( fZero )
                sVal.SetValFalse();
        }
            return true;
        case SSC_RESDEF0:
            fZero = true;
			FALLTHROUGH;
        case SSC_RESDEF:
        {
            auto gReader = g_ExprGlobals.mtEngineLockedReader();
            const CVarDefCont * pVar = gReader->m_VarResDefs.GetKey(ptcKey);
            if ( pVar )
                sVal = pVar->GetValStr();
            else if ( fZero )
                sVal.SetValFalse();
        }
        return true;
		case SSC_DEFMSG:
			sVal = g_Cfg.GetDefaultMsg(ptcKey);
			return true;

        case SSC_BETWEEN:
        case SSC_BETWEEN2:
        {
            int64	iMin = Exp_GetLLVal(ptcKey);
            SKIP_ARGSEP(ptcKey);
            int64	iMax = Exp_GetLLVal(ptcKey);
            SKIP_ARGSEP(ptcKey);
            int64 iCurrent = Exp_GetLLVal(ptcKey);
            SKIP_ARGSEP(ptcKey);
            int64 iAbsMax = Exp_GetLLVal(ptcKey);
            SKIP_ARGSEP(ptcKey);
            if ( index == SSC_BETWEEN2 )
            {
                iCurrent = iAbsMax - iCurrent;
            }

            if (( iMin >= iMax ) || ( iAbsMax <= 0 ) || ( iCurrent <= 0 ) )
                sVal.FormatLLVal(iMin);
            else if ( iCurrent >= iAbsMax )
                sVal.FormatLLVal(iMax);
            else
                sVal.FormatLLVal((iCurrent * (iMax - iMin))/iAbsMax + iMin);
        } break;
		case SSC_EVAL:
			sVal.FormatLLVal( Exp_GetLLVal( ptcKey ));
			return true;
		case SSC_UVAL:
			sVal.FormatULLVal(Exp_GetULLVal(ptcKey));
			return true;
		case SSC_FVAL:
			{
				llong iVal = Exp_GetLLVal(ptcKey);
                int64 iValAbs = SphereAbs((int64)iVal);
				sVal.Format( "%s%lld.%lld" , ((iVal >= 0) ? "" : "-"), (iValAbs / 10LL), (iValAbs % 10LL) );
				return true;
			}
		case SSC_HVAL:
			sVal.FormatLLHex(Exp_GetLLVal(ptcKey));
			return true;

//FLOAT STUFF BEGINS HERE
		case SSC_FEVAL: //Float EVAL
			sVal.FormatVal( atoi( ptcKey ) );
			break;
		case SSC_FHVAL: //Float HVAL
			sVal.FormatHex( atoi( ptcKey ) );
			break;
		case SSC_FLOATVAL: //Float math
			{
				sVal = CFloatMath::FloatMath( ptcKey );
				break;
			}
//FLOAT STUFF ENDS HERE

		case SSC_ISBIT:
		case SSC_SETBIT:
		case SSC_CLRBIT:
			{
				const int64 val = Exp_GetLLVal(ptcKey);
				SKIP_ARGSEP(ptcKey);

				const uint bit = Exp_GetUVal(ptcKey);
				if (bit >= 64)
				{
					g_Log.EventError("%s shift argument ('%u') exceeds the maximum value of 63.\n", sm_szLoadKeys[index], bit);
					return false;
				}

				if ( index == SSC_ISBIT )
					sVal.FormatLLVal(val & (1ULL << bit));
				else if ( index == SSC_SETBIT )
					sVal.FormatLLVal(val | (1ULL << bit));
				else
					sVal.FormatLLVal(val & (~ (1ULL << bit)));
				break;
			}

		case SSC_ISEMPTY:
			sVal.FormatVal( IsStrEmpty( ptcKey ) );
			return true;

		case SSC_ISNUM:
			GETNONWHITESPACE( ptcKey );
			if (*ptcKey == '-')
				++ptcKey;
			sVal.FormatVal( IsStrNumeric( ptcKey ) );
			return true;

        case SSC_STRRANDRANGE:
            sVal = CExpression::GetExprParser().GetRangeString(ptcKey);
        return true;

		case SSC_StrPos:
			{
				GETNONWHITESPACE( ptcKey );
				int64 iPos = Exp_GetVal( ptcKey );
				tchar ch;
				if (IsDigit(*ptcKey) && IsDigit(*(ptcKey + 1)))
				{
					ch = static_cast<tchar>(Exp_GetVal(ptcKey));
				}
				else
				{
					ch = *ptcKey;
					++ptcKey;
				}

				GETNONWHITESPACE( ptcKey );
				int64 iLen	= strlen( ptcKey );
				if ( iPos < 0 )
					iPos	= iLen + iPos;
				if ( iPos < 0 )
					iPos	= 0;
				else if ( iPos > iLen )
					iPos	= iLen;

				lpctstr pszPos = strchr( ptcKey + iPos, ch );
				if ( !pszPos )
					sVal.FormatVal( -1 );
				else
					sVal.FormatVal((int)( pszPos - ptcKey ) );
			}
			return true;
        case SSC_StrSub:
        {
            tchar * ppArgs[3];
            int iQty = Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, ARRAY_COUNT(ppArgs));
            if ( iQty < 3 )
                return false;

            int64 iPos = Exp_GetVal( ppArgs[0] );
            int64 iCnt = Exp_GetVal( ppArgs[1] );
            if (iCnt < 0)
                return false;

            if ( *ppArgs[2] == '"')
                ++ppArgs[2];

            for (tchar *pEnd = ppArgs[2] + strlen(ppArgs[2]) - 1; pEnd >= ppArgs[2]; --pEnd)
            {
                if ( *pEnd == '"')
                {
                    *pEnd = '\0';
                    break;
                }
            }
            int64 iLen = strlen(ppArgs[2]);

			const bool fBackwards = (iPos < 0);
			if (fBackwards)
				iPos = iLen - iCnt;

			if ((iPos > iLen) || (iPos < 0))
				iPos = 0;

			if ((iPos + iCnt > iLen) || (iCnt == 0))
				iCnt = iLen - iPos;

			tchar* buf = Str_GetTemp();
			Str_CopyLimitNull(buf, ppArgs[2] + iPos, (size_t)(iCnt + 1));

			sVal = buf;
            }
            return true;
		case SSC_StrArg:
			{
				tchar * buf = Str_GetTemp();
				GETNONWHITESPACE( ptcKey );
				if ( *ptcKey == '"' )
					++ptcKey;

				size_t len = 0;
				while ( *ptcKey && !IsSpace( *ptcKey ) && *ptcKey != ',' )
				{
					buf[len] = *ptcKey;
					++ptcKey;
					++len;
				}
				buf[len]= '\0';
				sVal = buf;
			}
			return true;
		case SSC_StrEat:
			{

				GETNONWHITESPACE( ptcKey );
				while ( *ptcKey && !IsSpace( *ptcKey ) && *ptcKey != ',' )
					++ptcKey;
				SKIP_ARGSEP( ptcKey );
				sVal = ptcKey;
			}
			return true;
		case SSC_StrTrim:
			{
				if ( *ptcKey )
					sVal = Str_TrimWhitespace(const_cast<tchar*>(ptcKey));
				else
					sVal.Clear();

				return true;
			}
		case SSC_ASC:
			{
				tchar	*buf = Str_GetTemp();
				REMOVE_QUOTES( ptcKey );
				sVal.FormatLLHex( *ptcKey );
                Str_CopyLimitNull( buf, sVal, Str_TempLength() );
				while ( *(++ptcKey) )
				{
					if ( *ptcKey == '"' )
                        break;
					sVal.FormatLLHex(*ptcKey);
                    Str_ConcatLimitNull( buf, " ", Str_TempLength() );
                    Str_ConcatLimitNull( buf, sVal, Str_TempLength() );
				}
				sVal = buf;
			}
			return true;
		case SSC_ASCPAD:
			{
				tchar * ppArgs[2];
				int iQty = Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, ARRAY_COUNT(ppArgs));
				if ( iQty < 2 )
					return false;

				int64	iPad = Exp_GetVal( ppArgs[0] );
				if ( iPad < 0 )
					return false;
				tchar	*buf = Str_GetTemp();
				REMOVE_QUOTES( ppArgs[1] );
				sVal.FormatLLHex(*ppArgs[1]);
                Str_CopyLimitNull( buf, sVal, Str_TempLength() );
				while ( --iPad )
				{
					if ( *ppArgs[1] == '"' )
                        continue;
					if ( *(ppArgs[1]) )
					{
						++ppArgs[1];
						sVal.FormatLLHex(*ppArgs[1]);
					}
					else
						sVal.FormatLLHex('\0');

                    Str_ConcatLimitNull( buf, " ", Str_TempLength() );
                    Str_ConcatLimitNull( buf, sVal, Str_TempLength() );
				}
				sVal	= buf;
			}
			return true;
		case SSC_SYSCMD:
		case SSC_SYSSPAWN:
			{
				if ( !IsSetOF(OF_FileCommands) )
					return false;

				GETNONWHITESPACE(ptcKey);
				tchar	*buf = Str_GetTemp();
				tchar	*Arg_ppCmd[10];		// limit to 9 arguments
				Str_CopyLimitNull(buf, ptcKey, Str_TempLength());
				int iQty = Str_ParseCmds(buf, Arg_ppCmd, ARRAY_COUNT(Arg_ppCmd));
				if ( iQty < 1 )
					return false;

				bool bWait = (index == SSC_SYSCMD);
				g_Log.EventDebug("Process execution started (%s).\n", (bWait ? "SYSCMD" : "SYSSPAWN"));

#ifdef _WIN32
				_spawnl( bWait ? _P_WAIT : _P_NOWAIT,
					Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1],
					Arg_ppCmd[2], Arg_ppCmd[3], Arg_ppCmd[4],
					Arg_ppCmd[5], Arg_ppCmd[6], Arg_ppCmd[7],
					Arg_ppCmd[8], Arg_ppCmd[9], nullptr );
#else
                char *argv[] = {
                    Arg_ppCmd[0], Arg_ppCmd[0], Arg_ppCmd[1],
					Arg_ppCmd[2], Arg_ppCmd[3], Arg_ppCmd[4],
					Arg_ppCmd[5], Arg_ppCmd[6], Arg_ppCmd[7],
					Arg_ppCmd[8], Arg_ppCmd[9], nullptr
                };

				pid_t child_pid;
				if (posix_spawn(&child_pid, argv[0], nullptr, nullptr, argv, environ) != 0)
				{
                    g_Log.EventError(
                        "%s failed with error %d (\"%s\") when executing '%s'\n",
                        sm_szLoadKeys[index], errno, strerror(errno), ptcKey);
				}

				if(bWait) // parent process here (do we have to wait?)
				{
					int status;
					do
					{
						if(waitpid(child_pid, &status, 0))
							break;
					} while (!WIFSIGNALED(status) && !WIFEXITED(status));
					sVal.FormatLLHex(WEXITSTATUS(status));
				}
#endif
				g_Log.EventDebug("Process execution finished.\n");
				return true;
			}
        case SSC_StrToken:
        {
            tchar *ppArgs[3];
            int iQty = Str_ParseCmdsAdv(const_cast<tchar *>(ptcKey), ppArgs, ARRAY_COUNT(ppArgs), ",");
            if ( iQty < 3 )
                return false;

            tchar *iSep = Str_UnQuote(ppArgs[2]); //New function, trim (") and (') chars directly.
            for (tchar *iSeperator = iSep + strlen(iSep) - 1; iSeperator > iSep; --iSeperator)
                *iSeperator = '\0';

            tchar *pScriptArgs = Str_UnQuote(ppArgs[0]);
            sVal.Clear();
            tchar *ppCmd[255];
            int count = Str_ParseCmdsAdv(pScriptArgs, ppCmd, ARRAY_COUNT(ppCmd), iSep); //Remove unnecessary chars from seperator to avoid issues.
            tchar *ppArrays[2];

            //Getting range of array index...
            int iArrays = Str_ParseCmdsAdv(ppArgs[1], ppArrays, ARRAY_COUNT(ppArrays), "-");
            llong iValue = Exp_GetLLVal(ppArgs[1]);
            llong iValueEnd = iValue;
            if (iArrays > 1)
            {
                iValue = Exp_GetLLVal(ppArrays[0]);
                iValueEnd = Exp_GetLLVal(ppArrays[1]);
                if (iValueEnd <= 0 || iValueEnd > count)
                    iValueEnd = count;
            }

            if (iValue < 0)
                return false;
            else if (iValue > 0)
            {
                if (iValue > count)
                    return false;
                else if (iValue == iValueEnd)
                    sVal.Format(ppCmd[iValue - 1]);
                else
                {
                    sVal.Add(ppCmd[iValue - 1]);
                    for (int64 i = iValue + 1 ; i <= iValueEnd; ++i)
                    {
                        sVal.Add(iSep);
                        sVal.Add(ppCmd[i - 1]);
                    }
                }
            }
            else
                sVal.FormatVal(count);
        } return true;
		case SSC_EXPLODE:
			{
				char separators[16];

				GETNONWHITESPACE(ptcKey);
				Str_CopyLimitNull(separators, ptcKey, sizeof(separators));
				{
					char *p = separators;
					while ( *p && *p != ',' )
						++p;
					*p = 0;
				}

				const char *p = ptcKey + strlen(separators) + 1;
				sVal.Clear();
				if (( p > ptcKey ) && *p )		//	we have list of accessible separators
				{
					tchar *ppCmd[255];
					tchar * z = Str_GetTemp();
					Str_CopyLimitNull(z, p, Str_TempLength());
					const int count = Str_ParseCmds(z, ppCmd, ARRAY_COUNT(ppCmd), separators);
					if (count > 0)
					{
						sVal.Add(ppCmd[0]);
						for (int i = 1; i < count; ++i)
						{
							sVal.Add(',');
							sVal.Add(ppCmd[i]);
						}
					}
				}
				return true;
			}

		case SSC_MD5HASH:
			{
				char digest[33];
				GETNONWHITESPACE(ptcKey);

				CMD5::fastDigest( digest, ptcKey );
				sVal.Format("%s", digest);
			} return true;

        case SSC_BCRYPTHASH:
        {
            tchar * ppCmd[3];
            int iQty = Str_ParseCmds(const_cast<tchar*>(ptcKey), ppCmd, ARRAY_COUNT(ppCmd), ", ");
            if ( iQty < 3 )
                return false;

            std::optional<int> iconv;

            iconv = Str_ToI(ppCmd[0]);
            if (!iconv.has_value())
                return false;
            int iPrefixCode = *iconv;

            iconv = Str_ToI(ppCmd[1]);
            if (!iconv.has_value())
                return false;
            int iCost = *iconv;

            CSString sHash(CBCrypt::HashBCrypt(ppCmd[2], iPrefixCode, maximum(4,minimum(31,iCost))));
            sVal.Format("%s", sHash.GetBuffer());
        } return true;

        case SSC_BCRYPTVALIDATE:
        {
            tchar * ppCmd[2];
            int iQty = Str_ParseCmds(const_cast<tchar*>(ptcKey), ppCmd, ARRAY_COUNT(ppCmd), ", ");
            if ( iQty < 2 )
                return false;

            bool fValidated = CBCrypt::ValidateBCrypt(ppCmd[0], ppCmd[1]);
            sVal.FormatVal((int)fValidated);
        } return true;

		case SSC_MULDIV:
			{
				int64 iNum = Exp_GetLLVal( ptcKey );
				SKIP_ARGSEP(ptcKey);
				int64 iMul = Exp_GetLLVal( ptcKey );
				SKIP_ARGSEP(ptcKey);
				int64 iDiv = Exp_GetLLVal( ptcKey );
				int64 iRes = 0;

				if ( iDiv == 0 )
					g_Log.EventWarn("MULDIV(%" PRId64 ",%" PRId64 ",%" PRId64 ") -> Dividing by '0'\n", iNum, iMul, iDiv);
				else
					iRes = IMulDivLL(iNum,iMul,iDiv);

				sVal.FormatLLVal(iRes);
			}
			return true;

		case SSC_StrRegexNew:
			{
				size_t uiLenString = Exp_GetUVal( ptcKey );
				tchar * sToMatch = Str_GetTemp();
				if ( uiLenString > 0 )
				{
					SKIP_ARGSEP(ptcKey);
					Str_CopyLimitNull(sToMatch, ptcKey, uiLenString + 1);
					ptcKey += uiLenString;
				}

				SKIP_ARGSEP(ptcKey);
				tchar * tLastError = Str_GetTemp();
				int iDataResult = Str_RegExMatch( ptcKey, sToMatch, tLastError );
				sVal.FormatVal(iDataResult);

				if ( iDataResult == -1 )
				{
					DEBUG_ERR(( "STRREGEX bad function usage. Error: %s\n", tLastError ));
				}
			} return true;

		default:
			StringFunction( index, ptcKey, sVal );
			return true;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

enum SSV_TYPE
{
	SSV_NEW,
	SSV_NEWDUPE,
	SSV_NEWITEM,
    SSV_NEWNPC,
    SSV_NEWSUMMON,
	SSV_OBJ,
	SSV_SHOW,
	SSV_QTY
};

lpctstr const CScriptObj::sm_szVerbKeys[SSV_QTY+1] =
{
	"NEW",
	"NEWDUPE",
	"NEWITEM",
	"NEWNPC",
    "NEWSUMMON",
	"OBJ",
	"SHOW",
	nullptr
};


bool CScriptObj::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CScriptObj::r_Verb");
	ASSERT( pSrc );
	lpctstr ptcKey = s.GetKey();

	EXC_TRY("Verb-Ref");

	CScriptObj * pRef = nullptr;
	bool fRef = r_GetRefFull(ptcKey, pRef);

	if (fRef)
	{
		if (pRef == this)
		{
			// "I." ref. For players, *this (so pRef) is the CClient instance.

			if (ptcKey[0] == '\0')
			{
				return ParseError_UndefinedKeyword(s.GetKey()); // We can't change the UID via the I ref.
			}

			//fRef = false;
			//pRef = nullptr;
			CScript script(ptcKey, s.GetArgStr());
			script.CopyParseState(s);
			return r_Verb(script, pSrc);
		}
		else if ( ptcKey[0] )
		{
			if (!pRef)
			{
				if (s._eParseFlags == CScript::ParseFlags::IgnoreInvalidRef)
					return true;
				return ParseError_UndefinedKeyword(s.GetKey());
			}

			CScript script(ptcKey, s.GetArgStr());
			script.CopyParseState(s);

			if ( dynamic_cast<CAccount*>(pRef) != nullptr)
			{
				// Dirty fix:
				// If the REF is an ACCOUNT, it does special checks with the SRC to compare the PrivLevel to allow read/write its values.
				//	If i'm running in a trigger, so in a script, get the max privileges and change the SRC.
				CObjBase* pThisObj = dynamic_cast<CObjBase*>(this);
				if (pThisObj)
				{
					if (pThisObj->IsRunningTrigger())
					{
						pSrc = &g_Serv;
						ASSERT(pSrc);
					}
				}
			}

			return pRef->r_Verb( script, pSrc );
		}
		// else just fall through. as they seem to be setting the pointer !?
	}

	if ( s.IsKeyHead("SRC.", 4 ))
	{
		ptcKey += 4;
		pRef = dynamic_cast <CScriptObj*> (pSrc->GetChar());	// if it can be converted.
		if ( ! pRef )
		{
			pRef = dynamic_cast <CScriptObj*> (pSrc);
			if ( !pRef )
				return false;
		}

		CScript script(ptcKey, s.GetArgStr());
		script.CopyParseState(s);
		return pRef->r_Verb(script, pSrc);
	}


	EXC_SET_BLOCK("Verb-Statement");

	if (!strnicmp(ptcKey, "CLEARVARS", 9))
	{
		ptcKey = s.GetArgStr();
		SKIP_SEPARATORS(ptcKey);
        g_ExprGlobals.mtEngineLockedWriter()->m_VarGlobals.ClearKeys(ptcKey);
		return true;
	}

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, ARRAY_COUNT( sm_szVerbKeys )-1 );
	switch (index)
	{
		case SSV_OBJ:
			g_World.m_uidObj.SetObjUID(s.GetArgVal());
			if ( !g_World.m_uidObj.ObjFind() )
				g_World.m_uidObj.ClearUID();
			break;

		case SSV_NEW:
			g_World.m_uidNew.SetObjUID(s.GetArgVal());
			if ( !g_World.m_uidNew.ObjFind() )
				g_World.m_uidNew.ClearUID();
			break;

		case SSV_NEWDUPE:
			{
				CUID uid(s.GetArgVal());
				CObjBase *pObj = uid.ObjFind();
				if (pObj == nullptr)
				{
					g_World.m_uidNew.ClearUID();
					return false;
				}

				g_World.m_uidNew = uid;
				CScript script("DUPE");
				script.CopyParseState(s);
				bool bRc = pObj->r_Verb(script, pSrc);

				if (this != &g_Serv)
				{
					CChar *pChar = dynamic_cast <CChar *>(this);
					if ( pChar )
						pChar->m_Act_UID = g_World.m_uidNew;
					else
					{
						CClient *pClient = dynamic_cast <CClient *> (this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_UID = g_World.m_uidNew;
					}
				}
				return bRc;
			}

		case SSV_NEWITEM:	// just add an item but don't put it anyplace yet..
			{
				tchar * ppCmd[4];
				int iQty = Str_ParseCmds(s.GetArgRaw(), ppCmd, ARRAY_COUNT(ppCmd), ",");
				if ( iQty <= 0 )
					return false;

				CItem *pItem = CItem::CreateHeader(ppCmd[0], nullptr, false, pSrc->GetChar());
				if ( !pItem )
				{
					g_World.m_uidNew.ClearUID();
					return false;
				}
				pItem->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
				pItem->_iCreatedResScriptLine = s.m_iLineNum;

				if ( ppCmd[1] )
					pItem->SetAmount(Exp_GetWVal(ppCmd[1]));

				if ( ppCmd[2] )
				{
					CUID uidEquipper(Exp_GetVal(ppCmd[2]));
					const bool fTriggerEquip = (ppCmd[3] != nullptr) ? (Exp_GetVal(ppCmd[3]) != 0) : false;

					if (!fTriggerEquip || uidEquipper.IsItem())
					{
						pItem->LoadSetContainer(uidEquipper, LAYER_NONE);
					}
					else
					{
						if ( fTriggerEquip )
						{
							CChar * pCharEquipper = uidEquipper.CharFind();
							if ( pCharEquipper != nullptr )
								pCharEquipper->ItemEquip(pItem);
						}
					}
				}

				g_World.m_uidNew.SetObjUID(pItem->GetUID());

				if ( this != &g_Serv )
				{
					CChar *pChar = dynamic_cast <CChar *> (this);
					if ( pChar )
						pChar->m_Act_UID = g_World.m_uidNew;
					else
					{
						CClient *pClient = dynamic_cast <CClient *> (this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_UID = g_World.m_uidNew;
					}
				}
			}
			break;

		case SSV_NEWNPC:
			{
				CREID_TYPE id = CREID_TYPE(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgRaw()));
				CChar * pChar = CChar::CreateNPC(id);
				if ( !pChar )
				{
					g_World.m_uidNew.ClearUID();
					return false;
				}

				pChar->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
				pChar->_iCreatedResScriptLine = s.m_iLineNum;

				g_World.m_uidNew.SetObjUID(pChar->GetUID());

				if ( this != &g_Serv )
				{
					pChar = dynamic_cast<CChar *>(this);
					if ( pChar )
						pChar->m_Act_UID = g_World.m_uidNew;
					else
					{
                        CClient *pClient = dynamic_cast<CClient *>(this);
						if ( pClient && pClient->GetChar() )
							pClient->GetChar()->m_Act_UID = g_World.m_uidNew;
					}
				}
			}
			break;
        case SSV_NEWSUMMON:
	        {
	            tchar * ppCmd[2];
	            int iQty = Str_ParseCmds(s.GetArgRaw(), ppCmd, ARRAY_COUNT(ppCmd), ",");
	            if (iQty <= 0)
	                return false;

	            CREID_TYPE id = CREID_TYPE(g_Cfg.ResourceGetIndexType(RES_CHARDEF, ppCmd[0]));
	            CChar * pChar = CChar::CreateNPC(id);
	            CChar * pCharSrc = nullptr;
	            if (!pChar)
	            {
	                g_World.m_uidNew.ClearUID();
	                return false;
	            }

				pChar->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
				pChar->_iCreatedResScriptLine = s.m_iLineNum;

	            if (this != &g_Serv)
	            {
	                pCharSrc = pSrc->GetChar();
	                if (pCharSrc)
	                    pCharSrc->m_Act_UID = g_World.m_uidNew;
	                else
	                {
                        CClient *pClient = dynamic_cast<CClient *>(this);
	                    if (pClient && pClient->GetChar())
	                        pClient->GetChar()->m_Act_UID = g_World.m_uidNew;
	                }
	            }

	            pChar->OnSpellEffect(SPELL_Summon, pCharSrc, pChar->Skill_GetAdjusted(SKILL_MAGERY), nullptr, false);
	            g_World.m_uidNew.SetObjUID(pChar->GetUID());
	            llong iDuration = Exp_GetVal(ppCmd[1]);
	            if (iDuration)
	            {
	                CItem * pItemRune = pChar->LayerFind(LAYER_SPELL_Summon);
	                if (pItemRune)
	                    pItemRune->SetTimeout(iDuration * MSECS_PER_SEC);
	            }
	        }
			break;
		case SSV_SHOW:
			{
				CSString sOriginalArg(s.GetArgRaw());
				CSString sVal;
				if ( ! r_WriteVal( s.GetArgStr(), sVal, pSrc ) )
					return false;
				tchar * pszMsg = Str_GetTemp();
				snprintf(pszMsg, Str_TempLength(), "'%s' for '%s' is '%s'\n", sOriginalArg.GetBuffer(), GetName(), sVal.GetBuffer());
				pSrc->SysMessage(pszMsg);
				break;
			}
		default:
			return r_LoadVal(s);		// default to loading values
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CScriptObj::r_Load( CScript & s )
{
	ADDTOCALLSTACK("CScriptObj::r_Load");
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead( "ON", 2 ) )	// trigger scripting marks the end
			break;

        r_LoadVal(s);
	}
	return true;
}

bool CScriptObj::Execute_Call(CScript& s, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc)
{
	ADDTOCALLSTACK("CScriptObj::Execute_Call");
	bool fRes = false;

	CSString sVal;
	tchar* argRaw = s.GetArgRaw();
	CScriptObj* pRef = this;

	// Parse object references, src.* is not parsed
	// by r_GetRef so do it manually
    r_GetRef(const_cast<lpctstr&>(reinterpret_cast<lptstr&>(argRaw)), pRef);
	if (!strnicmp("SRC.", argRaw, 4))
	{
		argRaw += 4;
		pRef = pSrc->GetChar();
	}

	// Check that an object is referenced
	if (pRef != nullptr)
	{
		// Locate arguments for the called function
		tchar* z = strchr(argRaw, ' ');

		if (z)
		{
			*z = 0;
			++z;
			GETNONWHITESPACE(z);
		}

		if (z && *z)
		{
            int64 iN1 = pScriptArgs->m_iN1;
            int64 iN2 = pScriptArgs->m_iN2;
            int64 iN3 = pScriptArgs->m_iN3;
            CScriptObj* pO1 = pScriptArgs->m_pO1;
            CSString s1 = pScriptArgs->m_s1;
            CSString s1_raw = pScriptArgs->m_s1_buf_vec;
            pScriptArgs->m_v.clear();
            pScriptArgs->Init(z);

            fRes = pRef->r_Call(argRaw, pScriptArgs, pSrc, &sVal);

            pScriptArgs->m_iN1 = iN1;
            pScriptArgs->m_iN2 = iN2;
            pScriptArgs->m_iN3 = iN3;
            pScriptArgs->m_pO1 = pO1;
            pScriptArgs->m_s1 = s1;
            pScriptArgs->m_s1_buf_vec = s1_raw;
            pScriptArgs->m_v.clear();
		}
		else
            fRes = pRef->r_Call(argRaw, pScriptArgs, pSrc, &sVal);
	}

	return fRes;
}

bool CScriptObj::Execute_FullTrigger(CScript& s, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc)
{
	ADDTOCALLSTACK("CScriptObj::Execute_FullTrigger");
	bool fRes = false;

	tchar* piCmd[7];
	tchar* ptcTmp = Str_GetTemp();
	Str_CopyLimitNull(ptcTmp, s.GetArgRaw(), Str_TempLength());
	int iArgQty = Str_ParseCmds(ptcTmp, piCmd, ARRAY_COUNT(piCmd), " ,\t");

	CScriptObj* pRef = this;
	if (iArgQty == 2)
	{
        std::optional<dword> iconv = Str_ToU(piCmd[1]);
        if (!iconv.has_value())
            return false;

		CChar* pCharFound = CUID::CharFindFromUID(*iconv);
		if (pCharFound)
			pRef = pCharFound;
	}

	// Parse object references, src.* is not parsed
	// by r_GetRef so do it manually
	//r_GetRef(const_cast<lpctstr &>(static_cast<lptstr &>(argRaw)), pRef);
	if (!strnicmp("SRC.", ptcTmp, 4))
	{
		ptcTmp += 4;
		pRef = pSrc->GetChar();
	}

	// Check that an object is referenced
	if (pRef != nullptr)
	{
		// Locate arguments for the called function
		TRIGRET_TYPE tRet;
		tchar* z = strchr(ptcTmp, ' ');

		if (z)
		{
			*z = 0;
			++z;
			GETNONWHITESPACE(z);
		}

		if (z && *z)
		{
            int64 iN1 = pScriptArgs->m_iN1;
            int64 iN2 = pScriptArgs->m_iN2;
            int64 iN3 = pScriptArgs->m_iN3;
            CScriptObj* pO1 = pScriptArgs->m_pO1;
            CSString s1 = pScriptArgs->m_s1;
            CSString s1_raw = pScriptArgs->m_s1_buf_vec;
            pScriptArgs->m_v.clear();
            pScriptArgs->Init(z);

            tRet = pRef->OnTrigger(ptcTmp, pScriptArgs, pSrc);

            pScriptArgs->m_iN1 = iN1;
            pScriptArgs->m_iN2 = iN2;
            pScriptArgs->m_iN3 = iN3;
            pScriptArgs->m_pO1 = pO1;
            pScriptArgs->m_s1 = s1;
            pScriptArgs->m_s1_buf_vec = s1_raw;
            pScriptArgs->m_v.clear();
		}
		else
            tRet = pRef->OnTrigger(ptcTmp, pScriptArgs, pSrc);

        pScriptArgs->m_VarsLocal.SetNum("return", tRet, false);
		fRes = (tRet > 0) ? 1 : 0;
	}

	return fRes;
}


bool CScriptObj::OnTriggerFind( CScript & s, lpctstr pszTrigName )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerFind");
	while ( s.ReadKey(false) )
	{
		// Is it a trigger ?
		if (strnicmp(s.GetKey(), "ON", 2) != 0)
			continue;

		// Is it the right trigger ?
		s.ParseKeyLate();
		if (strcmpi(s.GetArgRaw(), pszTrigName) == 0)
			return true;
	}
	return false;
}

TRIGRET_TYPE CScriptObj::OnTriggerScript( CScript & s, lpctstr pszTrigName, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerScript");
	// look for exact trigger matches
	if ( !OnTriggerFind(s, pszTrigName) )
		return TRIGRET_RET_DEFAULT;

	/*
	This is the same assignement found in the CChar::OnTrigger method (CCharAct.cpp).
	If pSrc is null (for example the caster is dead when the spell triggers @EffectRemove or @EffectTick are fired) we assign the server object to it.
	So the script can use safely use the isvalid command, otherwise Sphere will crash when accessing src in these scripts.
	*/
	if (!pSrc)
		pSrc = &g_Serv;

	const ProfileTask scriptsTask(PROFILE_SCRIPTS);

	CScriptProfiler::CScriptProfilerTrigger	*pTrig = nullptr;
	TIME_PROFILE_INIT;

	//	If script profiler is on, search this trigger record and get pointer to it
	//	if not, create the corresponding record
	if ( IsSetEF(EF_Script_Profiler) )
	{
		tchar * pName = Str_GetTemp();

		//	lowercase for speed
		Str_CopyLimitNull(pName, pszTrigName, Str_TempLength());
		_strlwr(pName);

		if ( g_profiler.initstate != 0xf1 )	// it is not initalised
		{
			memset(&g_profiler, 0, sizeof(g_profiler));
			g_profiler.initstate = (uchar)(0xf1); // ''
		}

		for ( pTrig = g_profiler.TriggersHead; pTrig != nullptr; pTrig = pTrig->next )
		{
			if ( !strcmp(pTrig->name, pName) )
				break;
		}

		// first time this trigger is called. so create a record for it
		if ( pTrig == nullptr )
		{
			pTrig = new CScriptProfiler::CScriptProfilerTrigger;
			memset(pTrig, 0, sizeof(CScriptProfiler::CScriptProfilerTrigger));
			Str_CopyLimitNull(pTrig->name, pName, sizeof(pTrig->name));
			if ( g_profiler.TriggersTail )
				g_profiler.TriggersTail->next = pTrig;
			else
				g_profiler.TriggersHead = pTrig;
			g_profiler.TriggersTail = pTrig;
		}

		//	prepare the informational block
		++ pTrig->called;
		++ g_profiler.called;
		TIME_PROFILE_START;
	}

    TRIGRET_TYPE iRet = OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc);

	if ( IsSetEF(EF_Script_Profiler) && pTrig != nullptr )
	{
		//	update the time call information
		TIME_PROFILE_END;
		llTicksStart = llTicksEnd - llTicksStart;
		pTrig->total += llTicksStart;
        pTrig->average = (pTrig->total / pTrig->called);
		if ( pTrig->max < llTicksStart )
			pTrig->max = llTicksStart;
		if (( pTrig->min > llTicksStart ) || ( !pTrig->min ))
			pTrig->min = llTicksStart;
		g_profiler.total += llTicksStart;
	}

	return iRet;
}

TRIGRET_TYPE CScriptObj::OnTrigger( lpctstr pszTrigName, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc)
{
	UnreferencedParameter(pszTrigName);
	UnreferencedParameter(pSrc);
    UnreferencedParameter(pScriptArgs);
    ASSERT(false); // I shouldn't get here?
	return( TRIGRET_RET_DEFAULT );
}


enum SK_TYPE : int
{
	SK_BEGIN,
	SK_BREAK,
	SK_CONTINUE,
	SK_DORAND,
	SK_DOSWITCH,
	SK_ELIF,
	SK_ELSE,
	SK_ELSEIF,
	SK_END,
	SK_ENDDO,
	SK_ENDFOR,
	SK_ENDIF,
	SK_ENDRAND,
	SK_ENDSWITCH,
	SK_ENDWHILE,
	SK_FOR,
	SK_FORCHARLAYER,
	SK_FORCHARMEMORYTYPE,
	SK_FORCHAR,
	SK_FORCLIENTS,
	SK_FORCONT,
	SK_FORCONTID,		// loop through all items with this ID in the cont
	SK_FORCONTTYPE,
	SK_FORINSTANCE,
	SK_FORITEM,
	SK_FOROBJ,
	SK_FORPLAYERS,		// not necessary to be online
	SK_FORTIMERF,
	SK_IF,
	SK_RETURN,
	SK_WHILE,
	SK_QTY
};


lpctstr const CScriptObj::sm_szScriptKeys[SK_QTY+1] =
{
	"BEGIN",
	"BREAK",
	"CONTINUE",
	"DORAND",
	"DOSWITCH",
	"ELIF",
	"ELSE",
	"ELSEIF",
	"END",
	"ENDDO",
	"ENDFOR",
	"ENDIF",
	"ENDRAND",
	"ENDSWITCH",
	"ENDWHILE",
	"FOR",
	"FORCHARLAYER",
	"FORCHARMEMORYTYPE",
	"FORCHARS",
	"FORCLIENTS",
	"FORCONT",
	"FORCONTID",
	"FORCONTTYPE",
	"FORINSTANCES",
	"FORITEMS",
	"FOROBJS",
	"FORPLAYERS",
	"FORTIMERF",
	"IF",
	"RETURN",
	"WHILE",
	nullptr
};

TRIGRET_TYPE CScriptObj::OnTriggerLoopGeneric(CScript& s, int iType, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc, CSString* pResult)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerLoopGeneric");
	// loop from start here to the ENDFOR
	// See WebPageScriptList for dealing with Arrays.

    ASSERT(pScriptArgs);

    CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext(StartContext);
	int LoopsMade = 0;

    CExpression& expr_parser = CExpression::GetExprParser();
    CScriptExprContext context;

	if (iType & 8)		// WHILE
	{
		TemporaryString tsConditionBuf;
		TemporaryString tsOrig;
		Str_CopyLimitNull(tsOrig.buffer(), s.GetArgStr(), tsOrig.capacity());

		int iWhile = 0;
		for (;;)
		{
			++LoopsMade;
			if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
				goto toomanyloops;

			tchar* ptcCond = tsConditionBuf.buffer();
			Str_CopyLimitNull(ptcCond, tsOrig.buffer(), tsConditionBuf.capacity());

            context = {._pScriptObjI = this};
            expr_parser.ParseScriptText(ptcCond, context, pScriptArgs, pSrc, 0);
			if (!Exp_GetLLVal(ptcCond))
				break;

            pScriptArgs->m_VarsLocal.SetNum("_WHILE", iWhile, false);
			++iWhile;

            TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
			if (iRet == TRIGRET_BREAK)
			{
				EndContext = StartContext;
				break;
			}
			if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
				return iRet;
			if (iRet == TRIGRET_CONTINUE)
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext(StartContext);
		}
	}
	else
	{
        context = {._pScriptObjI = this};
        expr_parser.ParseScriptText(s.GetArgStr(), context, pScriptArgs, pSrc, 0);
	}


	if (iType & 4)		// FOR
	{
		bool fCountDown = false;
		int iMin = 0;
		int iMax = 0;
		tchar* ppArgs[3];
		int iQty = Str_ParseCmds(s.GetArgStr(), ppArgs, ARRAY_COUNT(ppArgs), ", ");
		CSString sLoopVar("_FOR");

		switch (iQty)
		{
		case 1:		// FOR x
			iMin = 1;
			iMax = Exp_GetSingle(ppArgs[0]);
			break;
		case 2:
			if (IsDigit(*ppArgs[0]) || ((*ppArgs[0] == '-') && IsDigit(*(ppArgs[0] + 1))))
			{
				iMin = Exp_GetSingle(ppArgs[0]);
				iMax = Exp_GetSingle(ppArgs[1]);
			}
			else
			{
				sLoopVar = ppArgs[0];
				iMin = 1;
				iMax = Exp_GetSingle(ppArgs[1]);
			}
			break;
		case 3:
			sLoopVar = ppArgs[0];
			iMin = Exp_GetSingle(ppArgs[1]);
			iMax = Exp_GetSingle(ppArgs[2]);
			break;
		default:
			iMin = iMax = 1;
			break;
		}

		if (iMin > iMax)
			fCountDown = true;

		if (fCountDown)
			for (int i = iMin; i >= iMax; --i)
			{
				++LoopsMade;
				if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
					goto toomanyloops;

                pScriptArgs->m_VarsLocal.SetNum(sLoopVar, i, false);
                TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
				if (iRet == TRIGRET_BREAK)
				{
					EndContext = StartContext;
					break;
				}
				if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
					return iRet;

				if (iRet == TRIGRET_CONTINUE)
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);
			}
		else
			for (int i = iMin; i <= iMax; ++i)
			{
				++LoopsMade;
				if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
					goto toomanyloops;

                pScriptArgs->m_VarsLocal.SetNum(sLoopVar, i, false);
                TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
				if (iRet == TRIGRET_BREAK)
				{
					EndContext = StartContext;
					break;
				}
				if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
					return iRet;

				if (iRet == TRIGRET_CONTINUE)
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);
			}
	}

	if ((iType & 1) || (iType & 2))
	{
		int iDist;
		if (s.HasArgs())
			iDist = s.GetArgVal();
		else
			iDist = g_Cfg.m_iMapViewSize;

		CObjBaseTemplate* pObj = dynamic_cast <CObjBaseTemplate*>(this);
		if (pObj == nullptr)
		{
			iType = 0;
			DEBUG_ERR(("FOR Loop trigger on non-world object '%s'\n", GetName()));
		}
		else
		{
			CObjBaseTemplate* pObjTop = pObj->GetTopLevelObj();
			CPointMap pt = pObjTop->GetTopPoint();
			if (iType & 1)		// FORITEM, FOROBJ
			{
				auto AreaItems = CWorldSearchHolder::GetInstance(pt, iDist);
				for (;;)
				{
					++LoopsMade;
					if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
						goto toomanyloops;

					CItem* pItem = AreaItems->GetItem();
					if (pItem == nullptr)
						break;
                    TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
					if (iRet == TRIGRET_BREAK)
					{
						EndContext = StartContext;
						break;
					}
					if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
						return(iRet);
					if (iRet == TRIGRET_CONTINUE)
						EndContext = StartContext;
					else
						EndContext = s.GetContext();
					s.SeekContext(StartContext);
				}
			}
			if (iType & 2)		// FORCHAR, FOROBJ
			{
				auto AreaChars = CWorldSearchHolder::GetInstance(pt, iDist);
				AreaChars->SetAllShow((iType & 0x20) ? true : false);
				for (;;)
				{
					++LoopsMade;
					if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
						goto toomanyloops;

					CChar* pChar = AreaChars->GetChar();
					if (pChar == nullptr)
						break;
					if ((iType & 0x10) && (!pChar->IsClientActive()))	// FORCLIENTS
						continue;
					if ((iType & 0x20) && (pChar->m_pPlayer == nullptr))	// FORPLAYERS
						continue;
                    TRIGRET_TYPE iRet = pChar->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
					if (iRet == TRIGRET_BREAK)
					{
						EndContext = StartContext;
						break;
					}
					if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
						return(iRet);
					if (iRet == TRIGRET_CONTINUE)
						EndContext = StartContext;
					else
						EndContext = s.GetContext();
					s.SeekContext(StartContext);
				}
			}
		}
	}

	if (iType & 0x40)	// FORINSTANCES
	{
		CResourceID rid;
		tchar* pArg = s.GetArgStr();
		Str_Parse(pArg, nullptr, " \t,");

		if (*pArg != '\0')
		{
			rid = g_Cfg.ResourceGetID(RES_UNKNOWN, pArg);
		}
		else
		{
			g_Log.EventError("FORINSTANCES called without argument.\n");

			// This usage is not allowed anymore, it's an ambiguous and error prone usage
			/*
			const CObjBase * pObj = dynamic_cast <CObjBase*>(this);
			if ( pObj && pObj->Base_GetDef() )
			{
				rid = pObj->Base_GetDef()->GetResourceID();
			}
			*/
		}

		// No need to loop if there is no valid resource id
		if (rid.IsValidUID())
		{
			const dword dwTotal = g_World.GetUIDCount();
            if (dwTotal == 0)
                return TRIGRET_ENDIF;

			dword dwCount = dwTotal - 1;
			dword dwTotalInstances = 0; // Will acquire the correct value for this during the loop
			dword dwUID = 0;
			dword dwFound = 0;

			while (dwCount)
			{
				// Check the current UID to test is within our range
				if (++dwUID >= dwTotal)
					break;

                dwCount -= 1;

				// Acquire the object with this UID and check it exists
				CObjBase* pObj = g_World.FindUID(dwUID);
				if (pObj == nullptr)
					continue;

				const CBaseBaseDef* pBase = pObj->Base_GetDef();
				ASSERT(pBase);
				// Check to see if the object resource id matches what we're looking for
				if (pBase->GetResourceID() != rid)
					continue;

				// Check we do not loop too many times
				++LoopsMade;
				if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
					goto toomanyloops;

				// Execute script on this object
                TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult);
				if (iRet == TRIGRET_BREAK)
				{
					EndContext = StartContext;
					break;
				}
				if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
					return(iRet);
				if (iRet == TRIGRET_CONTINUE)
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
				s.SeekContext(StartContext);

				// Acquire the total instances that exist for this item if we can
				if (dwTotalInstances == 0)
					dwTotalInstances = pBase->GetRefInstances();

				++dwFound;

				// If we know how many instances there are, abort the loop once we've found them all
				if ((dwTotalInstances > 0) && (dwFound >= dwTotalInstances))
					break;
			}
		}
	}

	if (iType & 0x100)	// FORTIMERF
	{
		tchar* ptcArgs;
		if (Str_Parse(s.GetArgStr(), &ptcArgs, " \t,"))
		{
			char funcname[1024];
			Str_CopyLimitNull(funcname, ptcArgs, sizeof(funcname));

            TRIGRET_TYPE iRet = CWorldTimedFunctions::Loop(funcname, LoopsMade, StartContext, s, pScriptArgs, pSrc, pResult);
			if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
				return iRet;
		}
	}

	if (g_Cfg.m_iMaxLoopTimes)
	{
	toomanyloops:
		if (LoopsMade >= g_Cfg.m_iMaxLoopTimes)
			g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", LoopsMade);
	}

	if (EndContext.m_iOffset <= StartContext.m_iOffset)
	{
		// just skip to the end.
        TRIGRET_TYPE iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
		if (iRet != TRIGRET_ENDIF)
			return iRet;
	}
	else
		s.SeekContext(EndContext);

	return TRIGRET_ENDIF;
}

TRIGRET_TYPE CScriptObj::OnTriggerLoopForCharSpecial(CScript& s, SK_TYPE iCmd, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc, CSString* pResult)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerLoopForCharSpecial");
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
	CChar* pCharThis = dynamic_cast <CChar*> (this);

	if (pCharThis)
	{
		if (s.HasArgs())
		{
            CExpression& expr_parser = CExpression::GetExprParser();
            CScriptExprContext context = {._pScriptObjI = this};

            expr_parser.ParseScriptText(s.GetArgRaw(), context, pScriptArgs, pSrc, 0);
			if (iCmd == SK_FORCHARLAYER)
                iRet = pCharThis->OnCharTrigForLayerLoop(s, pScriptArgs, pSrc, pResult, (LAYER_TYPE)s.GetArgVal());
			else
                iRet = pCharThis->OnCharTrigForMemTypeLoop(s, pScriptArgs, pSrc, pResult, s.GetArgWVal());
		}
		else
		{
			g_Log.EventError("FORCHAR[layer/memorytype] called on char 0%" PRIx32 " (%s) without arguments.\n",
                             (dword)(pCharThis->GetUID()), pCharThis->GetName());
            iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
		}
	}
	else
	{
		g_Log.EventError("FORCHAR[layer/memorytype] called on non-char object '%s'.\n", GetName());
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
	}

	return iRet;
}

TRIGRET_TYPE CScriptObj::OnTriggerLoopForCont(CScript& s, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc, CSString* pResult)
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerLoopForCont");
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	if (s.HasArgs())
	{
		tchar* ppArgs[2];
		int iArgQty = Str_ParseCmds(const_cast<tchar*>(s.GetArgRaw()), ppArgs, ARRAY_COUNT(ppArgs), " \t,");

		if (iArgQty > 1)
		{
			TemporaryString tsOrigValue;
			tchar* ptcOrigValue = tsOrigValue.buffer();
			Str_ConcatLimitNull(ptcOrigValue, ppArgs[0], tsOrigValue.capacity());

            CExpression& expr_parser = CExpression::GetExprParser();
            CScriptExprContext context = {._pScriptObjI = this};
            expr_parser.ParseScriptText(ptcOrigValue, context, pScriptArgs, pSrc, 0);

			CUID pCurUid(Exp_GetDWVal(ptcOrigValue));
			if (pCurUid.IsValidUID())
			{
				CObjBase* pObj = pCurUid.ObjFind();
				if (pObj && pObj->IsContainer())
				{
					CContainer* pContThis = dynamic_cast<CContainer*>(pObj);
					ASSERT(pContThis);

					CScriptLineContext StartContext = s.GetContext();
					CScriptLineContext EndContext = StartContext;
                    iRet = pContThis->OnGenericContTriggerForLoop(s, pScriptArgs, pSrc, pResult, StartContext, EndContext, ppArgs[1] != nullptr ? Exp_GetVal(ppArgs[1]) : 255);
				}
				else
				{
					g_Log.EventError("FORCONT called on invalid uid/invalid container (UID: 0%x).\n", pCurUid.GetObjUID());
                    iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
				}
			}
			else
			{
				g_Log.EventError("FORCONT called with invalid arguments (UID: 0%x, LEVEL: %s).\n", pCurUid.GetObjUID(), (ppArgs[1] && *ppArgs[1]) ? ppArgs[1] : "255");
                iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
			}
		}
		else
		{
			g_Log.EventError("FORCONT called with insufficient arguments.\n");
            iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
		}
	}
	else
	{
		g_Log.EventError("FORCONT called without arguments.\n");
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
	}

	return iRet;
}

TRIGRET_TYPE CScriptObj::OnTriggerLoopForContSpecial(CScript& s, SK_TYPE iCmd, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc, CSString* pResult)
{
    ADDTOCALLSTACK("CScriptObj::OnTriggerLoopForContSpecial");
    TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

    CObjBase* pObjCont = dynamic_cast <CObjBase*> (this);
    CContainer* pCont = dynamic_cast <CContainer*> (this);
    if (!pObjCont || !pCont)
    {
        g_Log.EventError("FORCONT[id/type] called on non-container object '%s'.\n", GetName());
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
        return iRet;
    }

    if (!s.HasArgs())
    {
        g_Log.EventError("FORCONT[id/type] called on container 0%x without arguments.\n", (dword)pObjCont->GetUID());
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
        return iRet;
    }

    lptstr ptcKey = s.GetArgRaw();
    SKIP_SEPARATORS(ptcKey);

    tchar* ppArgs[2];
    if (Str_ParseCmds(ptcKey, ppArgs, ARRAY_COUNT(ppArgs), " \t,") < 1)
    {
        g_Log.EventError("FORCONT[id/type] called on container 0%x with incorrect arguments.\n", (dword)pObjCont->GetUID());
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
        return iRet;
    }

    CExpression& expr_parser = CExpression::GetExprParser();
    CScriptExprContext context = {._pScriptObjI = this};

    TemporaryString tsParsedArg0;
    Str_CopyLimitNull(tsParsedArg0.buffer(), ppArgs[0], tsParsedArg0.capacity());
    if (expr_parser.ParseScriptText(tsParsedArg0.buffer(), context, pScriptArgs, pSrc, 0) <= 0)
    {
        g_Log.EventError("FORCONT[id/type] called on container 0%x with incorrect argument 0.\n", (dword)pObjCont->GetUID());
        iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
        return iRet;
    }

    context = {._pScriptObjI = this};
    TemporaryString tsParsedArg1;
    if (ppArgs[1] != nullptr)
    {
        Str_CopyLimitNull(tsParsedArg1.buffer(), ppArgs[1], tsParsedArg0.capacity());
        if (expr_parser.ParseScriptText(tsParsedArg1.buffer(), context, pScriptArgs, pSrc, 0) <= 0)
        {
            g_Log.EventError("FORCONT[id/type] called on container 0%x with incorrect argument 1.\n", (dword)pObjCont->GetUID());
            iRet = OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult);
            return iRet;
        }
    }

    CScriptLineContext StartContext = s.GetContext();
    CScriptLineContext EndContext(StartContext);
    lpctstr ptcParsedArg1 = tsParsedArg1.buffer();

    const RES_TYPE ridExpectedType = ((iCmd == SK_FORCONTID) ? RES_ITEMDEF : RES_TYPEDEF);
    const CResourceID &rid = g_Cfg.ResourceGetID(ridExpectedType, tsParsedArg0.buffer());

    constexpr dword dwArg = 0;
    const int iDescendLevels = ((ppArgs[1] != nullptr) ? Exp_GetVal(ptcParsedArg1) : 255);
    iRet = pCont->OnContTriggerForLoop(
        s, pScriptArgs, pSrc, pResult,
        StartContext, EndContext,
        rid, dwArg, iDescendLevels);

    return iRet;
}

TRIGRET_TYPE CScriptObj::OnTriggerRun( CScript &s, TRIGRUN_TYPE trigrun, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc, CSString * pResult )
{
	ADDTOCALLSTACK("CScriptObj::OnTriggerRun");
	// ARGS:
	//	TRIGRUN_SECTION_SINGLE = just this 1 line.
	// RETURN:
	//  TRIGRET_RET_FALSE = 0 = return and continue processing.
	//  TRIGRET_RET_TRUE = 1 = return and handled. (halt further processing)
	//  TRIGRET_RET_DEFAULT = 2 = if process returns nothing specifically.

	// CScriptFileContext set g_Log.m_pObjectContext is the current context (we assume)
	// DEBUGCHECK( this == g_Log.m_pObjectContext );

	//	all scripts should have args for locals to work.

    ASSERT(pScriptArgs);

    static constexpr uint g_reentrant_OnTriggerRun_limit = 75;
    static thread_local size_t g_reentrant_OnTriggerRun = 0;
    auto clean_return = [](const TRIGRET_TYPE ret) noexcept -> TRIGRET_TYPE {
        g_reentrant_OnTriggerRun -= 1;
        return ret;
    };

    g_reentrant_OnTriggerRun += 1;
    if (g_reentrant_OnTriggerRun >= g_reentrant_OnTriggerRun_limit)
    {
        g_Log.Event(LOGL_CRIT, "Parsing of the current script is HALTED. Some code is calling itself recursively.\n");
        return clean_return(TRIGRET_RET_ABORTED);
    }

	//	Script execution is always not threaded action
	EXC_TRY("TriggerRun");

    const bool fSectionFalse = (trigrun == TRIGRUN_SECTION_FALSE || trigrun == TRIGRUN_SINGLE_FALSE);
	if ( trigrun == TRIGRUN_SECTION_EXEC || trigrun == TRIGRUN_SINGLE_EXEC )	// header was already read in.
		goto jump_in;

	EXC_SET_BLOCK("parsing");
	while ( s.ReadKeyParse())
	{
		// Hit the end of the next trigger.
		if ( s.IsKeyHead( "ON", 2 ))	// done with this section.
			break;

jump_in:
		SK_TYPE iCmd = (SK_TYPE) FindTableSorted( s.GetKey(), sm_szScriptKeys, ARRAY_COUNT( sm_szScriptKeys )-1 );
		TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

		switch ( iCmd )
		{
			case SK_ENDIF:
			case SK_END:
			case SK_ENDDO:
			case SK_ENDFOR:
			case SK_ENDRAND:
			case SK_ENDSWITCH:
			case SK_ENDWHILE:
				return clean_return(TRIGRET_ENDIF);

			case SK_ELIF:
			case SK_ELSEIF:
				return clean_return(TRIGRET_ELSEIF);

			case SK_ELSE:
				return clean_return(TRIGRET_ELSE);

			default:
				break;
		}

		if ( fSectionFalse )
		{
			// Ignoring this whole section. don't bother parsing it.
			switch ( iCmd )
			{
				case SK_IF:
					EXC_SET_BLOCK("if statement");
					do
					{
                        iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult );
					} while ( iRet == TRIGRET_ELSEIF || iRet == TRIGRET_ELSE );
					break;
				case SK_WHILE:
				case SK_FOR:
				case SK_FORCHARLAYER:
				case SK_FORCHARMEMORYTYPE:
				case SK_FORCHAR:
				case SK_FORCLIENTS:
				case SK_FORCONT:
				case SK_FORCONTID:
				case SK_FORCONTTYPE:
				case SK_FORINSTANCE:
				case SK_FORITEM:
				case SK_FOROBJ:
				case SK_FORPLAYERS:
				case SK_FORTIMERF:
				case SK_DORAND:
				case SK_DOSWITCH:
				case SK_BEGIN:
					EXC_SET_BLOCK("begin/loop cycle");
                    iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult );
					break;
				default:
					break;
			}
			if ( trigrun >= TRIGRUN_SINGLE_EXEC )
				return clean_return(TRIGRET_RET_DEFAULT);
			continue;	// just ignore it.
		}

		// At this point i should be in a new section and i want to parse it fully.
		// Start by parsing the arguments and/or executing special statements.
		switch ( iCmd )
		{
			case SK_BREAK:
				return clean_return(TRIGRET_BREAK);

			case SK_CONTINUE:
				return clean_return(TRIGRET_CONTINUE);

            case SK_FORITEM:	EXC_SET_BLOCK("foritem");		iRet = OnTriggerLoopGeneric(s, 1,    pScriptArgs, pSrc, pResult); break;
            case SK_FORCHAR:	EXC_SET_BLOCK("forchar");		iRet = OnTriggerLoopGeneric(s, 2,    pScriptArgs, pSrc, pResult);	break;
            case SK_FORCLIENTS:	EXC_SET_BLOCK("forclients");	iRet = OnTriggerLoopGeneric(s, 0x12, pScriptArgs, pSrc, pResult);	break;
            case SK_FOROBJ:		EXC_SET_BLOCK("forobjs");		iRet = OnTriggerLoopGeneric(s, 3,    pScriptArgs, pSrc, pResult);	break;
            case SK_FORPLAYERS:	EXC_SET_BLOCK("forplayers");	iRet = OnTriggerLoopGeneric(s, 0x22, pScriptArgs, pSrc, pResult);	break;
            case SK_FOR:		EXC_SET_BLOCK("for");			iRet = OnTriggerLoopGeneric(s, 4,    pScriptArgs, pSrc, pResult);	break;
            case SK_WHILE:		EXC_SET_BLOCK("while");		    iRet = OnTriggerLoopGeneric(s, 8,    pScriptArgs, pSrc, pResult);	break;
            case SK_FORINSTANCE:EXC_SET_BLOCK("forinstance");	iRet = OnTriggerLoopGeneric(s, 0x40, pScriptArgs, pSrc, pResult);	break;
            case SK_FORTIMERF:	EXC_SET_BLOCK("fortimerf");	    iRet = OnTriggerLoopGeneric(s, 0x100,pScriptArgs, pSrc, pResult);	break;

			case SK_FORCHARLAYER:
			case SK_FORCHARMEMORYTYPE:
				{
					EXC_SET_BLOCK("forchar[layer/memorytype]");
                    iRet = OnTriggerLoopForCharSpecial(s, iCmd, pScriptArgs, pSrc, pResult);
				} break;

			case SK_FORCONT:
				{
					EXC_SET_BLOCK("forcont");
                    iRet = OnTriggerLoopForCont(s, pScriptArgs, pSrc, pResult);
				} break;

			case SK_FORCONTID:
			case SK_FORCONTTYPE:
				{
					EXC_SET_BLOCK("forcont[id/type]");
                    iRet = OnTriggerLoopForContSpecial(s, iCmd, pScriptArgs, pSrc, pResult);
				} break;

			case SK_IF:
			case SK_ELIF:
			case SK_ELSEIF:
				// We want to do a lazy evaluation of the arguments. Don't parse this stuff now.
				break;

			default:
				{
					// Parse out any variables in it. (may act like a verb sometimes?)
					EXC_SET_BLOCK("parsing");
                    CExpression& expr_parser = CExpression::GetExprParser();
                    CScriptExprContext context = {._pScriptObjI = this};

					if( strchr(s.GetKey(), '<') )
					{
						EXC_SET_BLOCK("parsing <> in a key");
						TemporaryString tsBuf;
                        Str_CopyLimitNull  (tsBuf.buffer(), s.GetKey(),     tsBuf.capacity());
                        Str_ConcatLimitNull(tsBuf.buffer(), " ",            tsBuf.capacity());
                        Str_ConcatLimitNull(tsBuf.buffer(), s.GetArgRaw(),  tsBuf.capacity());
                        expr_parser.ParseScriptText(tsBuf.buffer(), context, pScriptArgs, pSrc, 0);

						s.ParseKey(tsBuf.buffer());
					}
					else
					{
                        expr_parser.ParseScriptText( s.GetArgRaw(), context, pScriptArgs, pSrc, 0);
					}
				}
		}

        // Logical block ended. What should i do?
        if (iRet == TRIGRET_RET_ABORTED)
            return clean_return(iRet);

		switch ( iCmd )
		{
			case SK_FORITEM:
			case SK_FORCHAR:
			case SK_FORCHARLAYER:
			case SK_FORCHARMEMORYTYPE:
			case SK_FORCLIENTS:
			case SK_FORCONT:
			case SK_FORCONTID:
			case SK_FORCONTTYPE:
			case SK_FOROBJ:
			case SK_FORPLAYERS:
			case SK_FORINSTANCE:
			case SK_FORTIMERF:
			case SK_FOR:
			case SK_WHILE:
				if ( iRet != TRIGRET_ENDIF )
					return clean_return(iRet);
				break;

			case SK_DORAND:	// Do a random line in here.
			case SK_DOSWITCH:
				{
					EXC_SET_BLOCK("dorand/doswitch");
					int64 iVal = s.GetArgLLVal();
					if ( iCmd == SK_DORAND )
						iVal = g_Rand.GetLLVal(iVal);
					for ( ; ; --iVal )
					{
                        iRet = OnTriggerRun( s, (iVal == 0) ? TRIGRUN_SINGLE_TRUE : TRIGRUN_SINGLE_FALSE, pScriptArgs, pSrc, pResult );
						if ( iRet == TRIGRET_RET_DEFAULT )
							continue;
						if ( iRet == TRIGRET_ENDIF )
							break;
						return clean_return(iRet);
					}
				}
				break;

			case SK_RETURN:		// Process the trigger.
				EXC_SET_BLOCK("return");
				if ( pResult )
				{
					pResult->Copy( s.GetArgStr() );
					return clean_return(TRIGRET_RET_TRUE);
				}
				return clean_return(TRIGRET_TYPE(s.GetArgVal()));

			case SK_IF:
				{
                EXC_SET_BLOCK("if statement");
                // At this point, we have to parse the conditional expression
                CExpression& expr_parser = CExpression::GetExprParser();
                CScriptExprContext context = {._pScriptObjI = this};

                const lptstr ptcArg = s.GetArgStr();
                bool fTrigger = expr_parser.EvaluateConditionalWhole(ptcArg, context, pScriptArgs, pSrc);
                bool fBeenTrue = false;
                for (;;)
                {
                    iRet = OnTriggerRun( s, fTrigger ? TRIGRUN_SECTION_TRUE : TRIGRUN_SECTION_FALSE, pScriptArgs, pSrc, pResult );
                    if (( iRet < TRIGRET_ENDIF ) || ( iRet >= TRIGRET_RET_HALFBAKED ))
                        return clean_return(iRet);
                    if ( iRet == TRIGRET_ENDIF )
                        break;

                    fBeenTrue |= fTrigger;
                    if ( fBeenTrue )
                        fTrigger = false;
                    else if ( iRet == TRIGRET_ELSE )
                        fTrigger = true;
                    else if ( iRet == TRIGRET_ELSEIF )
                    {
                        context = {._pScriptObjI = this};
                        fTrigger = expr_parser.EvaluateConditionalWhole(s.GetArgStr(), context, pScriptArgs, pSrc);
                    }
                }
                }
            break;

			case SK_BEGIN:
				// Do this block here.
				{
					EXC_SET_BLOCK("begin/loop cycle");
                    iRet = OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pScriptArgs, pSrc, pResult );
					if ( iRet != TRIGRET_ENDIF )
						return clean_return(iRet);
				}
				break;

			default:
				EXC_SET_BLOCK("parsing standard statement");
                if ( !pScriptArgs->r_Verb(s, pSrc) )
				{
					bool fRes;
					if (!strnicmp(s.GetKey(), "CALL", 4))
					{
						EXC_SET_BLOCK("call");
                        fRes = Execute_Call(s, pScriptArgs, pSrc);
					}
					else if ( !strnicmp(s.GetKey(), "FullTrigger", 11 ) )
					{
						EXC_SET_BLOCK("FullTrigger");
                        fRes = Execute_FullTrigger(s, pScriptArgs, pSrc);
					}
					else
					{
						EXC_SET_BLOCK("verb");
						fRes = r_Verb(s, pSrc);
					}

					if ( !fRes  )
						DEBUG_MSG(( "WARNING: Trigger Bad Verb '%s','%s'\n", s.GetKey(), s.GetArgStr()));
				}
				break;
		}

		if ( trigrun >= TRIGRUN_SINGLE_EXEC )
			return clean_return(TRIGRET_RET_DEFAULT);
	}
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("key '%s' runtype '%d' pargs '%p' ret '%s' [%p]\n",
        s.GetKey(), trigrun, static_cast<void *>(pScriptArgs.get()), (pResult == nullptr ? "" : pResult->GetBuffer()), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return clean_return(TRIGRET_RET_DEFAULT);
}

TRIGRET_TYPE CScriptObj::OnTriggerRunVal( CScript &s, TRIGRUN_TYPE trigrun, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc )
{
	// Get the TRIGRET_TYPE that is returned by the script
	// This should be used instead of OnTriggerRun() when pReturn is not used
	ADDTOCALLSTACK("CScriptObj::OnTriggerRunVal");

	CSString sVal;
	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;

    OnTriggerRun( s, trigrun, pScriptArgs, pSrc, &sVal );

	lpctstr pszVal = sVal.GetBuffer();
	if ( pszVal && *pszVal )
		tr = static_cast<TRIGRET_TYPE>(Exp_GetVal(pszVal));

	return tr;
}

