#include "../game/CServerConfig.h"
#include "sphere_library/CSRand.h"
#include "CException.h"
#include "CExpression.h"
#include <algorithm>
#include <complex>
#include <cmath>

tchar CExpression::sm_szMessages[DEFMSG_QTY][DEFMSG_MAX_LEN] =
{
	#define MSG(a,b) b,
	#include "../tables/defmessages.tbl"
};

lpctstr const CExpression::sm_szMsgNames[DEFMSG_QTY] =
{
	#define MSG(a,b) #a,
	#include "../tables/defmessages.tbl"
};

dword ahextoi( lpctstr pszStr ) // Convert decimal or hex string to 32 bit integer
{
	// Unfortunatly the library func cant handle the number FFFFFFFF
	// tchar * sstop; return( strtol( s, &sstop, 16 ));

	if ( pszStr == nullptr )
		return 0;
	bool bHex = false;

	GETNONWHITESPACE(pszStr);

	if ( *pszStr == '0' )
	{
		if (*++pszStr != '.')
			bHex = true;
		--pszStr;
	}

	dword val = 0;
	for (;;)
	{
		tchar ch = static_cast<tchar>(toupper(*pszStr));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( bHex && ( ch >= 'A' ) && ( ch <= 'F' ))
		{
			ch -= 'A' - 10;
		}
		else if ( !bHex && ( ch == '.' ) )
		{
			++ pszStr;
			continue;
		}
		else
			break;

		val *= ( bHex ? 0x10 : 10 );
		val += ch;
		++ pszStr;
	}
	return val;
}

int64 ahextoi64( lpctstr pszStr ) // Convert decimal or hex string to int64
{
	if ( pszStr == nullptr )
		return 0;
	bool bHex = false;

	GETNONWHITESPACE(pszStr);

	if ( *pszStr == '0' )
	{
		if (*++pszStr != '.')
			bHex = true;
		--pszStr;
	}

	int64 val = 0;
    ushort ndigits = 0;
	for (;;)
	{
		tchar ch = static_cast<tchar>(toupper(*pszStr));
		if ( IsDigit(ch) )
			ch -= '0';
		else if ( bHex && ( ch >= 'A' ) && ( ch <= 'F' ))
		{
			ch -= 'A' - 10;
            ++ndigits;
		}
		else if ( !bHex && ( ch == '.' ) )
		{
			++pszStr;
			continue;
		}
		else
			break;

		val *= ( bHex ? 0x10 : 10 );
		val += ch;
		++pszStr;
	}
    if (ndigits <= 8)
        return (llong)(int)val;

	return val;
}

int StrncpyCharBytesWritten(int iBytesToWrite, size_t uiBufSize, bool fPrintError)
{
    if (iBytesToWrite < 0)
        return 0;
    if (uiBufSize < 1)
        goto err;
    if ((uint)iBytesToWrite >= uiBufSize - 1)
        goto err;
    return iBytesToWrite;

err:
    //throw CSError(LOGL_ERROR, 0, "Buffer size too small for snprintf.\n");
    if (fPrintError) {
        g_Log.EventError("Buffer size too small for snprintf.\n");
    }
    return (uiBufSize > 1) ? int(uiBufSize - 1) : 0; // Bytes written, excluding the string terminator.
}

llong power(llong base, llong level)
{
	double rc = pow((double)base, (double)level);
	return (llong)rc;
}

bool IsStrEmpty( lpctstr pszTest )
{
	if ( !pszTest || !*pszTest )
		return true;

	do
	{
		if ( !IsSpace(*pszTest) )
			return false;
	}
	while ( *(++pszTest) );
	return true;
}

bool IsStrNumericDec( lpctstr pszTest )
{
	if ( !pszTest || !*pszTest )
		return false;

	do
	{
		if ( !IsDigit(*pszTest) )
			return false;
	}
	while ( *(++pszTest) );

	return true;
}


bool IsStrNumeric( lpctstr pszTest )
{
	if ( !pszTest || !*pszTest )
		return false;

	bool fHex = false;
	if ( pszTest[0] == '0' )
		fHex = true;

	do
	{
		if ( IsDigit( *pszTest ) )
			continue;
		if ( fHex && tolower(*pszTest) >= 'a' && tolower(*pszTest) <= 'f' )
			continue;
		return false;
	}
	while ( *(++pszTest) );
	return true;
}

bool IsSimpleNumberString( lpctstr pszTest )
{
	// is this a string or a simple numeric expression ?
	// string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
	// number = 1.0+-\*~|&!%^()2, 0aed, 123

    if (*pszTest == '\0')
        return false;   // empty string, no number

	bool fMathSep			= true;	// last non whitespace was a math sep.
	bool fHextDigitStart	= false;
	bool fWhiteSpace		= false;

	for ( ; ; ++pszTest )
	{
		tchar ch = *pszTest;
		if ( ! ch )
			return true;

		if (( ch >= 'A' && ch <= 'F') || ( ch >= 'a' && ch <= 'f' ))	// isxdigit
		{
			if ( ! fHextDigitStart )
				return false;

			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( IsSpace( ch ) )
		{
			fHextDigitStart = false;
			fWhiteSpace = true;
			continue;
		}
		if ( IsDigit( ch ) )
		{
			if ( fWhiteSpace && ! fMathSep )
				return false;

			if ( ch == '0' )
				fHextDigitStart = true;
			fWhiteSpace = false;
			fMathSep = false;
			continue;
		}
		if ( ch == '/' && pszTest[1] != '/' )
			fMathSep = true;
		else
			fMathSep = strchr("+-\\*~|&!%^()", ch ) ? true : false ;

		if ( ! fMathSep )
			return false;

		fHextDigitStart = false;
		fWhiteSpace = false;
	}
}

uint GetIdentifierString( tchar * szTag, lpctstr pszArgs )
{
	// Copy the identifier (valid char set) out to this buffer.
    uint i = 0;
	for ( ; pszArgs[i]; ++i )
	{
		if ( ! _ISCSYM(pszArgs[i]))
			break;
		if ( i >= EXPRESSION_MAX_KEY_LEN )
			return 0;
		szTag[i] = pszArgs[i];
	}

	szTag[i] = '\0';
	return i;
}

bool IsValidResourceDef( lpctstr ptcTest )
{
	return (nullptr != g_Exp.m_VarResDefs.CheckParseKey( ptcTest ));
}

bool IsValidGameObjDef( lpctstr ptcTest )
{
	if (!IsSimpleNumberString(ptcTest))
	{
		const CVarDefCont * pVarBase = g_Exp.m_VarResDefs.CheckParseKey( ptcTest );
		if ( pVarBase == nullptr )
			return false;

		const tchar ch = *pVarBase->GetValStr();
		if ( !ch || (ch == '<') )
			return false;

		const CResourceID rid = g_Cfg.ResourceGetID(RES_QTY, ptcTest);
        const RES_TYPE resType = rid.GetResType();
		if (resType == RES_QTY)
			return true; // It's just a numerical value, so we can't infer the resource type. Suppose it's a valid ID.
		if ((resType != RES_CHARDEF) && (resType != RES_ITEMDEF) && (resType != RES_SPAWN) && (resType != RES_TEMPLATE) && (resType != RES_CHAMPION))
			return false;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////
// -Calculus

int Calc_GetLog2( uint iVal )
{
	// This is really log2 + 1
	int i = 0;
	for ( ; iVal ; ++i )
	{
		ASSERT( i < 32 );
		iVal >>= 1 ;
	}
	return i;
}

int Calc_GetBellCurve( int iValDiff, int iVariance )
{
	// Produce a log curve.
	//
	// 50+
	//	 |
	//	 |
	//	 |
	// 25|  +
	//	 |
	//	 |	   +
	//	 |		  +
	//	0 --+--+--+--+------
	//    iVar				iValDiff
	//
	// ARGS:
	//  iValDiff = Given a value relative to 0
	//		0 = 50.0% chance.
	//  iVariance = the 25.0% point of the bell curve
	// RETURN:
	//  (0-100.0) % chance at this iValDiff.
	//  Chance gets smaller as Diff gets bigger.
	// EXAMPLE:
	//  if ( iValDiff == iVariance ) return( 250 )
	//  if ( iValDiff == 0 ) return( 500 );
	//

	if ( iVariance <= 0 )	// this really should not happen but just in case.
		return 500;
	if ( iValDiff < 0 )
        iValDiff = -iValDiff;

	int iChance = 500;
	while ( iValDiff > iVariance && iChance )
	{
		iValDiff -= iVariance;
		iChance /= 2;	// chance is halved for each Variance period.
	}

	return ( iChance - IMulDiv( iChance/2, iValDiff, iVariance ) );
}

int Calc_GetSCurve( int iValDiff, int iVariance )
{
	// ARGS:
	//   iValDiff = Difference between our skill level and difficulty.
	//		positive = high chance, negative = lower chance
	//		0 = 50.0% chance.
	//   iVariance = the 25.0% difference point of the bell curve
	// RETURN:
	//	 what is the (0-100.0)% chance of success = 0-1000
	// NOTE:
	//   Chance of skill gain is inverse to chance of success.
	//
	int iChance = Calc_GetBellCurve( iValDiff, iVariance );
	if ( iValDiff > 0 )
		return ( 1000 - iChance );
	return iChance;
}

CExpression::CExpression()
{
	_iGetVal_Reentrant = 0;
}

CExpression::~CExpression()
{
}

llong CExpression::GetSingle( lpctstr & pszArgs )
{
	ADDTOCALLSTACK("CExpression::GetSingle");
	// Parse just a single expression without any operators or ranges.
	ASSERT(pszArgs);
	GETNONWHITESPACE( pszArgs );

	lpctstr ptcStartingString = pszArgs;
	if (pszArgs[0]=='.')
		++pszArgs;

	if ( pszArgs[0] == '0' )	// leading '0' = hex value.
	{
		// A hex value.
		if ( pszArgs[1] == '.' )	// leading 0. means it really is decimal.
		{
			pszArgs += 2;
			goto try_dec;
		}

		lpctstr pStart = pszArgs;
		llong val = 0;
        ushort ndigits = 0;
		while (true)
		{
			tchar ch = *pszArgs;
			if ( IsDigit(ch) )
				ch -= '0';
			else
			{
				ch = static_cast<tchar>(tolower(ch));
				if ( ch > 'f' || ch < 'a' )
				{
					if ( ch == '.' && pStart[0] != '0' )	// ok i'm confused. it must be decimal.
					{
						pszArgs = pStart;
						goto try_dec;
					}
					break;
				}
				ch -= 'a' - 10;
                ++ ndigits;
			}
			val *= 0x10;
			val += ch;
            ++ pszArgs;
		}
        if (ndigits <= 8)
            return (llong)(int)val;
		return val;
	}
	else if ( pszArgs[0] == '.' || IsDigit(pszArgs[0]) )
	{
		// A decimal number
try_dec:
		llong iVal = 0;
		for ( ; ; ++pszArgs )
		{
			if ( *pszArgs == '.' )
				continue;	// just skip this.
			if ( ! IsDigit(*pszArgs) )
				break;
			iVal *= 10;
			iVal += (llong)(*pszArgs) - '0';
		}
		return iVal;
	}
	else if ( ! _ISCSYMF(pszArgs[0]) )
	{
	#pragma region maths
		// some sort of math op ?

		switch ( pszArgs[0] )
		{
		case '{':
			++pszArgs;
			return GetRangeNumber( pszArgs );
		case '[':
		case '(': // Parse out a sub expression.
			++pszArgs;
			return GetVal( pszArgs );
		case '+':
			++pszArgs;
			break;
		case '-':
			++pszArgs;
			return -GetSingle( pszArgs );
		case '~':	// Bitwise not.
			++pszArgs;
			return ~GetSingle( pszArgs );
		case '!':	// boolean not.
			++pszArgs;
			if ( pszArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
			{
				++pszArgs;		// so just skip it. and compare it to 0
				return GetSingle( pszArgs );
			}
			return !GetSingle( pszArgs );
		case ';':	// seperate field.
		case ',':	// seperate field.
		case '\0':
			return 0;
		}
#pragma endregion maths
	}
	else
	#pragma region intrinsics
	{
		// Symbol or intrinsinc function ?

		INTRINSIC_TYPE iIntrinsic = (INTRINSIC_TYPE) FindTableHeadSorted( pszArgs, sm_IntrinsicFunctions, ARRAY_COUNT(sm_IntrinsicFunctions)-1 );
		if ( iIntrinsic >= 0 )
		{
			size_t iLen = strlen(sm_IntrinsicFunctions[iIntrinsic]);
			if ( strchr("( ", pszArgs[iLen]) )
			{
				pszArgs += (iLen + 1);
				tchar * pszArgsNext;
				Str_Parse( const_cast<tchar*>(pszArgs), &(pszArgsNext), ")" );

				tchar * ppCmd[5];
				llong iResult;
				int iCount = 0;

				switch ( iIntrinsic )
				{
					case INTRINSIC_ID:
					{
						if ( *pszArgs )
						{
							iCount = 1;
                            iResult = ResGetIndex((dword)GetVal(pszArgs));
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

                    case INTRINSIC_MAX:
                    {
                        iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
                        if ( iCount < 2 )
                            iResult = 0;
                        else
                        {
                            const int64 iVal1 = GetVal(ppCmd[0]), iVal2 = GetVal(ppCmd[1]);
                            iResult = maximum(iVal1, iVal2);
                        }
                    } break;

                    case INTRINSIC_MIN:
                    {
                        iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
                        if ( iCount < 2 )
                            iResult = 0;
                        else
                        {
                            const int64 iVal1 = GetVal(ppCmd[0]), iVal2 = GetVal(ppCmd[1]);
                            iResult = minimum(iVal1, iVal2);
                        }
                    } break;

					case INTRINSIC_LOGARITHM:
					{
						iCount = 0;
						iResult = 0;

						if ( *pszArgs )
						{
							llong iArgument = GetVal(pszArgs);
							if ( iArgument <= 0 )
							{
								DEBUG_ERR(( "Exp_GetVal: (x)Log(%lld) is %s\n", iArgument, (!iArgument ? "infinite" : "undefined") ));
							}
							else
							{
								iCount = 1;

								if ( strchr(pszArgs, ',') )
								{
									++iCount;
									SKIP_ARGSEP(pszArgs);
									if ( !strcmpi(pszArgs, "e") )
									{
										iResult = (llong)log( (double)iArgument );
									}
									else if ( !strcmpi(pszArgs, "pi") )
									{
										iResult = (llong)(log( (double)iArgument ) / log( M_PI ) );
									}
									else
									{
										llong iBase = GetVal(pszArgs);
										if ( iBase <= 0 )
										{
											DEBUG_ERR(( "Exp_GetVal: (%lld)Log(%lld) is %s\n", iBase, iArgument, (!iBase ? "infinite" : "undefined") ));
											iCount = 0;
										}
										else
											iResult = (llong)(log( (double)iArgument ) / log( (double)iBase ));
									}
								}
								else
									iResult = (llong)log10( (double)iArgument );
							}
						}

					} break;

					case INTRINSIC_NAPIERPOW:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)exp( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_SQRT:
					{
						iCount = 0;
						iResult = 0;

						if ( *pszArgs )
						{
							llong iTosquare = GetVal(pszArgs);

							if (iTosquare >= 0)
							{
								++iCount;
								iResult = (llong)sqrt( (double)iTosquare );
							}
							else
							{
								++iCount;
								std::complex<double> number((double)iTosquare, 0);
								std::complex<double> result = sqrt(number);
								iResult = (llong)result.real();
							}
						}

					} break;

					case INTRINSIC_SIN:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)sin( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_ARCSIN:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)asin( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_COS:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)cos( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_ARCCOS:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)acos( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_TAN:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)tan( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_ARCTAN:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = (llong)atan( (double)GetVal( pszArgs ) );
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}

					} break;

					case INTRINSIC_StrIndexOf:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 3, "," );
						if ( iCount < 2 )
							iResult = -1;
						else
							iResult = Str_IndexOf( ppCmd[0] , ppCmd[1] , (iCount==3)?(int)GetVal(ppCmd[2]):0 );
					} break;

					case INTRINSIC_STRMATCH:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = (Str_Match( ppCmd[0], ppCmd[1] ) == MATCH_VALID ) ? 1 : 0;
					} break;

					case INTRINSIC_STRREGEX:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 0;
						else
						{
							tchar * tLastError = Str_GetTemp();
							iResult = Str_RegExMatch( ppCmd[0], ppCmd[1], tLastError );
							if ( iResult == -1 )
							{
								DEBUG_ERR(( "STRREGEX bad function usage. Error: %s\n", tLastError ));
							}
						}
					} break;

					case INTRINSIC_RANDBELL:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 0;
						else
							iResult = Calc_GetBellCurve( (int)GetVal( ppCmd[0] ), (int)GetVal( ppCmd[1] ) );
					} break;

					case INTRINSIC_STRASCII:
					{
						if ( *pszArgs )
						{
							iCount = 1;
							iResult = pszArgs[0];
						}
						else
						{
							iCount = 0;
							iResult = 0;
						}
					} break;

					case INTRINSIC_RAND:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount <= 0 )
							iResult = 0;
						else
						{
							int64 val1 = GetVal( ppCmd[0] );
							if ( iCount == 2 )
							{
								int64 val2 = GetVal( ppCmd[1] );
								iResult = g_Rand.GetLLVal2( val1, val2 );
							}
							else
								iResult = g_Rand.GetLLVal(val1);
						}
					} break;

					case INTRINSIC_STRCMP:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmp(ppCmd[0], ppCmd[1]);
					} break;

					case INTRINSIC_STRCMPI:
					{
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 2, "," );
						if ( iCount < 2 )
							iResult = 1;
						else
							iResult = strcmpi(ppCmd[0], ppCmd[1]);
					} break;

					case INTRINSIC_STRLEN:
					{
						iCount = 1;
						iResult = strlen(pszArgs);
					} break;

					case INTRINSIC_ISOBSCENE:
					{
						iCount = 1;
						iResult = g_Cfg.IsObscene( pszArgs );
					} break;

					case INTRINSIC_ISNUMBER:
					{
						iCount = 1;
                        SKIP_NONNUM( pszArgs );
						iResult = IsStrNumeric( pszArgs );
					} break;

					case INTRINSIC_QVAL:
					{
						// Here is handled the intrinsic QVAL form: QVAL(VALUE1,VALUE2,LESSTHAN,EQUAL,GREATERTHAN)
						iCount = Str_ParseCmds( const_cast<tchar*>(pszArgs), ppCmd, 5, "," );
						if (iCount < 3)
						{
							iResult = 0;
						}
						else
						{
							const llong a1 = GetSingle(ppCmd[0]);
							const llong a2 = GetSingle(ppCmd[1]);
							if (a1 < a2)
							{
								iResult = GetSingle(ppCmd[2]);
							}
							else if (a1 == a2)
							{
								iResult = (iCount < 4) ? 0 : GetSingle(ppCmd[3]);
							}
							else
							{
								iResult = (iCount < 5) ? 0 : GetSingle(ppCmd[4]);
							}
						}
					} break;

					case INTRINSIC_ABS:
					{
						iCount = 1;
						iResult = llabs(GetVal(pszArgs));
					} break;

					default:
						iCount = 0;
						iResult = 0;
						break;
				}

				pszArgs = pszArgsNext;

				if ( !iCount )
				{
					DEBUG_ERR(( "Bad intrinsic function usage: Missing arguments\n" ));
					return 0;
				}
				else
					return iResult;
			}
		}

		// Must be a symbol of some sort ?
        lpctstr ptcArgsOriginal = pszArgs;
		llong llVal;
		if ( m_VarGlobals.GetParseVal_Advance( pszArgs, &llVal ) )  // VAR.
			return llVal;
        if ( m_VarResDefs.GetParseVal( ptcArgsOriginal, &llVal ) )  // RESDEF.
            return llVal;
		if ( m_VarDefs.GetParseVal( ptcArgsOriginal, &llVal ) )     // DEF.
			return llVal;
	}
#pragma endregion intrinsics

	// hard end ! Error of some sort.
	if (ptcStartingString[0] != '\0')
	{
		tchar szTag[EXPRESSION_MAX_KEY_LEN];
		GetIdentifierString(szTag, ptcStartingString);
		const lpctstr ptcLast = (ptcStartingString[0] == '<') ? ">'" : "'";
		DEBUG_ERR(("Undefined symbol '%s' [Evaluated expression: '%s%s].\n", szTag, ptcStartingString, ptcLast));
	}
	else
	{
		DEBUG_ERR(("Undefined symbol (empty parameter?).\n"));
	}
	return 0;
}

llong CExpression::GetValMath( llong llVal, lpctstr & pExpr )
{
	ADDTOCALLSTACK("CExpression::GetValMath");
	GETNONWHITESPACE(pExpr);

	// Look for math type operator and eventually apply it to the second operand (which we evaluate here if a valid operator is found).
	llong llValSecond;
	switch ( pExpr[0] )
	{
		case '\0':
			break;

		case ')':  // expression end markers.
		case '}':
		case ']':
			++pExpr;	// consume this.
			break;

		case '+':
			++pExpr;
			llValSecond = GetVal(pExpr);
			llVal += llValSecond;
			break;

		case '-':
			llValSecond = GetVal(pExpr);
			//++pExpr; No need to consume the negative sign, we need to keep it!
			llVal += llValSecond; // a subtraction is an addiction with a negative number.
			break;
		case '*':
			++pExpr;
			llValSecond = GetVal(pExpr);
			llVal *= llValSecond;
			break;

		case '|':
			++pExpr;
			if ( pExpr[0] == '|' )	// boolean ?
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal = (llValSecond || llVal);
			}
			else	// bitwise
			{
				llValSecond = GetVal(pExpr);
				llVal |= llValSecond;
			}
			break;

		case '&':
			++pExpr;
			if ( pExpr[0] == '&' )	// boolean ?
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal = (llValSecond && llVal);	// tricky stuff here. logical ops must come first or possibly not get processed.
			}
			else	// bitwise
			{
				llValSecond = GetVal(pExpr);
				llVal &= llValSecond;
			}
			break;

		case '/':
			++pExpr;
			llValSecond = GetVal(pExpr);
			if (!llValSecond)
			{
				g_Log.EventError("Evaluating math: Divide by 0\n");
				break;
			}
			llVal /= llValSecond;
			break;

		case '%':
			++pExpr;
			llValSecond = GetVal(pExpr);
			if (!llValSecond)
			{
				g_Log.EventError("Evaluating math: Modulo 0\n");
				break;
			}
			llVal %= llValSecond;
			break;

		case '^':
			++pExpr;
			llValSecond = GetVal(pExpr);
			llVal ^= llValSecond;
			break;

		case '>': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal = ( llVal >= llValSecond );
			}
			else if ( pExpr[0] == '>' )	// shift
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal >>= llValSecond;
			}
			else
			{
				llValSecond = GetVal(pExpr);
				llVal = (llVal > llValSecond);
			}
			break;

		case '<': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal = ( llVal <= llValSecond );
			}
			else if ( pExpr[0] == '<' )	// shift
			{
				++pExpr;
				llValSecond = GetVal(pExpr);
				llVal <<= llValSecond;
			}
			else
			{
				llValSecond = GetVal(pExpr);
				llVal = (llVal < llValSecond);
			}
			break;

		case '!':
			++pExpr;
			if ( pExpr[0] != '=' )
				break; // boolean ! is handled as a single expresion.
			++pExpr;
			llValSecond = GetVal(pExpr);
			llVal = ( llVal != llValSecond );
			break;

		case '=': // boolean
			while ( pExpr[0] == '=' )
				++pExpr;
			llValSecond = GetVal(pExpr);
			llVal = ( llVal == llValSecond );
			break;

		case '@':
			++pExpr;
			llValSecond = GetVal(pExpr);
			if (llVal < 0)
			{
				llVal = power(llVal, llValSecond);
				break;
			}
			else if ((llVal == 0) && (llValSecond <= 0)) //The information from https://en.cppreference.com/w/cpp/numeric/math/pow says if both input are 0, it can cause errors too.
			{
				g_Log.EventError("Power of zero with zero or negative exponent is undefined.\n");
				break;
			}
			llVal = power(llVal, llValSecond);
			break;
	}

	return llVal;
}

llong CExpression::GetVal( lpctstr & pExpr )
{
	// This function moves the pointer forward, so you can retrieve the value only once!

	ADDTOCALLSTACK("CExpression::GetVal");
	// Get a value (default decimal) that could also be an expression.
	// This does not parse beyond a comma !
	//
	// These are all the type of expressions and defines we'll see:
	//
	//	all_skin_colors				// simple DEF value
	//	7933 						// simple decimal
	//	-100.0						// simple negative decimal
	//	.5							// simple decimal
	//	0.5							// simple decimal
	//	073a 						// hex value (leading zero and no .)
	//
	//	0 -1						// Subtraction. has a space separator. (Yes I know I hate this)
	//	{0-1}						// hyphenated simple range (GET RID OF THIS!)
	//		complex ranges must be in {}
	//	{ 3 6 }							// simple range
	//	{ 400 1 401 1 } 				// weighted values (2nd val = 1)
	//	{ 1102 1148 1 }					// weighted range (3rd val < 10)
	//	{ animal_colors 1 no_colors 1 } // weighted range
	//	{ red_colors 1 {34 39} 1 }		// same (red_colors expands to a range)

	if ( pExpr == nullptr )
		return 0;

	if (_iGetVal_Reentrant >= 128 )
	{
		g_Log.EventError( "Deadlock detected while parsing '%s'. Fix the error in your scripts.\n", pExpr );
		return 0;
	}

	++_iGetVal_Reentrant;

	// Get the first operand value: it may be a number or an expression
	llong llVal = GetSingle(pExpr);

	// Check if there is an operator (mathematical or logical), in that case apply it to the second operand (which we evaluate again with GetSingle).
	llVal = GetValMath(llVal, pExpr);

	--_iGetVal_Reentrant;

	return llVal;
}

int CExpression::GetRangeVals(lpctstr & pExpr, int64 * piVals, int iMaxQty, bool bNoWarn)
{
	ADDTOCALLSTACK("CExpression::GetRangeVals");
	// Get a list of values.

	if (pExpr == nullptr)
		return 0;
	ASSERT(piVals);

	int iQty = 0;
	while (pExpr[0] != '\0')
	{
		if (pExpr[0] == ';')	// seperate field - is this used anymore?
			return iQty;
		if (pExpr[0] == ',')
			++pExpr;

		piVals[iQty] = GetSingle(pExpr);
		if (++iQty >= iMaxQty)
			return iQty;

		GETNONWHITESPACE(pExpr);

		// Look for math type operator.
		switch (pExpr[0])
		{
		case ')':  // expression end markers.
		case '}':
		case ']':
			++pExpr;	// consume this and end.
			return iQty;

		case '+':
		case '*':
		case '/':
		case '%':
			// case '^':
		case '<':
		case '>':
		case '|':
		case '&':
			piVals[iQty - 1] = GetValMath(piVals[iQty - 1], pExpr);
			return iQty;
		}
	}

	if (!bNoWarn)
		g_Log.EventError("Range isn't closed by a '}' character\n");
	return iQty;
}


int CExpression::GetConditionalSubexpressions(lptstr& pExpr, SubexprData(&psSubexprData)[32], int iMaxQty) // static
{
	ADDTOCALLSTACK("CExpression::GetConditionalSubexpressions");
	// Get the start and end pointers for each logical subexpression (delimited by brackets or by logical operators || and &&) inside a conditional statement (IF/ELIF/ELSEIF and QVAL).
	// Parse from left to start (like it was always done in Sphere).
    // Start and end pointers are inclusive (pointed values are valid chars, the end pointer doesn't necessarily point to '\0').

	if (pExpr == nullptr)
		return 0;
	//ASSERT(pSubexprPos);

	//memset((void*)&pSubexprPos, 0, ARRAY_COUNT(pSubexprPos));
	int iSubexprQty = 0;	// number of subexpressions
	using SType = SubexprData::Type;
	while (pExpr[0] != '\0')
	{
		if (++iSubexprQty >= iMaxQty)
		{
			g_Log.EventWarn("Exceeded maximum allowed number of subexpressions (%d). Parsing halted.\n", iMaxQty);
			return iSubexprQty;
		}

		GETNONWHITESPACE(pExpr);
		SubexprData& sCurSubexpr = psSubexprData[iSubexprQty - 1];
		tchar ch = pExpr[0];

		// Init the data for the current subexpression and set the position of the first character of the subexpression.
		sCurSubexpr = {pExpr, nullptr, SType::None, 0};

        //  -- What's an expression and what's a subexpression.
        // An expression can contain a single statement, a single operation (enclosed, or not, by curly brackets), like: IF <EVAL 1> or IF <EVAL 1> == 1.
        // An expression can also be made of multiple subexpressions, like:
        //  IF 1 || 0               or: IF (1 || 0), where 1 is a subexpression and 0 another one.
        // Other examples of expressions containing subexpressions:
        //  IF 1 == 0 || 0 == 0     or: IF (1 == 0) || 0 == 0       or: IF 1 == 0 || (0 == 0)
        //  IF (1 == 0 || 0 == 0)   or: IF (1 == 0) || (0 == 0)     or: IF ((1 == 0) || (0 == 0))
        // Those are all valid expressions, with valid subexpressions.

        // In the case of a fully bracketed expression like IF !(<eval 1>...), we want to return a SubExpr with

        // -- Handling negations.
        // When we are parsing a whole expression, fully enclosed by curly brackets, we need to handle the possibility of having a negation before it,
        //  and we check for it here, by storing the '!' character position, if found:

		// Handle special characters: non associative operators (like !).
        // The first character here is guaranteed not to be a space.
        lptstr ptcTopLevelNegation = nullptr;
        if (ch == '!')
        {
            // Remember that i'm interested only in the special case of subexpressions preceded by '!'.
            //	If it's inside the subexpression, it will already be handled correctly.
            ptcTopLevelNegation = pExpr;
			++pExpr;
			GETNONWHITESPACE(pExpr);
			ch = *pExpr;
		}

        // Helper lambda functions for the next section.
        auto findLastClosingBracket = [](lptstr pExpr) -> lptstr
        {
            // Returns a pointer to the last closing bracket in the string.
            // If the last character in the string (ignoring comments) is not ')', it means that, if we find a closing bracket,
            //  it's past other characters, so there's other valid text after the ')'.
            // Eg: IF (1+2) > 10. The ')' is not at the end of the line, because there's the remaining part of the script.
            ASSERT(*pExpr != '\0');
            lptstr pExprFinder;
            const size_t uiExprLength = strlen(pExpr);
            const lptstr pComment = Str_FindSubstring(pExpr, "//", uiExprLength, 2);
            if (nullptr == pComment) {
                pExprFinder = pExpr + uiExprLength - 1;
                // Now pExprFinder is at the end of the string
            }
            else {
                pExprFinder = pComment;
                // Now pExprFinder is at the start of the comment
            }

            // Search for open brackets
            do {
                const bool fWhite = IsWhitespace(*pExprFinder);
                if (fWhite)
                    --pExprFinder;
                else
                    break;
            } while (pExprFinder > pExpr);
            return (*pExprFinder == ')') ? pExprFinder : nullptr;
        };

        auto skipBracketedSubexpression = [](lptstr pExpr) -> lptstr
        {
            ASSERT(*pExpr == '(');
            tchar ch;
            uint uiOpenedCurlyBrackets = 1;
            while (uiOpenedCurlyBrackets != 0)	// i'm interested only to the outermost range, not eventual sub-sub-sub-blah ranges
            {
                ch = *(++pExpr);
                if (ch == '(')
                    ++uiOpenedCurlyBrackets;
                else if (ch == ')')
                    --uiOpenedCurlyBrackets;
                else if (ch == '\0')
                    return nullptr; // Error
            }
            if (uiOpenedCurlyBrackets != 0)
                return nullptr; // Error
            return pExpr;
        };


        // -- Search an opening curly bracket.
        // Now i need to check if the expression is enclosed or not by curly brackets, knowing that have skipped all the 1whitespace characters at the beginning of the string.
        // If we find a bracket, then we need to store this information, because after that we'll move the string pointer after the bracket itself.
        // This ensures that we begin every parsing loop without any open curly bracket.

        // Start of a expression within curved brackets?
        lptstr ptcCurSubexprStart = pExpr;
        lptstr ptcTopBracket = (ch == '(') ? pExpr : nullptr;

        // -- Done with preliminar expression analysis. Now look for subexpressions.
        lptstr ptcLastClosingBracket = nullptr; // Needs to be preserved in the subexpression parsing.
		while (true)
		{
			// This loop parses a single subexpression. Remember that we checked for a negation prefix like !( ) in the block before.
			// The outside loop stores the subexpressions number and setups the subexpression for parsing inside here.

			if (ch == '\0')
			{
                // End of the subexpression.
                // We could have encountered one of the situations below and already found the end of the subexpression, or we could need to find it here.
				if (sCurSubexpr.ptcEnd == nullptr)
				{
					sCurSubexpr.ptcEnd = pExpr;
                    if (ptcTopBracket && ptcLastClosingBracket)
                    {
                        lptstr ptcLineLastClosingBracket = findLastClosingBracket(ptcCurSubexprStart);
                        // ptcLastClosingBracket: the last closing bracket found while parsing the subexpression (might not be at the end of the line).
                        // ptcExprLastClosingBracket: the last closing bracket ')', if any, of the string. The function used does NOT check if that's a valid closing bracket
                        //  (eg. if in the string for every opening bracket there is a closing bracket).
						if (iSubexprQty == 1)
						{
                            if (nullptr == ptcLineLastClosingBracket)
                            {
                                // There are other valid characters after the closing curly bracket, so leave ptcEnd unchanged, to the end of the string.
                                ;
                            }
							else if (ptcLastClosingBracket == ptcLineLastClosingBracket)
							{
								// I'm here because the whole expression is enclosed by parentheses
							    // + 1 because i want to point to the character after the ')', even if it's the string terminator.
								sCurSubexpr.ptcEnd = ptcLastClosingBracket + 1;
								sCurSubexpr.uiType |= SubexprData::TopParenthesizedExpr;
							}
                            else
                            {
                                sCurSubexpr.ptcEnd = ptcLastClosingBracket;
                            }
						}
						// else: // The starting bracket encloses only a part of the expression
                    }
				}
				break; // End of the current subexpr, go back to find another one
			}

			else if (ch == '(')
			{
                if (ptcCurSubexprStart == pExpr)
                {
                    // Start of a subexpression delimited by brackets (it can be preceded by an operator like '!', handled before).
                    // Now i want only to see where's the matching closing bracket.
                    sCurSubexpr.ptcStart = pExpr;
                }
                else
                {
                    // It can be the argument of an intrinsic function (es. STRCMP), which isn't enclosed by angular brackets but has an argument enclosed by curly brackets.
                    sCurSubexpr.ptcStart = ptcCurSubexprStart;
                }

				// The brackets can contain other special characters, like non-associative operators, but we don't care at this stage.
				// Those will be considered and eventually evaluated when fully parsing this subexpression.

				if ( ptcTopLevelNegation && // The whole expression is preceded by a '!' character.
                    (0 == sCurSubexpr.uiNonAssociativeOffset) ) // I've not yet checked if its position is valid.
				{
					uint uiTempOffset = uint(sCurSubexpr.ptcStart - ptcTopLevelNegation);
					if (uiTempOffset > USHRT_MAX)
					{
						g_Log.EventError("Too much characters before the the expression negation. Trimming to %d.\n", USHRT_MAX);
						uiTempOffset = USHRT_MAX;
					}
					sCurSubexpr.uiNonAssociativeOffset = uchar(uiTempOffset);
				}

                // Just skip what's enclosed in the subexpression.
                ptcLastClosingBracket = skipBracketedSubexpression(pExpr);
                if (ptcLastClosingBracket != nullptr)
                    pExpr = ptcLastClosingBracket;
                else
                {
                    g_Log.EventError("Expression started with '(' but isn't closed by a ')' character.\n");
                    sCurSubexpr.ptcEnd = pExpr - 1;	// Position of the char just before the last ')' of the bracketed subexpression -> this eats away the last closing bracket
                    return iSubexprQty;
                }

				// Okay, i've eaten the expression in brackets, now fall through the rest of the loop and continue.
			}

			else if ((ch == '|') && (pExpr[1] == '|'))
			{
				// Logical two-way OR operator: ||
				if (sCurSubexpr.ptcEnd == nullptr)
					sCurSubexpr.ptcEnd = pExpr;
				sCurSubexpr.uiType = SType::Or  | (sCurSubexpr.uiType & ~SType::None);
				pExpr += 2u; // Skip the second char of the operator
				break; // End of subexpr...
			}

			else if ((ch == '&') && (pExpr[1] == '&'))
			{
				// Logical two-way AND operator: &&
				if (sCurSubexpr.ptcEnd == nullptr)
					sCurSubexpr.ptcEnd = pExpr;
				sCurSubexpr.uiType = SType::And | (sCurSubexpr.uiType & ~SType::None);
				pExpr += 2u; // Skip the second char of the operator
				break; // End of subexpr...
			}

			else
			{
				// Look for an arithmetic two-way operator.
				// The subexpression may not be really ended.
				if (ch == '<')
				{
					// This can be: <, <= or the start of a bracketed expression < >
					if (pExpr[1] == '=')
					{
						sCurSubexpr.uiType = SType::BinaryNonLogical | (sCurSubexpr.uiType & ~SType::None);
						pExpr += 1u;
					}
					else
					{
						const ushort prevSubexprType = ((iSubexprQty == 1) ? (ushort)SType::None : psSubexprData[iSubexprQty - 2].uiType);
						if ((prevSubexprType & SType::None))
						{
							// This subexpr is not preceded by a two-way operator, so probably i'm an operator: skip me.
							sCurSubexpr.uiType = SType::BinaryNonLogical | (sCurSubexpr.uiType & ~SType::None);

                            // This is not a whole logical subexpression but a single operand, or piece/fragment of the current arithmetic subexpr.
						}
						else
						{
							// This subexpr is preceded by a two-way operator, so probably i'm not another operator, rather a < > expression.
							lptstr pExprSkipped = pExpr;
							Str_SkipEnclosedAngularBrackets(pExprSkipped);
							if (pExpr != pExprSkipped)
							{
								// I actually have something enclosed in angular brackets.
								// The function above moves the pointer after the last closing bracket '>', but we want to point here to it, not the character after.
                                pExpr = pExprSkipped;
								ch = *pExpr;
                                continue;   // This allows us to skip the "ch = *(++pExpr);" below, we don't want to advance further the pointer.
							}
						}
					}

				}
				else if (ch == '>')
				{
					if (pExpr[1] == '=')
					{
						sCurSubexpr.uiType = SType::BinaryNonLogical | (sCurSubexpr.uiType & ~SType::None);
						pExpr += 1u;
					}
					else
					{
						sCurSubexpr.uiType = SType::BinaryNonLogical | (sCurSubexpr.uiType & ~SType::None);
					}
				}
				// End of arithmetic subexpression parsing.
			}

			ch = *(++pExpr);
		} // End of the subexpression while loop
	} // End of the main while loop

	// Now that we found the subexpressions, prepare them for their evaluation.
	lptstr ptcStart, ptcEnd;
	for (int i = 0; i < iSubexprQty; ++i)
	{
		SubexprData& sCurSubexpr = psSubexprData[i];
		ptcStart = sCurSubexpr.ptcStart;
		ptcEnd = sCurSubexpr.ptcEnd;

		for (lptstr ptcTest = ptcStart; ptcTest != ptcEnd; ++ptcTest)
		{
			if (
				((ptcTest[0] == '|') && (ptcTest[1] == '|')) ||
				((ptcTest[0] == '&') && (ptcTest[1] == '&'))
				//((ptcTest[0] == '<') && (ptcTest[1] != '<')) ||
				//((ptcTest[0] == '>') && (ptcTest[1] != '>'))
			   )
			{
				// We have logical operators inside, so it's a nested subexpression.
				sCurSubexpr.uiType |= SType::MaybeNestedSubexpr;
				break;
			}
		}

		GETNONWHITESPACE(ptcStart);				// After this, ptcStart is the first char of the expression
		Str_EatEndWhitespace(ptcStart, ptcEnd);	// After this, ptcEnd is the last char of the expression (so it's before the \0)

		if (sCurSubexpr.uiNonAssociativeOffset)
		{
			// ptcStart might have changed, so update uiNonAssociativeOffset accordingly (given that it's relative to ptcStart).
			const int iDiff = int(ptcStart - sCurSubexpr.ptcStart);
			ASSERT(iDiff >= 0);
			const uint uiNewOff = std::min((uint)USHRT_MAX, (uint)iDiff);
			sCurSubexpr.uiNonAssociativeOffset += uchar(uiNewOff);
		}
		sCurSubexpr.ptcStart = ptcStart;
		sCurSubexpr.ptcEnd   = ptcEnd;
	}

	return iSubexprQty;
}


static constexpr int kiRangeMaxArgs = 96;
static int GetRangeArgsPos(lpctstr & pExpr, lpctstr (&pArgPos)[kiRangeMaxArgs][2], bool fIgnoreMissingEndBracket)
{
	ADDTOCALLSTACK("CExpression::GetRangeArgsPos");
	// Get the start and end pointers for each argument in the range

	if ( pExpr == nullptr )
		return 0;
	//ASSERT(pArgPos);

	int iQty = 0;	// number of arguments (not of value-weight couples)
	while (pExpr[0] != '\0')
	{
		if ( pExpr[0] == ';' )	// seperate field - is this used anymore?
			return iQty;
		if ( pExpr[0] == ',' )
			++pExpr;	// ignore

		if (++iQty >= kiRangeMaxArgs)
		{
			g_Log.EventWarn("Exceeded maximum allowed number of arguments in a range (%d). Parsing HALTED.\n", kiRangeMaxArgs);
			return iQty;
		}

		GETNONWHITESPACE(pExpr);
		pArgPos[iQty-1][0] = pExpr;		// Position of the first character of the argument

		if (pExpr[0] == '{')	// there's another range
		{
			int iSubRanges = 1;
			while (iSubRanges != 0)	// i'm interested only to the outermost range, not eventual sub-sub-sub-blah ranges
			{
				++pExpr;
				if (pExpr[0] == '\0')
					goto end_w_error;
				else if (pExpr[0] == '{')
					++iSubRanges;
				else if (pExpr[0] == '}')
					--iSubRanges;
			}
			++pExpr;
			pArgPos[iQty-1][1] = pExpr;		// Position of the char after the last '}' of the sub-range
		}
		else	// not another range, just plain text
		{
			while (true)
			{
				if (pExpr[0] == '\0')
				{
					pArgPos[iQty - 1][1] = pExpr;	// Position of the char ('\0') after of the last character of the argument
                    if (fIgnoreMissingEndBracket)
                        goto end_of_range;
                    else
					    goto end_w_error;
				}

				if (IsWhitespace(pExpr[0]) || (pExpr[0] == ','))
				{
					pArgPos[iQty-1][1] = pExpr;		// Position of the char after the last character of the argument

					// is there another argument?
					GETNONWHITESPACE(pExpr);
					if (pExpr[0] == '}')			// check if it's really another argument or it's simply the end of the range
						goto end_of_range;
					else if (pExpr[0] == '\0')
						goto end_w_error;
					break;
				}
				else if (pExpr[0] == '}')			// end of the range we are evaluating
				{
					pArgPos[iQty-1][1] = pExpr;		// Position of the char ('}') after the last character of the argument

				end_of_range:
					++pExpr;	// consume this and end.
					return iQty;
				}

				++pExpr;
			}
		}
	}

end_w_error:
	g_Log.EventError("Range isn't closed by a '}' character.\n");
	return iQty;
}

int64 CExpression::GetRangeNumber(lpctstr & pExpr)
{
	ADDTOCALLSTACK("CExpression::GetRangeNumber");

	// Parse the arguments in the range and get the start and end position of every of them.
	// When parsing and resolving the value of each argument, we create a substring for the argument
	//	and use GetSingle to parse and resolve it.
	// If iQty <= 2, parse each argument.
	// If iQty (number of arguments) is > 2, it's a weighted range; in this case, parse only the weights
	//	of the elements and then only the element which was randomly chosen.

	lpctstr pElementsStart[kiRangeMaxArgs][2] {};
	int iQty = GetRangeArgsPos( pExpr, pElementsStart, false );	// number of arguments (not of value-weight couples)

	if (iQty == 0)
		return 0;

	// I guess it's weighted values
	if ((iQty > 2) && ((iQty % 2) == 1))
	{
		g_Log.EventError("Even number of elements in the random range: invalid. Forgot to write an element?\n");
		return 0;
	}

	tchar pToParse[THREAD_STRING_LENGTH];

	if (iQty == 1) // It's just a simple value
	{
		ASSERT(pElementsStart[0] != nullptr);

		// Copy the value in a new string
		const size_t iToParseLen = (pElementsStart[0][1] - pElementsStart[0][0]);
		memcpy((void*)pToParse, pElementsStart[0][0], iToParseLen * sizeof(tchar));
		pToParse[iToParseLen] = '\0';

		lptstr pToParseCasted = static_cast<lptstr>(pToParse);
		return GetSingle(pToParseCasted);
	}

	if (iQty == 2) // It's just a simple range... pick one in range at random
	{
		ASSERT(pElementsStart[0] != nullptr);
		ASSERT(pElementsStart[1] != nullptr);

		// Copy the first element in a new string
		size_t iToParseLen = (pElementsStart[0][1] - pElementsStart[0][0]);
		memcpy((void*)pToParse, pElementsStart[0][0], iToParseLen * sizeof(tchar));
		pToParse[iToParseLen] = '\0';

		lptstr pToParseCasted = static_cast<lptstr>(pToParse);
		llong llValFirst = GetSingle(pToParseCasted);

		// Copy the second element in a new string
		iToParseLen = (pElementsStart[1][1] - pElementsStart[1][0]);
		memcpy((void*)pToParse, pElementsStart[1][0], iToParseLen * sizeof(tchar));
		pToParse[iToParseLen] = '\0';

		pToParseCasted = static_cast<lptstr>(pToParse);
		llong llValSecond = GetSingle(pToParseCasted);

		if (llValSecond < llValFirst)	// the first value has to be < than the second before passing it to g_Rand.GetLLVal2
		{
			const llong llValTemp = llValFirst;
			llValFirst = llValSecond;
			llValSecond = llValTemp;
		}
		return g_Rand.GetLLVal2(llValFirst, llValSecond);
	}

	// First get the total of the weights
	llong llTotalWeight = 0;
	llong llWeights[kiRangeMaxArgs]{};
	for ( int i = 1; i+1 <= iQty; i += 2 )
	{
		//if (pElementsStart[i] == nullptr)
		//	break;	// Shouldn't really happen...

		// Copy the weight element in a new string
		const size_t iToParseLen = (pElementsStart[i][1] - pElementsStart[i][0]);
		memcpy((void*)pToParse, pElementsStart[i][0], iToParseLen * sizeof(tchar));
		pToParse[iToParseLen] = '\0';

		lptstr pToParseCasted = static_cast<lptstr>(pToParse);
		llWeights[i] = GetSingle(pToParseCasted);	// GetSingle changes the pointer value, so i need to work with a copy

		if ( ! llWeights[i] )	// having a weight of 0 is very strange !
			g_Log.EventError( "Weight of 0 in random range: invalid. Value-weight couple number %d.\n", i ); // the whole table should really just be invalid here !
		llTotalWeight += llWeights[i];
	}

	// Now roll the dice to see what value to pick
	llTotalWeight = g_Rand.GetLLVal(llTotalWeight) + 1;

	// Now loop to that value
	int i = 1;
	for ( ; i+1 <= iQty; i += 2 )
	{
		llTotalWeight -= llWeights[i];
		if ( llTotalWeight <= 0 )
			break;
	}

	ASSERT(i < iQty);
	i -= 1;	// pick the value instead of the weight
	const size_t iToParseLen = (pElementsStart[i][1] - pElementsStart[i][0]);

	// Copy the value element in a new string
	ASSERT(nullptr != pElementsStart[i][0]);
	memcpy((void*)pToParse, pElementsStart[i][0], iToParseLen * sizeof(tchar));
	pToParse[iToParseLen] = '\0';

	lptstr pToParseCasted = static_cast<lptstr>(pToParse);
	return GetSingle(pToParseCasted);
}

CSString CExpression::GetRangeString(lpctstr & pExpr)
{
    ADDTOCALLSTACK("CExpression::GetRangeString");

    // Parse the arguments in the range and get the start and end position of every of them.
    // When parsing and resolving the value of each argument, we create a substring for the argument
    //	and use GetSingle to parse and resolve it.
    // If iQty (number of arguments) is > 2, it's a weighted range; in this case, parse only the weights
    //	of the elements and then only the element which was randomly chosen.

	lpctstr pElementsStart[kiRangeMaxArgs][2]{};
    int iQty = GetRangeArgsPos( pExpr, pElementsStart, true );	// number of arguments (not of value-weight couples)
    if (iQty <= 0)
        return {};

    if (iQty == 1) // It's just a simple value
    {
		ASSERT(pElementsStart[0] != nullptr);
		const int iToParseLen = int(pElementsStart[0][1] - pElementsStart[0][0]);
        return CSString(pElementsStart[0][0], iToParseLen - 1);
    }

    // I guess weighted values
    if ( (iQty % 2) == 1 )
    {
        g_Log.EventError("Even number of elements in the random string range: invalid. Forgot to write an element?\n");
        return {};
    }

    // First get the total of the weights
    llong llTotalWeight = 0;
	llong llWeights[kiRangeMaxArgs]{};
    tchar pToParse[THREAD_STRING_LENGTH];
    for ( int i = 1; i+1 <= iQty; i += 2 )
    {
		//if (pElementsStart[i] == nullptr)
		//	break;	// Shouldn't really happen...

        // Copy the weight element in a new string
        const size_t iToParseLen = (pElementsStart[i][1] - pElementsStart[i][0]);
        memcpy((void*)pToParse, pElementsStart[i][0], iToParseLen * sizeof(tchar));
        pToParse[iToParseLen] = '\0';
        lptstr pToParseCasted = reinterpret_cast<lptstr>(pToParse);
        if (!IsSimpleNumberString(pToParseCasted))
        {
            g_Log.EventError( "Non-numeric weight in random string range: invalid. Value-weight couple number %d\n", i );
            return {};
        }
        llWeights[i] = GetSingle(pToParseCasted);	// GetSingle changes the pointer value, so i need to work with a copy

        if ( ! llWeights[i] )	// having a weight of 0 is very strange !
			g_Log.EventError( "Weight of 0 in random string range: invalid. Value-weight couple number %d\n", i );	// the whole table should really just be invalid here !
        llTotalWeight += llWeights[i];
    }

    // Now roll the dice to see what value to pick
    llTotalWeight = g_Rand.GetLLVal(llTotalWeight) + 1;

    // Now loop to that value
    int i = 1;
    for ( ; i+1 <= iQty; i += 2 )
    {
        llTotalWeight -= llWeights[i];
        if ( llTotalWeight <= 0 )
            break;
    }

	ASSERT(i < iQty);
    i -= 1; // pick the value instead of the weight
    const int iToParseLen = int(pElementsStart[i][1] - pElementsStart[i][0]);
    return CSString(pElementsStart[i][0], iToParseLen);
}
