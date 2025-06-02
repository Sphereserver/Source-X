#ifndef _INC_SFASTMATH_H
#define _INC_SFASTMATH_H

//#include <cstdint>
//#include <limits>
#include <type_traits>
#include <concepts>


// Most (if not all) of this functions works on the assumption that negative numbers are represented
//  using the two's complement. To be fair most common archs use this, so the checks are over, over zealant.

// Two's complement validation for signed types
template<typename T>
[[maybe_unused]]
constexpr bool is_twos_complement_signed()
{
    if constexpr (! std::is_signed_v<T>)
        return true; // Unsigned types automatically pass

    // Check 1: All-ones pattern should equal -1
    constexpr bool check_all_ones = static_cast<T>(~T(0)) == T(-1);

    // Check 2: Arithmetic right shift preservation
    // Shifting a negative value right by 1 must preserve the sign bit (i.e. yield −1) for arithmetic shift.
    constexpr bool check_arith_shift = (T(-1) >> 1) == T(-1);

    return check_all_ones && check_arith_shift;
}
static_assert(is_twos_complement_signed<int>());


namespace sl
{
namespace fmath
{
    // Adapted from: https://github.com/ermig1979/Simd/blob/master/src/Simd/SimdMath.h

    template<std::integral T>
    inline T Average(T a, T b) noexcept
    {
        return (a + b + 1) >> 1;
    }

    template<std::integral T>
    inline T Average(T a, T b, T c, T d) noexcept
    {
        return (a + b + c + d + 2) >> 2;
    }

    inline float NotF16(float f) noexcept
    {
        const int i = ~(int&)f;
        return (float&)i;
    }

    // --

    /*
    inline void SortU8(int & a, int & b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        b += d & m;
        a -= d & m;
    }
    */

    // TODO: return a pair to unpack the return values with a structured binding?
    template<std::signed_integral T>
    inline void sSortPair(T &iMin, T &iMax) noexcept
    {
        // 1) Compute difference
        T d = iMin - iMax;

        // 2) Build mask = ~(d >> (bitwidth-1)):
        //    - If a>b, d>0 → d>>(W-1) == 0 → mask = ~0 == all-ones
        //    - If a<=b,d<=0→ d>>(W-1) == -1→ mask = ~(-1) == 0

        constexpr int W = sizeof(T) * 8 - 1;
        const T m = ~(d >> W);

        // 3) Conditionally add/subtract the difference
        const T t = d & m;
        iMax += t;   // if a>b: b = b + (a–b) → b = a
        iMin -= t;   // if a>b: a = a - (a–b) → a = b_old
    }

    // TODO: return a pair to unpack the return values with a structured binding?
    // Sort two unsigned integers so that a ≤ b, branchlessly
    template<std::unsigned_integral T>
    inline void uSortPair(T &uiMin, T &uiMax) noexcept
    {
        // Compute a mask of all-1’s if a < b, else 0
        // (a < b) → 1, negate → T(-1) ; (a < b) → 0, negate → T(0)
        const T mask = -T(uiMin < uiMax);

        // XOR-swap trick under mask:
        //   t = (a ^ b) & mask  → t = difference bits if a<b, else 0
        //   a ^= t; b ^= t;     → swaps a and b exactly when a<b
        const T t = (uiMin ^ uiMax) & mask;
        uiMin ^= t;
        uiMax ^= t;
    }

    /*
    inline int AbsDifferenceU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = d >> 8;
        return (d & ~m) | (-d & m);
    }
    */
    template<std::unsigned_integral T>
    inline T uAbsDiff(T a, T b) noexcept
    {
        // Use the corresponding signed type.
        using SignedT = typename std::make_signed<T>::type;

        // Compute the difference in signed space.
        const SignedT diff = static_cast<SignedT>(a) - static_cast<SignedT>(b);

        // Compute mask: arithmetic right-shift extracts the sign bit
        const int bits = sizeof(SignedT) * 8 - 1;
        const SignedT mask = diff >> bits;

        // The standard trick: (diff ^ mask) - mask produces the absolute value.
        return static_cast<T>((diff ^ mask) - mask);
    }

    template<std::signed_integral T>
    inline T sAbsDiff(T a, T b) noexcept
    {
        const T d    = a - b;
        const T mask = -(d < 0);    // 0 if d>=0, -1 if d<0
        return (d ^ mask) - mask;   // (d ^ mask) - mask  = |d| (absolute value)
    }

    /*
    inline int MaxU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return b + (d & m);
    }
    */
    template<std::signed_integral T>
    inline T sMax(T a, T b) noexcept
    {
        const T d    = a - b;
        const T mask = -(d >= 0);   // -1 if a>=b, 0 otherwise
        return b + (d & mask);      // b + (d & mask) gives max(a,b)
    }

    /*
    inline int MinU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return a - (d & m);
    }
    */
    template<std::signed_integral T>
    inline T sMin(T a, T b) noexcept
    {
        const T d    = a - b;
        const T mask = -(d >= 0);   // -1 if a>=b, 0 otherwise
        return a - (d & mask);      // a - (d & mask) gives min(a,b)
    }

    /*
    inline int ZeroSaturatedSubtractionU8(int a, int b) noexcept
    {
        const int d = a - b;
        const int m = ~(d >> 8);
        return (d & m);
    }
    */
    template<std::signed_integral T>
    inline T sZeroSatSub(T a, T b) noexcept // clamps negative value to 0
    {
        const T d    = a - b;
        const T mask = -(d >= 0);   // -1 if a>=b, 0 otherwise
        return d & mask;            // (d & mask) clamps negative d to 0
    }

/*  SatSub
    Case 1: When a ≥ b (positive difference)

    Example: a = 5, b = 2 → d = 3

    d     = 00000011  (3 in 8-bit)
    mask  = 00000011 >> 7 = 00000000  (positive→0)
    d ^ mask = 00000011 ^ 00000000 = 00000011
    (d ^ mask) - mask = 00000011 - 00000000 = 00000011 (3)

    Case 2: When a < b (negative difference)

    Example: a = 2, b = 5 → d = -3

    d     = 11111101  (-3 in 8-bit two's complement)
    mask  = 11111101 >> 7 = 11111111  (negative→all 1s)
    d ^ mask = 11111101 ^ 11111111 = 00000010  (bit flip)
    (d ^ mask) - mask = 00000010 - 11111111 = 00000010 + 1 = 00000011 (3)

    Why it works: When d is negative, XOR with all-1s flips all bits (one's complement), then subtracting -1 adds 1, completing the two's complement negation.
*/

    // ---

    /* Absolute value */

    // WARNING: these functions, and even the standard abs(), cannot fully handle the lowest (most negative) value for a signed integer type.
    //  This is a well-known, inherent limitation of two’s-complement representation.
    // Example: int8_t has range –128…127, abs of –128 returns –128	for our signed Abs* (and it's undefined behavior for abs()) and 128 for our Abs*u.

    template<std::signed_integral T>
    inline T sAbs(T x) noexcept
    {
        // Signed: branchless two’s‐complement abs
        //
        // mask = x >> (W-1) → 0 for x>=0, -1 for x<0
        // (x ^ mask) - mask → if mask=0: x; if mask=-1: (~x)+1 = -x
        constexpr unsigned W = sizeof(T) * 8 - 1;
        const T mask = x >> W;
        return (x ^ mask) - mask;
    }

    /*
    inline int8_t sAbs8(int8_t x) noexcept
    {
        const int8_t mask = x >> 7;
        return (x ^ mask) - mask;
    }

    inline uint8_t uAbs8(int8_t x) noexcept
    {
        const uint8_t mask = uint8_t(x) >> 7;
        return (uint8_t(x) ^ mask - mask);
    }

    inline int16_t sAbs16(int16_t x) noexcept
    {
        const int16_t mask = x >> 15;
        return (x ^ mask) - mask;
    }

    inline uint16_t uAbs16(int16_t x) noexcept
    {
        const uint16_t mask = uint16_t(x) >> 15;
        return (uint16_t(x) ^ mask - mask);
    }

    inline int32_t sAbs32(int32_t x) noexcept
    {
        const int32_t mask = x >> 31;// Arithmetic shift: 0 for x>=0, –1 for x<0
        return (x ^ mask) - mask;    // If mask=–1: (~x)–(–1)=–x; else x–0=x

    }
    inline uint32_t uAbs32(int32_t x) noexcept
    {
        const uint32_t mask = uint32_t(x) >> 31;
        return (uint32_t(x) ^ mask) - mask;
    }

    inline int64_t sAbs64(int64_t x) noexcept
    {
        const int64_t mask = x >> 63;
        return (x ^ mask) - mask;
    }

    inline uint64_t uAbs64(int64_t x) noexcept
    {
        const int64_t mask = x >> 63;
        return (uint64_t(x) ^ mask - mask);
    }
    */

} // namespace
} // namespace

#endif // _INC_SFASTMATH_H
