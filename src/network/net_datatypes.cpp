#include "net_datatypes.h"
#include <cstring>

#ifdef _WIN32
	#include <winsock2.h>	// this needs to be included after common.h, which sets some defines and then includes windows.h, since winsock2.h needs windows.h
#else
    #include <netinet/in.h>
#endif


nword::operator word () const noexcept
{
    return ntohs(m_val);
}

nword& nword::operator = (word val) noexcept
{
    m_val = htons(val);
    return (*this);
}


ndword::operator dword () const noexcept
{
    return ntohl(m_val);
}

ndword& ndword::operator = (dword val) noexcept
{
    m_val = htonl(val);
    return (*this);
}


static int CvtSystemToUTF16(wchar& wChar, lpctstr pInp, int iSizeInBytes)
{
    // Convert a UTF8 encoded string to a single unicode char.
    // RETURN: The length used from input string. < iSizeInBytes

    // bytes bits representation
    // 1 7	0bbbbbbb
    // 2 11 110bbbbb 10bbbbbb
    // 3 16 1110bbbb 10bbbbbb 10bbbbbb
    // 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

    byte ch = (byte)*pInp;
    ASSERT(ch >= 0x80);	// needs special UTF8 decoding.

    int iBytes;
    int iStartBits;
    if ((ch & 0xe0) == 0x0c0) // 2 bytes
    {
        iBytes = 2;
        iStartBits = 5;
    }
    else if ((ch & 0xf0) == 0x0e0) // 3 bytes
    {
        iBytes = 3;
        iStartBits = 4;
    }
    else if ((ch & 0xf8) == 0x0f0) // 3 bytes
    {
        iBytes = 4;
        iStartBits = 3;
    }
    else
    {
        return -1;	// invalid format !
    }

    if (iBytes > iSizeInBytes)	// not big enough to hold it.
        return 0;

    wchar wCharTmp = ch & ((1 << iStartBits) - 1);
    int iInp = 1;
    for (; iInp < iBytes; iInp++)
    {
        ch = (byte)pInp[iInp];
        if ((ch & 0xc0) != 0x80)	// bad coding.
            return -1;
        wCharTmp <<= 6;
        wCharTmp |= ch & 0x3f;
    }

    wChar = wCharTmp;
    return iBytes;
}

static int CvtUTF16ToSystem(tchar* pOut, int iSizeOutBytes, wchar wChar)
{
    // Convert a single unicode char to system string.
    // RETURN: The length < iSizeOutBytes

    // bytes bits representation
    // 1 7	0bbbbbbb
    // 2 11 110bbbbb 10bbbbbb
    // 3 16 1110bbbb 10bbbbbb 10bbbbbb
    // 4 21 11110bbb 10bbbbbb 10bbbbbb 10bbbbbb

    ASSERT(wChar >= 0x80);	// needs special UTF8 encoding.

    int iBytes;
    int iStartBits;

    if (wChar < (1 << 11))
    {
        iBytes = 2;
        iStartBits = 5;
    }
    else if constexpr (sizeof(wchar) > 16u)
    {
        if (wChar < (1 << 16))
        {
            iBytes = 3;
            iStartBits = 4;
        }
    }
    else if constexpr (sizeof(wchar) > 21u)
    {
        if (wChar < (1 << 21))
        {
            iBytes = 4;
            iStartBits = 3;
        }

    }
    else
    {
        return -1;	// not valid UNICODE char.
    }

    if (iBytes > iSizeOutBytes)	// not big enough to hold it.
        return 0;

    int iOut = iBytes - 1;
    for (; iOut > 0; --iOut)
    {
        pOut[iOut] = 0x80 | (wChar & ((1 << 6) - 1));
        wChar >>= 6;
    }

    ASSERT(wChar < (1 << iStartBits));
    pOut[0] = static_cast<tchar>((0xfe << iStartBits) | wChar);

    return iBytes;
}

int CvtSystemToNETUTF16(nachar* pOut, int iSizeOutChars, lpctstr pInp, int iSizeInBytes)
{
    //
    // Convert the system default text format UTF8 to UNICODE
    // May be network byte order !
    // Add null.
    // ARGS:
    //   iSizeInBytes = size ofthe input string. -1 = null terminated.
    // RETURN:
    //  Number of wide chars. not including null.
    //

    ASSERT(pOut);
    ASSERT(pInp);
    if (iSizeOutChars <= 0)
        return -1;

    if (iSizeInBytes <= -1)
    {
        iSizeInBytes = (int)strlen(pInp);
    }
    if (iSizeInBytes <= 0)
    {
        pOut[0] = '\0';
        return 0;
    }

    --iSizeOutChars;

    int iOut = 0;

#ifdef _WIN32
    const OSVERSIONINFO* posInfo = Sphere_GetOSInfo();
    if (posInfo->dwPlatformId == VER_PLATFORM_WIN32_NT ||
        posInfo->dwMajorVersion > 4)
    {
        const int iOutTmp = MultiByteToWideChar(
            CP_UTF8,			// code page
            0,					// character-type options
            pInp,				// address of string to map
            iSizeInBytes,		// number of bytes in string
            reinterpret_cast<lpwstr>(pOut),  // address of wide-character buffer
            iSizeOutChars		// size of buffer
        );

        if (iOutTmp <= 0)
        {
            pOut[0] = '\0';
            return 0;
        }
        if (iOutTmp > iSizeOutChars)	// this should never happen !
        {
            pOut[0] = '\0';
            return 0;
        }

        // flip all the words to network order .
        for (; iOut < iOutTmp; ++iOut)
        {
            pOut[iOut] = *(reinterpret_cast<wchar*>(&(pOut[iOut])));
        }
    }
    else
#endif // _WIN32
    {
        // Win95 or Linux
        int iInp = 0;
        for (; iInp < iSizeInBytes; )
        {
            byte ch = (byte)pInp[iInp];
            if (ch == 0)
                break;

            if (iOut >= iSizeOutChars)
                break;

            if (ch >= 0x80)	// special UTF8 encoded char.
            {
                wchar wChar;
                int iInpTmp = CvtSystemToUTF16(wChar, pInp + iInp, iSizeInBytes - iInp);
                if (iInpTmp <= 0)
                {
                    break;
                }
                pOut[iOut] = wChar;
                iInp += iInpTmp;
            }
            else
            {
                pOut[iOut] = ch;
                ++iInp;
            }

            ++iOut;
        }
    }

    pOut[iOut] = '\0';
    return iOut;
}

int CvtNETUTF16ToSystem(tchar* pOut, int iSizeOutBytes, const nachar* pInp, int iSizeInChars)
{
    // ARGS:
    //  iSizeInBytes = space we have (included null char)
    // RETURN:
    //  Number of bytes. (not including null)
    // NOTE:
    //  This need not be a properly terminated string.

    if (iSizeInChars > iSizeOutBytes)	// iSizeOutBytes should always be bigger
        iSizeInChars = iSizeOutBytes;
    if (iSizeInChars <= 0)
    {
        pOut[0] = 0;
        return 0;
    }

    --iSizeOutBytes;

    int iOut = 0;
    int iInp = 0;

#ifdef _WIN32
    const OSVERSIONINFO* posInfo = Sphere_GetOSInfo();
    if ((posInfo->dwPlatformId == VER_PLATFORM_WIN32_NT) || (posInfo->dwMajorVersion > 4))
    {
        // Windows 98, 2000 or NT

        // Flip all from network order.
        wchar szBuffer[1024 * 6];
        for (; iInp < (int)ARRAY_COUNT(szBuffer) - 1 && iInp < iSizeInChars && pInp[iInp]; ++iInp)
        {
            szBuffer[iInp] = pInp[iInp];
        }
        szBuffer[iInp] = '\0';

        // Convert to proper UTF8
        iOut = WideCharToMultiByte(
            CP_UTF8,        // code page
            0,              // performance and mapping flags
            szBuffer,       // address of wide-character string
            iInp,           // number of characters in string
            pOut,           // address of buffer for new string
            iSizeOutBytes,  // size of buffer in bytes
            nullptr,        // address of default for unmappable characters
            nullptr         // address of flag set when default char. used
        );
        if (iOut < 0)
        {
            pOut[0] = 0;	// make sure it's null terminated
            return 0;
        }
    }
    else
#endif // _WIN32
    {
        // Win95 or linux = just assume its really ASCII
        for (; iInp < iSizeInChars; ++iInp)
        {
            // Flip all from network order.
            wchar wChar = pInp[iInp];
            if (!wChar)
                break;

            if (iOut >= iSizeOutBytes)
                break;
            if (wChar >= 0x80)	// needs special UTF8 encoding.
            {
                int iOutTmp = CvtUTF16ToSystem(pOut + iOut, iSizeOutBytes - iOut, wChar);
                if (iOutTmp <= 0)
                    break;
                iOut += iOutTmp;
            }
            else
            {
                pOut[iOut] = static_cast<tchar>(wChar);
                ++iOut;
            }
        }
    }

    pOut[iOut] = 0;	// make sure it's null terminated
    return(iOut);
}
