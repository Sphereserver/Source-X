
// contains also some methods/functions declared in os_windows.h and os_unix.h

#include "common.h"
#include "sphereproto.h"
#include <cwctype> // iswalnum

extern "C"
{
    // Put this here as just the starting offset. Needed by Windows crash dump.
    void globalstartsymbol() {}
}

#ifndef _WIN32

	#ifdef _BSD
        #include <cstring>
		#include <time.h>
		#include <sys/types.h>
		int getTimezone() noexcept
		{
			tm tp;
			memset(&tp, 0x00, sizeof(tp));
			mktime(&tp);
			return (int)tp.tm_zone;
			}
	#endif // _BSD

#else // _WIN32

	const OSVERSIONINFO * Sphere_GetOSInfo() noexcept
	{
		// NEVER return nullptr !
		static OSVERSIONINFO g_osInfo;
		if ( g_osInfo.dwOSVersionInfoSize != sizeof(g_osInfo) )
		{
			g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
			if ( ! GetVersionExA(&g_osInfo))
			{
				// must be an old version of windows. win95 or win31 ?
				memset( &g_osInfo, 0, sizeof(g_osInfo));
				g_osInfo.dwOSVersionInfoSize = sizeof(g_osInfo);
				g_osInfo.dwPlatformId = VER_PLATFORM_WIN32s;	// probably not right.
			}
		}
		return &g_osInfo;
	}
#endif // !_WIN32


// TODO: move those in a separate cpp file.

CLanguageID::CLanguageID(int iDefault) noexcept :
	m_codes{}
{
		UnreferencedParameter(iDefault);
		ASSERT(iDefault == 0);
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
