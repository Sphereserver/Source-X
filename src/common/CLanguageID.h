#ifndef _INC_CLANGUAAGEID_H
#define _INC_CLANGUAAGEID_H

#include "common.h"

class CLanguageID
{
    // 3 letter code for language.
    // ENU,FRA,DEU,etc. (see langcode.iff)
    // terminate with a 0
    // 0 = english default.
    char m_codes[4]; // UNICODE language pref. ('ENU'=english)

public:
    CLanguageID() noexcept;
    CLanguageID( const char * pszInit ) noexcept;
    CLanguageID(int iDefault) noexcept;

    bool IsDef() const noexcept;
    void GetStrDef(tchar* pszLang) noexcept;
    void GetStr(tchar* pszLang) const noexcept;
    lpctstr GetStr() const;
    bool Set(lpctstr pszLang) noexcept;
};

#endif
