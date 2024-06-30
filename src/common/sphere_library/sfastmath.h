#ifndef _INC_SFASTMATH_H
#define _INC_SFASTMATH_H

// Adapted from: https://github.com/ermig1979/Simd/blob/master/src/Simd/SimdMath.h

namespace sfmath
{

    inline int Average(int a, int b) noexcept
    {
        return (a + b + 1) >> 1;
    }

    inline int Average(int a, int b, int c, int d) noexcept
    {
        return (a + b + c + d + 2) >> 2;
    }

    inline void SortU8(int & a, int & b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        b += d & m;
        a -= d & m;
    }

    inline int AbsDifferenceU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = d >> 8;
        return (d & ~m) | (-d & m);
    }

    inline int MaxU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return b + (d & m);
    }

    inline int MinU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return a - (d & m);
    }

    inline int SaturatedSubtractionU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return (d & m);
    }

    inline float NotF16(float f) noexcept
    {
        const int i = ~(int&)f;
        return (float&)i;
    }

} // namespace

#endif // _INC_SFASTMATH_H
