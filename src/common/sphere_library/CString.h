/**
* @file CString.
* @brief Custom string implementation.
*/

#pragma once
#ifndef _INC_CSTRING_H
#define _INC_CSTRING_H

#include "common.h"


/**
* @brief Custom String implementation.
*/
class CString
{
public:
	static const char *m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	/**
	* @brief Default constructor.
	*
	* Initializes string. If DEBUG_STRINGS setted, update statistical information (total CString instantiated).
	* @see Init()
	*/
	CString();
	/**
	* @brief CString destructor.
	*
	* If DEBUG_STRINGS setted, updates statistical information (total CString instantiated).
	*/
	~CString();
	/**
	* @brief Copy constructor.
	*
	* @see Copy()
	* @param pStr string to copy.
	*/
	CString(lpctstr pStr);
	/**
	* @brief Copy constructor.
	*
	* @see Copy()
	* @param pStr string to copy.
	*/
	CString(const CString &s);
	/**
	* @brief Copy supplied string into the CString.
	* @param pStr string to copy.
	* @return the CString.
	*/
	const CString& operator=(lpctstr pStr);
	/**
	* @brief Copy supplied CString into the CString.
	* @param s CString to copy.
	* @return the CString.
	*/
	const CString& operator=(const CString &s);
	///@}
	/** @name Capacity:
	 */
	///@{
	/**
	* @brief Sets length to zero.
	*
	* If bTotal is true, then free the memory allocated. If DEBUG_STRINGS setted, update statistical information (total memory allocated).
	* @param bTotal true for free the allocated memory.
	*/
	void Empty(bool bTotal = false);
	/**
	* @brief Check the length of the CString.
	* @return true if length is 0, false otherwise.
	*/
	bool IsEmpty() const;
	/**
	* @brief Check if there is data allocated and if has zero length.
	* @return false if no data or zero length, true otherwise.
	*/
	bool IsValid() const;
	/**
	* @brief Change the length of the CString.
	*
	* If the new length is lesser than the current lenght, only set a zero at the end of the string.
	* If the new length is bigger than the current length, alloc memory for the string and copy.
	* If DEBUG_STRINGS setted, update statistical information (reallocs count, total memory allocated).
	* @param iLen new length of the string.
	* @return the new length of the CString.
	*/
	int SetLength(int iLen);
	/**
	* @brief Get the length of the CString.
	* @return the length of the CString.
	*/
	int GetLength() const;
	///@}
	/** @name Element access:
	 */
	///@{
	/**
	* @brief Gets the caracter in a specified position (0 based).
	* @see GetAt()
	* @param nIndex position of the character.
	* @return character in position nIndex.
	*/
	tchar operator[](int nIndex) const;
	/**
	* @brief Gets the reference to character a specified position (0 based).
	* @see ReferenceAt()
	* @param nIndex position of the character.
	* @return reference to character in position nIndex.
	*/
	tchar & operator[](int nIndex);
	/**
	* @brief Gets the caracter in a specified position (0 based).
	* @param nIndex position of the character.
	* @return character in position nIndex.
	*/
	tchar GetAt(int nIndex) const;
	/**
	* @brief Gets the reference to character a specified position (0 based).
	* @param nIndex position of the character.
	* @return reference to character in position nIndex.
	*/
	tchar & ReferenceAt(int nIndex);
	/**
	* @brief Puts a character in a specified position (0 based).
	*
	* If character is 0, updates the length of the string (truncated).
	* @param nIndex position to put the character.
	* @param ch character to put.
	*/
	void SetAt(int nIndex, tchar ch);
	///@}
	/** @name Modifiers:
	 */
	///@{
	/**
	* @brief Concatenate CString with a string.
	* @param psz string to concatenate with.
	* @return The result of concatenate the CString with psz.
	*/
	const CString& operator+=(lpctstr psz);
	/**
	* @brief Concatenate CString with a character.
	* @param ch character to concatenate with.
	* @return The result of concatenate the CString with ch.
	*/
	const CString& operator+=(tchar ch);
	/**
	* @brief Adds a char at the end of the CString.
	* @param ch character to add.
	*/
	void Add(tchar ch);
	/**
	* @brief Adds a string at the end of the CString.
	* @parampszStrh string to add.
	*/
	void Add(lpctstr pszStr);
	/**
	* @brief Copy a string into the CString.
	* @see SetLength()
	* @see strcpylen()
	* @param pStr string to copy.
	*/
	void Copy(lpctstr pStr);
	/**
	* @brief Join a formated string (printf like) with values and copy into this.
	* @see FormatV()
	* @param pStr formated string.
	* @param ... list of values.
	*/
	void _cdecl Format(lpctstr pStr, ...) __printfargs(2, 3);
	/**
	* @brief Print a dword value into the string (hex format).
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatHex(dword dwVal);
	/**
	* @brief Print a ullong value into the string (hex format).
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatLLHex(ullong dwVal);
	/**
	* @brief Join a formated string (printf like) with values and copy into this.
	* @param pStr formated string.
	* @param args list of values.
	*/
	void FormatV(lpctstr pStr, va_list args);
	/**
	* @brief Print a char value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatCVal(char iVal);
	/**
	* @brief Print a unsigned char value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatUCVal(uchar iVal);
	/**
	* @brief Print a short value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatSVal(short iVal);
	/**
	* @brief Print a unsigned short value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatUSVal(ushort iVal);
	/**
	* @brief Print a long value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatVal(int iVal);
	/**
	* @brief Print a unsigned int value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatUVal(uint iVal);
	/**
	* @brief Print a llong value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatLLVal(llong iVal);
	/**
	* @brief Print a ullong value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatULLVal(ullong iVal);
	/**
	* @brief Print a size_t (unsigned) value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatSTVal(size_t iVal);
	/**
	* @brief Print a word value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatWVal(word iVal);
	/**
	* @brief Print a dword value into the string.
	* @see Format()
	* @param iVal value to print.
	*/
	void FormatDWVal(dword iVal);
	/**
	* @brief Changes the capitalization of CString to upper.
	*/
	void MakeUpper();
	/**
	* @brief Changes the capitalization of CString to lower.
	*/
	void MakeLower();
	/**
	* @brief Reverses the CString.
	*/
	void Reverse();
	///@}
	/** @name String operations:
	 */
	///@{
	/**
	* @brief cast as const lpcstr.
	* @return internal data pointer.
	*/
	operator lpctstr() const;
	/**
	* @brief Compares the CString to string pStr (strcmp wrapper).
	*
	* This function starts comparing the first character of CString and the string.
	* If they are equal to each other, it continues with the following
	* pairs until the characters differ or until a terminating null-character
	* is reached. This function performs a binary comparison of the characters.
	* @param pStr string to compare.
	* @return <0 if te first character that not match has lower value in CString than in pStr. 0 if hte contents of both are equal. >0 if the first character that does not match has greater value in CString than pStr.
	*/
	int Compare(lpctstr pStr) const;
	/**
	* @brief Compares the CString to string pStr (case insensitive) (_strcmpi wrapper).
	*
	* This function starts comparing the first character of CString and the string.
	* If they are equal to each other, it continues with the following
	* pairs until the characters differ or until a terminating null-character
	* is reached. This function performs a case insensitive comparison of the characters.
	* @param pStr string to compare.
	* @return <0 if te first character that not match has lower value in CString than in pStr. 0 if hte contents of both are equal. >0 if the first character that does not match has greater value in CString than pStr.
	*/
	int CompareNoCase(lpctstr pStr) const;
	/**
	* @brief Gets the internal pointer.
	* @return Pointer to internal data.
	*/
	lpctstr GetPtr() const;
	/**
	* @brief Look for the first occurence of c in CString.
	* @param c character to look for.
	* @return position of the character in CString if any, -1 otherwise.
	*/
	int indexOf(tchar c);
	/**
	* @brief Look for the first occurence of c in CString from a position.
	* @param c character to look for.
	* @param offset position from start the search.
	* @return position of the character in CString if any, -1 otherwise.
	*/
	int indexOf(tchar c, int offset);
	/**
	* @brief Look for the first occurence of a substring in CString.
	* @param str substring to look for.
	* @return position of the substring in CString if any, -1 otherwise.
	*/
	int indexOf(CString str);
	/**
	* @brief Look for the first occurence of a substring in CString from a position.
	* @param str substring to look for.
	* @param offset position from start the search.
	* @return position of the substring in CString if any, -1 otherwise.
	*/
	int indexOf(CString str, int offset);
	/**
	* @brief Look for the last occurence of c in CString.
	* @param c character to look for.
	* @return position of the character in CString if any, -1 otherwise.
	*/
	int lastIndexOf(tchar c);
	/**
	* @brief Look for the last occurence of c in CString from a position to the end.
	* @param c character to look for.
	* @param from position where stop the search.
	* @return position of the character in CString if any, -1 otherwise.
	*/
	int lastIndexOf(tchar c, int from);
	/**
	* @brief Look for the last occurence of a substring in CString.
	* @param str substring to look for.
	* @return position of the substring in CString if any, -1 otherwise.
	*/
	int lastIndexOf(CString str);
	/**
	* @brief Look for the last occurence of a substring in CString from a position to the end.
	* @param str substring to look for.
	* @param from position where stop the search.
	* @return position of the substring in CString if any, -1 otherwise.
	*/
	int lastIndexOf(CString str, int from);
	///@}

private:
	/**
	* @brief Initializes internal data.
	*
	* Allocs STRING_DEFAULT_SIZE by default. If DEBUG_STRINGS setted, updates statistical information (total memory allocated).
	*/
	void Init();

	tchar	*m_pchData;		///< Data pointer.
	int	m_iLength;		///< Length of string.
	int	m_iMaxLength;	///< Size of memory allocated pointed by m_pchData.
};

/**
* match result defines
*/
enum MATCH_TYPE
{
	MATCH_INVALID = 0,
	MATCH_VALID,		///< valid match
	MATCH_END,			///< premature end of pattern string
	MATCH_ABORT,		///< premature end of text string
	MATCH_RANGE,		///< match failure on [..] construct
	MATCH_LITERAL,		///< match failure on literal match
	MATCH_PATTERN		///< bad pattern
};


/** @name String utilities: Modifiers
 */
///@{
/**
* @brief Wrapper to cstring strcpy, but returns the length of the string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @return length of the string copied.
*/
int strcpylen(tchar * pDst, lpctstr pSrc);
/**
* @brief Wrapper to cstring strncpy, but returns the length of string copied.
* @param pDst dest memory space.
* @param pSrc source data.
* @param iMaxSize max data to be coppied.
* @return length of the string copied.
*/
int strcpylen(tchar * pDst, lpctstr pSrc, int imaxlen);
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
int Str_GetBare(tchar * pszOut, lpctstr pszInp, int iMaxSize, lpctstr pszStrip = NULL);
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
* @return new lenght of the string.
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
int FindTable(lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));
/**
* @brief Look for a string in a table (binary search).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableSorted(lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));/**
* @brief Look for a string header in a table (uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHead(lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));
/**
* @brief Look for a string header in a table (binary search, uses Str_CmpHeadI to compare instead of strcmpi).
* @param pFind string we are looking for.
* @param ppTable table where we are looking for the string.
* @param iCount max iterations.
* @param iElemSize size of elements of the table.
* @return the index of string if success, -1 otherwise.
*/
int FindTableHeadSorted(lpctstr pFind, lpctstr const * ppTable, int iCount, int iElemSize = sizeof(lpctstr));
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
bool Str_Parse(tchar * pLine, tchar ** ppArg = NULL, lpctstr pSep = NULL);
/**
* @brief Parse a list of arguments.
* @param pCmdLine list of arguments to parse.
* @param ppCmd where to store the parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, tchar ** ppCmd, int iMax, lpctstr pSep = NULL);
/**
* @brief Parse a list of arguments (integer version).
* @param pCmdLine list of arguments to parse.
* @param piCmd where to store de parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, int64 * piCmd, int iMax, lpctstr pSep = NULL);
/**
* @brief check if a string matches a regex.
* @param pPattern regex to match.
* @param pText text to match against the regex.
* @param lastError if any error, error description is stored here.
* @return 1 is regex is matched, 0 if not, -1 if errors.
*/
int Str_RegExMatch(lpctstr pPattern, lpctstr pText, tchar * lastError);
///@}

void CharToMultiByteNonNull(byte*, const char* , size_t);

// extern tchar * Str_GetTemporary(int amount = 1);
#define Str_GetTemp static_cast<AbstractSphereThread *>(ThreadHolder::current())->allocateBuffer

#endif // _INC_CSTRING_H
