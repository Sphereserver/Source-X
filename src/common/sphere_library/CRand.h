/**
* @file CRand.h
* @brief Wrapper for C++ Mersenne Twister pseudo-random number generator
*/

#pragma once
#ifndef _INC_CRAND_H

#include <random>
#include "../datatypes.h"


class CRand
{
public:
	static	int32 genRandInt32(int32 min, int32 max);			// integer number
	static	int64 genRandInt64(int64 min, int64 max);
	//static	float genRandReal32(float min, float max);		// floating point number
	static	RealType genRandReal64(RealType min, RealType max);
};

#endif // !_INC_CRAND_H