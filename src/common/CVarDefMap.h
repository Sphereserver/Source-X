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
	virtual ~CVarDefContNum() = default;

private:
	CVarDefContNum(const CVarDefContNum& copy);
	CVarDefContNum& operator=(const CVarDefContNum& other);

public:
    inline void SetValNum(int64 iVal) {
        m_iVal = iVal;
    }
    inline virtual int64 GetValNum() const override {
        return m_iVal;
    }
	virtual lpctstr GetValStr() const override;
    virtual CVarDefCont * CopySelf() const override;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );
};

class CVarDefContStr : public CVarDefCont
{
private:
	CSString m_sVal;	// the assigned value. (What if numeric?)

public:
	static const char *m_sClassName;

	CVarDefContStr( lpctstr pszKey, lpctstr pszVal );
	explicit CVarDefContStr( lpctstr pszKey );
	virtual ~CVarDefContStr() = default;

private:
	CVarDefContStr(const CVarDefContStr& copy);
	CVarDefContStr& operator=(const CVarDefContStr& other);

public:
    void SetValStr( lpctstr pszVal );
    inline virtual lpctstr GetValStr() const override {
        return m_sVal.GetPtr(); 
    }
    virtual int64 GetValNum() const override;
    virtual CVarDefCont * CopySelf() const override;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );
};


class CVarDefMap
{
private:
	struct ltstr
	{
		bool operator()(const CVarDefCont * s1, const CVarDefCont * s2) const;
	};

	typedef std::set<CVarDefCont *, ltstr> DefSet;
	typedef std::pair<DefSet::iterator, bool> DefPairResult;

	class CVarDefContTest : public CVarDefCont // This is to alloc CVarDefCont without allocing any other things
	{
		public:
			static const char *m_sClassName;

            inline CVarDefContTest( lpctstr pszKey ) : CVarDefCont( pszKey ) {}
			virtual ~CVarDefContTest() = default;

		private:
			CVarDefContTest(const CVarDefContTest& copy);
			CVarDefContTest& operator=(const CVarDefContTest& other);

		public:
			virtual lpctstr GetValStr() const override;
			virtual int64 GetValNum() const override;
			virtual CVarDefCont * CopySelf() const override;
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

    CVarDefContNum* SetNumOverride( lpctstr pszKey, int64 iVal );
    CVarDefContStr* SetStrOverride( lpctstr pszKey, lpctstr pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	bool Compare( const CVarDefMap * pArray );
	bool CompareAll( const CVarDefMap * pArray );
	void Empty();
	size_t GetCount() const;

public:
	CVarDefMap() = default;
	~CVarDefMap();
	CVarDefMap & operator = ( const CVarDefMap & array );

private:
	CVarDefMap(const CVarDefMap& copy);

public:
	lpctstr FindValNum( int64 iVal ) const;
	lpctstr FindValStr( lpctstr pVal ) const;

    CVarDefContNum* SetNumNew( lpctstr pszKey, int64 iVal );
    CVarDefContNum* SetNum( lpctstr pszKey, int64 iVal, bool fDeleteZero = false );
    CVarDefContNum* ModNum( lpctstr pszKey, int64 iMod, bool fDeleteZero = false );
    CVarDefContStr* SetStrNew( lpctstr pszKey, lpctstr pszVal );
    CVarDefCont* SetStr( lpctstr pszKey, bool fQuoted, lpctstr pszVal, bool fDeleteZero = false );

	CVarDefCont * GetAt( size_t at ) const;
	CVarDefCont * GetKey( lpctstr pszKey ) const;
    inline CVarDefContNum * GetKeyDefNum( lpctstr pszKey ) const;
	int64 GetKeyNum( lpctstr pszKey ) const;
    inline CVarDefContStr * GetKeyDefStr( lpctstr pszKey ) const;
	lpctstr GetKeyStr( lpctstr pszKey, bool fZero = false ) const;
	CVarDefCont * GetParseKey( lpctstr & pArgs ) const;
	CVarDefCont * CheckParseKey( lpctstr & pszArgs ) const;
	bool GetParseVal( lpctstr & pArgs, llong * plVal ) const;

	void DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix = nullptr ) const;
	void ClearKeys(lpctstr mask = nullptr);
	void DeleteKey( lpctstr key );

	bool r_LoadVal( CScript & s );
	void r_WritePrefix( CScript & s, lpctstr pszPrefix = nullptr, lpctstr pszKeyExclude = nullptr );
};


/* Inline methods definitions */

CVarDefContNum * CVarDefMap::GetKeyDefNum(lpctstr pszKey) const
{
    return dynamic_cast<CVarDefContNum*>(GetKey(pszKey));
}

CVarDefContStr * CVarDefMap::GetKeyDefStr(lpctstr pszKey) const
{
    return dynamic_cast<CVarDefContStr*>(GetKey(pszKey));
}

#endif // _INC_CVARDEFMAP_H
