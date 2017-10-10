/**
* @file CSRand.h
* @brief Wrapper for C++ Mersenne Twister pseudo-random number generator
*/

#pragma once
#ifndef _INC_CSRAND_H

#include "../datatypes.h"	// only the numeric data types


class CSRand
{
public:
	static	int32 genRandInt32(int32 min, int32 max);			// integer number
	static	int64 genRandInt64(int64 min, int64 max);
	//static	float genRandReal32(float min, float max);		// floating point number
	static	realtype genRandReal64(realtype min, realtype max);
};

#endif // !_INC_CSRAND_H