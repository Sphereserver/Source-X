
#include "../common/sphere_library/sstringobjs.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "CException.h"
#include "CExpression.h"
#include "CScript.h"
#include "CTextConsole.h"
#include "CVarDefMap.h"


static size_t GetIdentifierString( tchar * szTag, lpctstr pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
	size_t i = 0;
	for ( ;pszArgs[i]; ++i )
	{
		if ( ! _ISCSYM(pszArgs[i]))
			break;
		if ( i >= EXPRESSION_MAX_KEY_LEN )
			return 0;
		szTag[i] = pszArgs[i];
	}

	szTag[i] = '\0';
	return i;
}

/***************************************************************************
*
*
*	class CVarDefCont		Interface for variables
*
*
***************************************************************************/
CVarDefCont::CVarDefCont( lpctstr pszKey ) : m_Key( pszKey ) 
{ 
	m_Key.MakeLower(); 
}

CVarDefCont::~CVarDefCont()
{
}

lpctstr CVarDefCont::GetKey() const 
{ 
	return( m_Key.GetPtr() ); 
}

void CVarDefCont::SetKey( lpctstr pszKey )
{ 
	m_Key = pszKey;
	m_Key.MakeLower(); 
}

/***************************************************************************
*
*
*	class CVarDefContNum		Variable implementation (Number)
*
*
***************************************************************************/

CVarDefContNum::CVarDefContNum( lpctstr pszKey, int64 iVal ) : CVarDefCont( pszKey ), m_iVal( iVal )
{
}

CVarDefContNum::CVarDefContNum( lpctstr pszKey ) : CVarDefCont( pszKey ), m_iVal( 0 )
{
}

lpctstr CVarDefContNum::GetValStr() const
{
	TemporaryString tsTemp;
	tchar* pszTemp = static_cast<tchar *>(tsTemp);
	sprintf(pszTemp, "0%" PRIx64 , m_iVal);
	return pszTemp;
}

bool CVarDefContNum::r_LoadVal( CScript & s )
{
	SetValNum( s.GetArgVal());
	return true;
}

bool CVarDefContNum::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.FormatLLVal( GetValNum() );
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

CVarDefContStr::CVarDefContStr( lpctstr pszKey, lpctstr pszVal ) : CVarDefCont( pszKey ), m_sVal( pszVal ) 
{
}

CVarDefContStr::CVarDefContStr( lpctstr pszKey ) : CVarDefCont( pszKey )
{
}

inline int64 CVarDefContStr::GetValNum() const
{
	lpctstr pszStr = m_sVal;
	return( Exp_GetVal(pszStr) );
}

void CVarDefContStr::SetValStr( lpctstr pszVal ) 
{
    size_t uiLen = strlen(pszVal);
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

bool CVarDefContStr::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
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
*	class CVarDefMap::CVarDefContTest	Variable implementation (search-only internal useage)
*
*
***************************************************************************/

lpctstr CVarDefMap::CVarDefContTest::GetValStr() const 
{ 
	return nullptr; 
}
	
int64 CVarDefMap::CVarDefContTest::GetValNum() const 
{ 
	return -1; 
}

CVarDefCont * CVarDefMap::CVarDefContTest::CopySelf() const 
{ 
	return new CVarDefContTest( GetKey() ); 
}

/***************************************************************************
*
*
*	class CVarDefMap::ltstr			KEY part sorting wrapper over std::set
*
*
***************************************************************************/

bool CVarDefMap::ltstr::operator()(const CVarDefCont * s1, const CVarDefCont * s2) const
{
	//ADDTOCALLSTACK_INTENSIVE("CVarDefMap::ltstr::operator()");
	if (!s1)
		throw CSError(LOGL_ERROR, 0, "s1 empty!");
	else if (!s2)
		throw CSError(LOGL_ERROR, 0, "s2 empty!");
	return ( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
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
	Empty();
}

lpctstr CVarDefMap::FindValStr( lpctstr pVal ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::FindValStr");
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVarBase = (*i);
		ASSERT( pVarBase );
		
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>( pVarBase );
		if ( pVarStr == nullptr )
			continue;
		
		if ( ! strcmpi( pVal, pVarStr->GetValStr()))
			return( pVarBase->GetKey() );
	}

	return nullptr;
}

lpctstr CVarDefMap::FindValNum( int64 iVal ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::FindValNum");
    for (const CVarDefCont* pVarBase : m_Container)
	{
		ASSERT( pVarBase );
		const CVarDefContNum * pVarNum = dynamic_cast <const CVarDefContNum *>( pVarBase );
		if ( pVarNum == nullptr )
			continue;

		if ( pVarNum->GetValNum() == iVal )
			return( pVarBase->GetKey() );
	}

	return nullptr;
}

CVarDefCont * CVarDefMap::GetAt( size_t at ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetAt");
	if ( at > m_Container.size() )
		return nullptr;

	DefSet::const_iterator i = std::next(m_Container.begin(), at);
	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

CVarDefCont * CVarDefMap::GetAtKey( lpctstr at ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetAtKey");
	CVarDefContTest pVarBase(at);
	DefSet::const_iterator i = m_Container.find(&pVarBase);

	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

void CVarDefMap::DeleteAt( size_t at )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::DeleteAt");
	if ( at > m_Container.size() )
		return;

	DeleteAtIterator(std::next(m_Container.begin(), at));
}

void CVarDefMap::DeleteAtKey( lpctstr at )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::DeleteAtKey");
	CVarDefContStr pVarBased(at);
	DefSet::iterator i = m_Container.find(&pVarBased);

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtIterator( DefSet::iterator it )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::DeleteAtIterator");
	if ( it != m_Container.end() )
	{
		CVarDefCont *pVarBase = (*it);
		m_Container.erase(it);

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
		else
			delete *it;
	}
}

void CVarDefMap::DeleteKey( lpctstr key )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::DeleteKey");
	if ( key && *key)
		DeleteAtKey(key);
}

void CVarDefMap::Empty()
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::Empty");
	DefSet::iterator i = m_Container.begin();

	while ( i != m_Container.end() )
	{
		delete (*i);	// This calls the appropriate destructors, from derived to base class, because the destructors are virtual.
		m_Container.erase(i);
		i = m_Container.begin();
	}

	m_Container.clear();
}

void CVarDefMap::Copy( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::Copy");
	if ( !pArray || pArray == this )
		return;

	Empty();
	if ( pArray->GetCount() <= 0 )
		return;

    for (const CVarDefCont* pVar : pArray->m_Container)
	{
		m_Container.insert( pVar->CopySelf() );
	}
}

bool CVarDefMap::Compare( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::Compare");
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
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::CompareAll");
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

size_t CVarDefMap::GetCount() const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetCount");
	return m_Container.size();
}

CVarDefContNum* CVarDefMap::SetNumNew( lpctstr pszName, int64 iVal )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetNumNew");
	CVarDefContNum * pVarNum = new CVarDefContNum( pszName, iVal );
	if ( !pVarNum )
		return nullptr;

	DefPairResult res = m_Container.insert(static_cast<CVarDefCont*>(pVarNum));
	if ( res.second )
		return pVarNum;
	else
    {
        delete pVarNum;
		return nullptr;
    }
}

CVarDefContNum* CVarDefMap::SetNumOverride( lpctstr pszKey, int64 iVal )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetNumOverride");
    CVarDefContNum* pKeyNum = dynamic_cast<CVarDefContNum*>(GetKey(pszKey));
    if (pKeyNum)
    {
        pKeyNum->SetValNum(iVal);
        return pKeyNum;
    }
	DeleteAtKey(pszKey);
	return SetNumNew(pszKey,iVal);
}

CVarDefContNum* CVarDefMap::ModNum(lpctstr pszName, int64 iMod, bool fZero)
{
    ADDTOCALLSTACK_INTENSIVE("CVarDefMap::ModNum");
    ASSERT(pszName);
    CVarDefCont* pVarDef = GetKey(pszName);
    if (pVarDef)
    {
        CVarDefContNum* pVarDefNum = dynamic_cast<CVarDefContNum*>(pVarDef);
        if (pVarDefNum)
        {
            const int64 iNewVal = pVarDefNum->GetValNum() + iMod;
            if ((iNewVal == 0) && fZero)
            {
                DefSet::iterator it = m_Container.find(pVarDef);
                ASSERT (it != m_Container.end());
                DeleteAtIterator(it);
                return nullptr;
            }
            pVarDefNum->SetValNum(iNewVal);
            return pVarDefNum;
        }
    }
    return SetNum(pszName, iMod, fZero);
}

CVarDefContNum* CVarDefMap::SetNum( lpctstr pszName, int64 iVal, bool fZero )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetNum");
	ASSERT(pszName);

	if ( pszName[0] == '\0' )
		return nullptr;

	if ( (iVal == 0) && !fZero )
	{
		DeleteAtKey(pszName);
		return nullptr;
	}

	CVarDefContTest pVarSearch(pszName);
	DefSet::iterator iResult = m_Container.find(&pVarSearch);

	CVarDefCont * pVarBase = nullptr;
	if ( iResult != m_Container.end() )
		pVarBase = (*iResult);

	if ( !pVarBase )
		return SetNumNew( pszName, iVal );

	CVarDefContNum * pVarNum = dynamic_cast <CVarDefContNum *>( pVarBase );
	if ( pVarNum )
		pVarNum->SetValNum( iVal );
	else
	{
		if ( g_Serv.IsLoading() )
			DEBUG_WARN(( "Replace existing VarStr '%s' with number: 0x%" PRIx64" \n", pVarBase->GetKey(), iVal ));
		return SetNumOverride( pszName, iVal );
	}

	return pVarNum;
}

CVarDefContStr* CVarDefMap::SetStrNew( lpctstr pszName, lpctstr pszVal )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetStrNew");
	CVarDefContStr * pVarStr = new CVarDefContStr( pszName, pszVal );
	if ( !pVarStr )
		return nullptr;

	DefPairResult res = m_Container.insert(pVarStr);
	if ( res.second )
		return pVarStr;
	else
    {
        delete pVarStr;
		return nullptr;
    }
}

CVarDefContStr* CVarDefMap::SetStrOverride( lpctstr pszKey, lpctstr pszVal )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetStrOverride");
    CVarDefContStr* pKeyStr = dynamic_cast<CVarDefContStr*>(GetKey(pszKey));
    if (pKeyStr)
    {
        pKeyStr->SetValStr(pszVal);
        return pKeyStr;
    }
	DeleteAtKey(pszKey);
	return SetStrNew(pszKey,pszVal);
}

CVarDefCont* CVarDefMap::SetStr( lpctstr pszName, bool fQuoted, lpctstr pszVal, bool fZero )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::SetStr");
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	if ( !pszName || !pszName[0] )
		return nullptr;

	if ( !fZero && (pszVal == nullptr) || (pszVal[0] == '\0') )	// but not if empty
	{
		DeleteAtKey(pszName);
		return nullptr;
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal))
	{
		// Just store the number and not the string.
		return SetNum( pszName, Exp_GetLLVal( pszVal ), fZero);
	}

	CVarDefContTest pVarSearch(pszName);
	DefSet::iterator iResult = m_Container.find(&pVarSearch);

	CVarDefCont * pVarBase = nullptr;
	if ( iResult != m_Container.end() )
		pVarBase = (*iResult);

	if ( !pVarBase )
		return SetStrNew( pszName, pszVal );

	CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr *>( pVarBase );
	if ( pVarStr )
		pVarStr->SetValStr( pszVal );
	else
	{
		if ( g_Serv.IsLoading())
			DEBUG_WARN(( "Replace existing VarNum '%s' with string: '%s^\n", pVarBase->GetKey(), pszVal ));
		return SetStrOverride( pszName, pszVal );
	}
	return pVarStr;
}

CVarDefCont * CVarDefMap::GetKey( lpctstr pszKey ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetKey");
	CVarDefCont * pReturn = nullptr;

	if ( pszKey )
	{
		CVarDefContTest pVarBase(pszKey);
		DefSet::const_iterator i = m_Container.find(&pVarBase);
		
		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

int64 CVarDefMap::GetKeyNum( lpctstr pszKey ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetKeyNum");
	const CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == nullptr )
		return 0;
	return pVar->GetValNum();
}

lpctstr CVarDefMap::GetKeyStr( lpctstr pszKey, bool fZero ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetKeyStr");
	const CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == nullptr )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

CVarDefCont * CVarDefMap::CheckParseKey( lpctstr & pszArgs ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::CheckParseKey");
	tchar szTag[ EXPRESSION_MAX_KEY_LEN ];
	GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
		return pVar;

	return nullptr;
}

CVarDefCont * CVarDefMap::GetParseKey( lpctstr & pszArgs ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetParseKey");
	// Skip to the end of the expression name.
	// The name can only be valid.

	tchar szTag[ EXPRESSION_MAX_KEY_LEN ];
	size_t i = GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
	{
		pszArgs += i;
		return pVar;
	}

	return nullptr;
}

bool CVarDefMap::GetParseVal( lpctstr & pszArgs, long long * plVal ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::GetParseVal");
	CVarDefCont * pVarBase = GetParseKey( pszArgs );
	if ( pVarBase == nullptr )
		return false;
	*plVal = pVarBase->GetValNum();
	return true;
}

void CVarDefMap::DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix ) const
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == nullptr )
		pszPrefix = "";

    bool fIsClient = pSrc->GetChar();
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
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::ClearKeys");
	if ( mask && *mask )
	{
		if ( !m_Container.size() )
			return;

		CSString sMask(mask);
		sMask.MakeLower();

		DefSet::iterator i = m_Container.begin();
		CVarDefCont * pVarBase = nullptr;

		while ( i != m_Container.end() )
		{
			pVarBase = (*i);

			if ( pVarBase && ( strstr(pVarBase->GetKey(), sMask.GetPtr()) ) )
			{
				DeleteAtIterator(i);
				i = m_Container.begin();
			}
			else
			{
				++i;
			}
		}
	}
	else
	{
		Empty();
	}
}

bool CVarDefMap::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::r_LoadVal");
	bool fQuoted = false;
	return ( SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) ? true : false );
}

void CVarDefMap::r_WritePrefix( CScript & s, lpctstr pszPrefix, lpctstr pszKeyExclude )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::r_WritePrefix");
	TemporaryString tsZ;
	tchar* z = static_cast<tchar *>(tsZ);
	lpctstr	pszVal;
	bool fHasPrefix = (pszPrefix && *pszPrefix);
	bool fHasExclude = (pszKeyExclude && *pszKeyExclude);

    auto _WritePrefix = [&z, fHasPrefix, pszPrefix](lpctstr ptcKey) -> void
    {
        if ( fHasPrefix )
            sprintf(z, "%s.%s", pszPrefix, ptcKey);
        else
            sprintf(z, "%s", ptcKey);
    };

	// Write with any prefix.
    for (const CVarDefCont* pVar : m_Container)
	{
		if ( !pVar )
			continue;	// This should not happen, a warning maybe?

        lpctstr ptcKey = pVar->GetKey();
		if ( fHasExclude && !strcmpi(pszKeyExclude, ptcKey))
			continue;
		
        const CVarDefContNum * pVarNum = dynamic_cast<const CVarDefContNum*>(pVar);
        if (pVarNum)
        {
            // Save VarNums only if they are != 0, otherwise it's a waste of space in the save file
            if (pVarNum->GetValNum() != 0)
            {
                _WritePrefix(ptcKey);
                pszVal = pVar->GetValStr();
                s.WriteKey(z, pszVal);
            }
        }
        else
        {
            _WritePrefix(ptcKey);
            pszVal = pVar->GetValStr();
            s.WriteKeyFormat(z, "\"%s\"", pszVal);
        }
	}
}
