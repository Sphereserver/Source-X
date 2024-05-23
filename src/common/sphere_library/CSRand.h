/**
* @file CSRand.h
* @brief Wrapper for pseudo-random number generators.
*/

#ifndef _INC_CSRAND_H
#define _INC_CSRAND_H

#include "../common.h"
#include <rand/xorshift.hpp>


extern struct CSRand
{
    friend struct CFloatMath;

public:
    // Interfaces.
    int64 GetLLVal(int64 iQty);					// Get a random value between 0 and iQty - 1 (slower than the 32bit number variant)
    int64 GetLLVal2(int64 iMin, int64 iMax);
    int32 GetVal(int32 iQty);					// Get a random value between 0 and iQty - 1
    int32 GetVal2(int32 iMin, int32 iMax);

    int64 GetLLValFast(int64 iQty);             // (slower than the 32bit number variant)
    int64 GetLLVal2Fast(int64 iMin, int64 iMax);// (slower than the 32bit number variant)
    int32 GetValFast(int32 iQty);
    int32 GetVal2Fast(int32 iMin, int32 iMax);
    int16 Get16ValFast(int16 iQty);
    int16 Get16Val2Fast(int16 iMin, int16 iMax);

private:
    // Implementations.
  
    // Integer numbers
    static	int32 genRandInt32(int32 min, int32 max);
	static	int64 genRandInt64(int64 min, int64 max);

    // Floating point numbers
	//static	float genRandReal32(float min, float max);
	static	realtype genRandReal64(realtype min, realtype max);

private:
    // Fast generators, to be used where speed matters more than quality.
    xorshift64plain64a fast_generator_u64;
    xorshift32plain32a fast_generator_u32;
    xorshift16plain16a fast_generator_u16;
}
g_Rand;

#endif // !_INC_CSRAND_H
