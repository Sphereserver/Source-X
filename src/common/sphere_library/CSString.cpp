/**
* @file CSString.cpp
*/

#include "CSString.h"
#include "sstringobjs.h"


/**
* @brief Default memory alloc size for CSString.
*
* Those tests are OUTDATED!
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


// CSString:: Constructors

CSString::CSString(bool fDefaultInit)
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
		m_pchData = const_cast<char*>("");
	}
}

CSString::CSString(lpctstr pStr)
{
	Init();
	Copy(pStr);
}

CSString::CSString(lpctstr pStr, int iLen)
{
	Init();
	CopyLen(pStr, iLen);
}

CSString::CSString(const CSString& s)
{
	Init();
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
	if (!m_pchData || !m_iMaxLength)
		return false;
	return (m_pchData[m_iLength] == '\0');
}

int CSString::Resize(int iNewLength)
{
	if (iNewLength >= m_iMaxLength)
	{
		const bool fValid = IsValid();
#ifdef DEBUG_STRINGS
		gMemAmount -= m_iMaxLength;
#endif
		m_iMaxLength = iNewLength + (STRING_DEFAULT_SIZE >> 1);	// allow grow, and always expand only
#ifdef DEBUG_STRINGS
		gMemAmount += m_iMaxLength;
		++gReallocs;
#endif
		tchar *pNewData = new tchar[size_t(m_iMaxLength + 1)]; // new operator throws on error
		if (fValid)
		{
			const int iMinLength = minimum(iNewLength, m_iLength + 1);
			Str_CopyLimitNull(pNewData, m_pchData, iMinLength);
			delete[] m_pchData;
		}
		pNewData[m_iLength] = '\0';
		m_pchData = pNewData;
	}
	m_iLength = iNewLength;
	m_pchData[m_iLength] = '\0';
	return m_iLength;
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
		Resize((int)uiLen); // it adds a +1
		Str_CopyLimitNull(m_pchData, pszStr, size_t(uiLen + 1));
	}
}

void CSString::CopyLen(lpctstr pszStr, int iLen)
{
    if ((pszStr != m_pchData) && pszStr)
    {
        Resize(iLen); // it adds a +1
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
