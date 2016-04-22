/**
* @file CSRand.cpp
* @brief Wrapper for C++ Mersenne Twister pseudo-random number generator
*/

#include "CSRand.h"


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
	std::mt19937 rand_engine(rd());
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
	std::mt19937 rand_engine(rd());
	return distr(rand_engine);
}