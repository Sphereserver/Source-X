
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "../game/CServerConfig.h"
#include "CException.h"
#include "CExpression.h"
#include "CScript.h"
#include "CTextConsole.h"
#include "CVarDefMap.h"


inline static int VarDefCompare(const CVarDefCont* pVar, lpctstr ptcKey) noexcept
{
    return strcmpi(pVar->GetKey(), ptcKey);
}

lpctstr CVarDefCont::GetValStrZeroed(const CVarDefCont* pVar, bool fZero) // static
{
	ADDTOCALLSTACK_DEBUG("CVarDefCont::GetValStrZeroed");
	if (pVar)
	{
        lpctstr ptcValStr = pVar->GetValStr();
		return ptcValStr;
	}
	return (fZero ? "0" : "");
}

/***************************************************************************
*
*
*	class CVarDefContNum		Variable implementation (Number)
*
*
***************************************************************************/

CVarDefContNum::CVarDefContNum( lpctstr ptcKey, int64 iVal ) : m_sKey( ptcKey ), m_iVal( iVal )
{
}

CVarDefContNum::CVarDefContNum( lpctstr ptcKey ) : m_sKey( ptcKey ), m_iVal( 0 )
{
}

lpctstr CVarDefContNum::GetValStr() const
{
    return Str_FromLL_Fast(m_iVal, Str_GetTemp(), Str_TempLength(), (g_Cfg.m_fDecimalVariables ? 10 : 16));
}

bool CVarDefContNum::r_LoadVal( CScript & s )
{
	SetValNum( s.GetArg64Val() );
	return true;
}

bool CVarDefContNum::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc )
{
	UnreferencedParameter(pKey);
	UnreferencedParameter(pSrc);
	sVal.Format64Val( GetValNum() );
	return true;
}

CVarDefCont * CVarDefContNum::CopySelf() const
{ 
	return new CVarDefContNum( GetKey(), m_iVal );
}

/***************************************************************************
*
*
*	class CVarDefContStr		Variable implementation (String)
*
*
***************************************************************************/

CVarDefContStr::CVarDefContStr( lpctstr ptcKey, lpctstr pszVal ) : m_sKey( ptcKey ), m_sVal( pszVal ) 
{
}

CVarDefContStr::CVarDefContStr( lpctstr ptcKey ) : m_sKey( ptcKey )
{
}

int64 CVarDefContStr::GetValNum() const
{
	lpctstr pszStr = m_sVal;
	return( Exp_Get64Val(pszStr) );
}

void CVarDefContStr::SetValStr( lpctstr pszVal ) 
{
    const size_t uiLen = strlen(pszVal);
	if (uiLen <= SCRIPT_MAX_LINE_LEN/2)
		m_sVal.CopyLen( pszVal, (int)uiLen );
	else
		g_Log.EventWarn("Setting max length of %d was exceeded on (VAR,TAG,LOCAL).%s \r", SCRIPT_MAX_LINE_LEN/2, GetKey() );
}

bool CVarDefContStr::r_LoadVal( CScript & s )
{
	SetValStr( s.GetArgStr());
	return true;
}

bool CVarDefContStr::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc )
{
	UnreferencedParameter(pKey);
	UnreferencedParameter(pSrc);
	sVal = GetValStr();
	return true;
}

CVarDefCont * CVarDefContStr::CopySelf() const 
{ 
	return new CVarDefContStr( GetKey(), m_sVal ); 
}


/***************************************************************************
*
*
*	class CVarDefMap			Holds list of pairs KEY = VALUE and operates it
*
*
***************************************************************************/

CVarDefMap & CVarDefMap::operator = ( const CVarDefMap & array )
{
	Copy( &array );
	return( *this );
}

CVarDefMap::~CVarDefMap()
{
	Clear();
}

lpctstr CVarDefMap::FindValStr( lpctstr pVal ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::FindValStr");
	for ( const CVarDefCont * pVarBase : m_Container )
	{
		ASSERT( pVarBase );
		
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>( pVarBase );
		if ( pVarStr == nullptr )
			continue;
		
		if ( ! strcmpi( pVal, pVarStr->GetValStr()))
			return pVarBase->GetKey();
	}

	return nullptr;
}

lpctstr CVarDefMap::FindValNum( int64 iVal ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::FindValNum");
    for (const CVarDefCont* pVarBase : m_Container)
	{
		ASSERT( pVarBase );
		const CVarDefContNum * pVarNum = dynamic_cast <const CVarDefContNum *>( pVarBase );
		if ( pVarNum == nullptr )
			continue;

		if ( pVarNum->GetValNum() == iVal )
			return pVarBase->GetKey();
	}

	return nullptr;
}

CVarDefCont * CVarDefMap::GetAt( size_t at ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetAt");
	if ( at > m_Container.size() )
		return nullptr;
    return m_Container[at];
}

CVarDefCont * CVarDefMap::GetAtKey( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetAtKey");
    const size_t idx = m_Container.find_predicate(ptcKey, VarDefCompare);

	if ( idx != sl::scont_bad_index() )
		return m_Container[idx];
	return nullptr;
}

void CVarDefMap::DeleteAt( size_t at )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::DeleteAt");
	if ( at > m_Container.size() )
		return;

    CVarDefCont *pVarBase = m_Container[at];
    m_Container.erase(m_Container.begin() + at);

    if ( pVarBase )
    {
        CVarDefContNum *pVarNum = dynamic_cast<CVarDefContNum *>(pVarBase);
        if ( pVarNum )
        {
            delete pVarNum;
        }
        else
        {
            CVarDefContStr *pVarStr = dynamic_cast<CVarDefContStr *>(pVarBase);
            if ( pVarStr )
                delete pVarStr;
        }
    }
}

void CVarDefMap::DeleteAtKey( lpctstr ptcKey )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::DeleteAtKey");
    const size_t idx = m_Container.find_predicate(ptcKey, VarDefCompare);
    if (idx != sl::scont_bad_index())
        DeleteAt(idx);
}

void CVarDefMap::DeleteKey( lpctstr key )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::DeleteKey");
	if ( key && *key)
		DeleteAtKey(key);
}

void CVarDefMap::Clear()
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::Clear");
	iterator it = m_Container.begin();
	while ( it != m_Container.end() )
	{
		delete (*it);	// This calls the appropriate destructors, from derived to base class, because the destructors are virtual.
		m_Container.erase(it);
		it = m_Container.begin();
	}

	m_Container.clear();
}

void CVarDefMap::Copy( const CVarDefMap * pArray, bool fClearThis )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::Copy");
	if ( !pArray || pArray == this )
		return;

	if (fClearThis)
		Clear();

	if ( pArray->GetCount() <= 0 )
		return;

    for (const CVarDefCont* pVar : pArray->m_Container)
	{
		m_Container.insert( pVar->CopySelf() );
	}
}

bool CVarDefMap::Compare( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::Compare");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;
	if ( pArray->GetCount() != GetCount() )
		return false;

	if (pArray->GetCount())
	{
        for (const CVarDefCont* pVar : pArray->m_Container)
		{
			lpctstr sKey = pVar->GetKey();
			if (!GetKey(sKey))
				return false;

			if (strcmpi(GetKeyStr(sKey),pVar->GetValStr()))
				return false;
		}
	}
	return true;
}

bool CVarDefMap::CompareAll( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::CompareAll");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;

	if (pArray->GetCount())
	{
        for (const CVarDefCont* pVar : pArray->m_Container)
		{
			lpctstr sKey = pVar->GetKey();

			if (strcmpi(GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	if (GetCount())
	{
        for (const CVarDefCont* pVar : m_Container)
		{
			lpctstr sKey = pVar->GetKey();

			if (strcmpi(pArray->GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	return true;
}

size_t CVarDefMap::GetCount() const noexcept
{
	return m_Container.size();
}

CVarDefContNum* CVarDefMap::SetNumNew( lpctstr pszName, int64 iVal )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetNumNew");
	CVarDefContNum * pVarNum = new CVarDefContNum( pszName, iVal );
	if ( !pVarNum )
		return nullptr;

	iterator res = m_Container.emplace(static_cast<CVarDefCont*>(pVarNum));
	if ( res != m_Container.end() )
		return pVarNum;
	else
    {
        delete pVarNum;
		return nullptr;
    }
}

CVarDefContNum* CVarDefMap::SetNumOverride( lpctstr ptcKey, int64 iVal )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetNumOverride");
    CVarDefContNum* pKeyNum = dynamic_cast<CVarDefContNum*>(GetKey(ptcKey));
    if (pKeyNum)
    {
        pKeyNum->SetValNum(iVal);
        return pKeyNum;
    }
	DeleteAtKey(ptcKey);
	return SetNumNew(ptcKey,iVal);
}

CVarDefContNum* CVarDefMap::ModNum(lpctstr pszName, int64 iMod, bool fDeleteZero)
{
    ADDTOCALLSTACK_DEBUG("CVarDefMap::ModNum");
    ASSERT(pszName);
    CVarDefCont* pVarDef = GetKey(pszName);
    if (pVarDef)
    {
        CVarDefContNum* pVarDefNum = dynamic_cast<CVarDefContNum*>(pVarDef);
        if (pVarDefNum)
        {
            const int64 iNewVal = pVarDefNum->GetValNum() + iMod;
            if ((iNewVal == 0) && fDeleteZero)
            {
                const size_t idx = m_Container.find(pVarDef);
                ASSERT (idx != sl::scont_bad_index());
                DeleteAt(idx);
                return nullptr;
            }
            pVarDefNum->SetValNum(iNewVal);
            return pVarDefNum;
        }
    }
    return SetNum(pszName, iMod, fDeleteZero);
}

CVarDefContNum* CVarDefMap::SetNum( lpctstr pszName, int64 iVal, bool fDeleteZero, bool fWarnOverwrite )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetNum");
	ASSERT(pszName);

	if ( pszName[0] == '\0' )
		return nullptr;

	if ( (iVal == 0) && fDeleteZero )
	{
		DeleteAtKey(pszName);
		return nullptr;
	}

    const size_t idx = m_Container.find_predicate(pszName, VarDefCompare);

	CVarDefCont * pVarBase = nullptr;
	if ( idx != sl::scont_bad_index() )
		pVarBase = m_Container[idx];

	if ( !pVarBase )
		return SetNumNew( pszName, iVal );

	CVarDefContNum * pVarNum = dynamic_cast <CVarDefContNum *>( pVarBase );
	if ( pVarNum )
    {
        if ( fWarnOverwrite && !g_Serv.IsResyncing() && g_Serv.IsLoading() )
            DEBUG_WARN(( "Replacing existing VarNum '%s' with number: 0x%" PRIx64" \n", pVarBase->GetKey(), iVal ));
		pVarNum->SetValNum( iVal );
    }
	else
	{
		if ( fWarnOverwrite && !g_Serv.IsResyncing() && g_Serv.IsLoading() )
			DEBUG_WARN(( "Replacing existing VarStr '%s' with number: 0x%" PRIx64" \n", pVarBase->GetKey(), iVal ));
		return SetNumOverride( pszName, iVal );
	}

	return pVarNum;
}

CVarDefContStr* CVarDefMap::SetStrNew( lpctstr pszName, lpctstr pszVal )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetStrNew");
	CVarDefContStr * pVarStr = new CVarDefContStr( pszName, pszVal );
	if ( !pVarStr )
		return nullptr;

    iterator res = m_Container.emplace(static_cast<CVarDefCont*>(pVarStr));
    if ( res != m_Container.end() )
		return pVarStr;
	else
    {
        delete pVarStr;
		return nullptr;
    }
}

CVarDefContStr* CVarDefMap::SetStrOverride( lpctstr ptcKey, lpctstr pszVal )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetStrOverride");
    CVarDefContStr* pKeyStr = dynamic_cast<CVarDefContStr*>(GetKey(ptcKey));
    if (pKeyStr)
    {
        pKeyStr->SetValStr(pszVal);
        return pKeyStr;
    }
	DeleteAtKey(ptcKey);
	return SetStrNew(ptcKey,pszVal);
}

CVarDefCont* CVarDefMap::SetStr( lpctstr pszName, bool fQuoted, lpctstr pszVal, bool fDeleteZero, bool fWarnOverwrite )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::SetStr");
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
    ASSERT(pszName);
	if ( !pszName[0] )
		return nullptr;

    ASSERT(pszVal);
	if (!fQuoted)
	{
		if (pszVal[0] == '\0')
		{
			// If Val is an empty string, remove any previous def (and do not add a new def)
			DeleteAtKey(pszName);
			return nullptr;
		}

		if (IsSimpleNumberString(pszVal))
		{
			// Just store the number and not the string.
			return SetNum(pszName, Exp_Get64Val(pszVal), fDeleteZero, fWarnOverwrite);
		}
	}

    const size_t idx = m_Container.find_predicate(pszName, VarDefCompare);

	CVarDefCont * pVarBase = nullptr;
	if ( idx != sl::scont_bad_index() )
		pVarBase = m_Container[idx];

	if ( !pVarBase )
		return SetStrNew( pszName, pszVal );

	CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr *>( pVarBase );
	if ( pVarStr )
    {
        if ( fWarnOverwrite && !g_Serv.IsResyncing() && g_Serv.IsLoading() )
            DEBUG_WARN(( "Replacing existing VarStr '%s' with string: '%s'\n", pVarBase->GetKey(), pszVal ));
		pVarStr->SetValStr( pszVal );
    }
	else
	{
		if ( fWarnOverwrite && !g_Serv.IsResyncing() && g_Serv.IsLoading() )
			DEBUG_WARN(( "Replacing existing VarNum '%s' with string: '%s'\n", pVarBase->GetKey(), pszVal ));
		return SetStrOverride( pszName, pszVal );
	}
	return pVarStr;
}

CVarDefCont * CVarDefMap::GetKey( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetKey");
	CVarDefCont * pReturn = nullptr;

	if ( ptcKey )
	{
        const size_t idx = m_Container.find_predicate(ptcKey, VarDefCompare);
		
		if ( idx != sl::scont_bad_index() )
			pReturn = m_Container[idx];
	}

	return pReturn;
}

int64 CVarDefMap::GetKeyNum( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetKeyNum");
	const CVarDefCont * pVar = GetKey(ptcKey);
	if ( pVar == nullptr )
		return 0;
	return pVar->GetValNum();
}

lpctstr CVarDefMap::GetKeyStr( lpctstr ptcKey, bool fZero ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetKeyStr");
	const CVarDefCont * pVar = GetKey(ptcKey);
	return CVarDefCont::GetValStrZeroed(pVar, fZero);
}

CVarDefCont * CVarDefMap::CheckParseKey( lpctstr pszArgs ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::CheckParseKey");
	tchar szTag[ EXPRESSION_MAX_KEY_LEN ];
	GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
		return pVar;

	return nullptr;
}

CVarDefCont * CVarDefMap::GetParseKey_Advance( lpctstr & pszArgs ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetParseKey_Advance");
	// Skip to the end of the expression name.
	// The name can only be valid.

	tchar szTag[ EXPRESSION_MAX_KEY_LEN ];
    const uint i = GetIdentifierString( szTag, pszArgs );
    pszArgs += i;

	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
		return pVar;
	return nullptr;
}

bool CVarDefMap::GetParseVal_Advance( lpctstr & pszArgs, llong * pllVal ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::GetParseVal_Advance");
	CVarDefCont * pVarBase = GetParseKey_Advance( pszArgs );
	if ( pVarBase == nullptr )
		return false;
	*pllVal = pVarBase->GetValNum();
	return true;
}

void CVarDefMap::DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix ) const
{
	ADDTOCALLSTACK("CVarDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == nullptr )
		pszPrefix = "";

    const bool fIsClient = pSrc->GetChar();
    for (const CVarDefCont* pVar : m_Container)
	{
        if (fIsClient)
        {
            pSrc->SysMessagef("%s%s=%s", pszPrefix, pVar->GetKey(), pVar->GetValStr());
        }
        else
        {
            g_Log.Event(LOGL_EVENT, "%s%s=%s\n", pszPrefix, pVar->GetKey(), pVar->GetValStr());
        }
	}
}

void CVarDefMap::ClearKeys(lpctstr mask)
{
	ADDTOCALLSTACK("CVarDefMap::ClearKeys");
	if ( mask && *mask )
	{
		if ( !m_Container.size() )
			return;

		CSString sMask(mask);
		sMask.MakeLower();

        size_t i = 0;
		iterator it = m_Container.begin();
		while ( it != m_Container.end() )
		{
            CVarDefCont *pVarBase = (*it);
			if (!pVarBase)
				goto skip_cur;

			{
				CSString sKey(pVarBase->GetKey());
				sKey.MakeLower();
				if (strstr(sKey.GetBuffer(), sMask.GetBuffer()))
				{
					DeleteAt(i);
					it = m_Container.begin();
					i = 0;
					continue;
				}
			}
        skip_cur:
            ++it;
            ++i;
		}
	}
	else
	{
		Clear();
	}
}

/*
bool CVarDefMap::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::r_LoadVal");
	bool fQuoted = false;
    lpctstr ptcVal = s.GetArgStr( &fQuoted );
	return ( SetStr( s.GetKey(), fQuoted, ptcVal) ? true : false );
}
*/

void CVarDefMap::r_WritePrefix( CScript & s, lpctstr ptcPrefix, lpctstr ptcKeyExclude ) const
{
	ADDTOCALLSTACK_DEBUG("CVarDefMap::r_WritePrefix");
    if (m_Container.empty())
        return;

    const bool fHasPrefix = (ptcPrefix && *ptcPrefix);
    const bool fHasExclude = (ptcKeyExclude && *ptcKeyExclude);
	TemporaryString ts;

	// Prefix is usually TAG, VAR, etc...
    auto _WritePrefix = [&ts, fHasPrefix, ptcPrefix](lpctstr ptcKey) -> void
    {
		if (fHasPrefix)
			snprintf(ts.buffer(), ts.capacity(), "%s.%s", ptcPrefix, ptcKey);
		else
			Str_CopyLimitNull(ts.buffer(), ptcKey, ts.capacity());
    };

	// Write with any prefix.
    for (const CVarDefCont* pVar : m_Container)
	{
		if ( !pVar )
			continue;	// This should not happen, a warning maybe?

        const lpctstr ptcKey = pVar->GetKey();
		if ( fHasExclude && !strcmpi(ptcKeyExclude, ptcKey))
			continue;
		
        const CVarDefContNum * pVarNum = dynamic_cast<const CVarDefContNum*>(pVar);
        _WritePrefix(ptcKey);
        lpctstr ptcVal = pVar->GetValStr();
        if (pVarNum)
        {
            s.WriteKeyStr(ts.buffer(), ptcVal);
        }
        else
        {
            s.WriteKeyFormat(ts.buffer(), "\"%s\"", ptcVal);
        }
	}
}
