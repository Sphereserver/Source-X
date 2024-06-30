/**
* @file CSRand.cpp
* @brief Wrapper for pseudo-random number generators.
*/

#include <random>
#include "CSRand.h"


int32 CSRand::GetVal(int32 iQty)
{
    if (iQty < 2)
        return 0;
    return CSRand::genRandInt32(0, iQty - 1);
}

int32 CSRand::GetVal2(int32 iMin, int32 iMax)
{
    if (iMin > iMax)
    {
        const int tmp = iMin;
        iMin = iMax;
        iMax = tmp;
    }
    return genRandInt32(iMin, iMax);
}

int64 CSRand::GetLLVal(int64 iQty)
{
    if (iQty < 2)
        return 0;
    return genRandInt64(0, iQty - 1);
}

int64 CSRand::GetLLVal2(int64 iMin, int64 iMax)
{
    if (iMin > iMax)
    {
        const llong tmp = iMin;
        iMin = iMax;
        iMax = tmp;
    }
    return genRandInt64(iMin, iMax);
}


// Biased but fast bounding methods for the generated number (fitting it into a range): https://www.pcg-random.org/posts/bounded-rands.html

int16 CSRand::Get16ValFast(int16 iQty)
{
    if (iQty < 2)
        return 0;
    const uint16 r = fast_generator_u16();
    const uint16 s = (uint32(r) * uint32_t(iQty)) >> 16u;
    return int16(s);
}

int16 CSRand::Get16Val2Fast(int16 iMin, int16 iMax)
{
    if (iMin > iMax)
        std::swap(iMin, iMax);
    const int16 rb = Get16ValFast(iMax - iMin);
    const int16 rboff = rb + iMin;
    return rboff;
}

int32 CSRand::GetValFast(int32 iQty)
{
    if (iQty < 2)
        return 0;
    const uint32 r = fast_generator_u32();
    const uint32 b = (uint64(r) * uint64_t(iQty)) >> 32u;
    return int32(b);
}

int32 CSRand::GetVal2Fast(int32 iMin, int32 iMax)
{
    if (iMin > iMax)
        std::swap(iMin, iMax);
    const int32 rb = GetValFast(iMax - iMin);
    const int32 rboff = rb + iMin;
    return rboff;
}

int64 CSRand::GetLLValFast(int64 iQty)
{
    if (iQty < 2)
        return 0;
    const uint64 r = fast_generator_u64();
    const uint64 b = r % iQty;
    return int64(b);
}

int64 CSRand::GetLLVal2Fast(int64 iMin, int64 iMax)
{
    if (iMin > iMax)
        std::swap(iMin, iMax);
    const int64 rb = GetLLValFast(iMax - iMin);
    const int64 rboff = rb + iMin;
    return rboff;
}


// C++ std Mersenne Twister pseudo - random number generator

int32 CSRand::genRandInt32(int32 min, int32 max)
{
	std::uniform_int_distribution<int32> distr(min, max);
	std::random_device rd;				// Use random_device to get a random seed (we can use also system time).
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}

int64 CSRand::genRandInt64(int64 min, int64 max)
{
	std::uniform_int_distribution<int64> distr(min, max);
	std::random_device rd;
	std::mt19937_64 rand_engine(rd());
	return distr(rand_engine);
}

/*float CSRand::genRandReal32(float min, float max)
{
	std::uniform_real_distribution<float> distr(min, max);
	std::random_device rd;
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}*/

realtype CSRand::genRandReal64(realtype min, realtype max)
{
	std::uniform_real_distribution<realtype> distr(min, max);
	std::random_device rd;
	std::mt19937_64 rand_engine(rd());
	return distr(rand_engine);
}
