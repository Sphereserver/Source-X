/**
* @file CSString.cpp
*/

#include "CSString.h"
#include "sstringobjs.h"



/**
* @brief Default memory alloc size for CSString.
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
	uint	gAmount		= 0;		// Current amount of CSString.
	size_t	gMemAmount	= 0;		// Total mem allocated by CGStrings.
	uint	gReallocs	= 0;		// Total reallocs caused by CSString resizing.
#endif


// CSString:: Capacity

void CSString::Empty(bool bTotal)
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
	m_iLength = 0;
}

bool CSString::IsValid() const
{
	if (!m_iMaxLength)
		return false;
	return (m_pchData[m_iLength] == '\0');
}

int CSString::SetLength(int iNewLength)
{
	if (iNewLength >= m_iMaxLength)
	{
#ifdef DEBUG_STRINGS
		gMemAmount -= m_iMaxLength;
#endif
		m_iMaxLength = iNewLength + (STRING_DEFAULT_SIZE >> 1);	// allow grow, and always expand only
#ifdef DEBUG_STRINGS
		gMemAmount += m_iMaxLength;
		++gReallocs;
#endif
		tchar *pNewData = new tchar[m_iMaxLength + 1];
		ASSERT(pNewData);

		int iMinLength = minimum(iNewLength, m_iLength);
        ASSERT(m_pchData);
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

// CSString:: Element access

void CSString::SetAt(int nIndex, tchar ch)
{
	ASSERT(nIndex < m_iLength);
	m_pchData[nIndex] = ch;
	if (!ch)
		m_iLength = (int)strlen(m_pchData);	// \0 inserted. line truncated
}

// CSString:: Modifiers

void CSString::Add(tchar ch)
{
	int iLen = m_iLength;
	SetLength(iLen + 1);
	SetAt(iLen, ch);
}

void CSString::Add(lpctstr pszStr)
{
	int iLenCat = (int)strlen(pszStr);
	if (iLenCat)
	{
		SetLength(iLenCat + m_iLength);
		strcat(m_pchData, pszStr);
		m_iLength = (int)strlen(m_pchData);
	}
}

void CSString::Copy(lpctstr pszStr)
{
	if ((pszStr != m_pchData) && pszStr)
	{
		SetLength((int)strlen(pszStr));
		strncpy(m_pchData, pszStr, m_iLength);
	}
}

// CSString:: Formatting

void _cdecl CSString::Format(lpctstr pStr, ...)
{
	va_list vargs;
	va_start(vargs, pStr);
	FormatV(pStr, vargs);
	va_end(vargs);
}

void CSString::FormatV(lpctstr pszFormat, va_list args)
{
	TemporaryString tsTemp;
	tchar* pszTemp = static_cast<tchar *>(tsTemp);
	vsnprintf(pszTemp, tsTemp.realLength(), pszFormat, args);
	Copy(pszTemp);
}

// CSString:: String operations

int CSString::indexOf(tchar c, int offset)
{
	if (offset < 0)
		return -1;

	int len = (int)strlen(m_pchData);
	if (offset >= len)
		return -1;

	for (int i = offset; i<len; ++i)
	{
		if (m_pchData[i] == c)
			return i;
	}
	return -1;
}

int CSString::indexOf(CSString str, int offset)
{
	if (offset < 0)
		return -1;

	int len = (int)strlen(m_pchData);
	if (offset >= len)
		return -1;

	int slen = str.GetLength();
	if (slen > len)
		return -1;

	tchar * str_value = new tchar[slen + 1];
	strcpy(str_value, str.GetPtr());
	tchar firstChar = str_value[0];

	for (int i = offset; i < len; ++i)
	{
		tchar c = m_pchData[i];
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
					if (m_pchData[j] != str_value[k])
					{
						found = false;
						break;
					}
					++j; ++k;
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

int CSString::lastIndexOf(tchar c, int from)
{
	if (from < 0)
		return -1;

	int len = (int)strlen(m_pchData);
	if (from > len)
		return -1;

	for (int i = (len - 1); i >= from; --i)
	{
		if (m_pchData[i] == c)
			return i;
	}
	return -1;
}

int CSString::lastIndexOf(CSString str, int from)
{
	if (from < 0)
		return -1;

	int len = (int)strlen(m_pchData);
	if (from >= len)
		return -1;

	int slen = str.GetLength();
	if (slen > len)
		return -1;

	tchar * str_value = new tchar[slen + 1];
	strcpy(str_value, str.GetPtr());
	tchar firstChar = str_value[0];
	for (int i = (len - 1); i >= from; --i)
	{
		tchar c = m_pchData[i];
		if (c == firstChar)
		{
			int rem = i;
			if (rem >= slen)
			{
				int j = i;
				int k = 0;
				bool found = true;
				while (k < slen)
				{
					if (m_pchData[j] != str_value[k])
					{
						found = false;
						break;
					}
					++j; ++k;
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

// CSString:: private

void CSString::Init()
{
	m_iMaxLength = STRING_DEFAULT_SIZE;
#ifdef DEBUG_STRINGS
	gMemAmount += m_iMaxLength;
#endif
	m_iLength = 0;
	m_pchData = new tchar[m_iMaxLength + 1];
	m_pchData[m_iLength] = 0;
}

