#include "../sphere/threads.h"
#include "CExpression.h"
#include "CLog.h"
#include "CLocalVarsExtra.h"


bool CLocalFloatVars::LexNoCaseLess::operator()(const CSString& CGStr, const char* pBeg2) const
{
    const char* pBeg1 = CGStr.GetPtr();
    const char* pEnd1 = pBeg1;
    const char* pEnd2 = pBeg2;
    while (*pEnd1)
        ++pEnd1;
    while (*pEnd2)
        ++pEnd2;
    ++pEnd1, ++pEnd2;

	for (; pBeg1 != pEnd1 && pBeg2 != pEnd2; ++pBeg1, ++pBeg2)
	{
		const int f = tolower(*pBeg1), s = tolower(*pBeg2);
		if (f < s)
			return true;
		if (s < f)
			return false;
	}
    return (pBeg1 == pEnd1 && pBeg2 != pEnd2);
}


bool CLocalFloatVars::Set( const char* VarName, const char* VarValue )
{
	ADDTOCALLSTACK("CLocalFloatVars::Set");
	return Insert(VarName, VarValue, true);
}

bool CLocalFloatVars::Insert( const char* VarName, const char* VarValue, bool fForceSet) 
{
	ADDTOCALLSTACK("CLocalFloatVars::Insert");
	if (!VarValue || !VarName)
		return false;

	MapType::const_iterator i = m_VarMap.find(VarName);
	if ( i != m_VarMap.end() && !fForceSet)
		return false;

	SKIP_ARGSEP(VarValue);
	SKIP_ARGSEP(VarName);
	char* pEnd;
	realtype Real = strtod(VarValue, &pEnd);
	m_VarMap[CSString(VarName)] = Real;
	return true;
}

realtype CLocalFloatVars::GetVal( const char* VarName )
{
	ADDTOCALLSTACK("CLocalFloatVars::GetVal");
	if ( !VarName )
		return 0.0;

	SKIP_ARGSEP(VarName);
	MapType::const_iterator i = m_VarMap.find(VarName);
	if ( i == m_VarMap.end())
		return 0.0;
	return i->second;
}

CSString CLocalFloatVars::Get( const char* VarName )
{
	ADDTOCALLSTACK("CLocalFloatVars::Get");
	if ( !VarName )
		return CSString();

	SKIP_ARGSEP(VarName);
	if ( strlen(VarName) > VARDEF_FLOAT_MAXBUFFERSIZE )
		return CSString();

	realtype Real = GetVal(VarName);
	char szReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	sprintf(szReal, "%f", Real);

	return CSString(szReal);
}

void CLocalFloatVars::Clear()
{
	m_VarMap.clear();
}


//--

CObjBase * CLocalObjMap::Get( ushort Number )
{
	ADDTOCALLSTACK("CLocalObjMap::Get");
	if ( !Number )
		return nullptr;

	ObjMap::const_iterator i = m_ObjMap.find(Number);
	if ( i == m_ObjMap.end() )
		return nullptr;

	return i->second;
}

bool CLocalObjMap::Insert( ushort Number, CObjBase * pObj, bool fForceSet )
{
	ADDTOCALLSTACK("CLocalObjMap::Insert");
	if ( !Number )
		return false;

	ObjMap::const_iterator i = m_ObjMap.find(Number);
	if ( i != m_ObjMap.end() && !fForceSet )
		return false;

	m_ObjMap[Number] = pObj;
	return true;
}

void CLocalObjMap::Clear()
{
	m_ObjMap.clear();
}