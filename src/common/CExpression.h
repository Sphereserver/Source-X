/**
* @file CExpression.h
* @brief Parse SphereScript expressions.
*/

/* Random values have int64 type because the max size, due to the algorithm, can be an int64;
	Other calculations have type llong because in the future llong may be bigger than int64 and
	there's no reason to put an upper limit to the number.	*/

#ifndef _INC_CEXPRSSION_H
#define _INC_CEXPRSSION_H

#include "sphere_library/CSAssoc.h"
#include "sphere_library/sobjpool.h"
#include "CScriptParserBufs.h"
#include "CVarDefMap.h"
#include "ListDefContMap.h"
#include <array>


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


struct CScriptExprContext
{
    CScriptObj *_pScriptObjI;

    // Recursion counters and state variables
    short _iEvaluate_Conditional_Reentrant{};
    short _iParseScriptText_Reentrant{};
    bool  _fParseScriptText_Brackets{};
};

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


enum SKILL_TYPE	: int;

struct CExprGlobals
{
#ifdef MT_ENGINES
    MT_CMUTEX_DEF;
#endif
    static const char *m_sClassName;

    CVarDefMap		m_VarResDefs;		// Defined variables in sorted order (RESDEF/RESDEF0).
    CVarDefMap		m_VarDefs;			// Defined variables in sorted order (DEF/DEF0).
    CVarDefMap		m_VarGlobals;		// Global variables (VAR/VAR0)
    CListDefMap		m_ListGlobals;		// Global lists
    CListDefMap		m_ListInternals;     // Internal lists

private:
    friend class CServerConfig;
    friend class CScriptObj;    // For DEFMSG.*

    //	Defined default messages (DEFMESSAGEs)
    static constexpr ushort m_kiDefmsgMaxLen = 128;
    static tchar sm_szDefMessages[DEFMSG_QTY][m_kiDefmsgMaxLen];    // like: "You put %s to %s"
    static lpctstr const sm_szDefMsgNames[DEFMSG_QTY];              // like: "put_it"

    static constexpr ushort m_kiSkillTitlesQty = 12;
    std::array<CValStr, m_kiSkillTitlesQty> m_SkillTitles_Ninjitsu;
    std::array<CValStr, m_kiSkillTitlesQty> m_SkillTitles_Bushido;
    std::array<CValStr, m_kiSkillTitlesQty> m_SkillTitles_Generic;

public:
    CExprGlobals();
    ~CExprGlobals() = default;

    void UpdateDefMsgDependentData();

    lpctstr SkillTitle(SKILL_TYPE skill, uint uiVal) const;
};

extern sl::GuardedAccess<CExprGlobals> g_ExprGlobals;  // Declared in spheresvr.cpp


// Main SphereScript parsing utilities
class CExpression
{
    struct CScriptSubExprState
    {
        lptstr ptcStart, ptcEnd;
        enum Type : ushort
        {
            Unknown = 0,
            // Powers of two
            MaybeNestedSubexpr	        = 0x1 << 0, // 001
            TopParenthesizedExpr        = 0x1 << 1, // 002
            None				        = 0x1 << 2, // 004
            BinaryNonLogical	        = 0x1 << 3, // 008
            And					        = 0x1 << 4, // 010
            Or					        = 0x1 << 5  // 020
        };
        ushort uiType;
        ushort uiNonAssociativeOffset; // How much bytes/characters before the start is (if any) the first non-associative operator preceding the subexpression.
    };

    struct CSubExprStatesArena
    {
        static constexpr uint sm_kuiMaxConditionalSubexprsPerExpr = 32;

        CScriptSubExprState m_subexprs[sm_kuiMaxConditionalSubexprsPerExpr];
        uint m_uiQty;
    };

    struct PrvBuffersPool
    {
        // A pool of arenas.
        static constexpr uint sm_subexpr_pool_size = 1'000;
        static constexpr bool sm_allow_fallback_objects = false;
        using CSubExprStatesArenaPool_t = sl::ObjectPool<CSubExprStatesArena, sm_subexpr_pool_size, sm_allow_fallback_objects>;
        CSubExprStatesArenaPool_t m_poolCScriptExprSubStatesPool;
    };

    std::unique_ptr<PrvBuffersPool> _pBufs;
    int _iGetVal_Reentrant;

public:
	static const char *m_sClassName;

	// Evaluate using the stuff we know.
    int64 GetSingle(lpctstr & refStrExpr);

    int64 GetValMath(int64 iVal, lpctstr & refStrExpr);
    int64 GetVal(lpctstr & refStrExpr);

    int GetRangeVals(lpctstr& refStrExpr, int64* piVals, int iMaxQty, bool fNoWarn = false);
    int64 GetRangeNumber(lpctstr& refStrExpr);		// Evaluate a { } range
    CSString GetRangeString(lpctstr& refStrExpr);	// STRRANDRANGE

	// Strict G++ Prototyping produces an error when not casting char*& to const char*&
	// So this is a rather lazy and const-UNsafe workaround
    inline int64 GetSingle(lptstr &refArgs) {
        return GetSingle(const_cast<lpctstr &>(refArgs));
	}
    inline int64 GetVal(lptstr& refArgs) {
        return GetVal(const_cast<lpctstr&>(refArgs));
	}
    inline int GetRangeVals(lptstr &refStrExpr, int64 * piVals, int iMaxQty, bool fNoWarn = false) {
        return GetRangeVals(const_cast<lpctstr &>(refStrExpr), piVals, iMaxQty, fNoWarn);
	}
    inline int64 GetRangeNumber(lptstr &refStrArgs) {
        return GetRangeNumber(const_cast<lpctstr &>(refStrArgs));
	}

    /*
    * @brief Do the first-level parsing of a script line and eventually replace requested values got by r_WriteVal.
    */
    int ParseScriptText( tchar * pszResponse, CScriptExprContext& refExprContext, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole * pSrc, int iFlags = 0 );

    [[nodiscard]]
    bool EvaluateConditionalWhole(lptstr ptcExpression, CScriptExprContext& refExprContext, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc);

private:
    // Arguments inside conditional statements: IF, ELIF, ELSEIF
    [[nodiscard]]
    bool EvaluateConditionalSingle(CScriptSubExprState& refSubExprState, CScriptExprContext& refExprContext, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc);

    [[nodiscard]]
    bool EvaluateConditionalQval(lpctstr ptcKey, CSString& refStrVal, CScriptExprContext& refContext, CScriptTriggerArgsPtr const& pScriptArgs, CTextConsole* pSrc);

    [[nodiscard]]
    PrvBuffersPool::CSubExprStatesArenaPool_t::UniquePtr_t
    GetConditionalSubexpressions(lptstr& refStrExpr, PrvBuffersPool::CSubExprStatesArenaPool_t& bufs_arena);


public:
    CExpression() noexcept;
    ~CExpression() noexcept = default;

	CExpression(const CExpression& copy) = delete;
	CExpression& operator=(const CExpression& other) = delete;

    // One per thread
    [[nodiscard]]
    static CExpression& GetExprParser();
};


uint GetIdentifierString( tchar * szTag, lpctstr pszArgs );

bool IsValidResourceDef( lpctstr pszTest );
bool IsValidGameObjDef( lpctstr pszTest );


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
bool Str_Parse(tchar * pLine, tchar ** ppArg = nullptr, const tchar * pSep = nullptr) noexcept;

/*
 * @brief Totally works like Str_Parse but difference between both function is Str_ParseAdv
 * parsing line with inner quotes and has double quote and apostrophe support.
 */
bool Str_ParseAdv(tchar * pLine, tchar ** ppArg = nullptr, const tchar * pSep = nullptr) noexcept;

/**
* @brief Parse a list of arguments.
* @param pCmdLine list of arguments to parse.
* @param ppCmd where to store the parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, tchar ** ppCmd, int iMax, const tchar * pSep = nullptr) noexcept;

/*
 * @brief Totally works like Str_ParseCmds but difference between both function is Str_ParseCmdsAdv
 * parsing line with inner quotes and has double quote and apostrophe support.
 */
int Str_ParseCmdsAdv(tchar * pCmdLine, tchar ** ppCmd, int iMax, const tchar * pSep = nullptr) noexcept;

/**
* @brief Parse a list of arguments (integer version).
* @param pCmdLine list of arguments to parse.
* @param piCmd where to store de parsed arguments.
* @param iMax max count of arguments to parse.
* @param pSep the list of separators (by default "=, \t").
* @return count of arguments parsed.
*/
int Str_ParseCmds(tchar * pCmdLine, int64 * piCmd, int iMax, const tchar * pSep = nullptr) noexcept;


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

#define Exp_GetSingle( pa )		static_cast<int>	(CExpression::GetExprParser().GetSingle( pa ))
#define Exp_GetUSingle( pa )	static_cast<uint>	(CExpression::GetExprParser().GetSingle( pa ))
#define Exp_GetLLSingle( pa )						 CExpression::GetExprParser().GetSingle( pa )
#define Exp_GetDWSingle( pa )	static_cast<dword>	(CExpression::GetExprParser().GetSingle( pa ))

#define Exp_GetRange( pa )		static_cast<int>	(CExpression::GetExprParser().GetRangeNumber( pa ))
#define Exp_GetLLRange( pa )						 CExpression::GetExprParser().GetRangeNumber( pa )

#define Exp_GetCVal( pa )		static_cast<schar>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetUCVal( pa )		static_cast<uchar>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetSVal( pa )		static_cast<short>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetUSVal( pa )		static_cast<ushort> (CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetVal( pa )		static_cast<int>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetUVal( pa )		static_cast<uint>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetLLVal( pa )							 CExpression::GetExprParser().GetVal( pa )
#define Exp_GetULLVal( pa )		static_cast<ullong> (CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetBVal( pa )		static_cast<byte>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetWVal( pa )		static_cast<word>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetDWVal( pa )		static_cast<dword>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_Get8Val( pa )		static_cast<int8>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_Get16Val( pa )		static_cast<int16>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_Get32Val( pa )		static_cast<int32>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_Get64Val( pa )		static_cast<int64>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetU8Val( pa )		static_cast<uint8>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetU16Val( pa )		static_cast<uint16>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetU32Val( pa )		static_cast<uint32>	(CExpression::GetExprParser().GetVal( pa ))
#define Exp_GetU64Val( pa )		static_cast<uint64>	(CExpression::GetExprParser().GetVal( pa ))

#if INTPTR_MAX == INT32_MAX
	#define Exp_GetSTVal		Exp_GetU32Val
	#define Exp_GetSTSingle		Exp_GetUSingle
#else
	#define Exp_GetSTVal		Exp_GetU64Val
	#define Exp_GetSTSingle(pa) static_cast<size_t> (Exp_GetLLSingle(pa))
#endif

#endif	// _INC_CEXPRSSION_H
