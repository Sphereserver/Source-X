/**
* @file CRand.cpp
* @brief Wrapper for C++ Mersenne Twister pseudo-random number generator
*/

#include "CRand.h"


int32 CRand::genRandInt32(int32 min, int32 max)
{
	std::uniform_int_distribution<int32> distr(min, max);
	std::random_device rd;				// Use random_device to get a random seed (we can use also system time).
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}

int64 CRand::genRandInt64(int64 min, int64 max)
{
	std::uniform_int_distribution<int64> distr(min, max);
	std::random_device rd;
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}

/*float CRand::genRandReal32(float min, float max)
{
	std::uniform_real_distribution<float> distr(min, max);
	std::random_device rd;
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}*/

RealType CRand::genRandReal64(RealType min, RealType max)
{
	std::uniform_real_distribution<RealType> distr(min, max);
	std::random_device rd;
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}