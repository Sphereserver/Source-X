#include "sphere_library/sstringobjs.h"
#include "CLanguageID.h"
#include <cstring>
#include <cwctype>


CLanguageID::CLanguageID() noexcept :
    m_codes{}
{}

CLanguageID::CLanguageID( const char * pszInit ) noexcept
{
    Set( pszInit );
}

CLanguageID::CLanguageID(int iDefault) noexcept :
    m_codes{}
{
    UnreferencedParameter(iDefault);
    ASSERT(iDefault == 0);
}

bool CLanguageID::IsDef() const noexcept
{
    return ( m_codes[0] != 0 );
}

void CLanguageID::GetStrDef(tchar* pszLang) noexcept
{
    if (!IsDef())
    {
        strcpy(pszLang, "enu"); // NOLINT(clang-analyzer-security.insecureAPI.strcpy)
    }
    else
    {
        memcpy(pszLang, m_codes, 3);
        pszLang[3] = '\0';
    }
}

void CLanguageID::GetStr(tchar* pszLang) const noexcept
{
    memcpy(pszLang, m_codes, 3);
    pszLang[3] = '\0';
}

lpctstr CLanguageID::GetStr() const
{
    tchar* pszTmp = Str_GetTemp();
    GetStr(pszTmp);
    return pszTmp;
}

bool CLanguageID::Set(lpctstr pszLang) noexcept
{
    // needs not be terminated!
    if (pszLang != nullptr)
    {
        // Up to 3 chars, stopping at the terminator if there's one: the source can be shorter (like ""),
        //  a blind 3 bytes memcpy would read past its end.
        size_t i = 0;
        for (; (i < 3) && (pszLang[i] != '\0'); ++i)
            m_codes[i] = pszLang[i];
        for (; i < 4; ++i)
            m_codes[i] = 0;
        if (iswalnum(m_codes[0]))
            return true;
        // not valid !
    }
    m_codes[0] = 0;
    return false;
}
