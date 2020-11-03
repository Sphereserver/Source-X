/**
* @file CVarDefMap.h
*
*/

#ifndef _INC_CVARDEFMAP_H
#define _INC_CVARDEFMAP_H

#include "sphere_library/CSString.h"
#include "sphere_library/CSSortedVector.h"


class CTextConsole;
class CScript;

class CVarDefCont
{
public:
	static const char *m_sClassName;

	CVarDefCont()           = default;
	virtual ~CVarDefCont()  = default;

private:
	CVarDefCont(const CVarDefCont& copy);
	CVarDefCont& operator=(const CVarDefCont& other);

public:
    virtual lpctstr GetKey() const noexcept = 0;
    virtual void SetKey( lpctstr ptcKey )   = 0;

	virtual lpctstr GetValStr() const       = 0;
	virtual int64 GetValNum() const         = 0;
	virtual CVarDefCont * CopySelf() const  = 0;
};

class CVarDefContNum : public CVarDefCont
{
private:
    CSString m_sKey;    // reference to map key
	int64 m_iVal;       // the assigned value

public:
	static const char *m_sClassName;

	CVarDefContNum( lpctstr ptcKey, int64 iVal );
	CVarDefContNum( lpctstr ptcKey );
	virtual ~CVarDefContNum() = default;

private:
	CVarDefContNum(const CVarDefContNum& copy);
	CVarDefContNum& operator=(const CVarDefContNum& other);

public:
    inline virtual lpctstr GetKey() const noexcept override {
        return m_sKey.GetBuffer();
    }
    inline virtual void SetKey(lpctstr ptcKey) override {
        m_sKey = ptcKey;
    }

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
    CSString m_sKey;    // map key
	CSString m_sVal;    // the assigned value

public:
	static const char *m_sClassName;

	CVarDefContStr( lpctstr ptcKey, lpctstr pszVal );
	explicit CVarDefContStr( lpctstr ptcKey );
	virtual ~CVarDefContStr() = default;

private:
	CVarDefContStr(const CVarDefContStr& copy);
	CVarDefContStr& operator=(const CVarDefContStr& other);

public:
    inline virtual lpctstr GetKey() const noexcept override {
        return m_sKey.GetBuffer();
    }
    inline virtual void SetKey(lpctstr ptcKey) override {
        m_sKey = ptcKey;
    }

    void SetValStr( lpctstr pszVal );
    inline virtual lpctstr GetValStr() const override {
        return m_sVal.GetBuffer(); 
    }
    virtual int64 GetValNum() const override;
    virtual CVarDefCont * CopySelf() const override;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc );
};


class CVarDefMap
{
	struct ltstr
	{
		inline bool operator()(const CVarDefCont * s1, const CVarDefCont * s2) const noexcept
        {
            return ( strcmpi(s1->GetKey(), s2->GetKey()) < 0 );
        }
	};
	using DefCont = CSSortedVector<CVarDefCont *, ltstr>;

	DefCont m_Container;

public:
	static const char *m_sClassName;
    using iterator          = DefCont::iterator;
    using const_iterator    = DefCont::const_iterator;

private:
	CVarDefCont * GetAtKey( lpctstr ptcKey ) const;
	void DeleteAt( size_t at );
	void DeleteAtKey( lpctstr ptcKey );

    CVarDefContNum* SetNumOverride( lpctstr ptcKey, int64 iVal );
    CVarDefContStr* SetStrOverride( lpctstr ptcKey, lpctstr pszVal );

public:
	void Copy( const CVarDefMap * pArray );
	bool Compare( const CVarDefMap * pArray );
	bool CompareAll( const CVarDefMap * pArray );
	void Clear();
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

    CVarDefContNum* SetNumNew( lpctstr ptcKey, int64 iVal );
    CVarDefContNum* SetNum( lpctstr ptcKey, int64 iVal, bool fDeleteZero = true, bool fWarnOverwrite = true );
    CVarDefContNum* ModNum( lpctstr ptcKey, int64 iMod, bool fDeleteZero = true);
    CVarDefContStr* SetStrNew( lpctstr ptcKey, lpctstr pszVal );
    CVarDefCont* SetStr( lpctstr ptcKey, bool fQuoted, lpctstr pszVal, bool fDeleteZero = true, bool fWarnOverwrite = true );

	CVarDefCont * GetAt( size_t at ) const;
	CVarDefCont * GetKey( lpctstr ptcKey ) const;
    inline CVarDefContNum * GetKeyDefNum( lpctstr ptcKey ) const;
	int64 GetKeyNum( lpctstr ptcKey ) const;
    inline CVarDefContStr * GetKeyDefStr( lpctstr ptcKey ) const;
	lpctstr GetKeyStr( lpctstr ptcKey, bool fZero = false ) const;
    CVarDefCont * CheckParseKey( lpctstr pszArgs ) const;
	CVarDefCont * GetParseKey_Advance( lpctstr & pArgs ) const;
    inline CVarDefCont * GetParseKey( lpctstr pArgs ) const;
    bool GetParseVal_Advance( lpctstr & pArgs, llong * pllVal ) const;
    inline bool GetParseVal( lpctstr pArgs, llong * plVal ) const;

	void DumpKeys( CTextConsole * pSrc, lpctstr pszPrefix = nullptr ) const;
	void ClearKeys(lpctstr mask = nullptr);
	void DeleteKey( lpctstr key );

	//bool r_LoadVal( CScript & s );
	void r_WritePrefix( CScript & s, lpctstr pszPrefix = nullptr, lpctstr pszKeyExclude = nullptr ) const;

    // Iterators
    inline iterator begin();
    inline iterator end();
    inline const_iterator begin() const;
    inline const_iterator end() const;
};


/* Inline methods definitions */

CVarDefContNum * CVarDefMap::GetKeyDefNum(lpctstr ptcKey) const
{
    return dynamic_cast<CVarDefContNum*>(GetKey(ptcKey));
}

CVarDefContStr * CVarDefMap::GetKeyDefStr(lpctstr ptcKey) const
{
    return dynamic_cast<CVarDefContStr*>(GetKey(ptcKey));
}

CVarDefCont * CVarDefMap::GetParseKey(lpctstr pArgs) const
{
    return GetParseKey_Advance(pArgs);
}

bool CVarDefMap::GetParseVal(lpctstr pArgs, llong * pllVal) const
{
    return GetParseVal_Advance(pArgs, pllVal);
}

CVarDefMap::iterator CVarDefMap::begin()                { return m_Container.begin();   }
CVarDefMap::iterator CVarDefMap::end()                  { return m_Container.end();     }
CVarDefMap::const_iterator CVarDefMap::begin() const    { return m_Container.cbegin();  }
CVarDefMap::const_iterator CVarDefMap::end() const      { return m_Container.cend();    }

#endif // _INC_CVARDEFMAP_H
