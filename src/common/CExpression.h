/**
* @file CExpression.h
* @brief Parse SphereScript expressions.
*/

/* Random values have int64 type because the max size, due to the algorithm, can be an int64;
	Other calculations have type llong because in the future llong may be bigger than int64 and
	there's no reason to put an upper limit to the number.	*/

#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H

#include "sphere_library/CSRand.h"
#include "CVarDefMap.h"
#include "ListDefContMap.h"


#undef ISWHITESPACE
template <typename T>
inline bool IsWhitespace(const T ch) noexcept {
    if constexpr (std::is_same_v<T, char>) {
        if (static_cast<unsigned char>(ch) == 0xA0)
            return true;
    }
    return IsSpace(ch);
}

#define _IS_SWITCH(ch)		((ch) == '-' || (ch) == '/')	// command line switch.
#define _ISCSYMF(ch)        (IsAlpha(ch) || (ch)=='_')	    // __iscsymf
#define _ISCSYM(ch)         (isalnum(INT_CHARACTER(ch)) || (ch)=='_')    // __iscsym

#define SKIP_SEPARATORS(pStr)		while (*(pStr)=='.') { ++(pStr); }	// || IsWhitespace(*(pStr))
#define SKIP_ARGSEP(pStr)		    while ((*(pStr)==',' || IsSpace( *(pStr)) )) { ++(pStr); }
#define SKIP_IDENTIFIERSTRING(pStr) while (_ISCSYM(*(pStr))) { ++(pStr); }
#define SKIP_NONNUM(pStr)           while (*(pStr) && !isdigit( INT_CHARACTER(*(pStr))) ) { ++(pStr); }
#define SKIP_NONALPHA(pStr)         while (*(pStr) && !IsAlpha( *(pStr)) ) { ++(pStr); }

#define GETNONWHITESPACE(pStr)	    while (IsWhitespace(*(pStr))) { ++(pStr); }

#define REMOVE_QUOTES(x)			\
{									\
	GETNONWHITESPACE(x);			\
	if (*x == '"')				    \
		++x;						\
	tchar * psX	= const_cast<tchar*>(strchr(x,'"'));	\
	if (psX)						\
		*psX = '\0';				\
}

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif


#define EXPRESSION_MAX_KEY_LEN		SCRIPT_MAX_SECTION_LEN
#define VARDEF_FLOAT_MAXBUFFERSIZE	82


enum DEFMSG_TYPE
{
	#define MSG(a,b) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	DEFMSG_QTY
};

enum INTRINSIC_TYPE : int   // Even if it would implicitly be set to int, specify it to silence false warning by UBSanitizer.
{
	INTRINSIC_ABS = 0,
	INTRINSIC_ARCCOS,
	INTRINSIC_ARCSIN,
	INTRINSIC_ARCTAN,
	INTRINSIC_COS,
	INTRINSIC_ID,
	INTRINSIC_ISNUMBER,
	INTRINSIC_ISOBSCENE,
	INTRINSIC_LOGARITHM,
    INTRINSIC_MAX,
    INTRINSIC_MIN,
	INTRINSIC_NAPIERPOW,
	INTRINSIC_QVAL,
	INTRINSIC_RAND,
	INTRINSIC_RANDBELL,
	INTRINSIC_SIN,
	INTRINSIC_SQRT,
	INTRINSIC_STRASCII,
	INTRINSIC_STRCMP,
	INTRINSIC_STRCMPI,
	INTRINSIC_StrIndexOf,
	INTRINSIC_STRLEN,
	INTRINSIC_STRMATCH,
	INTRINSIC_STRREGEX,
	INTRINSIC_TAN,
	INTRINSIC_QTY
};

static lpctstr constexpr sm_IntrinsicFunctions[INTRINSIC_QTY+1] =
{
	"ABS",		    // absolute
	"ARCCOS",
	"ARCSIN",
	"ARCTAN",
	"COS",		    // cosinus
	"ID",		    // ID(x) = truncate the type portion of an Id
	"ISNUMBER",	    // ISNUMBER(var)
	"ISOBSCENE",    // test for non-allowed strings
	"LOGARITHM",    // log()/log10()
    "MAX",
    "MIN",
	"NAPIERPOW",    // exp()
	"QVAL",	        // QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",         // RAND(x) = flat random
	"RANDBELL",     // RANDBELL(center,variance25)
	"SIN",
	"SQRT",         // sqrt()
	"STRASCII",
	"STRCMP",       // STRCMP(str1,str2)
	"STRCMPI",      // STRCMPI(str1,str2)
	"STRINDEXOF",   // StrIndexOf(string,searchVal,[index]) = find the index of this, -1 = not here.
	"STRLEN",       // STRLEN(str)
	"STRMATCH",     // STRMATCH(str,*?pattern)
	"STRREGEX",
	"TAN",          // tan()
	nullptr
};

struct SubexprData
{
	lptstr ptcStart, ptcEnd;
	enum Type : ushort
	{
		Unknown = 0,
		// Powers of two
		MaybeNestedSubexpr	        = 0x1 << 0, // 001
        TopParenthesizedExpr     = 0x1 << 1, // 002
		None				        = 0x1 << 2, // 004
		BinaryNonLogical	        = 0x1 << 3, // 008
		And					        = 0x1 << 4, // 010
		Or					        = 0x1 << 5  // 020
	};
	ushort uiType;
	ushort uiNonAssociativeOffset; // How much bytes/characters before the start is (if any) the first non-associative operator preceding the subexpression.
};

extern class CExpression
{
	short _iGetVal_Reentrant;

public:
	static const char *m_sClassName;
    CVarDefMap		m_VarResDefs;		// Defined variables in sorted order (RESDEF/RESDEF0).
	CVarDefMap		m_VarDefs;			// Defined variables in sorted order (DEF/DEF0).
	CVarDefMap		m_VarGlobals;		// Global variables (VAR/VAR0)
	CListDefMap		m_ListGlobals;		// Global lists
	CListDefMap		m_ListInternals;	// Internal lists

	//	defined default messages
#define DEFMSG_MAX_LEN 128
	static tchar sm_szMessages[DEFMSG_QTY][DEFMSG_MAX_LEN];		// like: "You put %s to %s"
	static lpctstr const sm_szMsgNames[DEFMSG_QTY];		// like: "put_it"

public:
	// Evaluate using the stuff we know.
	llong GetSingle(lpctstr & pExpr);

	llong GetValMath(llong llVal, lpctstr & pExpr);
	llong GetVal(lpctstr & pExpr);

	int GetRangeVals(lpctstr& pExpr, int64* piVals, int iMaxQty, bool bNoWarn = false);
	int64 GetRangeNumber(lpctstr& pExpr);		// Evaluate a { } range
	CSString GetRangeString(lpctstr& pExpr);	// STRRANDRANGE

	static int GetConditionalSubexpressions(lptstr& pExpr, SubexprData(&psSubexprData)[32], int iMaxQty);

	// Strict G++ Prototyping produces an error when not casting char*& to const char*&
	// So this is a rather lazy and const-UNsafe workaround
	inline llong GetSingle(lptstr &pArgs) {
		return GetSingle(const_cast<lpctstr &>(pArgs));
	}
	inline llong GetVal(lptstr& pArgs) {
		return GetVal(const_cast<lpctstr&>(pArgs));
	}
	inline int GetRangeVals(lptstr &pExpr, int64 * piVals, int iMaxQty, bool bNoWarn = false) {
		return GetRangeVals(const_cast<lpctstr &>(pExpr), piVals, iMaxQty, bNoWarn);
	}
	inline int64 GetRangeNumber(lptstr &pArgs) {
		return GetRangeNumber(const_cast<lpctstr &>(pArgs));
	}


public:
	CExpression();
	~CExpression();

	CExpression(const CExpression& copy) = delete;
	CExpression& operator=(const CExpression& other) = delete;
} g_Exp;


uint GetIdentifierString( tchar * szTag, lpctstr pszArgs );

bool IsValidResourceDef( lpctstr pszTest );
bool IsValidGameObjDef( lpctstr pszTest );

bool IsSimpleNumberString( lpctstr pszTest );
bool IsStrNumericDec( lpctstr pszTest );
bool IsStrNumeric( lpctstr pszTest );
bool IsStrEmpty( lpctstr pszTest );

// strncpy does not always return the actual amount of bytes written. this doesn't count the string terminator.
int StrncpyCharBytesWritten(int iBytesToWrite, size_t uiBufSize, bool fPrintError = true);

// Numeric formulas
template<typename T> inline T SphereAbs(const T x) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Invalid data type.");
    static_assert(std::is_signed_v<T>, "Trying to get the absolute value of an unsigned number?");

    // This need to be the fastest (without optimizations this might be slower than calling the plain *abs function ?).
    if constexpr (std::is_same_v<T, int8_t>)
        return static_cast<int8_t>(abs(static_cast<int32_t>(x)));
    else if constexpr (std::is_same_v<T, int16_t>)
        return static_cast<int16_t>(abs(static_cast<int32_t>(x)));
    else if constexpr (std::is_same_v<T, int32_t>)
        return abs(x);
	else
	{
		static_assert (std::is_same_v<T, int64_t>, "Forgot to cast the type to a fixed size integer like int32_t? Or trying to use this on an unsigned number?");
		return llabs(x);
	}

    // This is good, but standard C functions might be even faster
	// return (x<0) ? -x : x;
}

int Calc_GetLog2( uint iVal );
int Calc_GetSCurve( int iValDiff, int iVariance );
int Calc_GetBellCurve( int iValDiff, int iVariance );

dword ahextoi( lpctstr pArgs );		// Convert decimal or (Sphere) hex string (staring with 0, not 0x) to integer
int64 ahextoi64( lpctstr pArgs );	// Convert decimal or (Sphere) hex string (staring with 0, not 0x) to int64

#define Exp_GetSingle( pa )		static_cast<int>	(g_Exp.GetSingle( pa ))
#define Exp_GetUSingle( pa )	static_cast<uint>	(g_Exp.GetSingle( pa ))
#define Exp_GetLLSingle( pa )						 g_Exp.GetSingle( pa )
#define Exp_GetDWSingle( pa )	static_cast<dword>	(g_Exp.GetSingle( pa ))

#define Exp_GetRange( pa )		static_cast<int>	(g_Exp.GetRangeNumber( pa ))
#define Exp_GetLLRange( pa )						 g_Exp.GetRangeNumber( pa )

#define Exp_GetCVal( pa )		static_cast<char>	(g_Exp.GetVal( pa ))
#define Exp_GetUCVal( pa )		static_cast<uchar>	(g_Exp.GetVal( pa ))
#define Exp_GetSVal( pa )		static_cast<short>	(g_Exp.GetVal( pa ))
#define Exp_GetUSVal( pa )		static_cast<ushort> (g_Exp.GetVal( pa ))
#define Exp_GetVal( pa )		static_cast<int>	(g_Exp.GetVal( pa ))
#define Exp_GetUVal( pa )		static_cast<uint>	(g_Exp.GetVal( pa ))
#define Exp_GetLLVal( pa )							 g_Exp.GetVal( pa )
#define Exp_GetULLVal( pa )		static_cast<ullong> (g_Exp.GetVal( pa ))
#define Exp_GetBVal( pa )		static_cast<byte>	(g_Exp.GetVal( pa ))
#define Exp_GetWVal( pa )		static_cast<word>	(g_Exp.GetVal( pa ))
#define Exp_GetDWVal( pa )		static_cast<dword>	(g_Exp.GetVal( pa ))
#define Exp_Get8Val( pa )		static_cast<int8>	(g_Exp.GetVal( pa ))
#define Exp_Get16Val( pa )		static_cast<int16>	(g_Exp.GetVal( pa ))
#define Exp_Get32Val( pa )		static_cast<int32>	(g_Exp.GetVal( pa ))
#define Exp_Get64Val( pa )		static_cast<int64>	(g_Exp.GetVal( pa ))
#define Exp_GetU8Val( pa )		static_cast<uint8>	(g_Exp.GetVal( pa ))
#define Exp_GetU16Val( pa )		static_cast<uint16>	(g_Exp.GetVal( pa ))
#define Exp_GetU32Val( pa )		static_cast<uint32>	(g_Exp.GetVal( pa ))
#define Exp_GetU64Val( pa )		static_cast<uint64>	(g_Exp.GetVal( pa ))

#if INTPTR_MAX == INT32_MAX
	#define Exp_GetSTVal		Exp_GetU32Val
	#define Exp_GetSTSingle		Exp_GetUSingle
#else
	#define Exp_GetSTVal		Exp_GetU64Val
	#define Exp_GetSTSingle(pa) static_cast<size_t> (Exp_GetLLSingle(pa))
#endif

#endif	// _INC_CEXPRSSION_H
