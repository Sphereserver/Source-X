#ifndef _INC_SSTRING_H
#define _INC_SSTRING_H

#include "../common.h"
#include <cstring>
#include <optional>


#define STRING_NULL     "\0"
#define TSTRING_NULL    static_cast<const tchar*>("\0")
//const tchar * const TSTRING_NULL = "\0";
//const char  * const STRING_NULL  = "\0";

#define INT_CHARACTER(c)     int((c) & 0xFF)

inline bool IsHexNumDigit(int c) noexcept;

#ifdef UNICODE
    #include <cwctype>  // for iswalnum
	#define IsDigit(c)		iswdigit((wint_t)c)
	#define IsSpace(c)		iswspace((wint_t)c)
	#define IsAlpha(c)		iswalpha((wint_t)c)
	#define IsAlnum(c)		iswalnum((wint_t)c)
#else
    // TODO: remove INT_CHARACTER and adapt those calls to unicode chars
	#define IsDigit(c)		isdigit(INT_CHARACTER(c))
	#define IsSpace(c)		isspace(INT_CHARACTER(c))
	#define IsAlpha(c)		isalpha(INT_CHARACTER(c))
	#define IsAlnum(c)		isalnum(INT_CHARACTER(c))
#endif

/*  cross-platform functions macros  */
#ifdef _WIN32
    #define strcmpi		    _strcmpi
    #define strnicmp	    _strnicmp
    #define Str_Reverse	    _strrev
#else
    #include <cctype>   // toupper/tolower
    #define strcmpi			strcasecmp
    #define strnicmp		strncasecmp
    void Str_Reverse(char* string) noexcept;
#endif


/**
* match result defines
*/
enum MATCH_TYPE
{
    MATCH_INVALID = 0,
    MATCH_VALID,		// valid match
    MATCH_END,			// premature end of pattern string
    MATCH_ABORT,		// premature end of text string
    MATCH_RANGE,		// match failure on [..] construct
    MATCH_LITERAL,		// match failure on literal match
    MATCH_PATTERN		// bad pattern
};

struct KeyTableDesc_s
{
    const lpctstr *pptcTable;
    int iTableSize;
};

/** @name String utilities: Modifiers
*/

// If you want to use base = 16 to convert an hexadecimal string, it has to be in the format: 0x***
[[nodiscard]] std::optional<char>   Str_ToI8 (const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<uchar>  Str_ToU8 (const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<short>  Str_ToI16(const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<ushort> Str_ToU16(const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<int>    Str_ToI  (const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<uint>   Str_ToU  (const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<llong>  Str_ToLL (const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] std::optional<ullong> Str_ToULL(const tchar * ptcStr, uint base = 0, bool fIgnoreExcessChars = true) noexcept;
[[nodiscard]] inline
std::optional<size_t> Str_ToST(const tchar * ptcStr, uint base = 10) noexcept;

// The _Fast variants write from the end of the given buffer and return a pointer to the new start of the string, which in most
//  cases is different from the pointer passed as argument!
[[nodiscard]] tchar* Str_FromI_Fast   (int val,    tchar* buf, size_t buf_length, uint base = 10) noexcept;
[[nodiscard]] tchar* Str_FromUI_Fast  (uint val,   tchar* buf, size_t buf_length, uint base = 10) noexcept;
[[nodiscard]] tchar* Str_FromLL_Fast  (llong val,  tchar* buf, size_t buf_length, uint base = 10) noexcept;
[[nodiscard]] tchar* Str_FromULL_Fast (ullong val, tchar* buf, size_t buf_length, uint base = 10) noexcept;

// These functions use the _Fast methods, but do move the string from the end of the buffer to the beginning, so that the input pointer is still valid.
void Str_FromI   (int val,    tchar* buf, size_t buf_length, uint base = 10) noexcept;
void Str_FromUI  (uint val,   tchar* buf, size_t buf_length, uint base = 10) noexcept;
void Str_FromLL  (llong val,  tchar* buf, size_t buf_length, uint base = 10) noexcept;
void Str_FromULL (ullong val, tchar* buf, size_t buf_length, uint base = 10) noexcept;


size_t FindStrWord(lpctstr_restrict pTextSearch, lpctstr_restrict pszKeyWord) noexcept;
int Str_CmpHeadI(lpctstr_restrict ptcFind, lpctstr_restrict ptcHere) noexcept;

/** @name String utilities: Modifiers
*/
///@{

/**
* @brief Like strncpy, but doesn't zero all the exceeding buffer length
* @param pDst dest memory space.
* @param pSrc source data.
* @param uiMaxSize max bytes to be copied.
* @return bytes copied (CAN count the string terminator)
*/
size_t Str_CopyLimit(lptstr_restrict pDst, lpctstr_restrict pSrc, size_t uiMaxSize) noexcept;

/**
* @brief Like strncpy, but always zero-terminates the copied string (eventually truncating the text) and doesn't zero all the exceeding buffer length
* @param pDst dest memory space.
* @param pSrc source data.
* @param uiMaxSize max bytes to be copied (including string terminator!).
* @return bytes copied (not counting the string terminator)
*/
size_t Str_CopyLimitNull(lptstr_restrict pDst, lpctstr_restrict pSrc, size_t uiMaxSize) noexcept;

/**
* @brief Wrapper to cstring strcpy, but returns the length of the string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @return length of the string copied (number of characters).
*/
size_t Str_CopyLen(lptstr_restrict pDst, lpctstr_restrict pSrc) noexcept;

/**
* @brief strlen equivalent to be used with UTF8 multi-byte string.
* @param pStr UTF8 string.
* @return number of characters in the string, excluding the '\0' terminator.
*/
size_t Str_UTF8CharCount(const char* pStr) noexcept;

/**
* @brief Appends pSrc to string pDst of maximum size uiMaxSize. Always '\0' terminates (unless uiMaxSize <= strlen(pDst)).
* @param pDst dest memory space.
* @param pSrc source data.
* @param uiMaxSize max bytes that pDst can hold.
* @return Number of characters in the concatenated string, excluding the '\0' terminator.
*/
size_t Str_ConcatLimitNull(tchar * pDst, const tchar *pSrc, size_t uiMaxSize) noexcept;

/*
 * @brief Find the first occurrence of substring in string, where the search is limited to the first str_len characters of string.
 * @param str haystack string
 * @param substr needle string
 * @param str_len haystack length
 * @param substr_len needle length
 */
tchar* Str_FindSubstring(lptstr_restrict str, lpctstr_restrict substr, size_t str_len, size_t substr_len) noexcept;


/**
* @brief Give the article and space to a word. For example, for "boot" will return "a ".
* @param pszWords word to add the article.
* @return string with the article and a space.
*/
const tchar * Str_GetArticleAndSpace(lpctstr_restrict pszWords) noexcept;

/**
* @brief Filter specific characters from a string.
* @param ptcOut output string.
* @param ptcSrc input string.
* @param uiMaxOutSize max output size.
* @param ptcStripList characters to strip (default "{|}~", non printable characters for client).
* @return size of the filtered string.
*/
int Str_GetBare(tchar * ptcOut, const tchar* ptcSrc, size_t uiMaxOutSize, const tchar *ptcStripList = nullptr) noexcept;

/**
* @brief Finds and skip heading and trailing double quotes in a string.
* @param pStr string where remove the quotes.
* @return pointer inside the string with the heading and trailing quotes removed.
*/
tchar * Str_GetUnQuoted(lptstr_restrict pStr) noexcept;

/*
 * @brief Removes heading and trailing double quotes and apostrophes in a string.
 * @param pStr string where remove the quotes.
 * @return string with the heading and trailing quotes removed.
 */
tchar * Str_UnQuote(tchar * pStr) noexcept;

/**
* @brief Removes heading and trailing double quotes in a string.
* @param pStr string where remove the quotes.
*/
void Str_MakeUnQuoted(tchar * pStr) noexcept;

/**
* @brief replace string representation of special characters by special characters.
*
* Strings replaced:
* - \b
* - \n
* - \r
* - \t
* - \\
* @param pStr string to make replaces on.
* @return string with replaces in (same as pStr).
*/
tchar * Str_MakeFiltered(lptstr_restrict pStr) noexcept;


/**
* @brief replace special characters by string representation.
*
* Speciual characters replaced:
* - \b
* - \n
* - \r
* - \t
* - \\
* @param pStrOut strint where store the computed string.
* @param pStrIn input string.
* @param iSizeMax length of the input string.
*/
void Str_MakeUnFiltered(tchar * pStrOut, const tchar * pStrIn, int iSizeMax) noexcept;

/**
* @brief remove trailing white spaces from a string.
* @param pStr string where remove trailing spaces.
* @param len length of the string.
* @return new length of the string.
*/
int Str_TrimEndWhitespace(tchar * pStr, int len) noexcept;

/**
* @brief Removes heading and trailing white spaces of a string.
* @param pStr string where remove the white spaces.
* @return string with the heading and trailing spaces removed.
*/
NODISCARD tchar * Str_TrimWhitespace(tchar * pStr) noexcept;

/**
* @brief Skips the trailing white spaces of a string.
* @param pStrBegin pointer to the start of the string (won't be modified).
* @param pStrEnd (reference to) pointer to the end of the string (it might be modified).
*/
void Str_EatEndWhitespace(const tchar* const pStrBegin, tchar*& pStrEnd) noexcept;


// TODO: move to cexpression
/**
* @brief Skips the first substring enclosed by angular brackets: < >.
* @param ptcLine (Reference to) pointer to the string.
*/
void Str_SkipEnclosedAngularBrackets(tchar*& ptcLine) noexcept;


///@}


/** @name String utilities: String operations
*/
///@{

//TODOC
bool IsSimpleNumberString( const tchar * pszTest );
bool IsStrNumericDec( const tchar * pszTest );
bool IsStrNumeric( const tchar * pszTest );
bool IsStrEmpty( const tchar * pszTest );

// strncpy does not always return the actual amount of bytes written. this doesn't count the string terminator.
int StrncpyCharBytesWritten(int iBytesToWrite, size_t uiBufSize, bool fPrintError = true);


/**
* @brief Look for a string in a table.
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param uiElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTable(const tchar * pFind, const tchar * const * ppTable, int iCount) noexcept;

/**
* @brief Look for a string in a table (binary search).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param uiElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableSorted(const tchar * pFind, const tchar * const * ppTable, int iCount) noexcept;

/**
* @brief Look for a string header in a CAssocReg table (uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param uiElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindCAssocRegTableHeadSorted(const tchar * pFind, const tchar * const* ppTable, int iCount, size_t uiElemSize) noexcept;

/**
* @brief Look for a string header in a table (uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param uiElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHead(const tchar * pFind, const tchar * const * ppTable, int iCount) noexcept;

/**
* @brief Look for a string header in a table (binary search, uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param uiElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHeadSorted(const tchar * pFind, const tchar * const * ppTable, int iCount) noexcept;

/**
* @param pszIn string to check.
* @return true if string is empty or has '\c' or '\n' characters, false otherwise.
*/
bool Str_Check(const tchar * pszIn) noexcept;

/**
* @param pszIn string to check.
* @return false if string match "[a-zA-Z0-9_- \'\.]+", true otherwise.
*/
bool Str_CheckName(const tchar * pszIn) noexcept;

/**
* @brief find a substring in a string from an offset.
* @param pStr1 string where find the substring.
* @param pStr2 substring to find.
* @param offset position where to start the search.
* @return -1 for a bad offset or if string if not found, otherwise the position of the substring in string.
*/
int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset = 0) noexcept;

/**
* @brief check if a string matches a pattern.
* @see MATCH_TYPE
* @param pPattern pattern to match.
* @param pText text to match against the pattern.
* @return a MATCH_TYPE
*/
MATCH_TYPE Str_Match(const tchar * pPattern, const tchar * pText) noexcept;

/**
* @brief check if a string matches a regex.
* @param pPattern regex to match.
* @param pText text to match against the regex.
* @param lastError if any error, error description is stored here.
* @return 1 is regex is matched, 0 if not, -1 if errors.
*/
int Str_RegExMatch(const tchar * pPattern, const tchar * pText, tchar * lastError);

///@}

//---

void CharToMultiByteNonNull(byte* Dest, const char* Src, int MBytes) noexcept;

// Class for converting tchar to Multi-Byte UTF-8 and vice versa
//  SHOULDN'T really be used nowadays, because we don't #define UNICODE, thus tchar will always have UTF-8 encoding.
//  Convert* methods may be useful only if we rework them to accept lpcwstr/lpwstr, they may be used to convert encodings
//   when interacting with Windows API calls and passing Unicode strings (since only Windows uses UTF-16 encoding encapsulated
//   in wchar_t* strings, instead of UTF-8 in char* strings).
class UTF8MBSTR
{
public:
    UTF8MBSTR();
    UTF8MBSTR(const tchar * lpStr);
    UTF8MBSTR(UTF8MBSTR& lpStr);
    virtual ~UTF8MBSTR();

    void operator =(const tchar * lpStr);
    void operator =(UTF8MBSTR& lpStr) noexcept;
    operator char* () noexcept
    {
        return m_strUTF8_MultiByte.data();
    }
    operator const char* () const noexcept
    {
        return m_strUTF8_MultiByte.data();
    }

    static size_t ConvertStringToUTF8(const tchar * strIn, std::vector<char>* strOutUTF8MB);
    static size_t ConvertUTF8ToString(const char* strInUTF8MB, std::vector<tchar>* strOut);

private:
    std::vector<char> m_strUTF8_MultiByte;
};


//---

ssize_t fReadUntilDelimiter(char **buf, size_t *bufsiz, int delimiter, FILE *fp) noexcept; // equivalent to POSIX getline
ssize_t fReadUntilDelimiter_StaticBuf(char *buf, const size_t bufsiz, const int delimiter, FILE *fp) noexcept;
inline ssize_t fReadLine_StaticBuf(char *buf, const size_t bufsiz, FILE *fp) noexcept
{
    return fReadUntilDelimiter_StaticBuf(buf, bufsiz, '\n', fp);
}
ssize_t sGetDelimiter_StaticBuf(const int delimiter, const char* ptr_string, const size_t datasize) noexcept;
inline ssize_t sGetLine_StaticBuf(const char *data, const size_t datasize) noexcept
{
    return sGetDelimiter_StaticBuf('\n', data, datasize);
}


//--- Inline methods

std::optional<size_t> Str_ToST(const tchar * ptcStr, uint base) noexcept
{
    if constexpr (sizeof(size_t) == 4)
        return Str_ToU(ptcStr, base);
    else
        return Str_ToULL(ptcStr, base);
}

bool IsHexNumDigit(int c) noexcept
{
    return
    (c >= 'A' && c <= 'F') ||
    (c >= 'a' && c <= 'f') ||
    (c >= '0' && c <= '9');
}


#endif // _INC_SSTRING_H
