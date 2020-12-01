
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
CListDefContNum::CListDefContNum( lpctstr ptcKey, int64 iVal ) : CListDefContElem( ptcKey ), m_iVal( iVal )
{
}

CListDefContNum::CListDefContNum( lpctstr ptcKey ) : CListDefContElem( ptcKey ), m_iVal( 0 )
{
}

lpctstr CListDefContNum::GetValStr() const
{
    return Str_FromLL_Fast(m_iVal, Str_GetTemp(), STR_TEMPLENGTH, 16);
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
CListDefContStr::CListDefContStr( lpctstr ptcKey, lpctstr pszVal ) : CListDefContElem( ptcKey ), m_sVal( pszVal ) 
{
}

CListDefContStr::CListDefContStr( lpctstr ptcKey ) : CListDefContElem( ptcKey )
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
CListDefCont::CListDefCont( lpctstr ptcKey ) : m_Key( ptcKey ) 
{ 
}

void CListDefCont::SetKey( lpctstr ptcKey )
{ 
	m_Key = ptcKey;
}

CListDefContElem* CListDefCont::GetAt(size_t nIndex) const
{
    ADDTOCALLSTACK("CListDefCont::GetAt");
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
    ADDTOCALLSTACK("CListDefCont::SetNumAt");
	CListDefContElem* pListElem = GetAt(nIndex);

	if ( !pListElem )
		return false;

	CListDefContElem* pListNewElem = new CListDefContNum(m_Key.GetBuffer(), iVal);

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    DeleteAtIterator(it, false);
    *it = pListNewElem;

	return true;
}

bool CListDefCont::SetStrAt(size_t nIndex, lpctstr pszVal)
{
    ADDTOCALLSTACK("CListDefCont::SetStrAt");
	CListDefContElem* pListElem = GetAt(nIndex);

	if ( !pListElem )
		return false;

	CListDefContElem* pListNewElem = new CListDefContStr(m_Key.GetBuffer(), pszVal);

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    DeleteAtIterator(it, false);
    *it = pListNewElem;

	return true;
}

lpctstr CListDefCont::GetValStr(size_t nIndex) const
{
    ADDTOCALLSTACK("CListDefCont::GetValStr");
	CListDefContElem* pElem = GetAt(nIndex);

	if ( !pElem )
		return nullptr;

	return pElem->GetValStr();
}

int64 CListDefCont::GetValNum(size_t nIndex) const
{
    ADDTOCALLSTACK("CListDefCont::GetValNum");
	CListDefContElem* pElem = GetAt(nIndex);

	if ( !pElem )
		return 0;

	return pElem->GetValNum();
}

int CListDefCont::FindValStr( lpctstr pVal, size_t nStartIndex /* = 0 */ ) const
{
    ADDTOCALLSTACK("CListDefCont::FindValStr");

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
    ADDTOCALLSTACK("CListDefCont::FindValNum");

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
    ADDTOCALLSTACK("CListDefCont::AddElementNum");
	if ( (m_listElements.size() + 1) >= INTPTR_MAX )	// overflow? is it even useful?
		return false;

	m_listElements.emplace_back( new CListDefContNum(m_Key.GetBuffer(), iVal) );

	return true;
}

bool CListDefCont::AddElementStr(lpctstr ptcKey)
{
    ADDTOCALLSTACK("CListDefCont::AddElementStr");
	if ( (m_listElements.size() + 1) >= INTPTR_MAX )	// overflow? is it even useful?
		return false;

	REMOVE_QUOTES( ptcKey );

	m_listElements.emplace_back( new CListDefContStr(m_Key.GetBuffer(), ptcKey) );

	return true;
}

void CListDefCont::DeleteAtIterator(DefList::iterator it, bool fEraseFromDefList)
{
    ADDTOCALLSTACK("CListDefCont::DeleteAtIterator");

    if (it == m_listElements.end())
        return;

    CListDefContElem* pListBase = (*it);
    if (fEraseFromDefList)
        m_listElements.erase(it);

    if (pListBase)
    {
        CListDefContNum* pListNum = dynamic_cast<CListDefContNum*>(pListBase);
        if (pListNum)
            delete pListNum;
        else
        {
            CListDefContStr* pListStr = dynamic_cast<CListDefContStr*>(pListBase);
            if (pListStr)
                delete pListStr;
        }
    }
}

bool CListDefCont::RemoveElement(size_t nIndex)
{
    ADDTOCALLSTACK("CListDefCont::RemoveElement");
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
    ADDTOCALLSTACK("CListDefCont::RemoveAll");
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
    ASSERT(firstelem);
    ASSERT(secondelem);
    const CListDefContNum *pFirst = dynamic_cast<const CListDefContNum*>(firstelem);
    const CListDefContNum *pSecond = dynamic_cast<const CListDefContNum*>(secondelem);
	if (pFirst && pSecond)
	{
		const int64 iFirst = pFirst->GetValNum();
        const int64 iSecond = pSecond->GetValNum();
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
    ASSERT(firstelem);
    ASSERT(secondelem);
    const CListDefContNum *pFirst = dynamic_cast<const CListDefContNum*>(firstelem);
    const CListDefContNum *pSecond = dynamic_cast<const CListDefContNum*>(secondelem);
    if (pFirst && pSecond)
    {
        const int64 iFirst = pFirst->GetValNum();
        const int64 iSecond = pSecond->GetValNum();
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
    ADDTOCALLSTACK("CListDefCont::InsertElementNum");
	if ( nIndex >= m_listElements.size() )
		return false;

	DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    if (it == m_listElements.end())
        return false;

    m_listElements.insert(it, new CListDefContNum(m_Key.GetBuffer(), iVal));
    return true;

	return false;
}

bool CListDefCont::InsertElementStr(size_t nIndex, lpctstr ptcKey)
{
    ADDTOCALLSTACK("CListDefCont::InsertElementStr");
	if ( nIndex >= m_listElements.size() )
		return false;

    DefList::iterator it = m_listElements.begin();
    std::advance(it, nIndex);
    if (it == m_listElements.end())
        return false;

    m_listElements.insert(it, new CListDefContStr(m_Key.GetBuffer(), ptcKey));
    return true;
}

CListDefCont* CListDefCont::CopySelf()
{
	ADDTOCALLSTACK("CListDefCont::CopySelf");
    if (m_listElements.empty())
        return nullptr;
	
    CListDefCont* pNewList = new CListDefCont(m_Key.GetBuffer());
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
        pSrc->SysMessagef("%s%s=%s\n", pszPrefix, m_Key.GetBuffer(), strResult.GetBuffer());
    }
    else
    {
        g_Log.Event(LOGL_EVENT, "%s%s=%s\n", pszPrefix, m_Key.GetBuffer(), strResult.GetBuffer());
    }
}

void CListDefCont::r_WriteSave( CScript& s ) const
{
    ADDTOCALLSTACK("CListDefCont::r_WriteSave");
	if ( m_listElements.empty() )
		return;

    const CListDefContStr *pListElemStr;
	CSString strElement;

	s.WriteSection("LIST %s", m_Key.GetBuffer());

    for ( const CListDefContElem *pListElem : m_listElements)
	{
		pListElemStr = dynamic_cast<const CListDefContStr*>(pListElem);

		if ( pListElemStr )
		{
			strElement.Format("\"%s\"", pListElemStr->GetValStr());
			s.WriteKey("ELEM", strElement.GetBuffer());
		}
		else if ( pListElem )
			s.WriteKey("ELEM", pListElem->GetValStr());
	}
}

bool CListDefCont::r_LoadVal( CScript& s )
{
    ADDTOCALLSTACK("CListDefCont::r_LoadVal");
	bool fQuoted = false;
	lpctstr ptcArg = s.GetArgStr(&fQuoted);

	if ( fQuoted || !IsSimpleNumberString(ptcArg) )
		return AddElementStr(ptcArg);

	return AddElementNum(Exp_Get64Val(ptcArg));
}

bool CListDefCont::r_LoadVal( lpctstr ptcArg )
{
    ADDTOCALLSTACK("CListDefCont::r_LoadVal");

	if (!IsSimpleNumberString(ptcArg) )
		return AddElementStr(ptcArg);

	return AddElementNum(Exp_Get64Val(ptcArg));
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
    ADDTOCALLSTACK("CListDefMap::GetAt");

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
    ADDTOCALLSTACK("CListDefMap::GetAtKey");

	CListDefCont pListBase(at);
	DefSet::const_iterator i = m_Container.find(&pListBase);

	if ( i != m_Container.end() )
		return (*i);
	else
		return nullptr;
}

void CListDefMap::DeleteAt( size_t at )
{
    ADDTOCALLSTACK("CListDefMap::DeleteAt");

	if ( at > m_Container.size() )
		return;

	DefSet::iterator i = m_Container.begin();
    std::advance(i, at);
	DeleteAtIterator(i);
}

void CListDefMap::DeleteAtKey( lpctstr at )
{
    ADDTOCALLSTACK("CListDefMap::DeleteAtKey");

	CListDefCont pListBase(at);
	DefSet::iterator i = m_Container.find(&pListBase);

	DeleteAtIterator(i);
}

void CListDefMap::DeleteAtIterator( DefSet::iterator it )
{
    ADDTOCALLSTACK("CListDefMap::DeleteAtIterator");

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
    ADDTOCALLSTACK("CListDefMap::DeleteKey");

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
		m_Container.erase(i); // This doesn't free all the resource
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

CListDefCont* CListDefMap::GetKey( lpctstr ptcKey ) const
{
    ADDTOCALLSTACK("CListDefMap::GetKey");

	CListDefCont * pReturn = nullptr;

	if ( ptcKey && *ptcKey )
	{
		CListDefCont pListBase(ptcKey);
		DefSet::const_iterator i = m_Container.find(&pListBase);

		if ( i != m_Container.end() )
			pReturn = (*i);
	}

	return pReturn;
}

CListDefCont* CListDefMap::AddList(lpctstr ptcKey)
{
	ADDTOCALLSTACK("CListDefMap::AddList");
	CListDefCont* pListBase = GetKey(ptcKey);
	
	if ( !pListBase && ptcKey && *ptcKey )
	{
		pListBase = new CListDefCont(ptcKey);
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

			if ( pListBase && ( strstr(pListBase->GetKey(), sMask.GetBuffer()) ) )
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

bool CListDefMap::r_LoadVal( lpctstr ptcKey, CScript & s )
{
    ADDTOCALLSTACK("CListDefMap::r_LoadVal");
	tchar* ppCmds[3];
	ppCmds[0] = const_cast<tchar*>(ptcKey);
	Str_Parse(ppCmds[0], &(ppCmds[1]), "." );

	CListDefCont* pListBase = GetKey(ppCmds[0]);
	lpctstr ptcArg = s.GetArgRaw();

	// LIST.<list_name>
	if ( ppCmds[1] && *(ppCmds[1]) )
	{
		Str_Parse(ppCmds[1], &(ppCmds[2]), "." );

		if ( !IsSimpleNumberString(ppCmds[1]) )
		{
			// LIST.<list_name>.<operation>
			// Am i calling a valid operation?

			if ( !strnicmp(ppCmds[1], "clear", 5) )
			{
				// Clears LIST.xxx
				if ( pListBase )
					DeleteKey(ppCmds[0]);

				return true;
			}
			else if ( !strnicmp(ppCmds[1], "add", 3) )
			{
				// Adds <args> as a new element in LIST.xxx
				if ( !ptcArg || !(*ptcArg) )
					return false;

				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				if ( IsSimpleNumberString(ptcArg) )
					return pListBase->AddElementNum(Exp_Get64Val(ptcArg));
				else
					return pListBase->AddElementStr(ptcArg);
			}
			else if ( (!strnicmp(ppCmds[1], "set", 3)) || (!strnicmp(ppCmds[1], "append", 6)) )
			{
				if ( !ptcArg || !(*ptcArg) )
					return false;

				if ( pListBase && ( !strnicmp(ppCmds[1], "set", 3) ))
				{
					// Set: Clears the list and sets each <args> as a new element in LIST.xxx
					DeleteKey(ppCmds[0]);
					pListBase = nullptr;
				}
				
				// Append: Sets each <args> as a new element in LIST.xxx
				if ( !pListBase )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);
				}

				tchar* ppCmd[2];
				ppCmd[0] = const_cast<tchar*>(ptcArg);
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
				// Re-orders LIST.xxx according to <args>. (possible values are: no args , i , asc , iasc , desc , idesc) 
				if ( !pListBase )
					return false;

				if ( ptcArg && *ptcArg )
				{
					if ( !strnicmp(ptcArg, "asc", 3) )
						pListBase->Sort();
					else if ( (!strnicmp(ptcArg, "i", 1) ) || (!strnicmp(ptcArg, "iasc", 4)) )
						pListBase->Sort(false, true);
					else if ( !strnicmp(ptcArg, "desc", 4) )
						pListBase->Sort(true);
					else if ( !strnicmp(ptcArg, "idesc", 5) )
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
			// LIST.<list_name>.<element index (zero-based)>...
			size_t nIndex = Exp_GetVal(ppCmds[1]);

			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				// LIST.<list_name>.<element index>.<operation>

				if (!strnicmp(ppCmds[2], "remove", 6))
				{
					// Removes the nth element in LIST.xxx
					return pListBase->RemoveElement(nIndex);
				}
				else if ( !strnicmp(ppCmds[2], "insert", 6) && ptcArg && *ptcArg )
				{
					// Inserts <args> at the nth index of LIST.xxx
					const bool fIsNum = ( IsSimpleNumberString(ptcArg) );

					if ( nIndex >= pListBase->GetCount() )
					{
						if ( fIsNum )
							return pListBase->AddElementNum(Exp_Get64Val(ptcArg));
						else
							return pListBase->AddElementStr(ptcArg);
					}

					CListDefContElem* pListElem = pListBase->GetAt(nIndex);

					if ( !pListElem )
						return false;

					if ( fIsNum )
						return pListBase->InsertElementNum(nIndex, Exp_Get64Val(ptcArg));
					else
						return pListBase->InsertElementStr(nIndex, ptcArg);
				}
			}
			else if ( ptcArg && *ptcArg )
			{
				// LIST.<list_name>.<element index> -> set value
				CListDefContElem* pListElem = pListBase->GetAt(nIndex);

				if ( !pListElem )
					return false;

				if ( IsSimpleNumberString(ptcArg) )
					return pListBase->SetNumAt(nIndex, Exp_Get64Val(ptcArg));
				else
					return pListBase->SetStrAt(nIndex, ptcArg);
			}
		}
		else
		{
			if ( ppCmds[2] && *(ppCmds[2]) )
			{
				if ( !strnicmp(ppCmds[2], "insert", 6) && ptcArg && *ptcArg )
				{
					pListBase = new CListDefCont(ppCmds[0]);
					m_Container.insert(pListBase);

					if ( IsSimpleNumberString(ptcArg) )
						return pListBase->AddElementNum(Exp_Get64Val(ptcArg));
					else
						return pListBase->AddElementStr(ptcArg);
				}
			}
		}
	}
	else if ( ptcArg && *ptcArg )
	{
		if (pListBase)
		{
			pListBase->RemoveAll();
		}
		else
		{
			pListBase = new CListDefCont(ppCmds[0]);
			m_Container.insert(pListBase);
		}

		if ( IsSimpleNumberString(ptcArg) )
			return pListBase->AddElementNum(Exp_Get64Val(ptcArg));
		else
			return pListBase->AddElementStr(ptcArg);
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
    ADDTOCALLSTACK("CListDefMap::r_Write");
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
		lpctstr ptcArg = s.GetArgStr(&fQuoted);

		if (( fQuoted ) || (! IsSimpleNumberString(ptcArg) ))
			strVal.Format("%d", pListBase->FindValStr(ptcArg, nStartIndex));
		else
			strVal.Format("%d", pListBase->FindValNum(Exp_Get64Val(ptcArg), nStartIndex));

		return true;
	}

	return false;
}


void CListDefMap::r_WriteSave( CScript& s )
{
    ADDTOCALLSTACK("CListDefMap::r_WriteSave");

    for (const CListDefCont *pCont : m_Container)
	{
        pCont->r_WriteSave(s);
	}
}
