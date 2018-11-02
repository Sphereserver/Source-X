
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

CVarDefContNum::~CVarDefContNum()
{
}

int64 CVarDefContNum::GetValNum() const 
{ 
	return( m_iVal ); 
}

void CVarDefContNum::SetValNum( int64 iVal ) 
{ 
	m_iVal = iVal;
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

CVarDefContStr::~CVarDefContStr()
{
}

inline int64 CVarDefContStr::GetValNum() const
{
	lpctstr pszStr = m_sVal;
	return( Exp_GetVal(pszStr) );
}

void CVarDefContStr::SetValStr( lpctstr pszVal ) 
{
	if (strlen(pszVal) <= SCRIPT_MAX_LINE_LEN/2)
		m_sVal.Copy( pszVal );
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

CVarDefMap::CVarDefContTest::CVarDefContTest( lpctstr pszKey ) : CVarDefCont( pszKey )
{
}

CVarDefMap::CVarDefContTest::~CVarDefContTest()
{
}

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

bool CVarDefMap::ltstr::operator()(CVarDefCont * s1, CVarDefCont * s2) const
{
	//ADDTOCALLSTACK("CVarDefMap::ltstr::operator()");
	if (!s1)
		throw CSError(LOGL_ERROR, 0, "s1 empty!");
	else if (!s2)
		throw CSError(LOGL_ERROR, 0, "s2 empty!");
	return( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
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
	ADDTOCALLSTACK("CVarDefMap::FindValStr");
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
	ADDTOCALLSTACK("CVarDefMap::FindValNum");
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVarBase = (*i);
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
	ADDTOCALLSTACK("CVarDefMap::GetAt");
	if ( at > m_Container.size() )
		return nullptr;

	DefSet::iterator i = m_Container.begin();
	while ( at-- )
		++i;

	if ( i != m_Container.end() )
		return( (*i) );
	else
		return nullptr;
}

CVarDefCont * CVarDefMap::GetAtKey( lpctstr at ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetAtKey");
	CVarDefContTest * pVarBase = new CVarDefContTest(at);
	DefSet::iterator i = m_Container.find(pVarBase);
	delete pVarBase;

	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

void CVarDefMap::DeleteAt( size_t at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAt");
	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
	while ( at-- ) 
		++i;

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtKey( lpctstr at )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtKey");
	CVarDefContStr * pVarBased = new CVarDefContStr(at);
	DefSet::iterator i = m_Container.find(pVarBased);
	delete pVarBased;

	DeleteAtIterator(i);
}

void CVarDefMap::DeleteAtIterator( DefSet::iterator it )
{
	ADDTOCALLSTACK("CVarDefMap::DeleteAtIterator");
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
	ADDTOCALLSTACK("CVarDefMap::DeleteKey");
	if ( key && *key)
		DeleteAtKey(key);
}

void CVarDefMap::Empty()
{
	ADDTOCALLSTACK("CVarDefMap::Empty");
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
	ADDTOCALLSTACK("CVarDefMap::Copy");
	if ( !pArray || pArray == this )
		return;

	Empty();
	if ( pArray->GetCount() <= 0 )
		return;

	for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
	{
		m_Container.insert( (*i)->CopySelf() );
	}
}

bool CVarDefMap::Compare( const CVarDefMap * pArray )
{
	ADDTOCALLSTACK("CVarDefMap::Compare");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;
	if ( pArray->GetCount() != GetCount() )
		return false;

	if (pArray->GetCount())
	{
		for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
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
	ADDTOCALLSTACK("CVarDefMap::Compare");
	if ( !pArray )
		return false;
	if ( pArray == this )
		return true;

	if (pArray->GetCount())
	{
		for ( DefSet::const_iterator i = pArray->m_Container.begin(); i != pArray->m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
			lpctstr sKey = pVar->GetKey();

			if (strcmpi(GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	if (GetCount())
	{
		for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
		{
			const CVarDefCont * pVar = (*i);
			lpctstr sKey = pVar->GetKey();

			if (strcmpi(pArray->GetKeyStr(sKey, true),pVar->GetValStr()))
				return false;
		}
	}
	return true;
}

size_t CVarDefMap::GetCount() const
{
	ADDTOCALLSTACK("CVarDefMap::GetCount");
	return m_Container.size();
}

int CVarDefMap::SetNumNew( lpctstr pszName, int64 iVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetNumNew");
	CVarDefCont * pVarNum = new CVarDefContNum( pszName, iVal );
	if ( !pVarNum )
		return -1;

	DefPairResult res = m_Container.insert(pVarNum);
	if ( res.second )
		return (int)(std::distance(m_Container.begin(), res.first));
	else
    {
        delete pVarNum;
		return -1;
    }
}

int CVarDefMap::SetNumOverride( lpctstr pszKey, int64 iVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetNumOverride");
	DeleteAtKey(pszKey);
	return SetNumNew(pszKey,iVal);
}

int CVarDefMap::SetNum( lpctstr pszName, int64 iVal, bool fZero )
{
	ADDTOCALLSTACK("CVarDefMap::SetNum");
	ASSERT(pszName);

	if ( pszName[0] == '\0' )
		return -1;

	if ( fZero && (iVal == 0) )
	{
		DeleteAtKey(pszName);
		return -1;
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find(pVarSearch);
	delete pVarSearch;

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

	return (int)(std::distance(m_Container.begin(), iResult));
}

int CVarDefMap::SetStrNew( lpctstr pszName, lpctstr pszVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetStrNew");
	CVarDefCont * pVarStr = new CVarDefContStr( pszName, pszVal );
	if ( !pVarStr )
		return -1;

	DefPairResult res = m_Container.insert(pVarStr);
	if ( res.second )
		return (int)(std::distance(m_Container.begin(), res.first));
	else
    {
        delete pVarStr;
		return -1;
    }
}

int CVarDefMap::SetStrOverride( lpctstr pszKey, lpctstr pszVal )
{
	ADDTOCALLSTACK("CVarDefMap::SetStrOverride");
	DeleteAtKey(pszKey);
	return SetStrNew(pszKey,pszVal);
}

int CVarDefMap::SetStr( lpctstr pszName, bool fQuoted, lpctstr pszVal, bool fZero )
{
	ADDTOCALLSTACK("CVarDefMap::SetStr");
	// ASSUME: This has been clipped of unwanted beginning and trailing spaces.
	if ( !pszName || !pszName[0] )
		return -1;

	if ( pszVal == nullptr || pszVal[0] == '\0' )	// but not if empty
	{
		DeleteAtKey(pszName);
		return -1;
	}

	if ( !fQuoted && IsSimpleNumberString(pszVal))
	{
		// Just store the number and not the string.
		return SetNum( pszName, Exp_GetLLVal( pszVal ), fZero);
	}

	CVarDefContTest * pVarSearch = new CVarDefContTest(pszName);
	DefSet::iterator iResult = m_Container.find(pVarSearch);
	delete pVarSearch;

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
	return (int)(std::distance(m_Container.begin(), iResult) );
}

CVarDefCont * CVarDefMap::GetKey( lpctstr pszKey ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKey");
	CVarDefCont * pReturn = nullptr;

	if ( pszKey )
	{
		CVarDefContTest * pVarBase = new CVarDefContTest(pszKey);
		DefSet::const_iterator i = m_Container.find(pVarBase);
		delete pVarBase;
		
		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

int64 CVarDefMap::GetKeyNum( lpctstr pszKey ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyNum");
	CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == nullptr )
		return 0;
	return pVar->GetValNum();
}

lpctstr CVarDefMap::GetKeyStr( lpctstr pszKey, bool fZero ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetKeyStr");
	CVarDefCont * pVar = GetKey(pszKey);
	if ( pVar == nullptr )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

CVarDefCont * CVarDefMap::CheckParseKey( lpctstr & pszArgs ) const
{
	ADDTOCALLSTACK("CVarDefMap::CheckParseKey");
	tchar szTag[ EXPRESSION_MAX_KEY_LEN ];
	GetIdentifierString( szTag, pszArgs );
	CVarDefCont * pVar = GetKey(szTag);
	if ( pVar )
		return pVar;

	return nullptr;
}

CVarDefCont * CVarDefMap::GetParseKey( lpctstr & pszArgs ) const
{
	ADDTOCALLSTACK("CVarDefMap::GetParseKey");
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
	ADDTOCALLSTACK("CVarDefMap::GetParseVal");
	CVarDefCont * pVarBase = GetParseKey( pszArgs );
	if ( pVarBase == nullptr )
		return false;
	*plVal = pVarBase->GetValNum();
	return true;
}

void CVarDefMap::DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix ) const
{
	ADDTOCALLSTACK("CVarDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);
	if ( pszPrefix == nullptr )
		pszPrefix = "";

    bool fIsClient = pSrc->GetChar();
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVar = (*i);

        if (fIsClient)
        {
            pSrc->SysMessagef(pSrc->GetChar() ? "%s%s=%s" : "%s%s=%s\n", static_cast<lpctstr>(pszPrefix), static_cast<lpctstr>(pVar->GetKey()), static_cast<lpctstr>(pVar->GetValStr()));
        }
        else
        {
            g_Log.Event(LOGL_EVENT, pSrc->GetChar() ? "%s%s=%s" : "%s%s=%s\n", static_cast<lpctstr>(pszPrefix), static_cast<lpctstr>(pVar->GetKey()), static_cast<lpctstr>(pVar->GetValStr()));
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
	ADDTOCALLSTACK("CVarDefMap::r_LoadVal");
	bool fQuoted = false;
	return ( ( SetStr( s.GetKey(), fQuoted, s.GetArgStr( &fQuoted )) >= 0 ) ? true : false );
}

void CVarDefMap::r_WritePrefix( CScript & s, lpctstr pszPrefix, lpctstr pszKeyExclude )
{
	ADDTOCALLSTACK_INTENSIVE("CVarDefMap::r_WritePrefix");
	TemporaryString tsZ;
	tchar* z = static_cast<tchar *>(tsZ);
	lpctstr		pszVal;
	bool bHasPrefix = (pszPrefix && *pszPrefix);
	bool bHasExclude = (pszKeyExclude && *pszKeyExclude);

	// Write with any prefix.
	for ( DefSet::const_iterator i = m_Container.begin(); i != m_Container.end(); ++i )
	{
		const CVarDefCont * pVar = (*i);
		if ( !pVar )
			continue;	// This should not happen, a warning maybe?

		if ( bHasExclude && !strcmpi(pszKeyExclude, pVar->GetKey()))
			continue;

		if ( bHasPrefix )
			sprintf(z, "%s.%s", pszPrefix, pVar->GetKey());
		else
			sprintf(z, "%s", pVar->GetKey());

		pszVal = pVar->GetValStr();
		const CVarDefContStr * pVarStr = dynamic_cast <const CVarDefContStr *>(pVar);
		if ( pVarStr ) // IsSpace(pszVal[0]) || IsSpace( pszVal[strlen(pszVal)-1] )
			s.WriteKeyFormat(z, "\"%s\"", pszVal);
		else
			s.WriteKey(z, pszVal);
	}
}
