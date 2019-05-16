#ifndef _INC_SSTRING_H
#define _INC_SSTRING_H

#include <cstring>
#include <cwctype>  // for iswalnum
#include "../common.h"

#define STRING_NULL     "\0"
#define TSTRING_NULL    static_cast<const tchar*>("\0")
//const tchar * const TSTRING_NULL = "\0";
//const char  * const STRING_NULL  = "\0";

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
int    Str_ToI  (lpctstr ptcStr, int base = 10) noexcept;
uint   Str_ToUI (lpctstr ptcStr, int base = 10) noexcept;
llong  Str_ToLL (lpctstr ptcStr, int base = 10) noexcept;
ullong Str_ToULL(lpctstr ptcStr, int base = 10) noexcept;

tchar* Str_FromI   (tchar *buf, int val,    int base = 10) noexcept;
tchar* Str_FromUI  (tchar *buf, uint val,   int base = 10) noexcept;
tchar* Str_FromLL  (tchar *buf, llong val,  int base = 10) noexcept;
tchar* Str_FromULL (tchar *buf, ullong val, int base = 10) noexcept;


size_t FindStrWord( lpctstr pTextSearch, lpctstr pszKeyWord );
int Str_CmpHeadI(lpctstr ptcFind, lpctstr ptcHere);

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
size_t Str_CopyLimit(tchar * pDst, lpctstr pSrc, size_t uiMaxSize);

/**
* @brief Like strncpy, but always zero-terminates the copied string (eventually truncating the text) and doesn't zero all the exceeding buffer length
* @param pDst dest memory space.
* @param pSrc source data.

* @return bytes copied (not counting the string terminator)
*/
size_t Str_CopyLimitNull(tchar * pDst, lpctstr pSrc, size_t uiMaxSize);

/**
* @brief Wrapper to cstring strcpy, but returns the length of the string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @return length of the string copied (number of characters).
*/
size_t strcpylen(tchar * pDst, lpctstr pSrc);

/**
* @brief Appends pSrc to string pDst of maximum size uiMaxSize. Always '\0' terminates (unless uiMaxSize <= strlen(pDst)).
* @param pDst dest memory space.
* @param pSrc source data.
* @param uiMaxSize max bytes that pDst can hold.
* @return strlen(src) + MIN(uiMaxSize, strlen(initial dst)).
*/
size_t Str_ConcatLimitNull(tchar *pDst, const tchar *pSrc, size_t uiMaxSize);

/**
* @brief Give the article and space to a word. For example, for "boot" will return "a ".
* @param pszWords word to add the article.
* @return string with the article and a space.
*/
lpctstr Str_GetArticleAndSpace(lpctstr pszWords);

/**
* @brief Filter specific characters from a string.
* @param pszOut output string.
* @param pszInp input string.
* @param iMaxSize max output size.
* @param pszStrip characters to strip (default "{|}~", non printable characters for client).
* @return size of the filtered string.
*/
int Str_GetBare(tchar * pszOut, lpctstr pszInp, int iMaxSize, lpctstr pszStrip = nullptr);

/**
* @brief Removes heading and trailing double quotes in a string.
* @param pStr string where remove the quotes.
* @return string with the heading and trailing quotes removed.
*/
tchar * Str_GetUnQuoted(tchar * pStr);

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
tchar * Str_MakeFiltered(tchar * pStr);


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
void Str_MakeUnFiltered(tchar * pStrOut, lpctstr pStrIn, int iSizeMax);

/**
* @brief remove trailing white spaces from a string.
* @param pStr string where remove trailing spaces.
* @param len length of the string.
* @return new length of the string.
*/
int Str_TrimEndWhitespace(tchar * pStr, int len);

/**
* @brief Removes heading and trailing white spaces of a string.
* @param pStr string where remove the white spaces.
* @return string with the heading and trailing spaces removed.
*/
tchar * Str_TrimWhitespace(tchar * pStr);


///@}
/** @name String utilities: String operations
*/
///@{
/**
* @brief Look for a string in a table.
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTable(const lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));

/**
* @brief Look for a string in a table (binary search).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableSorted(const lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));

/**
* @brief Look for a string header in a table (uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHead(const lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));

/**
* @brief Look for a string header in a table (binary search, uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHeadSorted(const lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));

/**
* @param pszIn string to check.
* @return true if string is empty or has '\c' or '\n' characters, false otherwise.
*/
bool Str_Check(lpctstr pszIn);

/**
* @param pszIn string to check.
* @return false if string match "[a-zA-Z0-9_- \'\.]+", true otherwise.
*/
bool Str_CheckName(lpctstr pszIn);

/**
* @brief find a substring in a string from an offset.
* @param pStr1 string where find the substring.
* @param pStr2 substring to find.
* @param offset position where to start the search.
* @return -1 for a bad offset or if string if not found, otherwise the position of the substring in string.
*/
int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset = 0);

/**
* @brief check if a string matches a pattern.
* @see MATCH_TYPE
* @param pPattern pattern to match.
* @param pText text to match against the pattern.
* @return a MATCH_TYPE
*/
MATCH_TYPE Str_Match(lpctstr pPattern, lpctstr pText);

/**
* @brief Parse a simple argument from a list of arguments.
*
* From a line like "    a, 2, 3" it will get "a" (Note that removes heading white spaces)
* on pLine and  "2, 3" on ppArg.
* @param pLine line to parse and where store the arg parsed.
* @param ppArg where to store the other args (non proccessed pLine).
* @param pSep the list of separators (by default "=, \t").
* @return false if there are no more args to parse, true otherwise.
*/
bool Str_Parse(tchar * pLine, tchar ** ppArg = nullptr, lpctstr pSep = nullptr);

/**
* @brief Parse a list of arguments.
* @param pCmdLine list of arguments to parse.
* @param ppCmd where to store the parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, tchar ** ppCmd, int iMax, lpctstr pSep = nullptr);

/**
* @brief Parse a list of arguments (integer version).
* @param pCmdLine list of arguments to parse.
* @param piCmd where to store de parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, int64 * piCmd, int iMax, lpctstr pSep = nullptr);

/**
* @brief check if a string matches a regex.
* @param pPattern regex to match.
* @param pText text to match against the regex.
* @param lastError if any error, error description is stored here.
* @return 1 is regex is matched, 0 if not, -1 if errors.
*/
int Str_RegExMatch(lpctstr pPattern, lpctstr pText, tchar * lastError);
///@}

void CharToMultiByteNonNull(byte * Dest, const char * Src, int MBytes);

// extern tchar * Str_GetTemporary(int amount = 1);
#define Str_GetTemp static_cast<AbstractSphereThread *>(ThreadHolder::current())->allocateBuffer
#define Str_GetWTemp static_cast<AbstractSphereThread *>(ThreadHolder::current())->allocateWBuffer
#define STR_TEMPLENGTH THREAD_STRING_LENGTH


#endif // _INC_SSTRING_H