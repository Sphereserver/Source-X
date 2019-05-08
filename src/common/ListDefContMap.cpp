
#include "../common/sphere_library/sstringobjs.h"
#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "CExpression.h"
#include "CScript.h"
#include "CTextConsole.h"
#include "ListDefContMap.h"
#include <algorithm>


/***************************************************************************
*
*
*	class CVarDefContNum		List element implementation (Number)
*
*
***************************************************************************/
CListDefContNum::CListDefContNum( lpctstr pszKey, int64 iVal ) : CListDefContElem( pszKey ), m_iVal( iVal )
{
}

CListDefContNum::CListDefContNum( lpctstr pszKey ) : CListDefContElem( pszKey ), m_iVal( 0 )
{
}

lpctstr CListDefContNum::GetValStr() const
{
	TemporaryString tsTemp;
	tchar* pszTemp = static_cast<tchar *>(tsTemp);
	sprintf(pszTemp, "0%" PRIx64 , m_iVal);
	return pszTemp;
}

bool CListDefContNum::r_LoadVal( CScript & s )
{
	SetValNum( s.GetArg64Val() );
	return true;
}

bool CListDefContNum::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal.Format64Val( GetValNum() );
	return true;
}

CListDefContElem * CListDefContNum::CopySelf() const
{ 
	return new CListDefContNum( GetKey(), m_iVal );
}

/***************************************************************************
*
*
*	class CListDefContStr		List element implementation (String)
*
*
***************************************************************************/
CListDefContStr::CListDefContStr( lpctstr pszKey, lpctstr pszVal ) : CListDefContElem( pszKey ), m_sVal( pszVal ) 
{
}

CListDefContStr::CListDefContStr( lpctstr pszKey ) : CListDefContElem( pszKey )
{
}

int64 CListDefContStr::GetValNum() const
{
	lpctstr pszStr = m_sVal;
	return( Exp_Get64Val(pszStr) );
}

void CListDefContStr::SetValStr( lpctstr pszVal ) 
{ 
	m_sVal.Copy( pszVal );
}


bool CListDefContStr::r_LoadVal( CScript & s )
{
	SetValStr( s.GetArgStr());
	return true;
}

bool CListDefContStr::r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc )
{
	UNREFERENCED_PARAMETER(pKey);
	UNREFERENCED_PARAMETER(pSrc);
	sVal = GetValStr();
	return true;
}

CListDefContElem * CListDefContStr::CopySelf() const 
{ 
	return new CListDefContStr( GetKey(), m_sVal ); 
}

/***************************************************************************
*
*
*	class CListDefCont		List of elements (numeric & string)
*
*
***************************************************************************/
CListDefCont::CListDefCont( lpctstr pszKey ) : m_Key( pszKey ) 
{ 
}

void CListDefCont::SetKey( lpctstr pszKey )
{ 
	m_Key = pszKey;
}

CListDefContElem* CListDefCont::GetAt(size_t nIndex) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::GetAt");
    if ( nIndex >= m_listElements.size() )
        return nullptr;
    return m_listElements[nIndex];

    /*
    DefList::const_iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    
    if (it != m_listElements.end())
        return (*it);
    return nullptr;
    */
}

bool CListDefCont::SetNumAt(size_t nIndex, int64 iVal)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::SetNumAt");
	CListDefContElem* pListElem = GetAt(nIndex);

	if ( !pListElem )
		return false;

	CListDefContElem* pListNewElem = new CListDefContNum(m_Key.GetPtr(), iVal);

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
	m_listElements.insert(it, pListNewElem);
	DeleteAtIterator(it);	

	return true;
}

bool CListDefCont::SetStrAt(size_t nIndex, lpctstr pszVal)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::SetStrAt");
	CListDefContElem* pListElem = GetAt(nIndex);

	if ( !pListElem )
		return false;

	CListDefContElem* pListNewElem = new CListDefContStr(m_Key.GetPtr(), pszVal);

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
	m_listElements.insert(it, pListNewElem);
	DeleteAtIterator(it);	

	return true;
}

lpctstr CListDefCont::GetValStr(size_t nIndex) const
{
	ADDTOCALLSTACK_INTENSIVE("CListDefCont::GetValStr");
	CListDefContElem* pElem = GetAt(nIndex);

	if ( !pElem )
		return nullptr;

	return pElem->GetValStr();
}

int64 CListDefCont::GetValNum(size_t nIndex) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::GetValNum");
	CListDefContElem* pElem = GetAt(nIndex);

	if ( !pElem )
		return 0;

	return pElem->GetValNum();
}

int CListDefCont::FindValStr( lpctstr pVal, size_t nStartIndex /* = 0 */ ) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::FindValStr");

	if ( !pVal || !(*pVal) )
		return -1;

	DefList::const_iterator it = m_listElements.begin(), itEnd = m_listElements.end();
    if (nStartIndex)
        std::advance(it, nStartIndex);

	for ( size_t nIndex = nStartIndex; it != itEnd; ++it, ++nIndex )
	{
		const CListDefContElem * pListBase = (*it);
		ASSERT( pListBase );

		const CListDefContStr * pListStr = dynamic_cast <const CListDefContStr *>( pListBase );
		if ( pListStr == nullptr )
			continue;

		if ( ! strcmpi( pVal, pListStr->GetValStr()))
			return (int)(nIndex);
	}

	return -1;
}

int CListDefCont::FindValNum( int64 iVal, size_t nStartIndex /* = 0 */ ) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::FindValNum");

    DefList::const_iterator it = m_listElements.begin(), itEnd = m_listElements.end();
    if (nStartIndex)
        std::advance(it, nStartIndex);

    for ( size_t nIndex = nStartIndex; it != itEnd; ++it, ++nIndex )
	{
		const CListDefContElem * pListBase = (*it);
		ASSERT( pListBase );

		const CListDefContNum * pListNum = dynamic_cast <const CListDefContNum *>( pListBase );
		if ( pListNum == nullptr )
			continue;

		if ( pListNum->GetValNum() == iVal )
			return (int)(nIndex);
	}

	return -1;
}

bool CListDefCont::AddElementNum(int64 iVal)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::AddElementNum");
	if ( (m_listElements.size() + 1) >= INTPTR_MAX )	// overflow? is it even useful?
		return false;

	m_listElements.emplace_back( new CListDefContNum(m_Key.GetPtr(), iVal) );

	return true;
}

bool CListDefCont::AddElementStr(lpctstr pszKey)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::AddElementStr");
	if ( (m_listElements.size() + 1) >= INTPTR_MAX )	// overflow? is it even useful?
		return false;

	REMOVE_QUOTES( pszKey );

	m_listElements.emplace_back( new CListDefContStr(m_Key.GetPtr(), pszKey) );

	return true;
}

void CListDefCont::DeleteAtIterator(DefList::iterator it)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::DeleteAtIterator");

	if ( it != m_listElements.end() )
	{
		CListDefContElem *pListBase = (*it);
		m_listElements.erase(it);

		if ( pListBase )
		{
			CListDefContNum *pListNum = dynamic_cast<CListDefContNum *>(pListBase);
			if ( pListNum )
				delete pListNum;
			else
			{
				CListDefContStr *pListStr = dynamic_cast<CListDefContStr *>(pListBase);
				if ( pListStr )
					delete pListStr;
			}
		}
	}
}

bool CListDefCont::RemoveElement(size_t nIndex)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::RemoveElement");
	if ( nIndex >= m_listElements.size() )
		return false;

	DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);

    if (it == m_listElements.end())
        return false;

    DeleteAtIterator(it);
    return true;
}

void CListDefCont::RemoveAll()
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::RemoveAll");
	if ( m_listElements.empty() )
		return;

	DefList::iterator it = m_listElements.begin();

	while ( it != m_listElements.end() )
	{
		DeleteAtIterator(it);
		it = m_listElements.begin();
	}
}

static bool compare_insensitive (const CListDefContElem * firstelem, const CListDefContElem * secondelem)
{
    const CListDefContNum *pFirst = dynamic_cast<const CListDefContNum*>(firstelem);
    const CListDefContNum *pSecond = dynamic_cast<const CListDefContNum*>(secondelem);
	if (pFirst && pSecond)
	{
		int64 iFirst = pFirst->GetValNum();
		int64 iSecond = pSecond->GetValNum();
		return ( iFirst < iSecond );
	}
	else
	{
        const lpctstr first = firstelem->GetValStr();
        const lpctstr second = secondelem->GetValStr();
        const size_t uiFirstLen = strlen(first), uiSecondLen = strlen(second);
		uint i = 0;
		while ( (i < uiFirstLen) && (i < uiSecondLen))
		{
            const int cFirst = tolower(first[i]), cSecond = tolower(second[i]);
			if (cFirst < cSecond)
                return true;
			else if (cFirst > cSecond)
                return false;
			++i;
		}
		return ( uiFirstLen < uiSecondLen );
	}
}

static bool compare_sensitive (const CListDefContElem * firstelem, const CListDefContElem * secondelem)
{
    const CListDefContNum *pFirst = dynamic_cast<const CListDefContNum*>(firstelem);
    const CListDefContNum *pSecond = dynamic_cast<const CListDefContNum*>(secondelem);
    if (pFirst && pSecond)
    {
        int64 iFirst = pFirst->GetValNum();
        int64 iSecond = pSecond->GetValNum();
        return ( iFirst < iSecond );
    }
	else
	{
        const lpctstr first = firstelem->GetValStr();
        const lpctstr second = secondelem->GetValStr();
        const size_t uiFirstLen = strlen(first), uiSecondLen = strlen(second);
        uint i = 0;
        while ( (i < uiFirstLen) && (i < uiSecondLen))
		{
			if (first[i] < second[i])
                return true;
			else if (first[i] > second[i])
                return false;
			++i;
		}
        return ( uiFirstLen < uiSecondLen );
	}
}

void CListDefCont::Sort(bool bDesc, bool bCase)
{
	ADDTOCALLSTACK("CListDefCont::Sort");
	if ( !m_listElements.size() )
		return;

	if (bCase)
		std::sort(m_listElements.begin(), m_listElements.end(), compare_insensitive);
	else
        std::sort(m_listElements.begin(), m_listElements.end(), compare_sensitive);

	if (bDesc)
		std::reverse(m_listElements.begin(), m_listElements.end());
}

bool CListDefCont::InsertElementNum(size_t nIndex, int64 iVal)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::InsertElementNum");
	if ( nIndex >= m_listElements.size() )
		return false;

	DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    if (it == m_listElements.end())
        return false;

    m_listElements.insert(it, new CListDefContNum(m_Key.GetPtr(), iVal));
    return true;

	return false;
}

bool CListDefCont::InsertElementStr(size_t nIndex, lpctstr pszKey)
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::InsertElementStr");
	if ( nIndex >= m_listElements.size() )
		return false;

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    if (it == m_listElements.end())
        return false;

    m_listElements.insert(it, new CListDefContStr(m_Key.GetPtr(), pszKey));
    return true;
}

CListDefCont* CListDefCont::CopySelf()
{
	ADDTOCALLSTACK("CListDefCont::CopySelf");
    if (m_listElements.empty())
        return nullptr;
	
    CListDefCont* pNewList = new CListDefCont(m_Key.GetPtr());
	if ( !pNewList )
        return nullptr;

    for (const CListDefContElem *pElem : m_listElements)
    {
        pNewList->m_listElements.push_back(pElem->CopySelf());
    }

	return pNewList;
}

void CListDefCont::PrintElements(CSString& strElements) const
{
	ADDTOCALLSTACK("CListDefCont::PrintElements");
	if ( m_listElements.empty() )
	{
		strElements = "";
		return;
	}

	strElements = "{";

    const CListDefContStr *pListElemStr;
	for ( const CListDefContElem *pListElem : m_listElements)
    {
		pListElemStr = dynamic_cast<const CListDefContStr*>(pListElem);

		if ( pListElemStr )
		{
			strElements += "\"";
			strElements += pListElemStr->GetValStr();
			strElements += "\"";
		}
		else if ( pListElem )
			strElements += pListElem->GetValStr();

		strElements += ",";
	}

	strElements.SetAt(strElements.GetLength() - 1, '}');
}

void CListDefCont::DumpElements( CTextConsole * pSrc, lpctstr pszPrefix /* = nullptr */ ) const
{
	ADDTOCALLSTACK("CListDefCont::DumpElements");

	CSString strResult;
	PrintElements(strResult);

    if (pSrc->GetChar())
    {
        pSrc->SysMessagef("%s%s=%s\n", static_cast<lpctstr>(pszPrefix), static_cast<lpctstr>(m_Key.GetPtr()), static_cast<lpctstr>(strResult));
    }
    else
    {
        g_Log.Event(LOGL_EVENT, "%s%s=%s\n", static_cast<lpctstr>(pszPrefix), static_cast<lpctstr>(m_Key.GetPtr()), static_cast<lpctstr>(strResult));
    }
}

void CListDefCont::r_WriteSave( CScript& s ) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::r_WriteSave");
	if ( m_listElements.empty() )
		return;

    const CListDefContStr *pListElemStr;
	CSString strElement;

	s.WriteSection("LIST %s", m_Key.GetPtr());

    for ( const CListDefContElem *pListElem : m_listElements)
	{
		pListElemStr = dynamic_cast<const CListDefContStr*>(pListElem);

		if ( pListElemStr )
		{
			strElement.Format("\"%s\"", pListElemStr->GetValStr());
			s.WriteKey("ELEM", strElement.GetPtr());
		}
		else if ( pListElem )
			s.WriteKey("ELEM", pListElem->GetValStr());
	}
}

bool CListDefCont::r_LoadVal( CScript& s )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::r_LoadVal");
	bool fQuoted = false;
	lpctstr pszArg = s.GetArgStr(&fQuoted);

	if ( fQuoted || !IsSimpleNumberString(pszArg) )
		return AddElementStr(pszArg);

	return AddElementNum(Exp_Get64Val(pszArg));
}

bool CListDefCont::r_LoadVal( lpctstr pszArg )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefCont::r_LoadVal");

	if (!IsSimpleNumberString(pszArg) )
		return AddElementStr(pszArg);

	return AddElementNum(Exp_Get64Val(pszArg));
}

/***************************************************************************
*
*
*	class CListDefMap			Holds list of pairs KEY = VALUE1... and operates it
*
*
***************************************************************************/

CListDefMap & CListDefMap::operator = ( const CListDefMap & array )
{
	Copy( &array );
	return( *this );
}

CListDefMap::~CListDefMap()
{
	Empty();
}

CListDefCont * CListDefMap::GetAt( size_t at )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::GetAt");

	if ( at > m_Container.size() )
		return nullptr;

	DefSet::const_iterator i = m_Container.begin();
	std::advance(i, at);

	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

CListDefCont * CListDefMap::GetAtKey( lpctstr at )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::GetAtKey");

	CListDefCont pListBase(at);
	DefSet::const_iterator i = m_Container.find(&pListBase);

	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

void CListDefMap::DeleteAt( size_t at )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::DeleteAt");

	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
    std::advance(i, at);
	DeleteAtIterator(i);
}

void CListDefMap::DeleteAtKey( lpctstr at )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::DeleteAtKey");

	CListDefCont pListBase(at);
	DefSet::iterator i = m_Container.find(&pListBase);

	DeleteAtIterator(i);
}

void CListDefMap::DeleteAtIterator( DefSet::iterator it )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::DeleteAtIterator");

	if ( it != m_Container.end() )
	{
		CListDefCont * pListBase = (*it);
		m_Container.erase(it);
		pListBase->RemoveAll();
		delete pListBase;
	}
}

void CListDefMap::DeleteKey( lpctstr key )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::DeleteKey");

	if ( key && *key)
	{
		DeleteAtKey(key);
	}
}

void CListDefMap::Empty()
{
	ADDTOCALLSTACK("CListDefMap::Empty");

	DefSet::iterator i = m_Container.begin();
	CListDefCont * pListBase = nullptr;

	while ( i != m_Container.end() )
	{
		pListBase = (*i);
		m_Container.erase(i); // This don't free all the resource
		pListBase->RemoveAll();
		delete pListBase;
		i = m_Container.begin();
	}

	m_Container.clear();
}

void CListDefMap::Copy( const CListDefMap * pArray )
{
	ADDTOCALLSTACK("CListDefMap::Copy");

	if ( !pArray || pArray == this )
		return;

	Empty();

	if ( pArray->GetCount() <= 0 )
		return;

	for ( DefSet::const_iterator it = pArray->m_Container.begin(), itEnd = pArray->m_Container.end(); it != itEnd; ++it )
	{
		m_Container.insert( (*it)->CopySelf() );
	}
}

CListDefCont* CListDefMap::GetKey( lpctstr pszKey ) const
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::GetKey");

	CListDefCont * pReturn = nullptr;

	if ( pszKey && *pszKey )
	{
		CListDefCont pListBase(pszKey);
		DefSet::const_iterator i = m_Container.find(&pListBase);

		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

CListDefCont* CListDefMap::AddList(lpctstr pszKey)
{
	ADDTOCALLSTACK("CListDefMap::AddList");
	CListDefCont* pListBase = GetKey(pszKey);
	
	if ( !pListBase && pszKey && *pszKey )
	{
		pListBase = new CListDefCont(pszKey);
		m_Container.insert(pListBase);
	}

	return pListBase;
}

void CListDefMap::DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix )
{
	ADDTOCALLSTACK("CListDefMap::DumpKeys");
	// List out all the keys.
	ASSERT(pSrc);

	if ( pszPrefix == nullptr )
		pszPrefix = "";

    for (const CListDefCont *pCont : m_Container)
	{
        pCont->DumpElements(pSrc, pszPrefix);
	}
}

void CListDefMap::ClearKeys(lpctstr mask)
{
	ADDTOCALLSTACK("CListDefMap::ClearKeys");

	if ( mask && *mask )
	{
		if ( m_Container.empty() )
			return;

		CSString sMask(mask);
		sMask.MakeLower();

		DefSet::iterator i = m_Container.begin();
		CListDefCont * pListBase = nullptr;

		while ( i != m_Container.end() )
		{
			pListBase = (*i);

			if ( pListBase && ( strstr(pListBase->GetKey(), sMask.GetPtr()) ) )
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

bool CListDefMap::r_LoadVal( lpctstr pszKey, CScript & s )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::r_LoadVal");
	tchar* ppCmds[3];
	ppCmds[0] = const_cast<tchar*>(pszKey);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);
	lpctstr pszArg = s.GetArgRaw();

	if ( ppCmds[1] && (*(ppCmds[1])) ) // LIST.<list_name>.<something...>
	{
		Str_Parse(ppCmds[1], &(ppCmds[2]), "." );

		if ( !IsSimpleNumberString(ppCmds[1]) )
		{
			if ( !strnicmp(ppCmds[1], "clear", 5) )
			{
				if ( pListBase )
					DeleteKey(ppCmds[0]);

				return true;
			}
			else if ( !strnicmp(ppCmds[1], "add", 3) )
			{
				if ( !pszArg || !(*pszArg) )
					return false;

				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				if ( IsSimpleNumberString(pszArg) )
					return pListBase->AddElementNum(Exp_Get64Val(pszArg));
				else
					return pListBase->AddElementStr(pszArg);
			}
			else if ( (!strnicmp(ppCmds[1], "set", 3)) || (!strnicmp(ppCmds[1], "append", 6)) )
			{
				if ( !pszArg || !(*pszArg) )
					return false;

				if (( pListBase ) && ( !strnicmp(ppCmds[1], "set", 3) ))
				{
					DeleteKey(ppCmds[0]);
					pListBase = nullptr;
				}
				
				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				tchar* ppCmd[2];
				ppCmd[0] = const_cast<tchar*>(pszArg);
				while ( Str_Parse( ppCmd[0], &(ppCmd[1]), "," ))
				{
					if ( IsSimpleNumberString(ppCmd[0]) )
						pListBase->AddElementNum(Exp_Get64Val(ppCmd[0]));
					else
						pListBase->AddElementStr(ppCmd[0]);
					ppCmd[0] = ppCmd[1];
				}
				//insert last element
				if ( IsSimpleNumberString(ppCmd[0]) )
					return pListBase->AddElementNum(Exp_Get64Val(ppCmd[0]));
				else
					return pListBase->AddElementStr(ppCmd[0]);
			}
			else if ( !strnicmp(ppCmds[1], "sort", 4) )
			{
				if ( !pListBase )
					return false;

				if ( pszArg && *pszArg )
				{
					if ( !strnicmp(pszArg, "asc", 3) )
						pListBase->Sort();
					else if ( (!strnicmp(pszArg, "i", 1) ) || (!strnicmp(pszArg, "iasc", 4)) )
						pListBase->Sort(false, true);
					else if ( !strnicmp(pszArg, "desc", 4) )
						pListBase->Sort(true);
					else if ( !strnicmp(pszArg, "idesc", 5) )
						pListBase->Sort(true, true);
					else
						return false;
					return true;
				}
				//default to asc if not specified.
				pListBase->Sort();
				return true;
			}
		}
		else if ( pListBase )
		{
			size_t nIndex = Exp_GetVal(ppCmds[1]);

			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				if ( !strnicmp(ppCmds[2], "remove", 6) )
					return pListBase->RemoveElement(nIndex);
				else if ( !strnicmp(ppCmds[2], "insert", 6) && pszArg && *pszArg )
				{
					bool bIsNum = ( IsSimpleNumberString(pszArg) );

					if ( nIndex >= pListBase->GetCount() )
					{
						if ( bIsNum )
							return pListBase->AddElementNum(Exp_Get64Val(pszArg));
						else
							return pListBase->AddElementStr(pszArg);
					}

					CListDefContElem* pListElem = pListBase->GetAt(nIndex);

					if ( !pListElem )
						return false;

					if ( bIsNum )
						return pListBase->InsertElementNum(nIndex, Exp_Get64Val(pszArg));
					else
						return pListBase->InsertElementStr(nIndex, pszArg);
				}
			}
			else if ( pszArg && *pszArg )
			{
				CListDefContElem* pListElem = pListBase->GetAt(nIndex);

				if ( !pListElem )
					return false;

				if ( IsSimpleNumberString(pszArg) )
					return pListBase->SetNumAt(nIndex, Exp_Get64Val(pszArg));
				else
					return pListBase->SetStrAt(nIndex, pszArg);
			}
		}
		else
		{
			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				if ( !strnicmp(ppCmds[2], "insert", 6) && pszArg && *pszArg )
				{
					if ( IsSimpleNumberString(pszArg) )
						return pListBase->AddElementNum(Exp_Get64Val(pszArg));
					else
						return pListBase->AddElementStr(pszArg);
				}
			}
		}
	}
	else if ( pszArg && *pszArg )
	{
		if ( pListBase )
			pListBase->RemoveAll();
		else
		{
			pListBase = new CListDefCont(ppCmds[0]);
			m_Container.insert(pListBase);
		}

		if ( IsSimpleNumberString(pszArg) )
			return pListBase->AddElementNum(Exp_Get64Val(pszArg));
		else
			return pListBase->AddElementStr(pszArg);
	}
	else if ( pListBase )
	{
		DeleteKey(ppCmds[0]);

		return true;
	}

	return false;
}

bool CListDefMap::r_Write( CTextConsole *pSrc, lpctstr pszString, CSString& strVal )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::r_Write");
	UNREFERENCED_PARAMETER(pSrc);
	tchar * ppCmds[3];
	ppCmds[0] = const_cast<tchar*>(pszString);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);

	if ( !pListBase )
		return false;

	if ( !ppCmds[1] || !(*(ppCmds[1])) ) // LIST.<list_name>
	{
		pListBase->PrintElements(strVal);

		return true;
	}

	// LIST.<list_name>.<list_elem_index>

	int nStartIndex = -1;
	Str_Parse(ppCmds[1], &(ppCmds[2]), "." );

	if ( IsSimpleNumberString(ppCmds[1]) )
	{
		nStartIndex = Exp_GetVal(ppCmds[1]);
		CListDefContElem *pListElem = pListBase->GetAt(nStartIndex);
		if ( pListElem )
		{
			if ( !(*(ppCmds[2])) )
			{
				CListDefContStr *pListElemStr = dynamic_cast<CListDefContStr*>(pListElem);
				if ( pListElemStr )
					strVal.Format("\"%s\"", pListElemStr->GetValStr());
				else
					strVal = pListElem->GetValStr();

				return true;
			}
		}
		else
			return false;
	}
	else if ( !strnicmp(ppCmds[1], "count", 5) )
	{
		strVal.Format("%" PRIuSIZE_T, pListBase->GetCount());

		return true;
	}

	CScript s(nStartIndex == -1 ? ppCmds[1]:ppCmds[2]);
	nStartIndex = maximum(0, nStartIndex);

	if ( !strnicmp(s.GetKey(), "findelem", 8) )
	{
		bool fQuoted = false;
		lpctstr pszArg = s.GetArgStr(&fQuoted);

		if (( fQuoted ) || (! IsSimpleNumberString(pszArg) ))
			strVal.Format("%d", pListBase->FindValStr(pszArg, nStartIndex));
		else
			strVal.Format("%d", pListBase->FindValNum(Exp_Get64Val(pszArg), nStartIndex));

		return true;
	}

	return false;
}


void CListDefMap::r_WriteSave( CScript& s )
{
    ADDTOCALLSTACK_INTENSIVE("CListDefMap::r_WriteSave");

    for (const CListDefCont *pCont : m_Container)
	{
        pCont->r_WriteSave(s);
	}
}
