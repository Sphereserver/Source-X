#include "sstring.h"
//#include "../../common/CLog.h"
#include "../../sphere/ProfileTask.h"
#include "../CExpression.h"


#ifdef MSVC_COMPILER
    #include <codeanalysis/warnings.h>
    #pragma warning( push )
    #pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Winvalid-utf8"
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    #if defined(__GNUC__) && !defined(__clang__)
    #   pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    #elif defined(__clang__)
    #   pragma GCC diagnostic ignored "-Wweak-vtables"
    #endif
#endif

#include <regex/deelx.h>

#ifdef MSVC_COMPILER
    #pragma warning( pop )
#else
    #pragma GCC diagnostic pop
#endif


// String utilities: Converters

#ifndef _WIN32
void Str_Reverse(char* string) noexcept
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


// --
// String to Number

template<class _IntType>
bool cstr_to_num(
    const char * RESTRICT str,
    _IntType   * const    out,
    uint        base = 0,
    bool const  ignore_trailing_extra_chars = false
    ) noexcept
{
    static_assert(std::is_integral_v<_IntType>, "Only integers supported");
    if (!str || !out || base == 1 || base > 16)
        return false;

    // 1) Skip leading spaces
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') { // Instead of using ISWHITESPACE
        ++str;
    }
    if (*str == '\0')
        return false;

    // 2) Optional sign
    bool neg = false;
    if constexpr (std::is_signed_v<_IntType>)
    {
        /*  // If we wish to support numbers with the '+' sign, but we need to update the other parses (mainly in CExpression).
        if (*p == '+' || *p == '-')
        {
            neg = (*p == '-');
            ++p;
        }
        */
        if (*str == '-')
        {
            neg = true;
            ++str;
        }
    }

    // 3) Hex‐prefix via leading '0'
    bool hex = false;
    if (base == 0)
    {
        // Auto-detect.
        if (*str == '0' && str[1] != '\0' && str[1] != '.')
        {
            if (IsHexNumDigit(str[1]))
            {
                hex = true;
                base = 16;
                ++str;  // skip the '0' prefix
            }
            else
            {
                base = 10;
            }
        }
        else
        {
            base = 10;
        }
    }
    if (base == 16 && str[0] == '0' && str[1] != '\0' && str[1] != '.')
    {
        hex = true;
        ++str;
    }

    using _UIntType = std::make_unsigned_t<_IntType>;
    const _UIntType base_casted = uint8_t(base);
    const _UIntType maxDiv = std::numeric_limits<_UIntType>::max() / _UIntType(base_casted);
    const _UIntType maxRem = std::numeric_limits<_UIntType>::max() % _UIntType(base_casted);
    _UIntType acc = 0;  // accumulator

    // 4) Parse digits
    ushort ndigits = 0;
    const char* startDigits = str;
    for (; *str; ++str)
    {
        const char c = *str;
        _UIntType digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (hex && c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else if (hex && c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else if (!hex && c == '.')
            continue;   // Skip decimal point
        else
            break;
        if (acc > maxDiv || (acc == maxDiv && digit > maxRem))
            return false;  // overflow
        acc = acc * base_casted + digit;
        ++ndigits;
    }
    if (str == startDigits)
        return false;  // no digits consumed

    // 5) Trailing‐space or end‐of‐string
    if (!ignore_trailing_extra_chars)
    {
        // Some old code expects string to num conversion to tolerate trailing whitespaces or even extra chars
        // (like the atoi C function).
        constexpr bool tolerate_trailing_whitespaces = true;
        if constexpr (tolerate_trailing_whitespaces)
        {
            while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
                ++str;
        }
        if (*str != '\0')
            return false;
    }

    // 6) Make short (< UINT32_MAX, or <= 8 hex digits) hex numbers behave like 32 bits numbers.
    /*
     * Why Sphere expects this and works like this is unknown to me (maybe TUS/Grayworld or
     * prehistoric Sphere versions worked with 32 bit numbers instead of 64 bit).
     *
    */
    if (hex && ndigits<=8 && std::is_signed_v<_IntType>)
    {
        // Reinterpret the bits as a signed 32 bit number, then expand that value to a 32 bit number.
        // if ndigits <= 8 (so number is always <= 0xFFFF FFFF), it will always be < UINT32_MAX.
        int32_t v32 = int32_t(uint32_t(acc));
        *out = _IntType(v32);
        return true;
    }

    // 7) Store result
    if constexpr (std::is_signed_v<_IntType>)
    {
        if (neg)
        {
            if (acc > _UIntType(std::numeric_limits<_IntType>::max()) + 1u)
                return false;  // too negative
            *out = _IntType(0) - _IntType(acc);
        }
        else
        {
            if (acc > _UIntType(std::numeric_limits<_IntType>::max()))
                return false;  // too large positive
            *out = _IntType(acc);
        }
    }
    else
    {
        *out = _IntType(acc);
    }
    return true;
}


std::optional<char> Str_ToI8 (const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    char val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<uchar> Str_ToU8 (const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    uchar val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<short> Str_ToI16 (const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    short val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<ushort> Str_ToU16 (const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    ushort val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<int> Str_ToI (const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    int val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<uint> Str_ToU(const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    uint val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<llong> Str_ToLL(const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    llong val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<ullong> Str_ToULL(const tchar * ptcStr, uint base, bool fIgnoreExcessChars) noexcept
{
    ullong val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}


// --
// Number to String

static constexpr tchar DIGITS[] = "0123456789abcdef";

template<typename _IntType>
tchar* Str_FromInt_Fast(_IntType val, lptstr_restrict out_buf, size_t buf_length, uint base) noexcept
{
    if (!out_buf || buf_length == 0 || base == 0 || base > 16)
        return nullptr;

    const bool fHex = (base == 16);
    // Handle zero or base==0 case
    if (val == 0)
    {
        if (fHex)
        {
            if (buf_length < 3)
                return nullptr;
            out_buf[0] = '0'; out_buf[1] = '0'; out_buf[2] = '\0';
        }
        else
        {
            if (buf_length < 2)
                return nullptr;
            out_buf[0] = '0'; out_buf[1] = '\0';
        }
        return out_buf;
    }

    using _UIntType = std::make_unsigned_t<_IntType>;
    _UIntType uval;
    bool fIsNegative = false;

    if constexpr (std::is_signed_v<_IntType>)
    {
        fIsNegative = (val < 0);
        // two's-complement bit-pattern reinterpret
        const auto utmp = static_cast<_UIntType>(val);
        if (fHex)
        {
            uval = utmp;
        }
        else
        {
            // Unsigned subtraction is always defined behaviour, it wraps around.
            uval = fIsNegative ? (_UIntType(0) - utmp) : utmp;
        }
    }
    else
    {
        uval = static_cast<_UIntType>(val);
    }

    // Write digits from back of buffer
#define WRITE_OUT(ch)           \
    do {                        \
        if (idx == 0)           \
            return nullptr;     \
        out_buf[--idx] = (ch);  \
    } while (0)

    size_t idx = buf_length;
    out_buf[--idx] = '\0';

    if (fHex)
    {
        // Hex path: mask & shift by 4 bits (it's the same as dividing by 16).
        do
        {
            WRITE_OUT(DIGITS[uval & 0xFu]);
            uval >>= 4;
        } while (uval);
        WRITE_OUT('0');  // leading 0 hex prefix
    }
    else
    {
        // General path: division + multiply to get remainder
        do
        {
            const _UIntType q = uval / base;
            const _UIntType d = uval - q * base;
            WRITE_OUT(DIGITS[d]);
            uval = q;
        } while (uval);

        if (fIsNegative) {
            WRITE_OUT('-');
        }
    }

#undef WRITE_OUT
    return &out_buf[idx];
}

tchar* Str_FromI_Fast(int val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromUI_Fast(uint val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromLL_Fast (llong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromULL_Fast (ullong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

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


size_t FindStrWord( lpctstr_restrict pTextSearch, lpctstr_restrict pszKeyWord ) noexcept
{
    // Find any of the pszKeyWord in the pTextSearch string.
    // Make sure we look for starts of words.

    size_t j = 0;
    for ( size_t i = 0; ; ++i )
    {
        if ( pszKeyWord[j] == '\0' || pszKeyWord[j] == ',')
        {
            if ( pTextSearch[i]== '\0' || IsWhitespace(pTextSearch[i]))
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

int Str_CmpHeadI(lpctstr_restrict ptcFind, lpctstr_restrict ptcHere) noexcept
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

static inline int Str_CmpHeadI_Table(const tchar * ptcFind, const tchar * ptcTable) noexcept
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

// Useful also for s(n)printf!
int StrncpyCharBytesWritten(int iBytesToWrite, size_t uiBufSize, bool fPrintError)
{
    if (iBytesToWrite < 0)
        return 0;
    if (uiBufSize < 1)
        goto err;
    if ((uint)iBytesToWrite >= uiBufSize - 1)
        goto err;
    return iBytesToWrite;

err:
    //throw CSError(LOGL_ERROR, 0, "Buffer size too small for snprintf.\n");
    if (fPrintError) {
        g_Log.EventError("Buffer size too small for snprintf.\n");
    }
    return (uiBufSize > 1) ? int(uiBufSize - 1) : 0; // Bytes written, excluding the string terminator.
}

bool IsStrEmpty( const tchar * pszTest )
{
    if ( !pszTest || !*pszTest )
        return true;

    do
    {
        if ( !IsSpace(*pszTest) )
            return false;
    }
    while ( *(++pszTest) );
    return true;
}

bool IsStrNumericDec( const tchar * pszTest )
{
    if ( !pszTest || !*pszTest )
        return false;

    do
    {
        if ( !IsDigit(*pszTest) )
            return false;
    }
    while ( *(++pszTest) );

    return true;
}


bool IsStrNumeric( const tchar * pszTest )
{
    if ( !pszTest || !*pszTest )
        return false;

    bool fHex = false;
    if ( pszTest[0] == '0' )
        fHex = true;

    do
    {
        if ( IsDigit( *pszTest ) )
            continue;
        if ( fHex && tolower(*pszTest) >= 'a' && tolower(*pszTest) <= 'f' )
            continue;
        return false;
    }
    while ( *(++pszTest) );
    return true;
}

bool IsSimpleNumberString( lpctstr_restrict pszTest )
{
    // is this a string or a simple numeric expression ?
    // string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
    // number = 1.0+-\*~|&!%^()2, 0aed, 123

    if (*pszTest == '\0')
        return false;   // empty string, no number

    bool fMathSep			= true;	// last non whitespace was a math sep.
    bool fHextDigitStart	= false;
    bool fWhiteSpace		= false;

    for ( ; ; ++pszTest )
    {
        tchar ch = *pszTest;
        if ( ! ch )
            return true;

        if (( ch >= 'A' && ch <= 'F') || ( ch >= 'a' && ch <= 'f' ))	// isxdigit
        {
            if ( ! fHextDigitStart )
                return false;

            fWhiteSpace = false;
            fMathSep = false;
            continue;
        }
        if ( IsSpace( ch ) )
        {
            fHextDigitStart = false;
            fWhiteSpace = true;
            continue;
        }
        if ( IsDigit( ch ) )
        {
            if ( fWhiteSpace && ! fMathSep )
                return false;

            if ( ch == '0' )
                fHextDigitStart = true;
            fWhiteSpace = false;
            fMathSep = false;
            continue;
        }
        if ( ch == '/' && pszTest[1] != '/' )
            fMathSep = true;
        else
            fMathSep = strchr("+-\\*~|&!%^()", ch ) ? true : false ;

        if ( ! fMathSep )
            return false;

        fHextDigitStart = false;
        fWhiteSpace = false;
    }
}


// strcpy doesn't have an argument to truncate the copy to the buffer length;
// strncpy doesn't null-terminate if it truncates the copy, and if uiMaxlen is > than the source string length, the remaining space is filled with '\0'
size_t Str_CopyLimit(lptstr_restrict pDst, lpctstr_restrict pSrc, const size_t uiMaxSize) noexcept
{
    if (uiMaxSize == 0)
        return 0;

    if (pSrc[0] == '\0')
    {

        pDst[0] = '\0';
        return 0;
    }

    // Find string terminator within the first uiMaxSize bytes (fast library call, usually vectorized)
    const void* nul = memchr(pSrc, '\0', uiMaxSize);
    const size_t toCopy = nul
                        ? ((static_cast<const char*>(nul) - pSrc) + 1) // +1 to include the terminator
                        : uiMaxSize;    // No terminator in range: copy full limit

    memcpy(pDst, pSrc, toCopy);
    return toCopy; // bytes copied in pDst string (CAN count the string terminator)
}

size_t Str_CopyLimitNull(lptstr_restrict pDst, lpctstr_restrict pSrc, size_t uiMaxSize) noexcept
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

    // Reserve one byte for the string terminator
    const size_t uiCopyMax = uiMaxSize - 1;

    // Find the terminator within the first copyMax bytes (fast memchr)
    const void* nul = memchr(pSrc, '\0', uiCopyMax);
    const size_t len = nul
                          ? (static_cast<const char*>(nul) - pSrc)  // length up to the terminator
                          : uiCopyMax;                                // no terminator found within limit

    // Copy the determined length and append terminator
    memcpy(pDst, pSrc, len);
    pDst[len] = '\0';

    return len; // bytes copied in pDst string (not counting the string terminator)
}

size_t Str_CopyLen(lptstr_restrict pDst, lpctstr_restrict pSrc) noexcept
{
    strcpy(pDst, pSrc);
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

size_t Str_UTF8CharCount(const char* strInUTF8MB) noexcept
{
    size_t len; // number of characters in the string
#ifdef MSVC_RUNTIME
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
size_t Str_ConcatLimitNull(tchar *dst, const tchar *src, size_t siz) noexcept
{
    if (siz == 0)
        return 0;

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

tchar* Str_FindSubstring(lptstr_restrict str, lpctstr_restrict substr, size_t str_len, size_t substr_len) noexcept
{
    if (str_len == 0 || substr_len == 0)
        return nullptr;

    tchar c, sc;
    if ((c = *substr++) != '\0')
    {
        do
        {
            do
            {
                if (str_len < 1 || (sc = *str++) == '\0')
                {
                    return nullptr;
                }
                str_len -= 1;
            } while (sc != c);
            if (substr_len > str_len)
            {
                return nullptr;
            }
        } while (0 != strncmp(str, substr, substr_len));
        --str;
    }
    return str;
}

const tchar * Str_GetArticleAndSpace(lpctstr_restrict pszWord) noexcept
{
    // NOTE: This is wrong many times.
    //  ie. some words need no article (plurals) : boots.

    // This function shall NOT be called if OF_NoPrefix is set!

    if (pszWord)
    {
        static constexpr tchar sm_Vowels[] = { 'A', 'E', 'I', 'O', 'U' };
        tchar chName = static_cast<tchar>(toupper(pszWord[0]));
        for (uint x = 0; x < ARRAY_COUNT(sm_Vowels); ++x)
        {
            if (chName == sm_Vowels[x])
                return "an ";
        }
    }
    return "a ";
}

int Str_GetBare(tchar * ptcOut, const tchar *ptcSrc, size_t uiMaxOutSize, const tchar * ptcStripList) noexcept
{
    // That the client can deal with. Basic punctuation and alpha and numbers.
    // RETURN: Output length.

    if (!ptcOut || !ptcSrc || uiMaxOutSize == 0)
        return 0;

    // Default strip set if none provided: client can't print these
    ptcStripList = ptcStripList ? ptcStripList : "{|}~";

    tchar* out          = ptcOut;
    tchar* const outEnd = ptcOut + (uiMaxOutSize - 1);

    // Process each char until SRC ends or output buffer is full
    for (; *ptcSrc && out < outEnd; ++ptcSrc)
    {
        const uchar ch = uchar(*ptcSrc);

        if (ch < ' ' || ch >= 127)  // or !std::isprint(ch)
            continue;	// Special format chars.
        if (strchr(ptcStripList, ch))
            continue;

        *out++ = tchar(ch);
    }

    // NUL-terminate and return length
    *out = '\0';
    return int(out - ptcOut);
}

/* Old impl.
int Str_GetBare(tchar * pszOut, const tchar * pszInp, int iMaxOutSize, const tchar * pszStrip) noexcept
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
*/

tchar * Str_MakeFiltered(lptstr_restrict pStr) noexcept
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

void Str_MakeUnFiltered(tchar * pStrOut, const tchar * pStrIn, int iSizeMax) noexcept
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

// Unquotes and trims whitespace in-place, shifting result to start of buffer.
void Str_MakeUnQuoted(tchar* pStr) noexcept
{
    tchar* src = pStr;
    // Skip leading whitespace (GETNONWHITESPACE)
    while (IsWhitespace(*src)) {
        ++src;
    }

    bool fQuoted = false;
    if (*src == '"')
    {
        fQuoted = true;
        ++src;
    }

    tchar* endPtr = src + std::strlen(src);

    // If quoted, locate closing quote and adjust endPtr
    if (fQuoted)
    {
        tchar* p = endPtr;
        while (p > src)
        {
            --p;
            if (*p == '"')
            {
                endPtr = p;
                break;
            }
        }
    }

    // Compute trimmed length (exclude trailing whitespace), like Str_TrimWhitespace.
    size_t len = endPtr - src;
    while (len > 0 && IsWhitespace(src[len - 1])) {
        --len;
    }

    // Shift content to the start of the buffer
    if (src != pStr && len > 0) {
        std::memmove(pStr, src, (len * sizeof(tchar)));
    }
    pStr[len] = '\0';
}

// Returns a pointer to the unquoted part (and overwrites the last quote with a string terminator '\0' char)
tchar * Str_GetUnQuoted(lptstr_restrict pStr) noexcept
{
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
    ASSERT(len >= 0);
    while (len > 0 && IsWhitespace(pStr[len - 1])) {
        --len;
    }
    pStr[len] = '\0';
    return len;
}

tchar * Str_TrimWhitespace(tchar * pStr) noexcept
{
    GETNONWHITESPACE(pStr);
    Str_TrimEndWhitespace(pStr, (int)strlen(pStr));
    return pStr;
}

void Str_EatEndWhitespace(const tchar* const pStrBegin, tchar*& pStrEnd) noexcept
{
    if (pStrBegin == pStrEnd)
        return;

    tchar* ptcPrev = pStrEnd - 1;
    while ((ptcPrev != pStrBegin) && IsWhitespace(*ptcPrev))
    {
        if (*ptcPrev == '\0')
            return;

        --pStrEnd;
        ptcPrev = pStrEnd - 1;
    }
}

/*
void Str_SkipEnclosedAngularBrackets(tchar*& ptcLine) noexcept
{
    // Move past a < > statement. It can have ( ) inside, if it happens, ignore < > characters inside ().
    bool fOpenedOneAngular = false;
    int iOpenAngular = 0, iOpenCurly = 0;
    tchar* ptcTest = ptcLine;
    while (const tchar ch = *ptcTest)
    {
        if (IsWhitespace(ch))
            ;
        else if (ch == '(')
            ++iOpenCurly;
        else if (ch == ')')
            --iOpenCurly;
        else if (iOpenCurly == 0)
        {
            if (ch == '<')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '<') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    // I want a whitespace after the operator and some text after it.
                    const tchar * ptcOpTest = &(ptcTest[3]);
                    if (*ptcOpTest != '\0')
                    {
                        GETNONWHITESPACE(ptcOpTest);
                        if (*ptcOpTest != '\0')  // There's more text to parse
                        {
                            // I guess i have sufficient proof: skip, it's a << operator
                            fOperator = true;
                            ptcTest += 2; // Skip the second > and the following whitespace
                        }
                    }
                }
                if (!fOperator)
                {
                    fOpenedOneAngular = true;
                    ++iOpenAngular;
                }
            }
            else if (ch == '>')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '>') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    if ((ptcLine == ptcTest) || ((iOpenAngular > 0) && IsWhitespace(*(ptcTest - 1))))
                    {
                        const tchar * ptcOpTest = &(ptcTest[3]);
                        if (*ptcOpTest != '\0')
                        {
                            GETNONWHITESPACE(ptcOpTest);
                            if (*ptcOpTest != '\0')  // There's more text to parse
                            {
                                // I guess i have sufficient proof: skip, it's a >> operator
                                fOperator = true;
                                ptcTest += 2; // Skip the second > and the following whitespace
                            }
                        }
                    }
                }
                if (!fOperator)
                {
                    --iOpenAngular;
                    if (fOpenedOneAngular && !iOpenAngular)
                    {
                        ptcLine = ptcTest + 1;
                        return;
                    }
                }
            }
        }
        ++ptcTest;
    }
}
*/

void Str_SkipEnclosedAngularBrackets(tchar*& ptcLine) noexcept
{
    // Move past a < > statement. It can have ( ) inside, if it happens, ignore < > characters inside ().
    bool fOpenedOneAngular = false;
    int iOpenAngular = 0, iOpenCurly = 0;
    tchar* ptcTest = ptcLine;
    while (const tchar ch = *ptcTest)
    {
        if (IsWhitespace(ch))
            ;
        else if (ch == '(')
            ++iOpenCurly;
        else if (ch == ')')
            --iOpenCurly;
        else if (iOpenCurly == 0)
        {
            if (ch == '<')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '<') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    // I guess i have sufficient proof: skip, it's a << operator
                    fOperator = true;
                }
                if (!fOperator)
                {
                    fOpenedOneAngular = true;
                    ++iOpenAngular;
                }
                else
                    ptcTest += 2; // Skip the second > and the following whitespace
            }
            else if (ch == '>')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '>') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    if ((ptcLine == ptcTest) || ((iOpenAngular > 0) && IsWhitespace(*(ptcTest - 1))))
                    {
                        // I guess i have sufficient proof: skip, it's a >> operator
                        fOperator = true;
                    }
                }
                if (!fOperator)
                {
                    --iOpenAngular;
                    if (fOpenedOneAngular && !iOpenAngular)
                    {
                        ptcLine = ptcTest + 1;
                        return;
                    }
                }
                else
                    ptcTest += 2; // Skip the second > and the following whitespace
            }
        }
        ++ptcTest;
    }
}


// String utilities: String operations

int FindTable(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept
{
    // A non-sorted table.
    for (int i = 0; i < iCount; ++i)
    {
        if (!strcmpi(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableSorted(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept
{
    // Do a binary search (un-cased) on a sorted table.
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
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
    const tchar * const* base = pptcTable;
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

int FindTableHead(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE
{
    for (int i = 0; i < iCount; ++i)
    {
        if (!Str_CmpHeadI_Table(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableHeadSorted(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
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
    const tchar * const* base = pptcTable;
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

int FindCAssocRegTableHeadSorted(const tchar * pszFind, const tchar * const* ppszTable, int iCount, size_t uiElemSize) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found
    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const tchar * pszName = *(reinterpret_cast<const tchar * const*>(reinterpret_cast<const byte*>(ppszTable) + (i * uiElemSize)));
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

bool Str_Check(const tchar * pszIn) noexcept
{
    if (pszIn == nullptr)
        return true;

    const tchar * p = pszIn;
    while (*p != '\0' && (*p != 0x0A) && (*p != 0x0D))
        ++p;

    return (*p != '\0');
}

bool Str_CheckName(const tchar * pszIn) noexcept
{
    if (pszIn == nullptr)
        return true;

    const tchar * p = pszIn;
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

int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset) noexcept
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

static MATCH_TYPE Str_Match_After_Star(const tchar * pPattern, const tchar * pText) noexcept
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

MATCH_TYPE Str_Match(const tchar * pPattern, const tchar * pText) noexcept
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

tchar * Str_UnQuote(tchar * pStr) noexcept
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

int Str_RegExMatch(const tchar * pPattern, const tchar * pText, tchar * lastError)
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
        GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
    catch (...)
    {
        Str_CopyLimitNull(lastError, "Unknown", SCRIPT_MAX_LINE_LEN);
        GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
}

//--

void CharToMultiByteNonNull(byte * Dest, const char * Src, int MBytes) noexcept
{
    for (int idx = 0; idx != MBytes * 2; idx += 2) {
        if (Src[idx / 2] == '\0')
            break;
        Dest[idx] = (byte)(Src[idx / 2]);
    }
}

UTF8MBSTR::UTF8MBSTR() = default;

UTF8MBSTR::UTF8MBSTR(const tchar * lpStr)
{
    operator=(lpStr);
}

UTF8MBSTR::UTF8MBSTR(UTF8MBSTR& lpStr)
{
    m_strUTF8_MultiByte = lpStr.m_strUTF8_MultiByte;
}

UTF8MBSTR::~UTF8MBSTR() = default;

void UTF8MBSTR::operator =(const tchar * lpStr)
{
    if (lpStr)
        ConvertStringToUTF8(lpStr, &m_strUTF8_MultiByte);
    else
        m_strUTF8_MultiByte.clear();
}

void UTF8MBSTR::operator =(UTF8MBSTR& lpStr) noexcept
{
    m_strUTF8_MultiByte = lpStr.m_strUTF8_MultiByte;
}

size_t UTF8MBSTR::ConvertStringToUTF8(const tchar * strIn, std::vector<char>* strOutUTF8MB)
{
    ASSERT(strOutUTF8MB);
    size_t len;
#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
    len = wcslen(strIn);
#else
    // tchar is char (UTF8 encoding)
    len = strlen(strIn);
#endif
    strOutUTF8MB->resize(len + 1);

#if defined(_WIN32) && defined(UNICODE)
#   ifdef MSVC_RUNTIME
    size_t aux = 0;
    wcstombs_s(&aux, strOutUTF8MB->data(), len + 1, strIn, len);
#   else
    wcstombs(strOutUTF8MB->data(), strIn, len);
#   endif
#else
    // already utf8
    memcpy(strOutUTF8MB->data(), strIn, len);
#endif

    (*strOutUTF8MB)[len] = '\0';
    return len;
}

size_t UTF8MBSTR::ConvertUTF8ToString(const char* strInUTF8MB, std::vector<tchar>* strOut)
{
    ASSERT(strOut);
    size_t len;

#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
    len = Str_UTF8CharCount(strInUTF8MB);
    strOut->resize(len + 1);
#ifdef MSVC_RUNTIME
    size_t aux = 0;
    mbstowcs_s(&aux, strInUTF8MB, len + 1, strInUTF8MB, len);
#else
    mbstowcs(strInUTF8MB, strInUTF8MB, len);
#endif
#else
    // tchar is char (UTF8 encoding)
    len = strlen(strInUTF8MB);
    strOut->resize(len + 1);
    memcpy(strOut->data(), strInUTF8MB, len);
#endif

    (*strOut)[len] = '\0';
    return len;
}


/*	$NetBSD: getdelim.c,v 1.2 2015/12/25 20:12:46 joerg Exp $	*/
/*	NetBSD-src: getline.c,v 1.2 2014/09/16 17:23:50 christos Exp 	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

ssize_t
//getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
fReadUntilDelimiter(char **buf, size_t *bufsiz, int delimiter, FILE *fp) noexcept
{
    char *ptr, *eptr;


    if (*buf == nullptr || *bufsiz == 0) {
        *bufsiz = BUFSIZ;
        if ((*buf = (char*)malloc(*bufsiz)) == nullptr)
            return -1;
    }

    for (ptr = *buf, eptr = *buf + *bufsiz;;) {
        int c = fgetc(fp);
        if (c == -1) {
            if (feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - *buf);
                if (diff != 0) {
                    *ptr = '\0';
                    return diff;
                }
            }
            return -1;
        }
        *ptr++ = (char)c;
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - *buf;
        }
        if (ptr + 2 >= eptr) {
            char *nbuf;
            size_t nbufsiz = *bufsiz * 2;
            ssize_t d = ptr - *buf;
            if ((nbuf = (char*)realloc(*buf, nbufsiz)) == nullptr)
                return -1;
            *buf = nbuf;
            *bufsiz = nbufsiz;
            eptr = nbuf + nbufsiz;
            ptr = nbuf + d;
        }
    }
    return -1;
}

ssize_t fReadUntilDelimiter_StaticBuf(char *buf, const size_t bufsiz, const int delimiter, FILE *fp) noexcept
{
    // buf: line array
    char *ptr, *eptr;

    for (ptr = buf, eptr = buf + bufsiz;;) {
        const int c = fgetc(fp);
        if (c == -1) {
            if (feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - buf);
                if (diff != 0) {
                    *ptr = '\0';
                    return diff;
                }
            }
            return -1;
        }
        *ptr++ = static_cast<char>(c);
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - buf;
        }
        if (ptr + 2 >= eptr) {
            return -1; // buffer too small
        }
    }
    return -1;
}

ssize_t sGetDelimiter_StaticBuf(const int delimiter, const char *ptr_string, const size_t datasize) noexcept
{
    // Returns the number of chars before the delimiter (or the end of the string).
    // buf: line array
    const char *ptr_cursor, *ptr_end;

    if (*ptr_string == '\0') {
        return -1;
    }

    for (ptr_cursor = ptr_string, ptr_end = ptr_string + datasize;; ++ptr_cursor) {
        if (*ptr_cursor == '\0') {
            if (ptr_cursor != ptr_string) {
                return ptr_cursor - ptr_string;
            }
            return -1;
        }
        if (*ptr_cursor == delimiter) {
            return ptr_cursor - ptr_string;
        }
        if (ptr_cursor + 1 > ptr_end) {
            return -1; // buffer too small
        }
    }
    return -1;
}
