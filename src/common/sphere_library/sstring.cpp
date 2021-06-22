#include "sstring.h"
#include "../../sphere/threads.h"
#include "../../common/CLog.h"
#include "../../sphere/ProfileTask.h"
#include "../CExpression.h"
#include "../CScript.h"


#if defined(_MSC_VER)
    #include <codeanalysis/warnings.h>
    #pragma warning( push )
    #pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "../regex/deelx.h"
#ifdef _MSC_VER
    #pragma warning( pop )
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif


// String utilities: Converters

#ifndef _WIN32
void Str_Reverse(char* string)
{
    char* pEnd = string;
    char temp;
    while (*pEnd)
        ++pEnd;
    --pEnd;
    while (string < pEnd)
    {
        temp = *pEnd;
        *pEnd-- = *string;
        *string++ = temp;
    }
}
#endif

int Str_ToI (lpctstr ptcStr, int base) noexcept
{
    const auto e = errno;
    const auto ret = int(std::strtol(ptcStr, nullptr, base));
    errno = e;
    return ret;
}

uint Str_ToUI(lpctstr ptcStr, int base) noexcept
{
    const auto e = errno;
    const auto ret = uint(std::strtoul(ptcStr, nullptr, base));
    errno = e;
    return ret;
}

llong Str_ToLL(lpctstr ptcStr, int base) noexcept
{
    const auto e = errno;
    const auto ret = std::strtoll(ptcStr, nullptr, base);
    errno = e;
    return ret;
}

ullong Str_ToULL(lpctstr ptcStr, int base) noexcept
{
    const auto e = errno;
    const auto ret = std::strtoull(ptcStr, nullptr, base);
    errno = e;
    return ret;
}

#define STR_FROM_SET_ZEROSTR \
    if (hex)    { buf[0] = '0'; buf[1] = '0'; buf[2] = '\0'; } \
    else        { buf[0] = '0'; buf[1] = '\0'; }

tchar* Str_FromI_Fast(int val, tchar* buf, size_t buf_length, uint base) noexcept
{
    if (!buf || !buf_length) {
        return nullptr;
    }

    const bool hex = (base == 16);

    if (!val || !base)
    {
        STR_FROM_SET_ZEROSTR;
        return buf;
    }
    static constexpr tchar chars[] = "0123456789abcdef";

    const bool sign = (val < 0);
    /*if (sign && !hex) {
        if (hex)
            val = INT_MAX - val;
        else
            val = -val;
    }
    */
    if (sign && !hex) {
        val = -val;
    }

    buf[--buf_length] = '\0';
    do
    {
        buf[--buf_length] = chars[val % base];
        val /= base;
    } while (val);

    if (hex) {
        buf[--buf_length] = '0';
    }
    else if (sign) {
        buf[--buf_length] = '-';
    }
    return &buf[buf_length];
}

tchar* Str_FromUI_Fast(uint val, tchar* buf, size_t buf_length, uint base) noexcept
{
    if (!buf || !buf_length) {
        return nullptr;
    }

    const bool hex = (base == 16);

    if (!val || !base)
    {
        STR_FROM_SET_ZEROSTR;
        return buf;
    }
    static constexpr tchar chars[] = "0123456789abcdef";

    buf[--buf_length] = '\0';
    do
    {
        buf[--buf_length] = chars[val % base];
        val /= base;
    } while (val);

    if (base == 16) {
        buf[--buf_length] = '0';
    }
    return &buf[buf_length];
}

tchar* Str_FromLL_Fast (llong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    if (!buf || !buf_length) {
        return nullptr;
    }

    const bool hex = (base == 16);

    if (!val || !base)
    {
        STR_FROM_SET_ZEROSTR;
        return buf;
    }
    static constexpr tchar chars[] = "0123456789abcdef";

    const bool sign = (val < 0);
    /*if (sign && !hex) {
        if (hex)
            val = LLONG_MAX - val;
        else
            val = -val;
    }
    */
    if (sign && !hex) {
        val = -val;
    }

    buf[--buf_length] = '\0';
    do
    {
        buf[--buf_length] = chars[val % base];
        val /= base;
    } while (val);

    if (hex) {
        buf[--buf_length] = '0';
    }
    else if (sign) {
        buf[--buf_length] = '-';
    }
    return &buf[buf_length];
}

tchar* Str_FromULL_Fast (ullong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    if (!buf || !buf_length) {
        return nullptr;
    }

    const bool hex = (base == 16);

    if (!val || !base)
    {
        STR_FROM_SET_ZEROSTR;
        return buf;
    }
    static constexpr tchar chars[] = "0123456789abcdef";

    buf[--buf_length] = '\0';
    do
    {
        buf[--buf_length] = chars[val % base];
        val /= base;
    } while (val);

    if (hex) {
        buf[--buf_length] = '0';
    }
    return &buf[buf_length];
}

#undef STR_FROM_SET_ZEROSTR

void Str_FromI(int val, tchar* buf, size_t buf_length, uint base) noexcept
{
    tchar* modified_buf = Str_FromI_Fast(val, buf, buf_length, base);
    const size_t offset = size_t(modified_buf - buf);
    if (offset > 0) {
        memmove(buf, modified_buf, buf_length - offset);
    }
}

void Str_FromUI(uint val, tchar* buf, size_t buf_length, uint base) noexcept
{
    tchar* modified_buf = Str_FromUI_Fast(val, buf, buf_length, base);
    const size_t offset = size_t(modified_buf - buf);
    if (offset > 0) {
        memmove(buf, modified_buf, buf_length - offset);
    }
}

void Str_FromLL(llong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    tchar* modified_buf = Str_FromLL_Fast(val, buf, buf_length, base);
    const size_t offset = size_t(modified_buf - buf);
    if (offset > 0) {
        memmove(buf, modified_buf, buf_length - offset);
    }
}

void Str_FromULL(ullong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    tchar* modified_buf = Str_FromULL_Fast(val, buf, buf_length, base);
    const size_t offset = size_t(modified_buf - buf);
    if (offset > 0) {
        memmove(buf, modified_buf, buf_length - offset);
    }
}


size_t FindStrWord( lpctstr pTextSearch, lpctstr pszKeyWord )
{
    // Find any of the pszKeyWord in the pTextSearch string.
    // Make sure we look for starts of words.

    size_t j = 0;
    for ( size_t i = 0; ; ++i )
    {
        if ( pszKeyWord[j] == '\0' || pszKeyWord[j] == ',')
        {
            if ( pTextSearch[i]== '\0' || ISWHITESPACE(pTextSearch[i]))
                return( i );
            j = 0;
        }
        if ( pTextSearch[i] == '\0' )
        {
            pszKeyWord = strchr(pszKeyWord, ',');
            if (pszKeyWord)
            {
                ++pszKeyWord;
                i = 0;
                j = 0;
            }
            else
                return 0;
        }
        if ( j == 0 && i > 0 )
        {
            if ( IsAlpha( pTextSearch[i-1] ))	// not start of word ?
                continue;
        }
        if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
            ++j;
        else
            j = 0;
    }
}

int Str_CmpHeadI(lpctstr ptcFind, lpctstr ptcHere)
{
    for (uint i = 0; ; ++i)
    {
		//	we should always use same case as in other places. since strcmpi lowers,
        //	we should lower here as well. if strcmpi changes, we have to change it here as well
        const tchar ch1 = static_cast<tchar>(tolower(ptcFind[i]));
        const tchar ch2 = static_cast<tchar>(tolower(ptcHere[i]));
        if (ch2 == 0)
        {
            if ( (!isalnum(ch1)) && (ch1 != '_') )
                return 0;
            return (ch1 - ch2);
        }
        if (ch1 != ch2)
            return (ch1 - ch2);
    }
}

static inline int Str_CmpHeadI_Table(lpctstr ptcFind, lpctstr ptcTable) noexcept
{
    for (uint i = 0; ; ++i)
    {
        const tchar ch1 = static_cast<tchar>(toupper(ptcFind[i]));
        const tchar ch2 = ptcTable[i];
        ASSERT(ch2 == toupper(ch2));    // for better performance, in the table all the names have to be LOWERCASE!
        if (ch2 == 0)
        {
            if ( (!isalnum(ch1)) && (ch1 != '_') )
                return 0;
            return (ch1 - ch2);
        }
        if (ch1 != ch2)
            return (ch1 - ch2);
    }
}

// String utilities: Modifiers

// strcpy doesn't have an argument to truncate the copy to the buffer length;
// strncpy doesn't null-terminate if it truncates the copy, and if uiMaxlen is > than the source string length, the remaining space is filled with '\0'
size_t Str_CopyLimit(tchar * pDst, lpctstr pSrc, size_t uiMaxSize)
{
    if (uiMaxSize == 0)
    {
        return 0;
    }
    if (pSrc[0] == '\0')
    {
        pDst[0] = '\0';
        return 0;
    }

    size_t qty = 0; // how much bytes do i have to copy? (1 based)
    do
    {
        if (pSrc[qty++] == '\0')
        {
            break;
        }
    } while (qty < uiMaxSize);
    memcpy(pDst, pSrc, qty);
    return qty; // bytes copied in pDst string (CAN count the string terminator)
}

size_t Str_CopyLimitNull(tchar * pDst, lpctstr pSrc, size_t uiMaxSize)
{
    if (uiMaxSize == 0)
    {
        return 0;
    }
    if (pSrc[0] == '\0')
    {
        pDst[0] = '\0';
        return 0;
    }
    
    size_t qty = 0; // how much bytes do i have to copy? (1 based)
    do
    {
        if (pSrc[qty++] == '\0')
        {
            break;
        }
    } while (qty < uiMaxSize);
    memcpy(pDst, pSrc, qty);
    pDst[qty - 1] = '\0'; // null terminate the string
    return qty - 1; // bytes copied in pDst string (not counting the string terminator)
}

size_t Str_CopyLen(tchar * pDst, lpctstr pSrc)
{
    strcpy(pDst, pSrc);
    // Remember: strlen returns the number of bytes before the terminator (if the string contains utf-8 characters,
    //  strlen will return a value != than the actual number of characters, because some of them are codified with more than a byte).
    return strlen(pDst);
}

// the number of characters in a multibyte string is the sum of mblen()'s
// note: the simpler approach is std::mbstowcs(NULL, s.c_str(), s.size())
/*
size_t strlen_mb(const char* ptr)  
{
    // From: https://en.cppreference.com/w/c/string/multibyte/mblen

    // ensure that at some point we have called setlocale:
    //--    // allow mblen() to work with UTF-8 multibyte encoding
    //--    std::setlocale(LC_ALL, "en_US.utf8");
    
    size_t result = 0;
    const char* end = ptr + strlen(ptr);
    mblen(nullptr, 0); // reset the conversion state
    while(ptr < end) {
        int next = mblen(ptr, end - ptr);
        if(next == -1) {
            throw std::runtime_error("strlen_mb(): conversion error");
            break;
        }
        ptr += next;
        ++result;
    }
    return result;
}
*/

size_t Str_LengthUTF8(const char* strInUTF8MB)
{
    size_t len; // number of characters in the string
#ifdef _MSC_VER
    mbstowcs_s(&len, nullptr, 0, strInUTF8MB, 0); // includes null terminator
    len -= 1;
#else
    len = mbstowcs(nullptr, strInUTF8MB, 0); // not including null terminator
#endif
    return len;
}

/*
* Appends src to string dst of size siz (unlike strncat, siz is the
* full size of dst, not space left). At most siz-1 characters
* will be copied. Always NULL terminates (unless siz <= strlen(dst)).
* Returns strlen(src) + MIN(siz, strlen(initial dst)). Count does not include '\0'
* If retval >= siz, truncation occurs.
*/
// Adapted from: OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38
size_t Str_ConcatLimitNull(tchar *dst, const tchar *src, size_t siz)
{
    tchar *d = dst;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while ((n-- != 0) && (*d != '\0'))
    {
        ++d;
    }
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
    {
        return (dlen + strlen(src));
    }

    const tchar *s = src;
    while (*s != '\0')
    {
        if (n != 1)
        {
            *d++ = *s;
            --n;

        }
        ++s;
    }
    *d = '\0';

    return (dlen + (s - src));	/* count does not include '\0' */
}

lpctstr Str_GetArticleAndSpace(lpctstr pszWord)
{
    // NOTE: This is wrong many times.
    //  ie. some words need no article (plurals) : boots.

    // This function shall NOT be called if OF_NoPrefix is set!

    if (pszWord)
    {
        static constexpr tchar sm_Vowels[] = { 'A', 'E', 'I', 'O', 'U' };
        tchar chName = static_cast<tchar>(toupper(pszWord[0]));
        for (uint x = 0; x < CountOf(sm_Vowels); ++x)
        {
            if (chName == sm_Vowels[x])
                return "an ";
        }
    }
    return "a ";
}

int Str_GetBare(tchar * pszOut, lpctstr pszInp, int iMaxOutSize, lpctstr pszStrip)
{
    // That the client can deal with. Basic punctuation and alpha and numbers.
    // RETURN: Output length.

    if (!pszStrip)
        pszStrip = "{|}~";	// client can't print these.

    //GETNONWHITESPACE( pszInp );	// kill leading white space.

    int j = 0;
    for (int i = 0; ; ++i)
    {
        tchar ch = pszInp[i];
        if (ch)
        {
            if (ch < ' ' || ch >= 127)
                continue;	// Special format chars.

            int k = 0;
            while (pszStrip[k] && pszStrip[k] != ch)
                ++k;

            if (pszStrip[k])
                continue;

            if (j >= iMaxOutSize - 1)
                ch = '\0';
        }
        pszOut[j++] = ch;
        if (ch == 0)
            break;
    }
    return (j - 1);
}

tchar * Str_MakeFiltered(tchar * pStr) noexcept
{
    int len = (int)strlen(pStr);
    for (int i = 0; len; ++i, --len)
    {
        if (pStr[i] == '\\')
        {
            switch (pStr[i + 1])
            {
                case 'b': pStr[i] = '\b'; break;
                case 'n': pStr[i] = '\n'; break;
                case 'r': pStr[i] = '\r'; break;
                case 't': pStr[i] = '\t'; break;
                case '\\': pStr[i] = '\\'; break;
            }
            --len;
            memmove(pStr + i + 1, pStr + i + 2, len);
        }
    }
    return pStr;
}

void Str_MakeUnFiltered(tchar * pStrOut, lpctstr pStrIn, int iSizeMax) noexcept
{
    int len = (int)strlen(pStrIn);
    int iIn = 0;
    int iOut = 0;
    for (; iOut < iSizeMax && iIn <= len; ++iIn, ++iOut)
    {
        tchar ch = pStrIn[iIn];
        switch (ch)
        {
            case '\b': ch = 'b'; break;
            case '\n': ch = 'n'; break;
            case '\r': ch = 'r'; break;
            case '\t': ch = 't'; break;
            case '\\': ch = '\\'; break;
            default:
                pStrOut[iOut] = ch;
                continue;
        }

        pStrOut[iOut++] = '\\';
        pStrOut[iOut] = ch;
    }
}

tchar * Str_GetUnQuoted(tchar * pStr) noexcept
{
    // TODO: WARNING! Possible Memory Leak here!
    GETNONWHITESPACE(pStr);
    if (*pStr != '"')
    {
        Str_TrimEndWhitespace(pStr, int(strlen(pStr)));
        return pStr;
    }

    ++pStr;
    // search for last quote symbol starting from the end
    tchar * pEnd = pStr + strlen(pStr) - 1;
    for (; pEnd >= pStr; --pEnd )
    {
        if ( *pEnd == '"' )
        {
            *pEnd = '\0';
            break;
        }
    }

    Str_TrimEndWhitespace(pStr, int(pEnd - pStr));
    return pStr;
}

int Str_TrimEndWhitespace(tchar * pStr, int len) noexcept
{
    while (len > 0)
    {
        --len;
        if (pStr[len] < 0 || !ISWHITESPACE(pStr[len]))
        {
            ++len;
            break;
        }
    }
    pStr[len] = '\0';
    return len;
}

tchar * Str_TrimWhitespace(tchar * pStr) noexcept
{
    // TODO: WARNING! Possible Memory Leak here!
    GETNONWHITESPACE(pStr);
    Str_TrimEndWhitespace(pStr, (int)strlen(pStr));
    return pStr;
}

void Str_EatEndWhitespace(const tchar* const pStrBegin, tchar*& pStrEnd) noexcept
{
    if (pStrBegin == pStrEnd)
        return;

    tchar* ptcPrev = pStrEnd - 1;
    while ((ptcPrev != pStrBegin) && ISWHITESPACE(*pStrEnd))
    {
        if (*ptcPrev == '\0')
            return;

        --pStrEnd;
        ptcPrev = pStrEnd - 1;
    }
}

void Str_SkipEnclosedAngularBrackets(tchar*& ptcLine) noexcept
{
    // Move past a < > statement. It can have ( ) inside, if it happens, ignore < > characters inside ().
    bool fOpenedOneAngular = false;
    int iOpenAngular = 0, iOpenCurly = 0;
    tchar* ptcTest = ptcLine;
    while (const tchar ch = *ptcTest)
    {
        if (ISWHITESPACE(ch))
            ;
        else if (ch == '(')
            ++iOpenCurly;
        else if (ch == ')')
            --iOpenCurly;
        else if (iOpenCurly == 0)
        {
            if (ch == '<')
            {
                fOpenedOneAngular = true;
                ++iOpenAngular;
            }
            else if (ch == '>')
            {
                --iOpenAngular;
                if (fOpenedOneAngular && !iOpenAngular)
                {
                    ptcLine = ptcTest + 1;
                    return;
                }
            }
        }
        ++ptcTest;
    }
}


// String utilities: String operations

int FindTable(const lpctstr ptcFind, lpctstr const * pptcTable, int iCount) noexcept
{
    // A non-sorted table.
    for (int i = 0; i < iCount; ++i)
    {
        if (!strcmpi(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableSorted(const lpctstr ptcFind, lpctstr const * pptcTable, int iCount) noexcept
{
    // Do a binary search (un-cased) on a sorted table.
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1;
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const int iCompare = strcmpi(ptcFind, pptcTable[i]);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;

    /*
    // Alternative implementation. Logarithmic time, but better use of CPU instruction pipelining and branch prediction, at the cost of more comparations.
    // It's worth running some benchmarks before switching to this.
    lpctstr const* base = pptcTable;
    if (iCount > 1)
    {
        do
        {
            const int half = iCount >> 1;
            base = (strcmpi(base[half], ptcFind) < 0) ? &base[half] : base;
            iCount -= half;
        } while (iCount > 1);
        base = (strcmpi(*base, ptcFind) < 0) ? &base[1] : base;
    }
    return (0 == strcmpi(*base, ptcFind)) ? int(base - pptcTable) : -1;
    */
}

int FindTableHead(const lpctstr ptcFind, lpctstr const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE
{
    for (int i = 0; i < iCount; ++i)
    {
        if (!Str_CmpHeadI_Table(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableHeadSorted(const lpctstr ptcFind, lpctstr const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1;
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const int iCompare = Str_CmpHeadI_Table(ptcFind, pptcTable[i]);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;

    /*
    // Alternative implementation. Logarithmic time, but better use of CPU instruction pipelining and branch prediction, at the cost of more comparations.
    // It's worth running some benchmarks before switching to this.
    lpctstr const* base = pptcTable;
    if (iCount > 1)
    {
        do
        {
            const int half = iCount >> 1;
            base = (Str_CmpHeadI_Table(base[half], ptcFind) < 0) ? &base[half] : base;
            iCount -= half;
        } while (iCount > 1);
        base = (Str_CmpHeadI_Table(*base, ptcFind) < 0) ? &base[1] : base;
    }
    return (0 == Str_CmpHeadI_Table(*base, ptcFind)) ? int(base - pptcTable) : -1;
    */
}

int FindCAssocRegTableHeadSorted(const lpctstr pszFind, lpctstr const* ppszTable, int iCount, size_t uiElemSize) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found
    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1;
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const lpctstr pszName = *(reinterpret_cast<lpctstr const*>(reinterpret_cast<const byte*>(ppszTable) + (i * uiElemSize)));
        const int iCompare = Str_CmpHeadI_Table(pszFind, pszName);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;
}

bool Str_Check(lpctstr pszIn)
{
    if (pszIn == nullptr)
        return true;

    lpctstr p = pszIn;
    while (*p != '\0' && (*p != 0x0A) && (*p != 0x0D))
        ++p;

    return (*p != '\0');
}

bool Str_CheckName(lpctstr pszIn)
{
    if (pszIn == nullptr)
        return true;

    lpctstr p = pszIn;
    while (*p != '\0' &&
        (
        ((*p >= 'A') && (*p <= 'Z')) ||
            ((*p >= 'a') && (*p <= 'z')) ||
            ((*p >= '0') && (*p <= '9')) ||
            ((*p == ' ') || (*p == '\'') || (*p == '-') || (*p == '.'))
            ))
        ++p;

    return (*p != '\0');
}

int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset)
{
    if (offset < 0)
        return -1;

    int len = (int)strlen(pStr1);
    if (offset >= len)
        return -1;

    int slen = (int)strlen(pStr2);
    if (slen > len)
        return -1;

    tchar firstChar = pStr2[0];

    for (int i = offset; i < len; ++i)
    {
        tchar c = pStr1[i];
        if (c == firstChar)
        {
            int rem = len - i;
            if (rem >= slen)
            {
                int j = i;
                int k = 0;
                bool found = true;
                while (k < slen)
                {
                    if (pStr1[j] != pStr2[k])
                    {
                        found = false;
                        break;
                    }
                    ++j; ++k;
                }
                if (found)
                    return i;
            }
        }
    }

    return -1;
}

static MATCH_TYPE Str_Match_After_Star(lpctstr pPattern, lpctstr pText)
{
    // pass over existing ? and * in pattern
    for (; *pPattern == '?' || *pPattern == '*'; ++pPattern)
    {
        // take one char for each ? and +
        if (*pPattern == '?' &&
            !*pText++)		// if end of text then no match
            return MATCH_ABORT;
    }

    // if end of pattern we have matched regardless of text left
    if (!*pPattern)
        return MATCH_VALID;

    // get the next character to match which must be a literal or '['
    tchar nextp = static_cast<tchar>(tolower(*pPattern));
    MATCH_TYPE match = MATCH_INVALID;

    // Continue until we run out of text or definite result seen
    do
    {
        // a precondition for matching is that the next character
        // in the pattern match the next character in the text or that
        // the next pattern char is the beginning of a range.  Increment
        // text pointer as we go here
        if (nextp == tolower(*pText) || nextp == '[')
        {
            match = Str_Match(pPattern, pText);
            if (match == MATCH_VALID)
                break;
        }

        // if the end of text is reached then no match
        if (!*pText++)
            return MATCH_ABORT;

    } while (
        match != MATCH_ABORT &&
        match != MATCH_PATTERN);

    return match;	// return result
}

MATCH_TYPE Str_Match(lpctstr pPattern, lpctstr pText)
{
    // case independant

    tchar range_start;
    tchar range_end;  // start and end in range

    for (; *pPattern; ++pPattern, ++pText)
    {
        // if this is the end of the text then this is the end of the match
        if (!*pText)
            return (*pPattern == '*' && *++pPattern == '\0') ? MATCH_VALID : MATCH_ABORT;

        // determine and react to pattern type
        switch (*pPattern)
        {
            // single any character match
            case '?':
                break;
                // multiple any character match
            case '*':
                return Str_Match_After_Star(pPattern, pText);
                // [..] construct, single member/exclusion character match
            case '[':
            {
                // move to beginning of range
                ++pPattern;
                // check if this is a member match or exclusion match
                bool fInvert = false;             // is this [..] or [!..]
                if (*pPattern == '!' || *pPattern == '^')
                {
                    fInvert = true;
                    ++pPattern;
                }
                // if closing bracket here or at range start then we have a
                // malformed pattern
                if (*pPattern == ']')
                    return MATCH_PATTERN;

                bool fMemberMatch = false;       // have I matched the [..] construct?
                for (;;)
                {
                    // if end of construct then fLoop is done
                    if (*pPattern == ']')
                        break;

                    // matching a '!', '^', '-', '\' or a ']'
                    if (*pPattern == '\\')
                        range_start = range_end = static_cast<tchar>(tolower(*++pPattern));
                    else
                        range_start = range_end = static_cast<tchar>(tolower(*pPattern));

                    // if end of pattern then bad pattern (Missing ']')
                    if (!*pPattern)
                        return MATCH_PATTERN;

                    // check for range bar
                    if (*++pPattern == '-')
                    {
                        // get the range end
                        range_end = static_cast<tchar>(tolower(*++pPattern));
                        // if end of pattern or construct then bad pattern
                        if (range_end == '\0' || range_end == ']')
                            return MATCH_PATTERN;
                        // special character range end
                        if (range_end == '\\')
                        {
                            range_end = static_cast<tchar>(tolower(*++pPattern));
                            // if end of text then we have a bad pattern
                            if (!range_end)
                                return MATCH_PATTERN;
                        }
                        // move just beyond this range
                        ++pPattern;
                    }

                    // if the text character is in range then match found.
                    // make sure the range letters have the proper
                    // relationship to one another before comparison
                    tchar chText = static_cast<tchar>(tolower(*pText));
                    if (range_start < range_end)
                    {
                        if (chText >= range_start && chText <= range_end)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                    else
                    {
                        if (chText >= range_end && chText <= range_start)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                }	// while

                    // if there was a match in an exclusion set then no match
                    // if there was no match in a member set then no match
                if ((fInvert && fMemberMatch) ||
                    !(fInvert || fMemberMatch))
                    return MATCH_RANGE;

                // if this is not an exclusion then skip the rest of the [...]
                //  construct that already matched.
                if (fMemberMatch)
                {
                    while (*pPattern != ']')
                    {
                        // bad pattern (Missing ']')
                        if (!*pPattern)
                            return MATCH_PATTERN;
                        // skip exact match
                        if (*pPattern == '\\')
                        {
                            ++pPattern;
                            // if end of text then we have a bad pattern
                            if (!*pPattern)
                                return MATCH_PATTERN;
                        }
                        // move to next pattern char
                        ++pPattern;
                    }
                }
            }
            break;

            // must match this character (case independant) ?exactly
            default:
                if (tolower(*pPattern) != tolower(*pText))
                    return MATCH_LITERAL;
        }
    }
    // if end of text not reached then the pattern fails
    if (*pText)
        return MATCH_END;
    else
        return MATCH_VALID;
}

#ifdef _MSC_VER
    // /GL + /LTCG flags inline in linking phase this function, but probably in a wrong way, so that
    // something gets corrupted on the memory and an exception is generated later
    #pragma auto_inline(off)
#endif
bool Str_Parse(tchar * pLine, tchar ** ppArg, lpctstr pszSep)
{
    // Parse a list of args. Just get the next arg.
    // similar to strtok()
    // RETURN: true = the second arg is valid.

    if (pszSep == nullptr)	// default sep.
        pszSep = "=, \t";

    // skip leading white space.
    GETNONWHITESPACE(pLine);

    tchar ch;
    // variables used to track opened/closed quotes and brackets
    bool fQuotes = false;
    int iCurly, iSquare, iRound, iAngle;
    iCurly = iSquare = iRound = iAngle = 0;

    // ignore opened/closed brackets if that type of bracket is also a separator
    bool fSepHasCurly, fSepHasSquare, fSepHasRound, fSepHasAngle;
    fSepHasCurly = fSepHasSquare = fSepHasRound = fSepHasAngle = false;
    for (uint j = 0; pszSep[j] != '\0'; ++j)		// loop through each separator
    {
        const tchar & sep = pszSep[j];
        if (sep == '{' || sep == '}')
            fSepHasCurly = true;
        else if (sep == '[' || sep == ']')
            fSepHasSquare = true;
        else if (sep == '(' || sep == ')')
            fSepHasRound = true;
        else if (sep == '<' || sep == '>')
            fSepHasAngle = true;
    }

    for (; ; ++pLine)
    {
        ch = *pLine;
        if (ch == '"')	// quoted argument
        {
            fQuotes = !fQuotes;
            continue;
        }
        if (ch == '\0')	// no more args i guess.
        {
            if (ppArg != nullptr)
                *ppArg = pLine;
            return false;
        }

        if (!fQuotes)
        {
            // We are not inside a quote, so let's check if the char is a bracket or a separator

            // Here we track opened and closed brackets.
            //	we'll ignore items inside brackets, if the bracket isn't a separator in the list
            if (ch == '{') {
                if (!fSepHasCurly) {
                    if (!iSquare && !iRound && !iAngle)
                        ++iCurly;
                }
            }
            else if (ch == '[') {
                if (!fSepHasSquare) {
                    if (!iCurly && !iRound && !iAngle)
                        ++iSquare;
                }
            }
            else if (ch == '(') {
                if (!fSepHasRound) {
                    if (!iCurly && !iSquare && !iAngle)
                        ++iRound;
                }
            }
            else if (ch == '<') {
                if (!fSepHasAngle) {
                    if (!iCurly && !iSquare && !iRound)
                        ++iAngle;
                }
            }
            else if (ch == '}') {
                if (!fSepHasCurly) {
                    if (iCurly)
                        --iCurly;
                }
            }
            else if (ch == ']') {
                if (!fSepHasSquare) {
                    if (iSquare)
                        --iSquare;
                }
            }
            else if (ch == ')') {
                if (!fSepHasRound) {
                    if (iRound)
                        --iRound;
                }
            }
            else if (ch == '>') {
                if (!fSepHasAngle) {
                    if (iAngle)
                        --iAngle;
                }
            }

            // separate the string when i encounter a separator, but only if at this point of the string we aren't inside an argument
            // enclosed by brackets. but, if one of the separators is a bracket, don't care if we are inside or outside, separate anyways.

            //	don't turn this if into an else if!
            //	We can choose as a separator also one of {[(< >)]} and they have to be treated as such!
            if ((iCurly<=0) && (iSquare<=0) && (iRound<=0))
            {
                if (strchr(pszSep, ch))		// if ch is a separator
                    break;
            }
        }	// end of the quotes if clause

    }	// end of the for loop

    if (*pLine == '\0')
        return false;

    *pLine = '\0';
    ++pLine;
    if (IsSpace(ch))	// space separators might have other seps as well ?
    {
        GETNONWHITESPACE(pLine);
        ch = *pLine;
        if (ch && strchr(pszSep, ch))
            ++pLine;
    }

    // skip trailing white space on args as well.
    if (ppArg != nullptr)
        *ppArg = Str_TrimWhitespace(pLine);

    if (iCurly || iSquare || iRound || fQuotes)
    {
        //g_Log.EventError("Not every bracket or quote was closed.\n");
        return false;
    }

    return true;
}
#ifdef _MSC_VER
    #pragma auto_inline(on)
#endif

int Str_ParseCmds(tchar * pszCmdLine, tchar ** ppCmd, int iMax, lpctstr pszSep)
{
    ASSERT(iMax > 1);
    int iQty = 0;
    GETNONWHITESPACE(pszCmdLine);

    if (pszCmdLine[0] != '\0')
    {
        ppCmd[0] = pszCmdLine;
        ++iQty;
        while (Str_Parse(ppCmd[iQty - 1], &(ppCmd[iQty]), pszSep))
        {
            if (++iQty >= iMax)
                break;
        }
    }
    for (int j = iQty; j < iMax; ++j)
        ppCmd[j] = nullptr;	// terminate if possible.
    return iQty;
}

int Str_ParseCmds(tchar * pszCmdLine, int64 * piCmd, int iMax, lpctstr pszSep)
{
    tchar * ppTmp[256];
    if (iMax > (int)CountOf(ppTmp))
        iMax = (int)CountOf(ppTmp);

    int iQty = Str_ParseCmds(pszCmdLine, ppTmp, iMax, pszSep);
    int i;
    for (i = 0; i < iQty; ++i)
        piCmd[i] = Exp_GetVal(ppTmp[i]);
    for (; i < iMax; ++i)
        piCmd[i] = 0;

    return iQty;
}

//I added this to parse commands by checking inline quotes directly.
//I tested it on every type of things but this is still experimental and being using under STRTOKEN.
//xwerswoodx
bool Str_ParseAdv(tchar * pLine, tchar ** ppArg, lpctstr pszSep)
{
    // Parse a list of args. Just get the next arg.
    // similar to strtok()
    // RETURN: true = the second arg is valid.

    if (pszSep == nullptr)	// default sep.
        pszSep = "=, \t";

    // skip leading white space.
    GETNONWHITESPACE(pLine);

    tchar ch, chNext;
    // variables used to track opened/closed quotes and brackets
    bool fQuotes = false;
    int iQuotes = 0;
    int iCurly, iSquare, iRound, iAngle;
    iCurly = iSquare = iRound = iAngle = 0;

    // ignore opened/closed brackets if that type of bracket is also a separator
    bool fSepHasCurly, fSepHasSquare, fSepHasRound, fSepHasAngle;
    fSepHasCurly = fSepHasSquare = fSepHasRound = fSepHasAngle = false;
    for (uint j = 0; pszSep[j] != '\0'; ++j)		// loop through each separator
    {
        const tchar & sep = pszSep[j];
        if (sep == '{' || sep == '}')
            fSepHasCurly = true;
        else if (sep == '[' || sep == ']')
            fSepHasSquare = true;
        else if (sep == '(' || sep == ')')
            fSepHasRound = true;
        else if (sep == '<' || sep == '>')
            fSepHasAngle = true;
    }
    
    for (; ; ++pLine)
    {
        tchar * pLineNext = pLine;
        ++pLineNext;
        ch = *pLine;
        chNext = *pLineNext;
        if ((ch == '"') || (ch == '\''))
        {
            if (!fQuotes) //Has first quote?
            {
                fQuotes = true;
            }
            else if (fQuotes) //We already has quote? Check for inner quotes...
            {
                while ((chNext == '"') || (chNext == '\''))
                {
                    ++pLineNext;
                    chNext = *pLineNext;
                }
                    
                if ((chNext == '\0') || (chNext == ',') || (chNext == ' ') || (chNext == '\''))
                    --iQuotes;
                else
                    ++iQuotes;
                    
                if (iQuotes < 0)
                {
                    iQuotes = 0;
                    fQuotes = false;
                }
            }
        }
        else if (ch == '\0')
        {
            if (ppArg != nullptr)
                *ppArg = pLine;
            return false;
        }
        else if (!fQuotes)
        {
            // We are not inside a quote, so let's check if the char is a bracket or a separator

            // Here we track opened and closed brackets.
            //	we'll ignore items inside brackets, if the bracket isn't a separator in the list
            if (ch == '{') {
                if (!fSepHasCurly) {
                    if (!iSquare && !iRound && !iAngle)
                        ++iCurly;
                }
            }
            else if (ch == '[') {
                if (!fSepHasSquare) {
                    if (!iCurly && !iRound && !iAngle)
                        ++iSquare;
                }
            }
            else if (ch == '(') {
                if (!fSepHasRound) {
                    if (!iCurly && !iSquare && !iAngle)
                        ++iRound;
                }
            }
            else if (ch == '<') {
                if (!fSepHasAngle) {
                    if (!iCurly && !iSquare && !iRound)
                        ++iAngle;
                }
            }
            else if (ch == '}') {
                if (!fSepHasCurly) {
                    if (iCurly)
                        --iCurly;
                }
            }
            else if (ch == ']') {
                if (!fSepHasSquare) {
                    if (iSquare)
                        --iSquare;
                }
            }
            else if (ch == ')') {
                if (!fSepHasRound) {
                    if (iRound)
                        --iRound;
                }
            }
            else if (ch == '>') {
                if (!fSepHasAngle) {
                    if (iAngle)
                        --iAngle;
                }
            }
            
            // separate the string when i encounter a separator, but only if at this point of the string we aren't inside an argument
            // enclosed by brackets. but, if one of the separators is a bracket, don't care if we are inside or outside, separate anyways.

            //	don't turn this if into an else if!
            //	We can choose as a separator also one of {[(< >)]} and they have to be treated as such!
            if ((iCurly<=0) && (iSquare<=0) && (iRound<=0))
            {
                if (strchr(pszSep, ch))		// if ch is a separator
                    break;
            }
        }
    }
    if (*pLine == '\0')
        return false;

    *pLine = '\0';
    ++pLine;
    if (IsSpace(ch))	// space separators might have other seps as well ?
    {
        GETNONWHITESPACE(pLine);
        ch = *pLine;
        if (ch && strchr(pszSep, ch))
            ++pLine;
    }

    // skip trailing white space on args as well.
    if (ppArg != nullptr)
        *ppArg = Str_TrimWhitespace(pLine);

    if (iCurly || iSquare || iRound || fQuotes)
    {
        //g_Log.EventError("Not every bracket or quote was closed.\n");
        return false;
    }

    return true;
}

int Str_ParseCmdsAdv(tchar * pszCmdLine, tchar ** ppCmd, int iMax, lpctstr pszSep)
{
    ASSERT(iMax > 1);
    int iQty = 0;
    GETNONWHITESPACE(pszCmdLine);

    if (pszCmdLine[0] != '\0')
    {
        ppCmd[0] = pszCmdLine;
        ++iQty;
        while (Str_ParseAdv(ppCmd[iQty - 1], &(ppCmd[iQty]), pszSep))
        {
            if (++iQty >= iMax)
                break;
        }
    }
    for (int j = iQty; j < iMax; ++j)
        ppCmd[j] = nullptr;	// terminate if possible.
    return iQty;
}

tchar * Str_UnQuote(tchar * pStr)
{
    GETNONWHITESPACE(pStr);
    
    tchar ch = *pStr;
    if ((ch == '"') || (ch == '\''))
        ++pStr;
        
    for (tchar *pEnd = pStr + strlen(pStr) - 1; pEnd >= pStr; --pEnd)
    {
        if ((*pEnd == '"') || (*pEnd == '\''))
        {
            *pEnd = '\0';
            break;
        }
    }
    return pStr;
}

int Str_RegExMatch(lpctstr pPattern, lpctstr pText, tchar * lastError)
{
    try
    {
        CRegexp expressionformatch(pPattern, NO_FLAG);
        MatchResult result = expressionformatch.Match(pText);
        if (result.IsMatched())
            return 1;

        return 0;
    }
    catch (const std::bad_alloc &e)
    {
        Str_CopyLimitNull(lastError, e.what(), SCRIPT_MAX_LINE_LEN);
        CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
    catch (...)
    {
        Str_CopyLimitNull(lastError, "Unknown", SCRIPT_MAX_LINE_LEN);
        CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
}

//--

void CharToMultiByteNonNull(byte * Dest, const char * Src, int MBytes)
{
    for (int idx = 0; idx != MBytes * 2; idx += 2) {
        if (Src[idx / 2] == '\0')
            break;
        Dest[idx] = (byte)(Src[idx / 2]);
    }
}

UTF8MBSTR::UTF8MBSTR()
{
    m_strUTF8_MultiByte = new char[1]();
}

UTF8MBSTR::UTF8MBSTR(lpctstr lpStr)
{
    if (lpStr)
    {
        m_strUTF8_MultiByte = nullptr;
        ConvertStringToUTF8(lpStr, m_strUTF8_MultiByte);
    }
    else
        m_strUTF8_MultiByte = new char[1]();
}

UTF8MBSTR::UTF8MBSTR(UTF8MBSTR& lpStr)
{
    size_t len = Str_LengthUTF8(lpStr.m_strUTF8_MultiByte);
    m_strUTF8_MultiByte = new char[len + 1]();
    memcpy(m_strUTF8_MultiByte, lpStr.m_strUTF8_MultiByte, len);
}

UTF8MBSTR::~UTF8MBSTR()
{
    if (m_strUTF8_MultiByte)
        delete[] m_strUTF8_MultiByte;
}

void UTF8MBSTR::operator =(lpctstr lpStr)
{
    if (m_strUTF8_MultiByte)
        delete[] m_strUTF8_MultiByte;

    if (lpStr)
    {
        m_strUTF8_MultiByte = nullptr;
        ConvertStringToUTF8(lpStr, m_strUTF8_MultiByte);
    }
    else
        m_strUTF8_MultiByte = new char[1]();
}

void UTF8MBSTR::operator =(UTF8MBSTR& lpStr)
{
    if (m_strUTF8_MultiByte)
        delete[] m_strUTF8_MultiByte;

    size_t len = Str_LengthUTF8(lpStr.m_strUTF8_MultiByte);
    m_strUTF8_MultiByte = new char[len + 1]();
    memcpy(m_strUTF8_MultiByte, lpStr.m_strUTF8_MultiByte, len);
}

UTF8MBSTR::operator char* ()
{
    return m_strUTF8_MultiByte;
}

size_t UTF8MBSTR::ConvertStringToUTF8(lpctstr strIn, char*& strOutUTF8MB)
{
    size_t len;
#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
    len = wcslen(strIn);
#else
    // tchar is char (UTF8 encoding)
    len = Str_LengthUTF8(strIn);
#endif
    strOutUTF8MB = new char[len + 1]();

#if defined(_WIN32) && defined(UNICODE)
#ifdef _MSC_VER
    size_t aux = 0;
    wcstombs_s(&aux, strOutUTF8MB, len + 1, strIn, len);
#else
    wcstombs(strOutUTF8MB, strIn, len);
#endif

#else
    memcpy(strOutUTF8MB, strIn, len);
#endif

    return len;
}

size_t UTF8MBSTR::ConvertUTF8ToString(const char* strInUTF8MB, lptstr& strOut)
{
    size_t len = Str_LengthUTF8(strInUTF8MB);
    if (!strOut)
        strOut = new tchar[len + 1]();

#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
#ifdef _MSC_VER
    size_t aux = 0;
    mbstowcs_s(&aux, strInUTF8MB, len + 1, strInUTF8MB, len);
#else
    mbstowcs(strInUTF8MB, strInUTF8MB, len);
#endif
#else
    // tchar is char (UTF8 encoding)
    memcpy(strOut, strInUTF8MB, len);
#endif

    return len;
}
