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
        memcpy(m_codes, pszLang, 3);
        m_codes[3] = 0;
        if (iswalnum(m_codes[0]))
            return true;
        // not valid !
    }
    m_codes[0] = 0;
    return false;
}
