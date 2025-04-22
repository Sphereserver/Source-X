/**
* @file CSRand.cpp
* @brief Wrapper for pseudo-random number generators.
*/

#include <rand/xorshift.hpp>
#include <random>
#include "CSRand.h"


struct RandEngines
{
    // Standard generator (Mersenne Twister).
    //  Just create std::random_device once, since it often queries an external entropy source (this action might have a considerable overhead).
    //  Also, the PRNG state evolves with each call, preventing predictable spacing.
#ifdef ARCH_64
    std::mt19937_64 mt64;
#endif
    std::mt19937    mt;

    // Fast generators, to be used where speed matters more than quality.
    xorshift64plain64a fast_generator_u64;
    xorshift32plain32a fast_generator_u32;
    xorshift16plain16a fast_generator_u16;

    RandEngines() :
#ifdef ARCH_64
        mt64(std::random_device{}()),
#endif
        mt(std::random_device{}())
    {}
};

static RandEngines& getRandEngines() {
    static thread_local RandEngines engines;
    return engines;
};


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

int16 CSRand::Get16ValFast(int16 iQty) noexcept
{
    if (iQty < 2)
        return 0;
    const uint16 r = getRandEngines().fast_generator_u16();
    const uint16 s = (uint32(r) * uint32_t(iQty)) >> 16u;
    return int16(s);
}

int16 CSRand::Get16Val2Fast(int16 iMin, int16 iMax) noexcept
{
    if (iMin > iMax)
        std::swap(iMin, iMax);
    const int16 rb = Get16ValFast(iMax - iMin);
    const int16 rboff = rb + iMin;
    return rboff;
}

int32 CSRand::GetValFast(int32 iQty) noexcept
{
    if (iQty < 2)
        return 0;
    const uint32 r = getRandEngines().fast_generator_u32();
    const uint32 b = (uint64(r) * uint64_t(iQty)) >> 32u;
    return int32(b);
}

int32 CSRand::GetVal2Fast(int32 iMin, int32 iMax) noexcept
{
    if (iMin > iMax)
        std::swap(iMin, iMax);
    const int32 rb = GetValFast(iMax - iMin);
    const int32 rboff = rb + iMin;
    return rboff;
}

int64 CSRand::GetLLValFast(int64 iQty) noexcept
{
    if (iQty < 2)
        return 0;
    const uint64 r = getRandEngines().fast_generator_u64();
    const uint64 b = r % iQty;
    return int64(b);
}

int64 CSRand::GetLLVal2Fast(int64 iMin, int64 iMax) noexcept
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
    return distr(getRandEngines().mt);
}

int64 CSRand::genRandInt64(int64 min, int64 max)
{
	std::uniform_int_distribution<int64> distr(min, max);
#ifdef ARCH_64
    return distr(getRandEngines().mt64);
#else
    return distr(getRandEngines().mt);
#endif
}

/*float CSRand::genRandReal32(float min, float max)
{
	std::uniform_real_distribution<float> distr(min, max);
    return distr(getRandEngines().mt);
}*/

realtype CSRand::genRandReal64(realtype min, realtype max)
{
	std::uniform_real_distribution<realtype> distr(min, max);
#ifdef ARCH_64
    return distr(getRandEngines().mt64);
#else
    return distr(getRandEngines().mt);
#endif
}
