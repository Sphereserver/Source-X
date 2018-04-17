/**
* @file CVarDefMap.h
*
*/

#ifndef _INC_CVARDEFMAP_H
#define _INC_CVARDEFMAP_H

#include <set>
#include "sphere_library/CSString.h"
#include "common.h"


class CTextConsole;
class CScript;

class CVarDefCont
{
private:
	CSString m_Key;	// reference to map key

public:
	static const char *m_sClassName;

	CVarDefCont( lpctstr pszKey );
	virtual ~CVarDefCont();

private:
	CVarDefCont(const CVarDefCont& copy);
	CVarDefCont& operator=(const CVarDefCont& other);

public:
	lpctstr GetKey() const;
	void SetKey( lpctstr pszKey );

	virtual lpctstr GetValStr() const = 0;
	virtual int64 GetValNum() const = 0;
	virtual CVarDefCont * CopySelf() const = 0;
};

class CVarDefContNum : public CVarDefCont
{
private:
	int64 m_iVal;

public:
	static const char *m_sClassName;

	CVarDefContNum( lpctstr pszKey, int64 iVal );
	CVarDefContNum( lpctstr pszKey );
	virtual ~CVarDefContNum();

private:
	CVarDefContNum(const CVarDefContNum& copy);
	CVarDefContNum& operator=(const CVarDefContNum& other);

public:
	int64 GetValNum() const;
	void SetValNum( int64 iVal );
	lpctstr GetValStr() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );

	virtual CVarDefCont * CopySelf() const;
};

class CVarDefContStr : public CVarDefCont
{
private:
	CSString m_sVal;	// the assigned value. (What if numeric?)

public:
	static const char *m_sClassName;

	CVarDefContStr( lpctstr pszKey, lpctstr pszVal );
	explicit CVarDefContStr( lpctstr pszKey );
	virtual ~CVarDefContStr();

private:
	CVarDefContStr(const CVarDefContStr& copy);
	CVarDefContStr& operator=(const CVarDefContStr& other);

public:
	lpctstr GetValStr() const;
	void SetValStr( lpctstr pszVal );
	int64 GetValNum() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );

	virtual CVarDefCont * CopySelf() const;
};


class CVarDefMap
{
private:
	struct ltstr
	{
		bool operator()(CVarDefCont * s1, CVarDefCont * s2) const;
	};

	typedef std::set<CVarDefCont *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

	class CVarDefContTest : public CVarDefCont // This is to alloc CVarDefCont without allocing any other things
	{
		public:
			static const char *m_sClassName;

			CVarDefContTest( lpctstr pszKey );
			virtual ~CVarDefContTest();

		private:
			CVarDefContTest(const CVarDefContTest& copy);
			CVarDefContTest& operator=(const CVarDefContTest& other);

		public:
			lpctstr GetValStr() const;
			int64 GetValNum() const;
			virtual CVarDefCont * CopySelf() const;
	};

private:
	DefSet m_Container;

public:
	static const char *m_sClassName;

private:
	CVarDefCont * GetAtKey( lpctstr at ) const;
	void DeleteAt( size_t at );
	void DeleteAtKey( lpctstr at );
	void DeleteAtIterator( DefSet::iterator it );

	int SetNumOverride( lpctstr pszKey, int64 iVal );
	int SetStrOverride( lpctstr pszKey, lpctstr pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	bool Compare( const CVarDefMap * pArray );
	bool CompareAll( const CVarDefMap * pArray );
	void Empty();
	size_t GetCount() const;

public:
	CVarDefMap() { };
	~CVarDefMap();
	CVarDefMap & operator = ( const CVarDefMap & array );

private:
	CVarDefMap(const CVarDefMap& copy);

public:
	lpctstr FindValNum( int64 iVal ) const;
	lpctstr FindValStr( lpctstr pVal ) const;

	int SetNumNew( lpctstr pszKey, int64 iVal );
	int SetNum( lpctstr pszKey, int64 iVal, bool fZero = false );
	int SetStrNew( lpctstr pszKey, lpctstr pszVal );
	int SetStr( lpctstr pszKey, bool fQuoted, lpctstr pszVal, bool fZero = false );

	CVarDefCont * GetAt( size_t at ) const;
	CVarDefCont * GetKey( lpctstr pszKey ) const;
	int64 GetKeyNum( lpctstr pszKey, bool fZero = false ) const;
	lpctstr GetKeyStr( lpctstr pszKey, bool fZero = false ) const;
	CVarDefCont * GetParseKey( lpctstr & pArgs ) const;
	CVarDefCont * CheckParseKey( lpctstr & pszArgs ) const;
	bool GetParseVal( lpctstr & pArgs, llong * plVal ) const;

	void DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix = NULL ) const;
	void ClearKeys(lpctstr mask = NULL);
	void DeleteKey( lpctstr key );

	bool r_LoadVal( CScript & s );
	void r_WritePrefix( CScript & s, lpctstr pszPrefix = NULL, lpctstr pszKeyExclude = NULL );
};

#endif // _INC_CVARDEFMAP_H
