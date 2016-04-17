/**
* @file CString.cpp
*/

#include <climits>
#include "../sphere/ProfileTask.h"
#include "../sphere/strings.h"
#include "regex/deelx.h"
#include "CExpression.h"
#include "CScript.h"
#include "CString.h"


/**
* @brief Default memory alloc size for CString.
*
* - Empty World! (total strings on start =480,154)
*  - 16 bytes : memory:  8,265,143 [Mem=42,516 K] [reallocations=12,853]
*  - 32 bytes : memory: 16,235,008 [Mem=50,916 K] [reallocations=11,232]
*  - 36 bytes : memory: 17,868,026 [Mem=50,592 K] [reallocations=11,144]
*  - 40 bytes : memory: 19,627,696 [Mem=50,660 K] [reallocations=11,108]
*  - 42 bytes : memory: 20,813,400 [Mem=50,240 K] [reallocations=11,081] BEST
*  - 48 bytes : memory: 23,759,788 [Mem=58,704 K] [reallocations=11,048]
*  - 56 bytes : memory: 27,689,157 [Mem=57,924 K] [reallocations=11,043]
*  - 64 bytes : memory: 31,618,882 [Mem=66,260 K] [reallocations=11,043]
*  - 128 bytes : memory: 62,405,304 [Mem=98,128 K] [reallocations=11,042] <- was in [0.55R4.0.2 - 0.56a]
* - Full World! (total strings on start ~1,388,081)
*  - 16 bytes : memory:  29,839,759 [Mem=227,232 K] [reallocations=269,442]
*  - 32 bytes : memory:  53,335,580 [Mem=250,568 K] [reallocations=224,023]
*  - 40 bytes : memory:  63,365,178 [Mem=249,978 K] [reallocations=160,987]
*  - 42 bytes : memory:  66,120,092 [Mem=249,896 K] [reallocations=153,181] BEST
*  - 48 bytes : memory:  74,865,847 [Mem=272,016 K] [reallocations=142,813]
*  - 56 bytes : memory:  87,050,665 [Mem=273,108 K] [reallocations=141,507]
*  - 64 bytes : memory:  99,278,582 [Mem=295,932 K] [reallocations=141,388]
*  - 128 bytes : memory: 197,114,039 [Mem=392,056 K] [reallocations=141,234] <- was in [0.55R4.0.2 - 0.56a]
*/
#define	STRING_DEFAULT_SIZE	42

//#define DEBUG_STRINGS
#ifdef DEBUG_STRINGS
	uint	gAmount		= 0;		///< Current amount of CString.
	size_t	gMemAmount	= 0;		///< Total mem allocated by CGStrings.
	uint	gReallocs	= 0;		///< Total reallocs caused by CString resizing.
#endif


// CString:: Constructors, Destructor, Asign operator.

CString::CString()
{
#ifdef DEBUG_STRINGS
	gAmount++;
#endif
	Init();
}

CString::~CString()
{
#ifdef DEBUG_STRINGS
	gAmount--;
#endif
	Empty(true);
}

CString::CString(lpctstr pStr)
{
	m_iMaxLength = m_iLength = 0;
	m_pchData = NULL;
	Copy(pStr);
}

CString::CString(const CString &s)
{
	m_iMaxLength = m_iLength = 0;
	m_pchData = NULL;
	Copy(s.GetPtr());
}

const CString& CString::operator=(lpctstr pStr)
{
	Copy(pStr);
	return *this;
}

const CString& CString::operator=(const CString &s)
{
	Copy(s.GetPtr());
	return *this;
}

// CString:: Capacity

void CString::Empty(bool bTotal)
{
	if (bTotal)
	{
		if (m_iMaxLength && m_pchData)
		{
#ifdef DEBUG_STRINGS
			gMemAmount -= m_iMaxLength;
#endif
			delete[] m_pchData;
			m_pchData = NULL;
			m_iMaxLength = 0;
		}
	}
	else m_iLength = 0;
}

bool CString::IsEmpty() const
{
	return (!m_iLength);
}

bool CString::IsValid() const
{
	if (!m_iMaxLength)
		return false;
	return (m_pchData[m_iLength] == '\0');
}

size_t CString::SetLength(size_t iNewLength)
{
	if (iNewLength >= m_iMaxLength)
	{
#ifdef DEBUG_STRINGS
		gMemAmount -= m_iMaxLength;
#endif
		m_iMaxLength = iNewLength + (STRING_DEFAULT_SIZE >> 1);	// allow grow, and always expand only
#ifdef DEBUG_STRINGS
		gMemAmount += m_iMaxLength;
		gReallocs++;
#endif
		tchar	*pNewData = new tchar[m_iMaxLength + 1];
		ASSERT(pNewData);

		size_t iMinLength = minimum(iNewLength, m_iLength);
		strncpy(pNewData, m_pchData, iMinLength);
		pNewData[m_iLength] = 0;

		if (m_pchData)
			delete[] m_pchData;
		m_pchData = pNewData;
	}
	m_iLength = iNewLength;
	m_pchData[m_iLength] = 0;
	return m_iLength;
}

size_t CString::GetLength() const
{
	return m_iLength;
}

// CString:: Element access

tchar CString::operator[](int nIndex) const
{
	return GetAt(nIndex);
}

tchar & CString::operator[](int nIndex)
{
	return ReferenceAt(nIndex);
}

tchar CString::GetAt(int nIndex) const
{
	ASSERT(nIndex <= m_iLength);  // Allow to get the null char.
	return m_pchData[nIndex];
}

tchar & CString::ReferenceAt(int nIndex)
{
	ASSERT(nIndex < m_iLength);
	return m_pchData[nIndex];
}

void CString::SetAt(int nIndex, tchar ch)
{
	ASSERT(nIndex < m_iLength);
	m_pchData[nIndex] = ch;
	if (!ch)
		m_iLength = strlen(m_pchData);	// \0 inserted. line truncated
}

// CString:: Modifiers

const CString& CString::operator+=(lpctstr psz)
{
	Add(psz);
	return(*this);
}

const CString& CString::operator+=(tchar ch)
{
	Add(ch);
	return(*this);
}

void CString::Add(tchar ch)
{
	size_t iLen = m_iLength;
	SetLength(iLen + 1);
	SetAt(iLen, ch);
}

void CString::Add(lpctstr pszStr)
{
	size_t iLenCat = strlen(pszStr);
	if (iLenCat)
	{
		SetLength(iLenCat + m_iLength);
		strcat(m_pchData, pszStr);
		m_iLength = strlen(m_pchData);
	}
}

void CString::Copy(lpctstr pszStr)
{
	if ((pszStr != m_pchData) && pszStr)
	{
		SetLength(strlen(pszStr));
		strcpy(m_pchData, pszStr);
	}
}

void _cdecl CString::Format(lpctstr pStr, ...)
{
	va_list vargs;
	va_start(vargs, pStr);
	FormatV(pStr, vargs);
	va_end(vargs);
}

void CString::FormatHex(dword dwVal)
{
	// In principle, all values in sphere logic are signed...
	// dwVal may contain a (signed) number "big" as the numeric representation of an unsigned ( +(INT_MAX*2) ),
	// but in this case its bit representation would be considered as negative, yet we know it's a positive number.
	// So if it's negative we MUST hexformat it as 64 bit int or reinterpreting it in a
	// script WILL completely mess up
	if (dwVal > (dword)INT_MIN)			// if negative (remember two's complement)
		return FormatLLHex(dwVal);
	Format("0%" PRIx32, dwVal);
}

void CString::FormatLLHex(ullong dwVal)
{
	Format("0%" PRIx64 , dwVal);
}

void CString::FormatCVal(char iVal)
{
	Format("%" PRId8, iVal);
}

void CString::FormatUCVal(uchar iVal)
{
	Format("%" PRIu8, iVal);
}

void CString::FormatSVal(short iVal)
{
	Format("%" PRId16, iVal);
}

void CString::FormatUSVal(ushort iVal)
{
	Format("%" PRIu16, iVal);
}

void CString::FormatVal(int iVal)
{
	Format("%" PRId32, iVal);
}

void CString::FormatUVal(uint iVal)
{
	Format("%" PRIu32, iVal);
}

void CString::FormatLLVal(llong iVal)
{
	Format("%" PRId64 , iVal);
}

void CString::FormatULLVal(ullong iVal)
{
	Format("%" PRIu64 , iVal);
}

void CString::FormatSTVal(size_t iVal)
{
	Format("%" PRIuSIZE_T, iVal);
}

void CString::FormatWVal(word iVal)
{
	Format("0%" PRIx16, iVal);
}

void CString::FormatDWVal(dword iVal)
{
	Format("0%" PRIx32, iVal);
}

void CString::FormatV(lpctstr pszFormat, va_list args)
{
	TemporaryString pszTemp;
	_vsnprintf(static_cast<char *>(pszTemp), pszTemp.realLength(), pszFormat, args);
	Copy(pszTemp);
}

void CString::MakeUpper()
{
	_strupr(m_pchData);
}

void CString::MakeLower()
{
	_strlwr(m_pchData);
}

void CString::Reverse()
{
	STRREV(m_pchData);
}

// CString:: String operations

CString::operator lpctstr() const
{
	return GetPtr();
}

int CString::Compare(lpctstr pStr) const
{
	return strcmp(m_pchData, pStr);
}

int CString::CompareNoCase(lpctstr pStr) const
{
	return strcmpi(m_pchData, pStr);
}

lpctstr CString::GetPtr() const
{
	return m_pchData;
}

ssize_t CString::indexOf(tchar c)
{
	return indexOf(c, 0);
}

ssize_t CString::indexOf(tchar c, size_t offset)
{
	if (offset < 0)
		return -1;	//std::string::npos;

	size_t len = strlen(m_pchData);
	if (offset >= len)
		return -1;

	for (size_t i = offset; i<len; i++)
	{
		if (m_pchData[i] == c)
		{
			return i;
		}
	}
	return (size_t)-1;
}

ssize_t CString::indexOf(CString str)
{
	return indexOf(str, 0);
}

ssize_t CString::indexOf(CString str, size_t offset)
{
	if (offset < 0)
		return -1;

	size_t len = strlen(m_pchData);
	if (offset >= len)
		return -1;

	size_t slen = str.GetLength();
	if (slen > len)
		return -1;

	tchar * str_value = new tchar[slen + 1];
	strcpy(str_value, str.GetPtr());
	tchar firstChar = str_value[0];

	for (size_t i = offset; i < len; i++)
	{
		tchar c = m_pchData[i];
		if (c == firstChar)
		{
			size_t rem = len - i;
			if (rem >= slen)
			{
				size_t j = i;
				size_t k = 0;
				bool found = true;
				while (k < slen)
				{
					if (m_pchData[j] != str_value[k])
					{
						found = false;
						break;
					}
					j++; k++;
				}
				if (found)
				{
					delete[] str_value;
					return i;
				}
			}
		}
	}

	delete[] str_value;
	return (size_t)-1;
}

ssize_t CString::lastIndexOf(tchar c)
{
	return lastIndexOf(c, 0);
}

ssize_t CString::lastIndexOf(tchar c, size_t from)
{
	if (from < 0)
		return -1;

	size_t len = strlen(m_pchData);
	if (from > len)
		return -1;

	for (size_t i = (len - 1); i >= from; i--)
	{
		if (m_pchData[i] == c)
		{
			return i;
		}
	}
	return -1;
}

ssize_t CString::lastIndexOf(CString str)
{
	return lastIndexOf(str, 0);
}

ssize_t CString::lastIndexOf(CString str, size_t from)
{
	if (from < 0)
		return -1;

	size_t len = strlen(m_pchData);
	if (from >= len)
		return -1;
	size_t slen = str.GetLength();
	if (slen > len)
		return -1;

	tchar * str_value = new tchar[slen + 1];
	strcpy(str_value, str.GetPtr());
	tchar firstChar = str_value[0];
	for (size_t i = (len - 1); i >= from; i--)
	{
		tchar c = m_pchData[i];
		if (c == firstChar)
		{
			size_t rem = i;
			if (rem >= slen)
			{
				size_t j = i;
				size_t k = 0;
				bool found = true;
				while (k < slen)
				{
					if (m_pchData[j] != str_value[k])
					{
						found = false;
						break;
					}
					j++; k++;
				}
				if (found)
				{
					delete[] str_value;
					return i;
				}
			}
		}
	}

	delete[] str_value;
	return -1;
}

// CString:: private

void CString::Init()
{
	m_iMaxLength = STRING_DEFAULT_SIZE;
#ifdef DEBUG_STRINGS
	gMemAmount += m_iMaxLength;
#endif
	m_iLength = 0;
	m_pchData = new tchar[m_iMaxLength + 1];
	m_pchData[m_iLength] = 0;
}

// String utilities: Modifiers

size_t strcpylen(tchar * pDst, lpctstr pSrc)
{
	strcpy(pDst, pSrc);
	return strlen(pDst);
}


size_t strcpylen(tchar * pDst, lpctstr pSrc, size_t iMaxSize)
{
	// it does NOT include the iMaxSize element! (just like memcpy)
	// so iMaxSize=sizeof() is ok !
	ASSERT(iMaxSize);
	strncpy(pDst, pSrc, iMaxSize - 1);
	pDst[iMaxSize - 1] = '\0';	// always terminate.
	return strlen(pDst);
}

lpctstr Str_GetArticleAndSpace(lpctstr pszWord)
{
	// NOTE: This is wrong many times.
	//  ie. some words need no article (plurals) : boots.

	// This function shall NOT be called if OF_NoPrefix is set!

	if (pszWord)
	{
		static const tchar sm_Vowels[] = { 'A', 'E', 'I', 'O', 'U' };
		tchar chName = static_cast<tchar>(toupper(pszWord[0]));
		for (size_t x = 0; x < CountOf(sm_Vowels); x++)
		{
			if (chName == sm_Vowels[x])
				return "an ";
		}
	}
	return "a ";
}

size_t Str_GetBare(tchar * pszOut, lpctstr pszInp, size_t iMaxOutSize, lpctstr pszStrip)
{
	// That the client can deal with. Basic punctuation and alpha and numbers.
	// RETURN: Output length.

	if (!pszStrip)
		pszStrip = "{|}~";	// client cant print these.

	//GETNONWHITESPACE( pszInp );	// kill leading white space.

	size_t j = 0;
	for (size_t i = 0; ; i++)
	{
		tchar ch = pszInp[i];
		if (ch)
		{
			if (ch < ' ' || ch >= 127)
				continue;	// Special format chars.

			size_t k = 0;
			while (pszStrip[k] && pszStrip[k] != ch)
				k++;

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

tchar * Str_MakeFiltered(tchar * pStr)
{
	int len = (int)strlen(pStr);
	for (int i = 0; len; i++, len--)
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
			len--;
			memmove(pStr + i + 1, pStr + i + 2, len);
		}
	}
	return pStr;
}

void Str_MakeUnFiltered(tchar * pStrOut, lpctstr pStrIn, int iSizeMax)
{
	int len = (int)strlen(pStrIn);
	int iIn = 0;
	int iOut = 0;
	for (; iOut < iSizeMax && iIn <= len; iIn++, iOut++)
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

size_t Str_TrimEndWhitespace(tchar * pStr, size_t len)
{
	while (len > 0)
	{
		len--;
		if (pStr[len] < 0 || !ISWHITESPACE(pStr[len]))
		{
			++len;
			break;
		}
	}
	pStr[len] = '\0';
	return len;
}

tchar * Str_TrimWhitespace(tchar * pStr)
{
	// TODO: WARNING! Possible Memory Lake here!
	GETNONWHITESPACE(pStr);
	Str_TrimEndWhitespace(pStr, strlen(pStr));
	return pStr;
}

// String utilities: String operations

int FindTable(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
	// A non-sorted table.
	for (int i = 0; i<iCount; i++)
	{
		if (!strcmpi(*ppszTable, pszFind))
			return i;
		ppszTable = (lpctstr const *)(((const byte*)ppszTable) + iElemSize);
	}
	return -1;
}

int FindTableSorted(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
	// Do a binary search (un-cased) on a sorted table.
	// RETURN: -1 = not found
	int iHigh = iCount - 1;
	if (iHigh < 0)
	{
		return -1;
	}
	int iLow = 0;
	while (iLow <= iHigh)
	{
		int i = (iHigh + iLow) / 2;
		lpctstr pszName = *((lpctstr const *)(((const byte*)ppszTable) + (i*iElemSize)));
		int iCompare = strcmpi(pszFind, pszName);
		if (iCompare == 0)
			return i;
		if (iCompare > 0)
		{
			iLow = i + 1;
		}
		else
		{
			iHigh = i - 1;
		}
	}
	return -1;
}

static int Str_CmpHeadI(lpctstr pszFind, lpctstr pszTable)
{
	tchar ch0 = '_';
	for (size_t i = 0; ; i++)
	{
		//	we should always use same case as in other places. since strcmpi lowers,
		//	we should lower here as well. fucking shit!
		tchar ch1 = static_cast<tchar>(tolower(pszFind[i]));
		tchar ch2 = static_cast<tchar>(tolower(pszTable[i]));
		if (ch2 == 0)
		{
			if ((!isalnum(ch1)) && (ch1 != ch0))
				return 0;
			return(ch1 - ch2);
		}
		if (ch1 != ch2)
		{
			return (ch1 - ch2);
		}
	}
}

int FindTableHead(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
	for (int i = 0; i<iCount; i++)
	{
		int iCompare = Str_CmpHeadI(pszFind, *ppszTable);
		if (!iCompare)
			return i;
		ppszTable = (lpctstr const *)( (const byte*)ppszTable + iElemSize );
	}
	return -1;
}

int FindTableHeadSorted(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
	// Do a binary search (un-cased) on a sorted table.
	// RETURN: -1 = not found
	int iHigh = iCount - 1;
	if (iHigh < 0)
	{
		return -1;
	}
	int iLow = 0;
	while (iLow <= iHigh)
	{
		int i = (iHigh + iLow) / 2;
		lpctstr pszName = *((lpctstr const *)(((const byte*)ppszTable) + (i*iElemSize)));
		int iCompare = Str_CmpHeadI(pszFind, pszName);
		if (iCompare == 0)
			return i;
		if (iCompare > 0)
		{
			iLow = i + 1;
		}
		else
		{
			iHigh = i - 1;
		}
	}
	return -1;
}

bool Str_Check(lpctstr pszIn)
{
	if (pszIn == NULL)
		return true;

	lpctstr p = pszIn;
	while (*p != '\0' && (*p != 0x0A) && (*p != 0x0D))
		p++;

	return (*p != '\0');
}

bool Str_CheckName(lpctstr pszIn)
{
	if (pszIn == NULL)
		return true;

	lpctstr p = pszIn;
	while (*p != '\0' &&
		   (
				   ((*p >= 'A') && (*p <= 'Z')) ||
				   ((*p >= 'a') && (*p <= 'z')) ||
				   ((*p >= '0') && (*p <= '9')) ||
				   ((*p == ' ') || (*p == '\'') || (*p == '-') || (*p == '.'))
		   ))
		p++;

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

	for (int i = offset; i < len; i++)
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
					j++; k++;
				}
				if (found)
				{
					return i;
				}
			}
		}
	}

	return -1;
}

static MATCH_TYPE Str_Match_After_Star(lpctstr pPattern, lpctstr pText)
{
	// pass over existing ? and * in pattern
	for (; *pPattern == '?' || *pPattern == '*'; pPattern++)
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

	for (; *pPattern; pPattern++, pText++)
	{
		// if this is the end of the text then this is the end of the match
		if (!*pText)
		{
			return (*pPattern == '*' && *++pPattern == '\0') ?
				   MATCH_VALID : MATCH_ABORT;
		}
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
				pPattern++;
				// check if this is a member match or exclusion match
				bool fInvert = false;             // is this [..] or [!..]
				if (*pPattern == '!' || *pPattern == '^')
				{
					fInvert = true;
					pPattern++;
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
					{
						break;
					}

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
						pPattern++;
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
							pPattern++;
							// if end of text then we have a bad pattern
							if (!*pPattern)
								return MATCH_PATTERN;
						}
						// move to next pattern char
						pPattern++;
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
bool Str_Parse(tchar * pLine, tchar ** ppLine2, lpctstr pszSep)
{
	// Parse a list of args. Just get the next arg.
	// similar to strtok()
	// RETURN: true = the second arg is valid.

	if (pszSep == NULL)	// default sep.
		pszSep = "=, \t";

	// skip leading white space.
	tchar * pNonWhite = pLine;
	GETNONWHITESPACE(pNonWhite);
	if (pNonWhite != pLine)
	{
		memmove(pLine, pNonWhite, strlen(pNonWhite) + 1);
	}

	tchar ch;
	bool bQuotes = false;
	for (; ; pLine++)
	{
		ch = *pLine;
		if (ch == '"')	// quoted argument
		{
			bQuotes = !bQuotes;
			continue;
		}
		if (ch == '\0')	// no args i guess.
		{
			if (ppLine2 != NULL)
			{
				*ppLine2 = pLine;
			}
			return false;
		}
		if (strchr(pszSep, ch) && (bQuotes == false))
			break;
	}

	*pLine++ = '\0';
	if (IsSpace(ch))	// space separators might have other seps as well ?
	{
		GETNONWHITESPACE(pLine);
		ch = *pLine;
		if (ch && strchr(pszSep, ch))
		{
			pLine++;
		}
	}

	// skip leading white space on args as well.
	if (ppLine2 != NULL)
	{
		*ppLine2 = Str_TrimWhitespace(pLine);
	}
	return true;
}

size_t Str_ParseCmds(tchar * pszCmdLine, tchar ** ppCmd, size_t iMax, lpctstr pszSep)
{
	size_t iQty = 0;
	if (pszCmdLine != NULL && pszCmdLine[0] != '\0')
	{
		ppCmd[0] = pszCmdLine;
		iQty++;
		while (Str_Parse(ppCmd[iQty - 1], &(ppCmd[iQty]), pszSep))
		{
			if (++iQty >= iMax)
				break;
		}
	}
	for (size_t j = iQty; j < iMax; j++)
		ppCmd[j] = NULL;	// terminate if possible.
	return iQty;
}

size_t Str_ParseCmds(tchar * pszCmdLine, int64_t * piCmd, size_t iMax, lpctstr pszSep)
{
	tchar * ppTmp[256];
	if (iMax > CountOf(ppTmp))
		iMax = CountOf(ppTmp);

	size_t iQty = Str_ParseCmds(pszCmdLine, ppTmp, iMax, pszSep);
	size_t i;
	for (i = 0; i < iQty; i++)
	{
		piCmd[i] = Exp_GetVal(ppTmp[i]);
	}
	for (; i < iMax; i++)
	{
		piCmd[i] = 0;
	}
	return iQty;
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
	catch (const std::bad_alloc e)
	{
		strcpylen(lastError, e.what(), SCRIPT_MAX_LINE_LEN);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return -1;
	}
	catch (...)
	{
		strcpylen(lastError, "Unknown", SCRIPT_MAX_LINE_LEN);
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return -1;
	}
}

void CharToMultiByteNonNull(byte * Dest, const char * Src, size_t MBytes) {
	for (size_t idx = 0; idx != MBytes * 2; idx += 2) {
		if (Src[idx / 2] == '\0')
			break;
		Dest[idx] = (byte)(Src[idx / 2]);
	}
}
