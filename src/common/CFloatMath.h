/**
* @file CFloatMath.h
* @brief Custom float implementation.
*/

#ifndef _INC_CFLOATMATH_H
#define _INC_CFLOATMATH_H

#include "sphere_library/CSString.h"


struct CFloatMath
{
	// Expression Parsing
    static CSString FloatMath( lpctstr & ptcRefExpr );

private:
    static realtype MakeFloatMath( lpctstr & ptcRefExpr );
	static realtype GetRandVal( realtype dQty );
	static realtype GetRandVal2( realtype dMin, realtype dMax );
	//Does not work as it should, would be too slow, and nobody needs that
	/*static realtype GetRangeNumber( lpctstr & pExpr );
	static int GetRangeVals( lpctstr & pExpr, realtype * piVals, short int iMaxQty );*/

    static realtype GetValMath( realtype dVal, lpctstr & ptcRefExpr );
    static realtype GetSingle(lpctstr & ptcRefArgs );
};

#endif // _INC_CFLOATMATH_H
