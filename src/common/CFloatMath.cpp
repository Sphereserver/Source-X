
#include <cmath>
#include <complex>
#include <limits>
#include "../game/CObjBase.h"
#include "../game/CServerConfig.h"
#include "../sphere/threads.h"
#include "sphere_library/CSRand.h"
#include "CExpression.h"
#include "CLog.h"
#include "CFloatMath.h"


CSString CFloatMath::FloatMath(lpctstr& Expr)
{
	ADDTOCALLSTACK("CFloatMath::FloatMath");
	char szReal[VARDEF_FLOAT_MAXBUFFERSIZE];
	snprintf(szReal, VARDEF_FLOAT_MAXBUFFERSIZE, "%f", MakeFloatMath(Expr));
	return CSString(szReal);
}


short int _iReentrant_Count = 0;

realtype CFloatMath::MakeFloatMath( lpctstr & Expr )
{
	ADDTOCALLSTACK("CFloatMath::MakeFloatMath");
	if ( ! Expr )
		return 0;

	GETNONWHITESPACE( Expr );

	++_iReentrant_Count;
	if ( _iReentrant_Count > 128 )
	{
		DEBUG_WARN(( "Deadlock detected while parsing '%s'. Fix the error in your scripts.\n", Expr ));
		--_iReentrant_Count;
		return 0;
	}
	//DEBUG_ERR(("Expr: '%s' GetSingle(Expr) '%f' GetValMath(GetSingle(Expr), Expr) '%f'\n",Expr,GetSingle(Expr),GetValMath(GetSingle(Expr), Expr)));
	realtype dVal = GetValMath(GetSingle(Expr), Expr);
	--_iReentrant_Count;
	return dVal;
}

realtype CFloatMath::GetValMath( realtype dVal, lpctstr & pExpr )
{
	ADDTOCALLSTACK("CFloatMath::GetValMath");
	//DEBUG_ERR(("GetValMath  dVal %f  pExpr %s\n",dVal,pExpr));
	GETNONWHITESPACE(pExpr);

	// Look for math type operator.
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
			dVal += MakeFloatMath( pExpr );
			break;
		case '-':
			++pExpr;
			dVal -= MakeFloatMath( pExpr );
			break;
		case '*':
			++pExpr;
			dVal *= MakeFloatMath( pExpr );
			break;
		case '/':
			++pExpr;
			{
				realtype dTempVal = MakeFloatMath( pExpr );
				if ( ! dTempVal )
				{
					g_Log.EventError("Evaluating float math: Divide by 0\n");
					break;
				}
				dVal /= dTempVal;
			}
			break;
		case '!':
			++pExpr;
			if ( pExpr[0] != '=' )
				break; // boolean ! is handled as a single expresion.
			++pExpr;
			dVal = ( dVal != MakeFloatMath( pExpr ));
			break;
		case '=': // boolean
			while ( pExpr[0] == '=' )
				++pExpr;
			dVal = ( dVal == MakeFloatMath( pExpr ));
			break;
		case '@':
			++pExpr;
			{
				realtype dTempVal = MakeFloatMath( pExpr );
				if ( (dVal == 0) && (dTempVal <= 0) )
				{
					DEBUG_ERR(( "Float_MakeFloatMath: Power of zero with zero or negative exponent is undefined\n" ));
					break;
				}
				//DEBUG_ERR(("dVal %f  dTempVal %f  Result %f\n",dVal,dTempVal,pow(dVal, dTempVal)));
				dVal = pow(dVal, dTempVal);
			}
			break;
		//Following operations are not allowed with Double
		case '|':
			++pExpr;
			if ( pExpr[0] == '|' )	// boolean ?
			{
				++pExpr;
				dVal = ( MakeFloatMath( pExpr ) || dVal );
			}
			else	// bitwise
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","|"));
			break;
		case '&':
			++pExpr;
			if ( pExpr[0] == '&' )	// boolean ?
			{
				++pExpr;
				dVal = ( MakeFloatMath( pExpr ) && dVal );	// tricky stuff here. logical ops must come first or possibly not get processed.
			}
			else	// bitwise
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","&"));
			break;
		case '%':
			++pExpr;
			DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","%"));
			break;
		case '^':
			++pExpr;
			DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","^"));
			break;
		case '>': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				dVal = ( dVal >= MakeFloatMath( pExpr ));
			}
			else if ( pExpr[0] == '>' )	// shift
			{
				++pExpr;
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n",">>"));
			}
			else
			{
				dVal = ( dVal > MakeFloatMath( pExpr ));
			}
			break;
		case '<': // boolean
			++pExpr;
			if ( pExpr[0] == '=' )	// boolean ?
			{
				++pExpr;
				dVal = ( dVal <= MakeFloatMath( pExpr ));
			}
			else if ( pExpr[0] == '<' )	// shift
			{
				++pExpr;
				DEBUG_ERR(("Operator '%s' is not allowed with floats.\n","<<"));
			}
			else
			{
				dVal = ( dVal < MakeFloatMath( pExpr ));
			}
			break;
	}
	return dVal;
}

realtype CFloatMath::GetSingle( lpctstr & pArgs )
{
	ADDTOCALLSTACK("CFloatMath::GetSingle");
	//DEBUG_ERR(("GetSingle  pArgs %s\n",pArgs));
	GETNONWHITESPACE( pArgs );
	const size_t uiArgsCopySize = strlen(pArgs) + 1;
	char * pArgsCopy = new char[uiArgsCopySize];
	Str_CopyLimitNull(pArgsCopy, pArgs, uiArgsCopySize);
	/*bool IsNum = true; // Old Ellessar's code without support for negative numbers
	for( char ch = tolower(*pArgs); ch; ch = tolower(*(++pArgs)) )
	{
		if (( IsDigit( ch ) ) || ( ch == '.' ) || ( ch == ',' ))
			continue;

		if ((( ch >= '*' ) && ( ch <= '/' )) || (( ch == ')' ) || ( ch == ']' )) || ( ch == '@' ))
			break;
		//DEBUG_ERR(("ch '0%x'\n",ch));
		IsNum = false;
		break;
	}*/
	bool IsNum = false;
	for (tchar ch = static_cast<tchar>(tolower(*pArgs)); ch; ch = static_cast<tchar>(tolower(*(++pArgs))))
    {
        if (( IsDigit( ch ) ) || ( ch == '.' ) || ( ch == ',' ))
        {
			if ( IsNum == false)
				IsNum = (IsDigit(ch) != 0);
            continue;
        }

        if ((( ch >= '*' ) && ( ch <= '/' )) || (( ch == ')' ) || ( ch == ']' )) || ( ch == '@' ))
            break;
        //DEBUG_ERR(("ch '0%x'\n",ch));
        // IsNum = false;
        break;
    }
	if ( IsNum )
	{
		char * pEnd;
		realtype ret = strtod(pArgsCopy,&pEnd);
		//DEBUG_ERR(("IsNum: '%d' pArgsCopy '%s' Ret: '%f'\n",IsNum,pArgsCopy,strtod(pArgsCopy,&pEnd)));
		delete[] pArgsCopy;
		return( ret );
	}
	delete[] pArgsCopy;
	switch ( pArgs[0] )
	{
		case '{':
		//	++pArgs;
		//	return( GetRangeNumber( pArgs ));
		case '[':
		case '(': // Parse out a sub expression.
			++pArgs;
			return( MakeFloatMath( pArgs ));
		case '+':
			++pArgs;
			break;
		case '-':
			++pArgs;
			return( -GetSingle( pArgs ));
		case '~':	// Bitwise not.
			++pArgs;
			DEBUG_ERR(("Operator '~' is not allowed with floats.\n"));
			return 0;
		case '!':	// boolean not.
			++pArgs;
			if ( pArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
			{
				++pArgs;		// so just skip it. and compare it to 0
				return( GetSingle( pArgs ));
			}
			return( !GetSingle( pArgs ));
		case ';':	// seperate field.
		case ',':	// seperate field.
		case '\0':
			return 0;
	}
	INTRINSIC_TYPE iIntrinsic = (INTRINSIC_TYPE) FindTableHeadSorted( pArgs, sm_IntrinsicFunctions, ARRAY_COUNT(sm_IntrinsicFunctions)-1 );
	if ( iIntrinsic >= 0 )
	{
		size_t iLen = strlen(sm_IntrinsicFunctions[iIntrinsic]);
        if ( strchr("( ", pArgs[iLen]) )
		{
			pArgs += (iLen + 1);
			tchar * pArgsNext;
			Str_Parse( const_cast<tchar*>(pArgs), &(pArgsNext), ")" );

			tchar * ppCmd[5];
			realtype rResult;
			int iCount;
			const char * cparg1 = nullptr; //some functions need a const char instead of a char and GCC cannot bear it :)
			const char * cparg2 = nullptr; //some functions need a const char instead of a char and GCC cannot bear it :)

			switch ( iIntrinsic )
			{
				case INTRINSIC_ID:
				{
					if ( *pArgs )
					{
						iCount = 1;
						rResult = ResGetIndex(int(MakeFloatMath(pArgs))); // ResGetIndex
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

                case INTRINSIC_MAX:
                {
                    iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
                    if ( iCount < 2 )
                        rResult = 0;
                    else
                    {
                        realtype rVal1, rVal2;
                        lpctstr ptcTemp = ppCmd[0];
                        rVal1 = MakeFloatMath(ptcTemp);
                        ptcTemp = ppCmd[1];
                        rVal2 = MakeFloatMath(ptcTemp);
                        rResult = maximum(rVal1, rVal2);
                    }
                } break;

                case INTRINSIC_MIN:
                {
                    iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
                    if ( iCount < 2 )
                        rResult = 0;
                    else
                    {
                        realtype rVal1, rVal2;
                        lpctstr ptcTemp = ppCmd[0];
                        rVal1 = MakeFloatMath(ptcTemp);
                        ptcTemp = ppCmd[1];
                        rVal2 = MakeFloatMath(ptcTemp);
                        rResult = minimum(rVal1, rVal2);
                    }
                } break;

				case INTRINSIC_LOGARITHM:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 3, "," );
					if ( iCount < 1 )
					{
						rResult = 0;
						break;
					}

					lpctstr tCmd = ppCmd[0];
					realtype dArgument = MakeFloatMath( tCmd );

					if ( iCount < 2 )
					{
						rResult = log10(dArgument);
					}
					else
					{
						if ( !strcmpi(ppCmd[1], "e") )
						{
							rResult = log(dArgument);
						}
						else if ( !strcmpi(ppCmd[1], "pi") )
						{
							rResult = log(dArgument) / log(M_PI);
						}
						else
						{
							tCmd = ppCmd[1];
							realtype dBase = MakeFloatMath( tCmd );
							if ( dBase <= 0 )
							{
								DEBUG_ERR(( "Float_MakeFloatMath: (%f)Log(%f) is %s\n", dBase, dArgument, (!dBase) ? "infinite" : "undefined" ));
								iCount = 0;
								rResult = 0;
							}
							else
							{
								rResult = (log(dArgument)/log(dBase));
							}
						}
					}
				} break;

				case INTRINSIC_NAPIERPOW:
				{
					if ( *pArgs )
					{
						iCount = 1;
						rResult = exp(MakeFloatMath(pArgs));
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_SQRT:
				{
					iCount = 0;

					if ( *pArgs )
					{
						realtype dTosquare = MakeFloatMath(pArgs);

						if (dTosquare >= 0)
						{
							++iCount;
							rResult = sqrt(dTosquare);
						}
						else
						{
							++iCount;
							std::complex<double> number(dTosquare, 0);
							std::complex<double> result = sqrt(number);
							rResult = result.real();
						}
					}
					else
					{
						rResult = 0;
					}

				} break;

				case INTRINSIC_SIN:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = sin(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_ARCSIN:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = asin(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_COS:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = cos(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_ARCCOS:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = acos(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_TAN:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = tan(dArgument * M_PI / 180);
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;

				case INTRINSIC_ARCTAN:
				{
					if ( *pArgs )
					{
						iCount = 1;
						realtype dArgument = MakeFloatMath(pArgs);
						rResult = atan(dArgument) * 180 / M_PI;
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}

				} break;


				case INTRINSIC_StrIndexOf:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 3, "," );
					if ( iCount < 2 )
						rResult = -1;
					else
					{
						cparg1 = ppCmd[2];
						rResult = Str_IndexOf(ppCmd[0], ppCmd[1], (iCount == 3)? (int)(MakeFloatMath(cparg1)) : 0);
					}
				} break;

				case INTRINSIC_STRMATCH:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						rResult = 0;
					else
						rResult = (Str_Match( ppCmd[0], ppCmd[1] ) == MATCH_VALID ) ? 1 : 0;
				} break;

				case INTRINSIC_STRREGEX:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						rResult = 0;
					else
					{
						tchar * tLastError = Str_GetTemp();
						rResult = Str_RegExMatch( ppCmd[0], ppCmd[1], tLastError );
						if ( rResult < 0 )
						{
							DEBUG_ERR(( "STRREGEX bad function usage. Error: %s\n", tLastError ));
						}
					}
				} break;

				case INTRINSIC_RANDBELL:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						rResult = 0;
					else
					{
						cparg1 = ppCmd[0];
						cparg2 = ppCmd[1];
						rResult = Calc_GetBellCurve((int)(MakeFloatMath(cparg1)), (int)(MakeFloatMath(cparg2)));
					}
				} break;

				case INTRINSIC_STRASCII:
				{
					if ( *pArgs )
					{
						iCount = 1;
						rResult = pArgs[0];
					}
					else
					{
						iCount = 0;
						rResult = 0;
					}
				} break;

				case INTRINSIC_RAND:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount <= 0 )
						rResult = 0;
					else
					{
						cparg1 = ppCmd[0];
						realtype val1 = MakeFloatMath( cparg1 );
						if ( iCount >= 2 )
						{
							cparg2 = ppCmd[1];
							realtype val2 = MakeFloatMath( cparg2 );
							rResult = GetRandVal2( val1, val2 );
						}
						else
							rResult = GetRandVal(val1);
					}
				} break;

				case INTRINSIC_STRCMP:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						rResult = 1;
					else
						rResult = strcmp(ppCmd[0], ppCmd[1]);
				} break;

				case INTRINSIC_STRCMPI:
				{
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 2, "," );
					if ( iCount < 2 )
						rResult = 1;
					else
						rResult = strcmpi(ppCmd[0], ppCmd[1]);
				} break;

				case INTRINSIC_STRLEN:
				{
					iCount = 1;
					rResult = (realtype)strlen(pArgs);
				} break;

				case INTRINSIC_ISOBSCENE:
				{
					iCount = 1;
					rResult = g_Cfg.IsObscene( pArgs );
				} break;

				case INTRINSIC_ISNUMBER:
				{
					iCount = 1;
                    SKIP_NONNUM( pArgs );
                    rResult = IsStrNumeric( pArgs );
				} break;

				case INTRINSIC_QVAL:
				{
					// Here is handled the intrinsic QVAL form: QVAL(VALUE1,VALUE2,LESSTHAN,EQUAL,GREATERTHAN)
					iCount = Str_ParseCmds( const_cast<tchar*>(pArgs), ppCmd, 5, "," );
					if (iCount < 3)
					{
						rResult = 0;
					}
					else
					{
						cparg1 = ppCmd[0];
						cparg2 = ppCmd[1];
						const realtype a1 = GetSingle(cparg1);
						const realtype a2 = GetSingle(cparg2);
						if ( a1 < a2 )
						{
							cparg1 = ppCmd[2];
							rResult = GetSingle(cparg1);
						}
						else if ( a1 == a2 )
						{
							cparg1 = ppCmd[3];
							rResult = ( iCount < 4 ) ? 0 : GetSingle(cparg1);
						}
						else
						{
							cparg1 = ppCmd[4];
							rResult = ( iCount < 5 ) ? 0 : GetSingle(cparg1);
						}
					}
				} break;

				default:
					iCount = 0;
					rResult = 0;
					break;
			}

			pArgs = pArgsNext;

			if ( iCount <= 0 )
			{
				DEBUG_ERR(( "Bad intrinsic function usage. missing )\n" ));
				return 0;
			}
			else
			{
				return rResult;
			}
		}
	}
	llong llVal;
	if ( g_Exp.m_VarGlobals.GetParseVal( pArgs, &llVal ) )
		return (int)llVal;
	if ( g_Exp.m_VarResDefs.GetParseVal( pArgs, &llVal ) )
		return (int)llVal;
	if ( g_Exp.m_VarDefs.GetParseVal( pArgs, &llVal ) )
			return (int)llVal;
	return 0;
}

realtype CFloatMath::GetRandVal( realtype dQty )
{
	ADDTOCALLSTACK("CFloatMath::GetRandVal");
	if ( dQty <= 0 )
		return 0;
	if ( dQty >= std::numeric_limits<realtype>::max() )
		return IMulDivRT( CSRand::genRandReal64(0,dQty), dQty, std::numeric_limits<realtype>::max() );
	return CSRand::genRandReal64(0, dQty);
}

realtype CFloatMath::GetRandVal2( realtype dMin, realtype dMax )
{
	ADDTOCALLSTACK("CFloatMath::GetRandVal2");
	if ( dMin > dMax )
	{
		realtype tmp = dMin;
		dMin = dMax;
		dMax = tmp;
	}
	return CSRand::genRandReal64(dMin, dMax);
}

//Does not work as it should, would be too slow, and nobody needs that
/*realtype CFloatMath::GetRangeNumber( lpctstr & pExpr )
{
	realtype dVals[256];		// Maximum elements in a list

	short int iQty = GetRangeVals( pExpr, dVals, ARRAY_COUNT(dVals));

	if (iQty == 0)
	{
		DEBUG_ERR(("1"));
		return 0;
	}
	if (iQty == 1) // It's just a simple value
	{
		DEBUG_ERR(("2"));
		return( dVals[0] );
	}
	if (iQty == 2) // It's just a simple range....pick one in range at random
	{
		DEBUG_ERR(("3"));
		return( GetRandVal2( minimum(dVals[0],dVals[1]), maximum(dVals[0],dVals[1]) ) );
	}

	// I guess it's weighted values
	// First get the total of the weights

	realtype dTotalWeight = 0;
	int i = 1;
	for ( ; i+1 <= iQty; i+=2 )
	{
		if ( ! dVals[i] )	// having a weight of 0 is very strange !
		{
			DEBUG_ERR(( "Weight of 0 in random set?\n" ));	// the whole table should really just be invalid here !
		}
		dTotalWeight += dVals[i];
	}

	// Now roll the dice to see what value to pick
	dTotalWeight = GetRandVal(dTotalWeight) + 1;
	// Now loop to that value
	i = 1;
	for ( ; i+1 <= iQty; i+=2 )
	{
		dTotalWeight -= dVals[i];
		if ( dTotalWeight <= 0 )
			break;
	}

	return( dVals[i-1] );
}

int CFloatMath::GetRangeVals( lpctstr & pExpr, realtype * piVals, short int iMaxQty )
{
	ADDTOCALLSTACK("CFloatMath::GetRangeVals");
	// Get a list of values.
	if ( pExpr == nullptr )
		return 0;

	ASSERT(piVals);

	int iQty = 0;
	for (;;)
	{
		if ( !pExpr[0] ) break;
		if ( pExpr[0] == ';' )
			break;	// seperate field.
		if ( pExpr[0] == ',' )
			++pExpr;

		piVals[iQty] = GetSingle( pExpr );
		if ( ++iQty >= iMaxQty )
			break;

		GETNONWHITESPACE(pExpr);

		// Look for math type operator.
		switch ( pExpr[0] )
		{
			case ')':  // expression end markers.
			case '}':
			case ']':
				pExpr++;	// consume this and end.
				return( iQty );

			case '+':
			case '*':
			case '/':
			case '%':
			// case '^':
			case '<':
			case '>':
			case '|':
			case '&':
				piVals[iQty-1] = GetValMath( piVals[iQty-1], pExpr );
				break;
		}
	}

	return( iQty );
}*/
