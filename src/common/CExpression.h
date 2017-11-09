/**
* @file CExpression.h
*/

/* Random values have int64 type because the max size, due to the algorithm, can be an int64;
	Other calculations have type llong because in the future llong may be bigger than int64 and
	there's no reason to put an upper limit to the number.	*/

#pragma once
#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H

#include "common.h"
#include "CVarDefMap.h"
#include "ListDefContMap.h"


#define SKIP_SEPARATORS(p)			while ( *(p)=='.' ) { ++(p); }	// || ISWHITESPACE(*(p))
#define SKIP_ARGSEP(p)				while ( *(p)== ',' || IsSpace(*p) ){ ++(p); }
#define SKIP_IDENTIFIERSTRING(p)	while ( _ISCSYM(*p) ){ ++(p); }

#define ISWHITESPACE(ch)			(IsSpace(ch) || ((uchar)ch==0xa0))	// IsSpace
#define GETNONWHITESPACE( pStr )	while ( ISWHITESPACE( (pStr)[0] )) { ++(pStr); }
#define _IS_SWITCH(c)				((c) == '-' || (c) == '/' )	// command line switch.

#define REMOVE_QUOTES( x )			\
{									\
	GETNONWHITESPACE( x );			\
	if ( *x == '"' )				\
		++x;						\
	tchar * psX	= const_cast<tchar*>(strchr( x, '"' ));	\
	if ( psX )						\
		*psX	= '\0';				\
}

#define _ISCSYMF(ch) ( IsAlpha(ch) || (ch)=='_')	// __iscsym or __iscsymf
#define _ISCSYM(ch) ( isalnum(ch) || (ch)=='_')		// __iscsym or __iscsymf


#ifndef M_PI 
	#define M_PI 3.14159265358979323846
#endif

#define EXPRESSION_MAX_KEY_LEN	SCRIPT_MAX_SECTION_LEN


enum DEFMSG_TYPE
{
	#define MSG(a,b) DEFMSG_##a,
	#include "../tables/defmessages.tbl"
	DEFMSG_QTY
};

enum INTRINSIC_TYPE
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

static lpctstr const sm_IntrinsicFunctions[INTRINSIC_QTY+1] =
{
	"ABS",		// absolute
	"ARCCOS",
	"ARCSIN",
	"ARCTAN",
	"COS",		// cosinus
	"ID",		// ID(x) = truncate the type portion of an Id
	"ISNUMBER",		// ISNUMBER(var)
	"ISOBSCENE",	// test for non-allowed strings
	"LOGARITHM",	// log()/log10()
	"NAPIERPOW",	// exp()
	"QVAL",		// QVAL(test1,test2,ret1,ret2,ret3) - test1 ? test2 (< ret1, = ret2, > ret3)
	"RAND",		// RAND(x) = flat random
	"RANDBELL",	// RANDBELL(center,variance25)
	"SIN",
	"SQRT",		// sqrt()
	"StrAscii",
	"STRCMP",	// STRCMP(str1,str2)
	"STRCMPI",	// STRCMPI(str1,str2)
	"StrIndexOf", // StrIndexOf(string,searchVal,[index]) = find the index of this, -1 = not here.
	"STRLEN",	// STRLEN(str)
	"STRMATCH",	// STRMATCH(str,*?pattern)
	"STRREGEX",
	"TAN",		// tan()
	NULL
};

extern class CExpression
{
public:
	static const char *m_sClassName;
	CVarDefMap		m_VarDefs;			// Defined variables in sorted order.
	CVarDefMap		m_VarGlobals;		// Global variables
	CListDefMap		m_ListGlobals;		// Global lists
	CListDefMap		m_ListInternals;	// Internal lists
	CSString		m_sTmp;

	//	defined default messages
	static tchar sm_szMessages[DEFMSG_QTY][128];		// like: "You put %s to %s"
	static lpctstr const sm_szMsgNames[DEFMSG_QTY];		// like: "put_it"

public:
	// Evaluate using the stuff we know.
	llong GetSingle(lpctstr & pArgs);
	int GetRangeVals(lpctstr & pExpr, int64 * piVals, int iMaxQty, bool bNoWarn = false);
	int GetRangeArgsPos(lpctstr & pExpr, lpctstr (&pArgPos)[128][2], int iMaxQty);
	int64 GetRange(lpctstr & pArgs);
	llong GetValMath(llong llVal, lpctstr & pExpr);
	llong GetVal(lpctstr & pArgs);

	// Strict G++ Prototyping produces an error when not casting char*& to const char*&
	// So this is a rather lazy workaround
	llong GetSingle(lptstr &pArgs) {
		return GetSingle(const_cast<lpctstr &>(pArgs));
	}
	int GetRangeVals(lptstr &pExpr, int64 * piVals, int iMaxQty, bool bNoWarn = false) {
		return GetRangeVals(const_cast<lpctstr &>(pExpr), piVals, iMaxQty, bNoWarn);
	}
	int GetRangeArgsPos(lptstr &pExpr, lpctstr (&pArgPos)[128][2], int iMaxQty) {
		return GetRangeArgsPos(const_cast<lpctstr &>(pExpr), pArgPos, iMaxQty);
	}
	int64 GetRange(lptstr &pArgs) {
		return GetRange(const_cast<lpctstr &>(pArgs));
	}
	llong GetVal(lptstr &pArgs) {
		return GetVal(const_cast<lpctstr &>(pArgs));
	}

public:
	CExpression();
	~CExpression();

private:
	CExpression(const CExpression& copy);
	CExpression& operator=(const CExpression& other);
} g_Exp;

bool IsValidDef( lpctstr pszTest );
bool IsValidGameObjDef( lpctstr pszTest );

bool IsSimpleNumberString( lpctstr pszTest );
bool IsStrNumericDec( lpctstr pszTest );
bool IsStrNumeric( lpctstr pszTest );
bool IsStrEmpty( lpctstr pszTest );
bool IsCharNumeric( char & Test );

// Numeric formulas
template<typename T> inline const T SphereAbs(T const & x)
{	// TODO: we can do some specialization
	return (x<0) ? -x : x;
}
int64 Calc_GetRandLLVal( int64 iqty );					// Get a random value between 0 and iQty - 1
int64 Calc_GetRandLLVal2( int64 iMin, int64 iMax );
int32 Calc_GetRandVal( int32 iQty );					// Get a random value between 0 and iQty - 1
int32 Calc_GetRandVal2( int32 iMin, int32 iMax );
int Calc_GetLog2( uint iVal );
int Calc_GetSCurve( int iValDiff, int iVariance );
int Calc_GetBellCurve( int iValDiff, int iVariance );

dword ahextoi( lpctstr pArgs );		// Convert hex string to integer
int64 ahextoi64( lpctstr pArgs );	// Convert hex string to int64

#define Exp_GetSingle( pa )		(int)	g_Exp.GetSingle( pa )
#define Exp_GetLLSingle( pa )			g_Exp.GetSingle( pa )

#define Exp_GetRange( pa )		(int)	g_Exp.GetRange( pa )
#define Exp_GetLLRange( pa )			g_Exp.GetRange( pa )

#define Exp_GetCVal( pa )		(char)	g_Exp.GetVal( pa )
#define Exp_GetUCVal( pa )		(uchar)	g_Exp.GetVal( pa )
#define Exp_GetSVal( pa )		(short)	g_Exp.GetVal( pa )
#define Exp_GetUSVal( pa )		(ushort)g_Exp.GetVal( pa )
#define Exp_GetVal( pa )		(int)	g_Exp.GetVal( pa )
#define Exp_GetUVal( pa )		(uint)	g_Exp.GetVal( pa )
#define Exp_GetLLVal( pa )				g_Exp.GetVal( pa )
#define Exp_GetULLVal( pa )		(ullong)g_Exp.GetVal( pa )
#define Exp_GetBVal( pa )		(byte)	g_Exp.GetVal( pa )
#define Exp_GetWVal( pa )		(word)	g_Exp.GetVal( pa )
#define Exp_GetDWVal( pa )		(dword)	g_Exp.GetVal( pa )

#ifdef _32BITS
	#define Exp_GetSTVal		Exp_GetUVal
#else
	#define Exp_GetSTVal		Exp_GetULLVal
#endif

#endif	// _INC_CEXPRSSION_H
