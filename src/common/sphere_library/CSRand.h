/**
* @file CSRand.h
* @brief Wrapper for pseudo-random number generators.
*/

#ifndef _INC_CSRAND_H
#define _INC_CSRAND_H

#include "../common.h"


extern struct CSRand
{
    friend struct CFloatMath;

public:
    // Interfaces.
    static int64 GetLLVal(int64 iQty);					// Get a random value between 0 and iQty - 1 (slower than the 32bit number variant)
    static int64 GetLLVal2(int64 iMin, int64 iMax);
    static int32 GetVal(int32 iQty);					// Get a random value between 0 and iQty - 1
    static int32 GetVal2(int32 iMin, int32 iMax);

    static int64 GetLLValFast(int64 iQty) noexcept;             // (slower than the 32bit number variant)
    static int64 GetLLVal2Fast(int64 iMin, int64 iMax) noexcept;// (slower than the 32bit number variant)
    static int32 GetValFast(int32 iQty) noexcept;
    static int32 GetVal2Fast(int32 iMin, int32 iMax) noexcept;
    static int16 Get16ValFast(int16 iQty) noexcept;
    static int16 Get16Val2Fast(int16 iMin, int16 iMax) noexcept;

private:
    // Implementations, using the standard random generator.
  
    // Integer numbers
    static int32 genRandInt32(int32 min, int32 max);
    static int64 genRandInt64(int64 min, int64 max);

    // Floating point numbers
    //static float genRandReal32(float min, float max);
    static realtype genRandReal64(realtype min, realtype max);
}
g_Rand;

#endif // !_INC_CSRAND_H
