
// contains also some methods/functions declared in os_windows.h and os_unix.h

extern "C"
{
    // Put this here as just the starting offset. Needed by Windows crash dump.
    void globalstartsymbol() {}
}

#ifndef _WIN32

	#ifdef _BSD
        #include "common.h"
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

    #include "common.h"
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
