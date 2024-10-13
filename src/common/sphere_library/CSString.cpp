/**
* @file CSString.cpp
*/

#include "CSString.h"
#include "sstringobjs.h"

#ifndef _WIN32
#include <cstdio>   // for vsnprintf
#endif

#define	STRING_DEFAULT_SIZE	42

//#define DEBUG_STRINGS
#ifdef DEBUG_STRINGS
	uint	gAmount		= 0;		// Current amount of CSString.
	size_t	gMemAmount	= 0;		// Total mem allocated by CGStrings.
	uint	gReallocs	= 0;		// Total reallocs caused by CSString resizing.
#endif


// CSString:: Constructors

CSString::CSString(bool fDefaultInit) :
	m_pchData(nullptr)
{
#ifdef DEBUG_STRINGS
	++gAmount;
#endif
	if (fDefaultInit)
	{
		Init();
	}
	else
	{
		m_iMaxLength = m_iLength = 0;

		// Suppress GCC warning by using the const_cast.
		// I know that i shouldn't set the non-cast buffer to a string constant, but i make sure in every method that i can't modify it.
        // Dirty, but this will not pass IsValid, so it will be reallocated.
		m_pchData = const_cast<char*>("");
	}
}

CSString::CSString(lpctstr pStr) :
	m_pchData(nullptr), m_iLength(0), m_iMaxLength(0)
{
    //Init();
	Copy(pStr);
}

CSString::CSString(lpctstr pStr, int iLen) :
	m_pchData(nullptr), m_iLength(0), m_iMaxLength(0)
{
    //Init();
	CopyLen(pStr, iLen);
}

CSString::CSString(const CSString& s) :
	m_pchData(nullptr), m_iLength(0), m_iMaxLength(0)
{
    //Init();
	Copy(s.GetBuffer());
}


// private
void CSString::Init()
{
	m_iMaxLength = STRING_DEFAULT_SIZE;
#ifdef DEBUG_STRINGS
	gMemAmount += m_iMaxLength;
#endif
	m_iLength = 0;
	if (m_pchData == nullptr)
		m_pchData = new tchar[size_t(m_iMaxLength + 1)];
	m_pchData[m_iLength] = '\0';
}

// CSString:: Capacity

void CSString::Clear(bool fTotal) noexcept
{
	if (fTotal)
	{
		if (m_pchData && m_iMaxLength)
		{
#ifdef DEBUG_STRINGS
			gMemAmount -= m_iMaxLength;
#endif
			delete[] m_pchData;
			m_pchData = nullptr;
			m_iMaxLength = 0;
		}
	}
	m_iLength = 0;
}

bool CSString::IsValid() const noexcept
{
    return ((m_iMaxLength != 0) && (m_pchData != nullptr) && (m_pchData[m_iLength] == '\0'));
}

int CSString::Resize(int iNewLength, bool fPreciseSize)
{
    const bool fValid = IsValid();
    if ((iNewLength >= m_iMaxLength) || !fValid)
	{
#ifdef DEBUG_STRINGS
		gMemAmount -= m_iMaxLength;
#endif

        // allow grow, and expand only
        if (fPreciseSize)
        {
            m_iMaxLength = iNewLength;
        }
        else
        {
            //m_iMaxLength = iNewLength + (STRING_DEFAULT_SIZE >> 1);   // Probably too much...
            m_iMaxLength = (iNewLength * 3) >> 1;   // >> 2 is equal to doing / 2
        }

#ifdef DEBUG_STRINGS
		gMemAmount += m_iMaxLength;
		++gReallocs;
#endif

		tchar *pNewData = new tchar[size_t(m_iMaxLength + 1)]; // new operator throws on error
		if (fValid)
		{
			const int iMinLength = 1 + minimum(iNewLength, m_iLength);
			Str_CopyLimitNull(pNewData, m_pchData, iMinLength);
		}
        if (fValid)
            delete[] m_pchData;
		m_pchData = pNewData;
	}
	ASSERT(m_pchData);
	m_iLength = iNewLength;
	m_pchData[m_iLength] = '\0';
	return m_iLength;
}

void CSString::SetValFalse()
{
    Copy("0");
}

void CSString::SetValTrue()
{
    Copy("1");
}


// CSString:: Element access

void CSString::SetAt(int nIndex, tchar ch)
{
	if (!IsValid())
	{
		Init();
	}
	ASSERT(nIndex < m_iLength);

	m_pchData[nIndex] = ch;
	if (!ch)
	{
		m_iLength = (int)strlen(m_pchData);	// \0 inserted. line truncated
	}
}


// CSString:: Modifiers

void CSString::Add(tchar ch)
{
	const int iLen = m_iLength;
	Resize(iLen + 1);
	SetAt(iLen, ch);
}

void CSString::Add(lpctstr pszStr)
{
	const int iLenCat = (int)strlen(pszStr);
	if (iLenCat)
	{
		Resize(iLenCat + m_iLength);
        m_iLength = (int)Str_ConcatLimitNull(m_pchData, pszStr, m_iLength + 1);
	}
}

void CSString::Copy(lpctstr pszStr)
{
	if ((pszStr != m_pchData) && pszStr)
	{
		const size_t uiLen = strlen(pszStr);
		Resize((int)uiLen, true); // it adds a +1
		Str_CopyLimitNull(m_pchData, pszStr, size_t(uiLen + 1));
	}
}

void CSString::CopyLen(lpctstr pszStr, int iLen)
{
    if ((pszStr != m_pchData) && pszStr)
    {
        Resize(iLen, true); // it adds a +1
        Str_CopyLimitNull(m_pchData, pszStr, size_t(iLen + 1));
    }
}


// CSString:: Operators

const CSString& CSString::operator=(const CSString& s)
{
	Copy(s.GetBuffer());
	return *this;
}

const CSString& CSString::operator=(lpctstr pStr)
{
	Copy(pStr);
	return *this;
}

const CSString& CSString::operator+=(lpctstr string)
{
	Add(string);
	return(*this);
}

const CSString& CSString::operator+=(tchar ch)
{
	Add(ch);
	return(*this);
}

CSString CSString::operator+(lpctstr string)
{
	CSString temp(*this);
	temp += string;
	return temp;
}

CSString& CSString::operator=(CSString&& s) noexcept
{
	if (this != &s)
	{
		if (m_pchData != nullptr)
		{
			delete[] m_pchData;
		}
		m_iLength = s.m_iLength;
		m_iMaxLength = s.m_iMaxLength;
		m_pchData = s.m_pchData;
		s.m_pchData = nullptr;
	}
	return *this;
}

// CSString:: Formatting

void CSString::Format(lpctstr pStr, ...)
{
	va_list vargs;
	va_start(vargs, pStr);
	FormatV(pStr, vargs);
	va_end(vargs);
}

void CSString::FormatV(lpctstr pszFormat, va_list args)
{
	TemporaryString tsTemp;
	vsnprintf(tsTemp.buffer(), tsTemp.capacity(), pszFormat, args);
	Copy(tsTemp.buffer());
}

#define FORMATNUM_WRAPPER(function, arg, base) \
    tchar ptcBuf[24]; \
    Copy(function(arg, ptcBuf, sizeof(ptcBuf), base))

void CSString::FormatLLHex(llong iVal)
{
    //Format("0%" PRIx64, iVal);
    FORMATNUM_WRAPPER(Str_FromLL_Fast, iVal, 16);
}
void CSString::FormatULLHex(ullong uiVal)
{
    //Format("0%" PRIx64, uiVal);
    FORMATNUM_WRAPPER(Str_FromULL_Fast, uiVal, 16);
}
void CSString::FormatHex(dword dwVal)
{
    // As a general rule, all values in sphere logic are signed...
    // dwVal may contain a (signed) number "big" as the numeric representation of an unsigned ( +(INT_MAX*2) ),
    // but in this case its bit representation would be considered as negative, yet we know it's a positive number.
    // So if it's negative we MUST hexformat it as 64 bit int or reinterpreting it in a
    // script WILL completely mess up
    if (dwVal > (dword)INT32_MIN)			// if negative (remember two's complement)
        return FormatULLHex(dwVal);
    //Format("0%" PRIx32, dwVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, dwVal, 16);
}
void CSString::FormatCVal(char iVal)
{
    //Format("%hhd", iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatUCVal(uchar uiVal)
{
    //Format("%hhu", uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::FormatSVal(short iVal)
{
    //Format("%hd", iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatUSVal(ushort uiVal)
{
    //Format("%hu", uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::FormatVal(int iVal)
{
    //Format("%d", iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatUVal(uint uiVal)
{
    //Format("%u", uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::FormatLLVal(llong iVal)
{
    //Format("%lld", iVal);
    FORMATNUM_WRAPPER(Str_FromLL_Fast, iVal, 10);
}
void CSString::FormatULLVal(ullong uiVal)
{
    //Format("%llu", uiVal);
    FORMATNUM_WRAPPER(Str_FromULL_Fast, uiVal, 10);
}
void CSString::FormatSTVal(size_t uiVal)
{
    static_assert(sizeof(size_t) <= sizeof(ullong), "You can't use StrFromULL with a size_t argument on this architecture. Use the old call to Format instead.");
    //Format("%" PRIuSIZE_T, iVal);
    FORMATNUM_WRAPPER(Str_FromULL_Fast, uiVal, 10);
}
void CSString::FormatBVal(byte uiVal)
{
    //Format("0%" PRIx8, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 16);
}
void CSString::FormatWVal(word uiVal)
{
    //Format("0%" PRIx16, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 16);
}
void CSString::FormatDWVal(dword uiVal)
{
    //Format("0%" PRIx32, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 16);
}
void CSString::Format8Val(int8 iVal)
{
    //Format("%" PRId8, iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatU8Val(uint8 uiVal)
{
    //Format("%" PRIu8, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::Format16Val(int16 iVal)
{
    //Format("%" PRId16, iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatU16Val(uint16 uiVal)
{
    //Format("%" PRIu16, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::Format32Val(int32 iVal)
{
    //Format("%" PRId32, iVal);
    FORMATNUM_WRAPPER(Str_FromI_Fast, iVal, 10);
}
void CSString::FormatU32Val(uint32 uiVal)
{
    //Format("%" PRIu32, uiVal);
    FORMATNUM_WRAPPER(Str_FromUI_Fast, uiVal, 10);
}
void CSString::Format64Val(int64 iVal)
{
    //Format("%" PRId64, iVal);
    FORMATNUM_WRAPPER(Str_FromLL_Fast, iVal, 10);
}
void CSString::FormatU64Val(uint64 uiVal)
{
    //Format("%" PRIu64, uiVal);
    FORMATNUM_WRAPPER(Str_FromULL_Fast, uiVal, 10);
}

#undef FORMATNUM_WRAPPER

// CSString:: String operations

tchar CSString::GetAt(int nIndex) const
{
    ASSERT(nIndex >= 0);
    ASSERT(nIndex <= m_iLength);  // Allow to get the null char.
    return m_pchData[nIndex];
}

tchar& CSString::ReferenceAt(int nIndex)
{
    ASSERT(nIndex >= 0);
    ASSERT(nIndex < m_iLength);
    return m_pchData[nIndex];
}

void CSString::MakeUpper() noexcept
{
    _strupr(m_pchData);
}

void CSString::MakeLower() noexcept
{
    _strlwr(m_pchData);
}

void CSString::Reverse() noexcept
{
    Str_Reverse(m_pchData);
}

int CSString::Compare(lpctstr pStr) const noexcept
{
    return strcmp(m_pchData, pStr);
}

int CSString::CompareNoCase(lpctstr pStr) const noexcept
{
    return strcmpi(m_pchData, pStr);
}

int CSString::indexOf(tchar c) noexcept
{
    return indexOf(c, 0);
}

int CSString::indexOf(const CSString& str) noexcept
{
    return indexOf(str, 0);
}

int CSString::lastIndexOf(tchar c) noexcept
{
    return lastIndexOf(c, 0);
}

int CSString::lastIndexOf(const CSString& str) noexcept
{
    return lastIndexOf(str, 0);
}

int CSString::indexOf(tchar c, int offset) noexcept
{
	if ((offset < 0) || !IsValid())
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

int CSString::indexOf(const CSString& str, int offset) noexcept
{
	if ((offset < 0) || !IsValid())
		return -1;

	int len = (int)strlen(m_pchData);
	if (offset >= len)
		return -1;

	int slen = str.GetLength();
	if (slen > len)
		return -1;

	tchar * str_value = new tchar[size_t(slen + 1)];
	Str_CopyLimitNull(str_value, str.GetBuffer(), size_t(slen+1));
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

int CSString::lastIndexOf(tchar c, int from) noexcept
{
	if ((from < 0) || !IsValid())
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

int CSString::lastIndexOf(const CSString& str, int from) noexcept
{
	if ((from < 0) || !IsValid())
		return -1;

	int len = (int)strlen(m_pchData);
	if (from >= len)
		return -1;

	int slen = str.GetLength();
	if (slen > len)
		return -1;

	lpctstr str_value = str.GetBuffer();
	const tchar firstChar = str_value[0];
	for (int i = (len - 1); i >= from; --i)
	{
        const tchar c = m_pchData[i];
		if (c == firstChar)
		{
			if (i >= slen)
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
					return i;
				}
			}
		}
	}

	return -1;
}
