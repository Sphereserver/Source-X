#include "sstring.h"
//#include "../../common/CLog.h"
#include "../../sphere/ProfileTask.h"
#include "../CExpression.h" // included in the precompiled header
#include <bit>  // for std::countl_zero


#ifdef MSVC_COMPILER
    #include <codeanalysis/warnings.h>
    #pragma warning( push )
    #pragma warning ( disable : ALL_CODE_ANALYSIS_WARNINGS )
#else
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpragmas"
    #pragma GCC diagnostic ignored "-Winvalid-utf8"
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
    #if defined(__GNUC__) && !defined(__clang__)
    #   pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    #elif defined(__clang__)
    #   pragma GCC diagnostic ignored "-Wweak-vtables"
    #endif
#endif

#include <regex/deelx.h>

#ifdef MSVC_COMPILER
    #pragma warning( pop )
#else
    #pragma GCC diagnostic pop
#endif


// String utilities: Converters

#ifndef _WIN32
void Str_Reverse(char* string) noexcept
{
    char* pEnd = string;
    char temp;
    while (*pEnd)
        ++pEnd;
    --pEnd;
    while (string < pEnd)
    {
        temp = *pEnd;
        *pEnd-- = *string;
        *string++ = temp;
    }
}
#endif


// --
// String to Number

template<class _IntType>
bool cstr_to_num(
    const char * RESTRICT str,
    _IntType   * const    out,
    uint        base = 0,
    size_t      stop_at_len = 0,
    bool const  ignore_trailing_extra_chars = false
    ) noexcept
{
    static_assert(std::is_integral_v<_IntType>, "Only integers supported");
    if (!str || !out || base == 1 || base > 16) [[unlikely]]
        return false;

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
        ++str;
    if (*str == '\0')
        return false;

    // Optional sign (only for signed types)
    bool neg = false;
    if constexpr (std::is_signed_v<_IntType>)
    {
        /*  // If we wish to support numbers with the '+' sign, but we need to update the other parses (mainly in CExpression).
        if (*p == '+' || *p == '-')
        {
            neg = (*p == '-');
            ++p;
        }
        */
        if (*str == '-')
        {
            neg = true;
            ++str;
        }
    }

    // Auto-detect base or handle explicit hex prefix
    bool hex = false;
    if (base == 0)
    {
        // Auto-detect: '0' followed by hex digit → base 16; else base 10
        if (*str == '0' && str[1] != '\0' && str[1] != '.')
        {
            const char next = str[1];
            if ((next >= '0' && next <= '9') ||
                (next >= 'A' && next <= 'F') ||
                (next >= 'a' && next <= 'f'))
            {
                hex = true;
                base = 16;
                ++str;  // skip '0' prefix
            }
            else
            {
                base = 10;
            }
        }
        else
        {
            base = 10;
        }
    }
    else if (base == 16 && *str == '0' && str[1] != '\0' && str[1] != '.')
    {
        hex = true;
        ++str;  // consume '0' prefix
    }

    using _UIntType = std::make_unsigned_t<_IntType>;

    // Compute overflow limits based on target type and sign
    _UIntType limit;
    if constexpr (std::is_signed_v<_IntType>)
    {
        // For negative: can go one past max (to represent min value in two's complement)
        // For positive: limited to max
        limit = neg ? (_UIntType(std::numeric_limits<_IntType>::max()) + 1u)
                      : _UIntType(std::numeric_limits<_IntType>::max());
    }
    else
    {
        limit = std::numeric_limits<_UIntType>::max();
    }

    _UIntType acc = 0;
    ushort ndigits = 0;
    bool fIgnoreZeroDigits = false;
    const char* startDigits = str;

    switch (base)
    {
        // Fast path: base 10 (most common)
        case 10:
        {
            const _UIntType maxDiv = limit / 10u;
            const _UIntType maxRem = limit % 10u;

            while (*str)
            {
                if (stop_at_len && (size_t(str - startDigits) >= stop_at_len))
                    break;

                const char c = *str;

                // Skip dots (Sphere convention: no true floats)
                if (c == '.')
                {
                    ++str;
                    continue;
                }

                if (c < '0' || c > '9')
                    break;

                const _UIntType digit = c - '0';

                // Check overflow before multiplying
                if (acc > maxDiv || (acc == maxDiv && digit > maxRem))
                    return false;

                acc = acc * 10u + digit;
                ++ndigits;
                ++str;
            }
        }
        // Fast path: base 16 (second most common)
        case 16:
        {
            const _UIntType maxDiv = limit / 16u;
            const _UIntType maxRem = limit % 16u;

            // Skip other leading zeroes.
            fIgnoreZeroDigits = (*str == '0');  // If the number is a single zero, it's still a valid hex num.
            while (*str == '0')
                ++str;
            startDigits = str;

            while (*str)
            {
                if (stop_at_len && (size_t(str - startDigits) >= stop_at_len))
                    break;

                const char c = *str;
                _UIntType digit;

                if (c >= '0' && c <= '9')
                    digit = c - '0';
                else if (c >= 'A' && c <= 'F')
                    digit = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f')
                    digit = c - 'a' + 10;
                else
                    break;

                // Check overflow
                if (acc > maxDiv || (acc == maxDiv && digit > maxRem))
                    return false;

                acc = acc * 16u + digit;
                ++ndigits;
                ++str;
            }

        }
        // Generic path: other bases (2-15, cold path)
        default:
        {
            const _UIntType base_casted = uint8_t(base);
            const _UIntType maxDiv = limit / base_casted;
            const _UIntType maxRem = limit % base_casted;

            while (*str)
            {
                if (stop_at_len && (size_t(str - startDigits) >= stop_at_len))
                    break;

                const char c = *str;
                _UIntType digit;

                if (c >= '0' && c <= '9')
                    digit = c - '0';
                else if (c >= 'A' && c <= 'F')
                    digit = c - 'A' + 10;
                else if (c >= 'a' && c <= 'f')
                    digit = c - 'a' + 10;
                else
                    break;

                // Validate digit is within base
                if (digit >= base_casted)
                    return false;

                // Check overflow
                if (acc > maxDiv || (acc == maxDiv && digit > maxRem))
                    return false;

                acc = acc * base_casted + digit;
                ++ndigits;
                ++str;
            }
        }
    } // switch

    if (!fIgnoreZeroDigits && ndigits == 0)
        return false;  // no valid digits consumed

    // Check trailing characters
    if (!ignore_trailing_extra_chars)
    {
        // Skip trailing whitespace
        while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
            ++str;

        if (*str != '\0')
            return false;  // unexpected trailing characters
    }

    if (fIgnoreZeroDigits && ndigits == 0)
    {
        *out = 0;
        return true;
    }

    // Sphere hex convention: ≤8 hex digits → interpret as signed 32-bit, then extend
    if (hex && ndigits <= 8)
    {
        // Reinterpret the bits as a signed 32 bit number, then expand that value to a 32 bit number.
        // if ndigits <= 8 (so number is always <= 0xFFFF FFFF), it will always be < UINT32_MAX.

        // ..Why Sphere expects this and works like this is unknown to me (maybe TUS/Grayworld or
        // prehistoric Sphere versions worked with 32 bit numbers instead of 64 bit).

        const int32_t v32 = static_cast<int32_t>(static_cast<uint32_t>(acc));

        // Apply the leading minus if present
        const int64_t v = neg ? -static_cast<int64_t>(v32) : static_cast<int64_t>(v32);
        // branchless:
        //uint64_t m = 0 - static_cast<uint64_t>(neg); // 0 or 0xFFFF..FFFF, defined modulo 2^64
        //int64_t v = static_cast<int64_t>((static_cast<uint64_t>(v64) ^ m) - m); // relies only on defined unsigned wraparound

        // Verify it fits in the target type (important for int8_t, int16_t)
        if constexpr (sizeof(_IntType) < sizeof(int32_t))
        {
            if (v32 < std::numeric_limits<_IntType>::min() ||
                v32 > std::numeric_limits<_IntType>::max())
                return false;
        }

        *out = static_cast<_IntType>(v);
        return true;
    }

    // Non-hex path. Store final result
    if constexpr (std::is_signed_v<_IntType>)
    {
        if (neg)
        {
            // Negate using well-defined unsigned arithmetic, then cast
            *out = static_cast<_IntType>(_UIntType(0) - acc);
        }
        else
        {
            *out = static_cast<_IntType>(acc);
        }
    }
    else
    {
        *out = static_cast<_IntType>(acc);
    }

    return true;
}

// Wrapper functions

std::optional<char> Str_ToI8 (const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    char val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<uchar> Str_ToU8 (const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    uchar val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<short> Str_ToI16 (const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    short val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<ushort> Str_ToU16 (const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    ushort val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<int> Str_ToI (const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    int val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<uint> Str_ToU(const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    uint val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<llong> Str_ToLL(const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    llong val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}

std::optional<ullong> Str_ToULL(const tchar * ptcStr, uint base, size_t uiStopAtLen, bool fIgnoreExcessChars) noexcept
{
    ullong val = 0;
    const bool fSuccess = cstr_to_num(ptcStr, &val, base, uiStopAtLen, fIgnoreExcessChars);
    if (!fSuccess)
        return std::nullopt;
    return val;
}


// --
// Number to String

/*
Hex width and two’s-complement:
After the hex marker '0', leading zeros are ignored for width selection.
Count significant hex digits starting at the first non-zero nibble:
    ≤ 8 significant digits → interpret the bit pattern as signed 32-bit two’s-complement (then widen to 64-bit to return).
    9..16 significant digits → interpret as signed 64-bit two’s-complement.
    16 significant digits → warn about 64-bit overflow, return −1, and consume the entire hex token.
*/

namespace str2int_detail
{

/*
// Uppercase hex digit table
static constexpr char DIGITS_UPPER[16] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
};

// Map a nibble [0,15] to its uppercase hex character
static constexpr char hexdig_upper(uint32_t v) noexcept
{
    return DIGITS_UPPER[v & 0xF];
}
*/

// Lowercase hex digit table
static constexpr char DIGITS_LOWER[16] = {
    '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
};

// Map a nibble [0,15] to its lowercase hex character
static constexpr char hexdig_lower(uint32_t v) noexcept
{
    return DIGITS_LOWER[v & 0xF];
}


// Compute hex digits to emit within a fixed width (8 or 16 nibbles) after
// trimming leading zero nibbles within that width.
// Special-case zero as exactly one hex digit ("0"), so final hex "00".
static constexpr int hex_digits_from_width(uint64_t u, int iWidthNibbles) noexcept
{
    if (iWidthNibbles == 8)
    {
        uint32 v = static_cast<uint32_t>(u);
        if (v == 0)
            return 1; // represent 0 as "0" (the caller will prefix a single '0' for "00")
        // Count leading zero bits in 32-bit domain, then convert to nibbles.
        // countl_zero(0) would be 32, but we have v != 0 here.
        int lz = std::countl_zero(v);
        return 8 - (lz / 4);
    }
    else
    {
        if (u == 0)
            return 1;
        int lz = std::countl_zero(u);        // counts in 64-bit domain
        return 16 - (lz / 4);
    }
}

// Base-10 two-digit lookup table (LUT): "00", "01", ..., "99" laid out consecutively.
// Emission logic divides by 100, uses remainder r in [0,99], and copies DEC_00_99[2*r+0..1].
// This halves the number of division/mod operations versus single-digit steps.
static constexpr std::array<char, 200> DEC_00_99 = []{
    std::array<char, 200> a{};
    for (int i = 0; i < 100; ++i) {
        const int tens = i / 10;            // exact for 0..99
        const int ones = i - tens * 10;     // exact for 0..99
        a[2*i + 0] = char('0' + tens);
        a[2*i + 1] = char('0' + ones);
    }
    return a;
}();

} // namespace

// Template entry point: returns ptcOutBuf on success, nullptr on failure.
// uiBase must be in [2,16]. Hex path (uiBase 16) emits lowercase and starts with '0'.
// Decimal path (uiBase 10) uses a two-digit LUT (lookup table) for throughput.
// Other bases use a generic fallback (reverse into tmp, then write forward).
template<typename _IntType>
tchar* Str_FromInt_Fast(_IntType val, tchar* ptcOutBuf, size_t uiBufLength, uint32 uiBase) noexcept
{
    static_assert(std::is_integral_v<_IntType>, "Str_FromInt_Fast requires an integral type");
    static_assert(sizeof(_IntType) <= 8, "Only up to 64-bit integers are supported");

    if (!ptcOutBuf || uiBufLength == 0)
    {
#ifdef _DEBUG
        g_Log.EventError("Str_FromInt_Fast: null buffer or zero length.\n");
#endif
        return nullptr;
    }
    if (uiBase < 2 || uiBase > 16)
    {
#ifdef _DEBUG
        g_Log.EventError("Str_FromInt_Fast: invalid base %u (supported 2..16).\n", uiBase);
#endif
        return nullptr;
    }

    // Zero fast path matches existing semantics exactly.
    if (val == 0)
    {
        if (uiBase == 16)
        {
            // Hex zero as "00"
            if (uiBufLength < 3)
            {
#ifdef _DEBUG
                g_Log.EventError("Str_FromInt_Fast[hex]: insufficient buffer (need 3 for \"00\\0\").\n");
#endif
                return nullptr;
            }
            ptcOutBuf[0] = '0';
            ptcOutBuf[1] = '0';
            ptcOutBuf[2] = '\0';
            return ptcOutBuf;
        }
        else
        {
            if (uiBufLength < 2)
            {
#ifdef _DEBUG
                g_Log.EventError("Str_FromInt_Fast[base=%u]: insufficient buffer (need 2 for \"0\\0\").\n", uiBase);
#endif
                return nullptr;
            }
            ptcOutBuf[0] = '0';
            ptcOutBuf[1] = '\0';
            return ptcOutBuf;
        }
    }

    // Specialize only 16 and 10; generic fallback for others.
    switch (uiBase)
    {
        // Sphere hex formatting:
        // - Always consider an input hex number in scripts as the representation of an unsigned number, but keep in mind that internally
        //      everything is converted to a signed number, so the upper numeric limit is INT64_MAX, not UINT64_MAX.
        // - The string is written as a 32 bit number if number < UINT32_MAX. Using INT32_MAX as upper limit is wrong,
        //      since we said that we consider the input string to be the representation of an unsigned number.
        //      So, if value fits in int32 (<= 0xFFFFFFFF) → 32-bit t.c.; else 64-bit t.c.
        // - Uppercase, prefix '0'
        // - Variable width after trimming within chosen width:
        case 16:
        {
            // Sphere hex formatting (uppercase, prefix '0', trimmed within 32/64-bit width).
            uint64_t u = 0;
            int width_nibbles = 0;

                // If type is <=32-bit or runtime value fits 32 bit integer, use 32-bit two's-complement view; else 64-bit.
                if (sizeof(_IntType) <= 4
                    || static_cast<int64_t>(val) <= static_cast<int64_t>(UINT32_MAX))
                {
                    // Casting through signed then to unsigned preserves the bit pattern modulo 2^32.
                    const uint32_t u32 = static_cast<uint32_t>(static_cast<int32_t>(val));
                    u = static_cast<uint64_t>(u32);
                    width_nibbles = 8;
                }
                else
                {
                    // View through 64-bit signed then to unsigned to preserve bit pattern.
                    u = static_cast<uint64_t>(static_cast<int64_t>(val));
                    width_nibbles = 16;
                }

            // Compute number of significant hex digits using count-leading-zero in the selected width.
            const int iDigits = str2int_detail::hex_digits_from_width(u, width_nibbles);

            // Required: '0' prefix + digits + NUL.
            const size_t need = static_cast<size_t>(iDigits) + 2;
            if (uiBufLength < need)
            {
#ifdef _DEBUG
                g_Log.EventWarn("Str_FromInt_Fast[hex]: insufficient buffer (need %" PRIuSIZE_T ", have %" PRIuSIZE_T ").\n", need, uiBufLength);
#endif
                return nullptr;
            }

            // Emit: '0' + exactly 'digits' hex characters (no re-emitted trimmed zeros).
            // Right-shift on unsigned is well-defined and zero-filling on the left.
            ptcOutBuf[0] = '0';
            size_t uiPos = 1;
            int iShift = (iDigits - 1) * 4; // start at the most significant non-zero nibble
            for (; iShift >= 0; iShift -= 4)
                ptcOutBuf[uiPos++] = str2int_detail::hexdig_lower(static_cast<uint32_t>((u >> iShift) & 0xFull));
            ptcOutBuf[uiPos] = '\0';
            return ptcOutBuf;
        }

        case 10:
        {
            // In Sphere scripting decimal numbers are always signed. There's no concept of unsigned number.
            // Base-10 fast path using two-digit LUT:
            // Strategy:
            //   1) Convert to unsigned magnitude |val| in a way that is correct even for INT_MIN.
            //   2) While magnitude >= 100: divide by 100 => quotient q and remainder r in [0,99].
            //   3) Use r to copy two chars from DEC_00_99 to a small reverse buffer.
            //   4) Emit the final one or two digits.
            //   5) Copy forward to out_buf (prepend '-' if original value was negative).
            using _UInt = std::make_unsigned_t<_IntType>;

            _UInt uiMagnitude;
            bool fNeg = false;

            if constexpr (std::is_signed_v<_IntType>)
            {
                // Two's-complement safe: interpret val as unsigned (no UB), then negate if negative.
                _UInt utwos = static_cast<_UInt>(val);
                fNeg = (val < 0);
                uiMagnitude = fNeg ? (_UInt(0) - utwos) : utwos;
            }
            else
            {
                uiMagnitude = static_cast<_UInt>(val);
            }

            // Reverse buffer; 32 bytes sufficient for 64-bit decimal (max 20 digits).
            tchar ptcTemp[32];
            size_t uiPos = 0;

            // Emit two digits per iteration using /100, significantly reducing division/mod count.
            while (uiMagnitude >= 100)
            {
                const _UInt q = uiMagnitude / 100;                    // quotient
                const uint32_t r = static_cast<uint32_t>(uiMagnitude - q * 100); // remainder in [0,99]
                // Store in reverse order: ones then tens, since we're building least significant first.
                ptcTemp[uiPos + 0] = str2int_detail::DEC_00_99[2 * r + 1];
                ptcTemp[uiPos + 1] = str2int_detail::DEC_00_99[2 * r + 0];
                uiPos += 2;
                uiMagnitude = q;
            }

            // Handle the last one or two digits without branching on zero.
            if (uiMagnitude < 10)
            {
                ptcTemp[uiPos++] = static_cast<tchar>('0' + static_cast<uint32_t>(uiMagnitude));
            }
            else
            {
                const uint32_t r = static_cast<uint32_t>(uiMagnitude); // 10..99
                ptcTemp[uiPos + 0] = str2int_detail::DEC_00_99[2 * r + 1];
                ptcTemp[uiPos + 1] = str2int_detail::DEC_00_99[2 * r + 0];
                uiPos += 2;
            }

            // Buffer need: optional '-' + digits + NUL.
            const size_t uiNeed = (fNeg ? 1 : 0) + uiPos + 1;
            if (uiBufLength < uiNeed)
            {
#ifdef _DEBUG
                g_Log.EventWarn("Str_FromInt_Fast[base=10]: insufficient buffer (need %" PRIuSIZE_T ", have %" PRIuSIZE_T ").\n", uiNeed, uiBufLength);
#endif
                return nullptr;
            }

            // Write sign and digits forward.
            size_t n = uiPos; // save digit count before resetting
            size_t pos = 0;
            if (fNeg)
                ptcOutBuf[pos++] = '-';
            while (n)
                ptcOutBuf[pos++] = ptcTemp[--n];
            ptcOutBuf[pos] = '\0';
            return ptcOutBuf;
        }

        default:
        {
            // Generic fallback for other bases in [2,16], reverse-then-forward.
            using _UInt = std::make_unsigned_t<_IntType>;

            _UInt uiMagnitude;
            bool fNeg = false;

            if constexpr (std::is_signed_v<_IntType>)
            {
                const _UInt utwos = static_cast<_UInt>(val);
                fNeg = (val < 0);
                uiMagnitude = fNeg ? (_UInt(0) - utwos) : utwos;
            }
            else
            {
                uiMagnitude = static_cast<_UInt>(val);
            }

            // 65 bytes: 64 bits in base 2 = 64 digits max, plus room for sign handling if needed
            tchar ptcTemp[65];
            size_t n = 0;
            const _UInt B = static_cast<_UInt>(uiBase);

            // Repeated division/modulo; acceptable since this path is cold in our workload.
            do
            {
                const _UInt q = uiMagnitude / B;
                const uint32_t r = static_cast<uint32_t>(uiMagnitude - q * B);
                uiMagnitude = q;
                ptcTemp[n++] = str2int_detail::DIGITS_LOWER[r];
            }
            while (uiMagnitude);

            const size_t uiNeed = (fNeg ? 1 : 0) + n + 1;
            if (uiBufLength < uiNeed)
            {
#ifdef _DEBUG
                g_Log.EventWarn("Str_FromInt_Fast[base=%u]: insufficient buffer (need %" PRIuSIZE_T ", have %" PRIuSIZE_T ").\n", uiBase, uiNeed, uiBufLength);
#endif
                return nullptr;
            }

            size_t uiPos = 0;
            if (fNeg)
                ptcOutBuf[uiPos++] = '-';
            while (n)
                ptcOutBuf[uiPos++] = ptcTemp[--n];
            ptcOutBuf[uiPos] = '\0';
            return ptcOutBuf;
        }
    } // switch
}

// Typed front-writing wrappers: they return buf on success, nullptr on failure.
// For now keep _Fast and standard variants. They were there for historical purposes (previous implementation was back-writing).
tchar* Str_FromI_Fast(int val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromUI_Fast(uint val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromLL_Fast (llong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

tchar* Str_FromULL_Fast (ullong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    return Str_FromInt_Fast(val, buf, buf_length, base);
}

void Str_FromI(int val, tchar* buf, size_t buf_length, uint base) noexcept
{
    (void) Str_FromI_Fast(val, buf, buf_length, base);
}

void Str_FromUI(uint val, tchar* buf, size_t buf_length, uint base) noexcept
{
    (void) Str_FromUI_Fast(val, buf, buf_length, base);
}

void Str_FromLL(llong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    (void) Str_FromLL_Fast(val, buf, buf_length, base);
}

void Str_FromULL(ullong val, tchar* buf, size_t buf_length, uint base) noexcept
{
    (void) Str_FromULL_Fast(val, buf, buf_length, base);
}


size_t FindStrWord( lpctstr_restrict pTextSearch, lpctstr_restrict pszKeyWord ) noexcept
{
    // Find any of the pszKeyWord in the pTextSearch string.
    // Make sure we look for starts of words.

    size_t j = 0;
    for ( size_t i = 0; ; ++i )
    {
        if ( pszKeyWord[j] == '\0' || pszKeyWord[j] == ',')
        {
            if ( pTextSearch[i]== '\0' || IsWhitespace(pTextSearch[i]))
                return( i );
            j = 0;
        }
        if ( pTextSearch[i] == '\0' )
        {
            pszKeyWord = strchr(pszKeyWord, ',');
            if (pszKeyWord)
            {
                ++pszKeyWord;
                i = 0;
                j = 0;
            }
            else
                return 0;
        }
        if ( j == 0 && i > 0 )
        {
            if ( IsAlpha( pTextSearch[i-1] ))	// not start of word ?
                continue;
        }
        if ( toupper( pTextSearch[i] ) == toupper( pszKeyWord[j] ))
            ++j;
        else
            j = 0;
    }
}

int Str_CmpHeadI(lpctstr_restrict ptcFind, lpctstr_restrict ptcHere) noexcept
{
    for (uint i = 0; ; ++i)
    {
		//	we should always use same case as in other places. since strcmpi lowers,
        //	we should lower here as well. if strcmpi changes, we have to change it here as well
        const tchar ch1 = static_cast<tchar>(tolower(ptcFind[i]));
        const tchar ch2 = static_cast<tchar>(tolower(ptcHere[i]));
        if (ch2 == 0)
        {
            if ( (!isalnum(ch1)) && (ch1 != '_') )
                return 0;
            return (ch1 - ch2);
        }
        if (ch1 != ch2)
            return (ch1 - ch2);
    }
}

static inline int Str_CmpHeadI_Table(const tchar * ptcFind, const tchar * ptcTable) noexcept
{
    for (uint i = 0; ; ++i)
    {
        const tchar ch1 = static_cast<tchar>(toupper(ptcFind[i]));
        const tchar ch2 = ptcTable[i];
        ASSERT(ch2 == toupper(ch2));    // for better performance, in the table all the names have to be LOWERCASE!
        if (ch2 == 0)
        {
            if ( (!isalnum(ch1)) && (ch1 != '_') )
                return 0;
            return (ch1 - ch2);
        }
        if (ch1 != ch2)
            return (ch1 - ch2);
    }
}

// String utilities: Modifiers

// Useful also for s(n)printf!
int StrncpyCharBytesWritten(int iBytesToWrite, size_t uiBufSize, bool fPrintError)
{
    if (iBytesToWrite < 0)
        return 0;
    if (uiBufSize < 1)
        goto err;
    if ((uint)iBytesToWrite >= uiBufSize - 1)
        goto err;
    return iBytesToWrite;

err:
    //throw CSError(LOGL_ERROR, 0, "Buffer size too small for snprintf.\n");
    if (fPrintError) {
        g_Log.EventError("Buffer size too small for snprintf.\n");
    }
    return (uiBufSize > 1) ? int(uiBufSize - 1) : 0; // Bytes written, excluding the string terminator.
}

bool IsStrEmpty( const tchar * pszTest ) noexcept
{
    if ( !pszTest || !*pszTest )
        return true;

    do
    {
        if ( !IsSpace(*pszTest) )
            return false;
    }
    while ( *(++pszTest) );
    return true;
}

bool IsStrNumericDec( const tchar * pszTest ) noexcept
{
    if ( !pszTest || !*pszTest )
        return false;

    do
    {
        if ( !IsDigit(*pszTest) )
            return false;
    }
    while ( *(++pszTest) );

    return true;
}


bool IsStrNumeric( const tchar * pszTest ) noexcept
{
    if ( !pszTest || !*pszTest )
        return false;

    bool fHex = false;
    if ( pszTest[0] == '0' )
        fHex = true;

    do
    {
        if ( IsDigit( *pszTest ) )
            continue;
        if ( fHex && tolower(*pszTest) >= 'a' && tolower(*pszTest) <= 'f' )
            continue;
        return false;
    }
    while ( *(++pszTest) );
    return true;
}

bool IsSimpleNumberString( lpctstr_restrict pszTest ) noexcept
{
    // is this a string or a simple numeric expression ?
    // string = 1 2 3, sdf, sdf sdf sdf, 123d, 123 d,
    // number = 1.0+-\*~|&!%^()2, 0aed, 123

    if (*pszTest == '\0')
        return false;   // empty string, no number

    bool fMathSep			= true;	// last non whitespace was a math sep.
    bool fHextDigitStart	= false;
    bool fWhiteSpace		= false;

    for ( ; ; ++pszTest )
    {
        tchar ch = *pszTest;
        if ( ! ch )
            return true;

        if (( ch >= 'A' && ch <= 'F') || ( ch >= 'a' && ch <= 'f' ))	// isxdigit
        {
            if ( ! fHextDigitStart )
                return false;

            fWhiteSpace = false;
            fMathSep = false;
            continue;
        }
        if ( IsSpace( ch ) )
        {
            fHextDigitStart = false;
            fWhiteSpace = true;
            continue;
        }
        if ( IsDigit( ch ) )
        {
            if ( fWhiteSpace && ! fMathSep )
                return false;

            if ( ch == '0' )
                fHextDigitStart = true;
            fWhiteSpace = false;
            fMathSep = false;
            continue;
        }
        if ( ch == '/' && pszTest[1] != '/' )
            fMathSep = true;
        else
            fMathSep = strchr("+-\\*~|&!%^()", ch ) ? true : false ;

        if ( ! fMathSep )
            return false;

        fHextDigitStart = false;
        fWhiteSpace = false;
    }
}


// strcpy doesn't have an argument to truncate the copy to the buffer length;
// strncpy doesn't null-terminate if it truncates the copy, and if uiMaxlen is > than the source string length, the remaining space is filled with '\0'
size_t Str_CopyLimit(lptstr_restrict pDst, lpctstr_restrict pSrc, const size_t uiMaxSize) noexcept
{
    if (uiMaxSize == 0) [[unlikely]]
        return 0;

    if (pSrc[0] == '\0') [[unlikely]]
    {

        pDst[0] = '\0';
        return 0;
    }

    // Find string terminator within the first uiMaxSize bytes (fast library call, usually vectorized)
    const void* nul = memchr(pSrc, '\0', uiMaxSize);
    const size_t toCopy = nul
                        ? ((static_cast<const char*>(nul) - pSrc) + 1) // +1 to include the terminator
                        : uiMaxSize;    // No terminator in range: copy full limit

    memcpy(pDst, pSrc, toCopy);
    return toCopy; // bytes copied in pDst string (CAN count the string terminator)
}

size_t Str_CopyLimitNull(lptstr_restrict pDst, lpctstr_restrict pSrc, size_t uiMaxSize) noexcept
{
    if (uiMaxSize == 0) [[unlikely]]
    {
        return 0;
    }
    if (pSrc[0] == '\0') [[unlikely]]
    {
        pDst[0] = '\0';
        return 0;
    }

    // Reserve one byte for the string terminator
    const size_t uiCopyMax = uiMaxSize - 1;

    // Find the terminator within the first copyMax bytes (fast memchr)
    const void* nul = memchr(pSrc, '\0', uiCopyMax);
    const size_t len = nul
                          ? (static_cast<const char*>(nul) - pSrc)  // length up to the terminator
                          : uiCopyMax;                                // no terminator found within limit

    // Copy the determined length and append terminator
    memcpy(pDst, pSrc, len);
    pDst[len] = '\0';

    return len; // bytes copied in pDst string (not counting the string terminator)
}

/*
// Copy up to max_len-1 chars from src to dst, NUL-terminate.
// Returns number of chars written (excluding the terminator).
size_t Str_CopyLimitNull_ShortStr(char* dst, const char* src, size_t max_len)
{
    if (max_len == 0) [[unlikely]]
        return 0;

    char* const start = dst;
    size_t remaining = max_len - 1;

    // Use 64-bit words on modern 64-bit targets
    using word_t = std::conditional_t<sizeof(void*) >= 8, uint64_t, uint32_t>;
    constexpr ushort W = static_cast<ushort>(sizeof(word_t));

    // Magic constants for zero detection
    // lomagic (0x0101010101010101ULL) subtracts 1 from each byte.
    // himagic (0x8080808080808080ULL) is used to mask out the high bit of each byte.
    // The expression ((w - lomagic) & ~w & himagic) evaluates to non-zero if any byte in the word was zero.
    constexpr word_t lomagic = (W == 8 ? 0x0101010101010101ULL : 0x01010101U);
    constexpr word_t himagic = (W == 8 ? 0x8080808080808080ULL : 0x80808080U);

    // Word-wise loop (assumes unaligned loads/stores are fast)
    while (remaining >= W)
    {
        // Direct unaligned load/store, acceptable for short strings
        word_t w = *reinterpret_cast<const word_t*>(src);
        *reinterpret_cast<word_t*>(dst) = w;

        // Detect any zero byte
        if (((w - lomagic) & ~w & himagic) != 0)
        {
            // Scan this word byte-by-byte for the exact NUL
            for (size_t i = 0; i < W; ++i)
            {
                char c = src[i];
                dst[i] = c;
                if (c == '\0')
                    return (dst + i) - start;
            }
            // Should never get here
            ASSERT(false);
        }
        src += W;
        dst += W;
        remaining -= W;
    }

    // Tail bytes
    while (remaining-- > 0)
    {
        const char c = *src++;
        *dst++ = c;
        if (c == '\0')
            return dst - start - 1;
    }

    // Buffer full – append NUL
    *dst = '\0';
    return dst - start;
}
*/

// Acceptable overhead for out use cases (non-performance critical).
size_t Str_CopyLen(lptstr_restrict pDst, lpctstr_restrict pSrc) noexcept
{
    strcpy(pDst, pSrc);
    return strlen(pDst);
}

// the number of characters in a multibyte string is the sum of mblen()'s
// note: the simpler approach is std::mbstowcs(NULL, s.c_str(), s.size())
/*
size_t strlen_mb(const char* ptr)
{
    // From: https://en.cppreference.com/w/c/string/multibyte/mblen

    // ensure that at some point we have called setlocale:
    //--    // allow mblen() to work with UTF-8 multibyte encoding
    //--    std::setlocale(LC_ALL, "en_US.utf8");

    size_t result = 0;
    const char* end = ptr + strlen(ptr);
    mblen(nullptr, 0); // reset the conversion state
    while(ptr < end) {
        int next = mblen(ptr, end - ptr);
        if(next == -1) {
            throw std::runtime_error("strlen_mb(): conversion error");
            break;
        }
        ptr += next;
        ++result;
    }
    return result;
}
*/

size_t Str_UTF8CharCount(const char* strInUTF8MB) noexcept
{
    size_t len; // number of characters in the string
#ifdef MSVC_RUNTIME
    mbstowcs_s(&len, nullptr, 0, strInUTF8MB, 0); // includes null terminator
    len -= 1;
#else
    len = mbstowcs(nullptr, strInUTF8MB, 0); // not including null terminator
#endif
    return len;
}

/*
* Appends src to string dst of size siz (unlike strncat, siz is the
* full size of dst, not space left). At most siz-1 characters
* will be copied. Always NULL terminates (unless siz <= strlen(dst)).
* Returns strlen(src) + MIN(siz, strlen(initial dst)). Count does not include '\0'
* If retval >= siz, truncation occurs.
*/
// Adapted from: OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38
size_t Str_ConcatLimitNull(tchar *dst, const tchar *src, size_t siz) noexcept
{
    if (siz == 0)
        return 0;

    tchar *d = dst;
    size_t n = siz;
    size_t dlen;

    /* Find the end of dst and adjust bytes left but don't go past end */
    while ((n-- != 0) && (*d != '\0'))
    {
        ++d;
    }
    dlen = d - dst;
    n = siz - dlen;

    if (n == 0)
    {
        return (dlen + strlen(src));
    }

    const tchar *s = src;
    while (*s != '\0')
    {
        if (n != 1)
        {
            *d++ = *s;
            --n;

        }
        ++s;
    }
    *d = '\0';

    return (dlen + (s - src));	/* count does not include '\0' */
}

tchar* Str_FindSubstring(lptstr_restrict str, lpctstr_restrict substr, size_t str_len, size_t substr_len) noexcept
{
    if (str_len == 0 || substr_len == 0)
        return nullptr;

    tchar c, sc;
    if ((c = *substr++) != '\0')
    {
        do
        {
            do
            {
                if (str_len < 1 || (sc = *str++) == '\0')
                {
                    return nullptr;
                }
                str_len -= 1;
            } while (sc != c);
            if (substr_len > str_len)
            {
                return nullptr;
            }
        } while (0 != strncmp(str, substr, substr_len));
        --str;
    }
    return str;
}

const tchar * Str_GetArticleAndSpace(lpctstr_restrict pszWord) noexcept
{
    // NOTE: This is wrong many times.
    //  ie. some words need no article (plurals) : boots.

    // This function shall NOT be called if OF_NoPrefix is set!

    if (pszWord)
    {
        static constexpr tchar sm_Vowels[] = { 'A', 'E', 'I', 'O', 'U' };
        tchar chName = static_cast<tchar>(toupper(pszWord[0]));
        for (uint x = 0; x < ARRAY_COUNT(sm_Vowels); ++x)
        {
            if (chName == sm_Vowels[x])
                return "an ";
        }
    }
    return "a ";
}

int Str_GetBare(tchar * ptcOut, const tchar *ptcSrc, size_t uiMaxOutSize, const tchar * ptcStripList) noexcept
{
    // That the client can deal with. Basic punctuation and alpha and numbers.
    // RETURN: Output length.

    if (!ptcOut || !ptcSrc || uiMaxOutSize == 0)
        return 0;

    // Default strip set if none provided: client can't print these
    ptcStripList = ptcStripList ? ptcStripList : "{|}~";

    tchar* out          = ptcOut;
    tchar* const outEnd = ptcOut + (uiMaxOutSize - 1);

    // Process each char until SRC ends or output buffer is full
    for (; *ptcSrc && out < outEnd; ++ptcSrc)
    {
        const uchar ch = uchar(*ptcSrc);

        if (ch < ' ' || ch >= 127)  // or !std::isprint(ch)
            continue;	// Special format chars.
        if (strchr(ptcStripList, ch))
            continue;

        *out++ = tchar(ch);
    }

    // NUL-terminate and return length
    *out = '\0';
    return int(out - ptcOut);
}

/* Old impl.
int Str_GetBare(tchar * pszOut, const tchar * pszInp, int iMaxOutSize, const tchar * pszStrip) noexcept
{
    // That the client can deal with. Basic punctuation and alpha and numbers.
    // RETURN: Output length.

    if (!pszStrip)
        pszStrip = "{|}~";	// client can't print these.

    //GETNONWHITESPACE( pszInp );	// kill leading white space.

    int j = 0;
    for (int i = 0; ; ++i)
    {
        tchar ch = pszInp[i];
        if (ch)
        {
            if (ch < ' ' || ch >= 127)
                continue;	// Special format chars.

            int k = 0;
            while (pszStrip[k] && pszStrip[k] != ch)
                ++k;

            if (pszStrip[k])
                continue;

            if (j >= iMaxOutSize - 1)
                ch = '\0';
        }
        pszOut[j++] = ch;
        if (ch == 0)
            break;
    }
    return (j - 1);
}
*/

tchar * Str_MakeFiltered(lptstr_restrict pStr) noexcept
{
    int len = (int)strlen(pStr);
    for (int i = 0; len; ++i, --len)
    {
        if (pStr[i] == '\\')
        {
            switch (pStr[i + 1])
            {
                case 'b': pStr[i] = '\b'; break;
                case 'n': pStr[i] = '\n'; break;
                case 'r': pStr[i] = '\r'; break;
                case 't': pStr[i] = '\t'; break;
                case '\\': pStr[i] = '\\'; break;
            }
            --len;
            memmove(pStr + i + 1, pStr + i + 2, len);
        }
    }
    return pStr;
}

void Str_MakeUnFiltered(tchar * pStrOut, const tchar * pStrIn, int iSizeMax) noexcept
{
    int len = (int)strlen(pStrIn);
    int iIn = 0;
    int iOut = 0;
    for (; iOut < iSizeMax && iIn <= len; ++iIn, ++iOut)
    {
        tchar ch = pStrIn[iIn];
        switch (ch)
        {
            case '\b': ch = 'b'; break;
            case '\n': ch = 'n'; break;
            case '\r': ch = 'r'; break;
            case '\t': ch = 't'; break;
            case '\\': ch = '\\'; break;
            default:
                pStrOut[iOut] = ch;
                continue;
        }

        pStrOut[iOut++] = '\\';
        pStrOut[iOut] = ch;
    }
}

// Unquotes and trims whitespace in-place, shifting result to start of buffer.
void Str_MakeUnQuoted(tchar* pStr) noexcept
{
    tchar* src = pStr;
    // Skip leading whitespace (GETNONWHITESPACE)
    while (IsWhitespace(*src)) {
        ++src;
    }

    bool fQuoted = false;
    if (*src == '"')
    {
        fQuoted = true;
        ++src;
    }

    tchar* endPtr = src + std::strlen(src);

    // If quoted, locate servClosing quote and adjust endPtr
    if (fQuoted)
    {
        tchar* p = endPtr;
        while (p > src)
        {
            --p;
            if (*p == '"')
            {
                endPtr = p;
                break;
            }
        }
    }

    // Compute trimmed length (exclude trailing whitespace), like Str_TrimWhitespace.
    size_t len = endPtr - src;
    while (len > 0 && IsWhitespace(src[len - 1])) {
        --len;
    }

    // Shift content to the start of the buffer
    if (src != pStr && len > 0) {
        std::memmove(pStr, src, (len * sizeof(tchar)));
    }
    pStr[len] = '\0';
}

// Returns a pointer to the unquoted part (and overwrites the last quote with a string terminator '\0' char)
tchar * Str_GetUnQuoted(lptstr_restrict pStr) noexcept
{
    GETNONWHITESPACE(pStr);
    if (*pStr != '"')
    {
        Str_TrimEndWhitespace(pStr, int(strlen(pStr)));
        return pStr;
    }

    ++pStr;
    // search for last quote symbol starting from the end
    tchar * pEnd = pStr + strlen(pStr) - 1;
    for (; pEnd >= pStr; --pEnd )
    {
        if ( *pEnd == '"' )
        {
            *pEnd = '\0';
            break;
        }
    }

    Str_TrimEndWhitespace(pStr, int(pEnd - pStr));
    return pStr;
}

int Str_TrimEndWhitespace(tchar * pStr, int len) noexcept
{
    if (!pStr) [[unlikely]]
        return -1;
    while (len > 0 && IsWhitespace(pStr[len - 1])) {
        --len;
    }
    pStr[len] = '\0';
    return len;
}

tchar * Str_TrimWhitespace(tchar * pStr) noexcept
{
    if (!pStr) [[unlikely]]
        return nullptr;
    GETNONWHITESPACE(pStr);
    Str_TrimEndWhitespace(pStr, (int)strlen(pStr));
    return pStr;
}

void Str_EatEndWhitespace(const tchar* const pStrBegin, tchar*& pStrEnd) noexcept
{
    if (pStrBegin == pStrEnd) [[unlikely]]
        return;

    tchar* ptcPrev = pStrEnd - 1;
    while ((ptcPrev != pStrBegin) && IsWhitespace(*ptcPrev))
    {
        if (*ptcPrev == '\0')
            return;

        --pStrEnd;
        ptcPrev = pStrEnd - 1;
    }
}

void Str_SkipEnclosedAngularBrackets(tchar*& ptcLine) noexcept
{
    // Move past a < > statement. It can have ( ) inside, if it happens, ignore < > characters inside ().
    bool fOpenedOneAngular = false;
    int iOpenAngular = 0, iOpenCurly = 0;
    tchar* ptcTest = ptcLine;
    while (const tchar ch = *ptcTest)
    {
        if (IsWhitespace(ch))
            ;
        else if (ch == '(')
            ++iOpenCurly;
        else if (ch == ')')
            --iOpenCurly;
        else if (iOpenCurly == 0)
        {
            if (ch == '<')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '<') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    // I guess i have sufficient proof: skip, it's a << operator
                    fOperator = true;
                }
                if (!fOperator)
                {
                    fOpenedOneAngular = true;
                    ++iOpenAngular;
                }
                else
                    ptcTest += 2; // Skip the second > and the following whitespace
            }
            else if (ch == '>')
            {
                bool fOperator = false;
                if ((ptcTest[1] == '>') && (ptcTest[2] != '\0') && IsWhitespace(ptcTest[2]))
                {
                    if ((ptcLine == ptcTest) || ((iOpenAngular > 0) && IsWhitespace(*(ptcTest - 1))))
                    {
                        // I guess i have sufficient proof: skip, it's a >> operator
                        fOperator = true;
                    }
                }
                if (!fOperator)
                {
                    --iOpenAngular;
                    if (fOpenedOneAngular && !iOpenAngular)
                    {
                        ptcLine = ptcTest + 1;
                        return;
                    }
                }
                else
                    ptcTest += 2; // Skip the second > and the following whitespace
            }
        }
        ++ptcTest;
    }
}


// String utilities: String operations

int FindTable(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept
{
    // A non-sorted table.
    for (int i = 0; i < iCount; ++i)
    {
        if (!strcmpi(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableSorted(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept
{
    // Do a binary search (un-cased) on a sorted table.
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const int iCompare = strcmpi(ptcFind, pptcTable[i]);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;

    /*
    // Alternative implementation. Logarithmic time, but better use of CPU instruction pipelining and branch prediction, at the cost of more comparations.
    // It's worth running some benchmarks before switching to this.
    const tchar * const* base = pptcTable;
    if (iCount > 1)
    {
        do
        {
            const int half = iCount >> 1;
            base = (strcmpi(base[half], ptcFind) < 0) ? &base[half] : base;
            iCount -= half;
        } while (iCount > 1);
        base = (strcmpi(*base, ptcFind) < 0) ? &base[1] : base;
    }
    return (0 == strcmpi(*base, ptcFind)) ? int(base - pptcTable) : -1;
    */
}

int FindTableHead(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE
{
    for (int i = 0; i < iCount; ++i)
    {
        if (!Str_CmpHeadI_Table(pptcTable[i], ptcFind))
            return i;
    }
    return -1;
}

int FindTableHeadSorted(const tchar * ptcFind, const tchar * const * pptcTable, int iCount) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found

    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const int iCompare = Str_CmpHeadI_Table(ptcFind, pptcTable[i]);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;

    /*
    // Alternative implementation. Logarithmic time, but better use of CPU instruction pipelining and branch prediction, at the cost of more comparations.
    // It's worth running some benchmarks before switching to this.
    const tchar * const* base = pptcTable;
    if (iCount > 1)
    {
        do
        {
            const int half = iCount >> 1;
            base = (Str_CmpHeadI_Table(base[half], ptcFind) < 0) ? &base[half] : base;
            iCount -= half;
        } while (iCount > 1);
        base = (Str_CmpHeadI_Table(*base, ptcFind) < 0) ? &base[1] : base;
    }
    return (0 == Str_CmpHeadI_Table(*base, ptcFind)) ? int(base - pptcTable) : -1;
    */
}

int FindCAssocRegTableHeadSorted(const tchar * pszFind, const tchar * const* ppszTable, int iCount, size_t uiElemSize) noexcept // REQUIRES the table to be UPPERCASE, and sorted
{
    // Do a binary search (un-cased) on a sorted table.
    // Uses Str_CmpHeadI, which checks if we have reached, during comparison, ppszTable end ('\0'), ignoring if pszFind is longer (maybe has arguments?)
    // RETURN: -1 = not found
    if (iCount < 1)
        return -1;
    int iHigh = iCount - 1; // Count starts from 1, array index from 0.
    int iLow = 0;

    while (iLow <= iHigh)
    {
        const int i = (iHigh + iLow) >> 1;
        const tchar * pszName = *(reinterpret_cast<const tchar * const*>(reinterpret_cast<const byte*>(ppszTable) + (i * uiElemSize)));
        const int iCompare = Str_CmpHeadI_Table(pszFind, pszName);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;
}

bool Str_Untrusted_InvalidTermination(const tchar * pszIn, size_t uiMaxAcceptableSize) noexcept
{
    if (pszIn == nullptr)
        return true;

    const tchar * p = pszIn;
    while ((*p != '\0') && (*p != 0x0A /* '\n' */) && (*p != 0x0D /* '\r' */)
           && ((p - pszIn) < ptrdiff_t(uiMaxAcceptableSize)))
    {
        ++p;
    }

    return (*p != '\0');
}

bool Str_Untrusted_InvalidName(const tchar * pszIn, size_t uiMaxAcceptableSize) noexcept
{
    if (pszIn == nullptr)
        return true;

    const tchar * p = pszIn;
    while (*p != '\0' && ((p - pszIn) < ptrdiff_t(uiMaxAcceptableSize))
        &&  (
            ((*p >= 'A') && (*p <= 'Z')) ||
            ((*p >= 'a') && (*p <= 'z')) ||
            ((*p >= '0') && (*p <= '9')) ||
            ((*p == ' ') || (*p == '\'') || (*p == '-') || (*p == '.'))
            ))
        ++p;

    return (*p != '\0');
}

int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset) noexcept
{
    if (offset < 0)
        return -1;

    int len = (int)strlen(pStr1);
    if (offset >= len)
        return -1;

    int slen = (int)strlen(pStr2);
    if (slen > len)
        return -1;

    tchar firstChar = pStr2[0];

    for (int i = offset; i < len; ++i)
    {
        tchar c = pStr1[i];
        if (c == firstChar)
        {
            int rem = len - i;
            if (rem >= slen)
            {
                int j = i;
                int k = 0;
                bool found = true;
                while (k < slen)
                {
                    if (pStr1[j] != pStr2[k])
                    {
                        found = false;
                        break;
                    }
                    ++j; ++k;
                }
                if (found)
                    return i;
            }
        }
    }

    return -1;
}

static MATCH_TYPE Str_Match_After_Star(const tchar * pPattern, const tchar * pText) noexcept
{
    // pass over existing ? and * in pattern
    for (; *pPattern == '?' || *pPattern == '*'; ++pPattern)
    {
        // take one char for each ? and +
        if (*pPattern == '?' &&
            !*pText++)		// if end of text then no match
            return MATCH_ABORT;
    }

    // if end of pattern we have matched regardless of text left
    if (!*pPattern)
        return MATCH_VALID;

    // get the next character to match which must be a literal or '['
    tchar nextp = static_cast<tchar>(tolower(*pPattern));
    MATCH_TYPE match = MATCH_INVALID;

    // Continue until we run out of text or definite result seen
    do
    {
        // a precondition for matching is that the next character
        // in the pattern match the next character in the text or that
        // the next pattern char is the beginning of a range.  Increment
        // text pointer as we go here
        if (nextp == tolower(*pText) || nextp == '[')
        {
            match = Str_Match(pPattern, pText);
            if (match == MATCH_VALID)
                break;
        }

        // if the end of text is reached then no match
        if (!*pText++)
            return MATCH_ABORT;

    } while (
        match != MATCH_ABORT &&
        match != MATCH_PATTERN);

    return match;	// return result
}

MATCH_TYPE Str_Match(const tchar * pPattern, const tchar * pText) noexcept
{
    // case independant

    tchar range_start;
    tchar range_end;  // start and end in range

    for (; *pPattern; ++pPattern, ++pText)
    {
        // if this is the end of the text then this is the end of the match
        if (!*pText)
            return (*pPattern == '*' && *++pPattern == '\0') ? MATCH_VALID : MATCH_ABORT;

        // determine and react to pattern type
        switch (*pPattern)
        {
            // single any character match
            case '?':
                break;
                // multiple any character match
            case '*':
                return Str_Match_After_Star(pPattern, pText);
                // [..] construct, single member/exclusion character match
            case '[':
            {
                // move to beginning of range
                ++pPattern;
                // check if this is a member match or exclusion match
                bool fInvert = false;             // is this [..] or [!..]
                if (*pPattern == '!' || *pPattern == '^')
                {
                    fInvert = true;
                    ++pPattern;
                }
                // if servClosing bracket here or at range start then we have a
                // malformed pattern
                if (*pPattern == ']')
                    return MATCH_PATTERN;

                bool fMemberMatch = false;       // have I matched the [..] construct?
                for (;;)
                {
                    // if end of construct then fLoop is done
                    if (*pPattern == ']')
                        break;

                    // matching a '!', '^', '-', '\' or a ']'
                    if (*pPattern == '\\')
                        range_start = range_end = static_cast<tchar>(tolower(*++pPattern));
                    else
                        range_start = range_end = static_cast<tchar>(tolower(*pPattern));

                    // if end of pattern then bad pattern (Missing ']')
                    if (!*pPattern)
                        return MATCH_PATTERN;

                    // check for range bar
                    if (*++pPattern == '-')
                    {
                        // get the range end
                        range_end = static_cast<tchar>(tolower(*++pPattern));
                        // if end of pattern or construct then bad pattern
                        if (range_end == '\0' || range_end == ']')
                            return MATCH_PATTERN;
                        // special character range end
                        if (range_end == '\\')
                        {
                            range_end = static_cast<tchar>(tolower(*++pPattern));
                            // if end of text then we have a bad pattern
                            if (!range_end)
                                return MATCH_PATTERN;
                        }
                        // move just beyond this range
                        ++pPattern;
                    }

                    // if the text character is in range then match found.
                    // make sure the range letters have the proper
                    // relationship to one another before comparison
                    tchar chText = static_cast<tchar>(tolower(*pText));
                    if (range_start < range_end)
                    {
                        if (chText >= range_start && chText <= range_end)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                    else
                    {
                        if (chText >= range_end && chText <= range_start)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                }	// while

                    // if there was a match in an exclusion set then no match
                    // if there was no match in a member set then no match
                if ((fInvert && fMemberMatch) ||
                    !(fInvert || fMemberMatch))
                    return MATCH_RANGE;

                // if this is not an exclusion then skip the rest of the [...]
                //  construct that already matched.
                if (fMemberMatch)
                {
                    while (*pPattern != ']')
                    {
                        // bad pattern (Missing ']')
                        if (!*pPattern)
                            return MATCH_PATTERN;
                        // skip exact match
                        if (*pPattern == '\\')
                        {
                            ++pPattern;
                            // if end of text then we have a bad pattern
                            if (!*pPattern)
                                return MATCH_PATTERN;
                        }
                        // move to next pattern char
                        ++pPattern;
                    }
                }
            }
            break;

            // must match this character (case independant) ?exactly
            default:
                if (tolower(*pPattern) != tolower(*pText))
                    return MATCH_LITERAL;
        }
    }
    // if end of text not reached then the pattern fails
    if (*pText)
        return MATCH_END;
    else
        return MATCH_VALID;
}

tchar * Str_UnQuote(tchar * pStr) noexcept
{
    GETNONWHITESPACE(pStr);

    tchar ch = *pStr;
    if ((ch == '"') || (ch == '\''))
        ++pStr;

    for (tchar *pEnd = pStr + strlen(pStr) - 1; pEnd >= pStr; --pEnd)
    {
        if ((*pEnd == '"') || (*pEnd == '\''))
        {
            *pEnd = '\0';
            break;
        }
    }
    return pStr;
}

int Str_RegExMatch(const tchar * pPattern, const tchar * pText, tchar * lastError)
{
    try
    {
        CRegexp expressionformatch(pPattern, NO_FLAG);
        MatchResult result = expressionformatch.Match(pText);
        if (result.IsMatched())
            return 1;

        return 0;
    }
    catch (const std::bad_alloc &e)
    {
        Str_CopyLimitNull(lastError, e.what(), SCRIPT_MAX_LINE_LEN);
        GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
    catch (...)
    {
        Str_CopyLimitNull(lastError, "Unknown", SCRIPT_MAX_LINE_LEN);
        GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
}

//--

void CharToMultiByteNonNull(byte * Dest, const char * Src, int MBytes) noexcept
{
    for (int idx = 0; idx != MBytes * 2; idx += 2) {
        if (Src[idx / 2] == '\0')
            break;
        Dest[idx] = (byte)(Src[idx / 2]);
    }
}

UTF8MBSTR::UTF8MBSTR() = default;

UTF8MBSTR::UTF8MBSTR(const tchar * lpStr)
{
    operator=(lpStr);
}

UTF8MBSTR::UTF8MBSTR(UTF8MBSTR& lpStr)
{
    m_strUTF8_MultiByte = lpStr.m_strUTF8_MultiByte;
}

UTF8MBSTR::~UTF8MBSTR() = default;

void UTF8MBSTR::operator =(const tchar * lpStr)
{
    if (lpStr)
        ConvertStringToUTF8(lpStr, &m_strUTF8_MultiByte);
    else
        m_strUTF8_MultiByte.clear();
}

void UTF8MBSTR::operator =(UTF8MBSTR& lpStr) noexcept
{
    m_strUTF8_MultiByte = lpStr.m_strUTF8_MultiByte;
}

size_t UTF8MBSTR::ConvertStringToUTF8(const tchar * strIn, std::vector<char>* strOutUTF8MB)
{
    ASSERT(strOutUTF8MB);
    size_t len;
#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
    len = wcslen(strIn);
#else
    // tchar is char (UTF8 encoding)
    len = strlen(strIn);
#endif
    strOutUTF8MB->resize(len + 1);

#if defined(_WIN32) && defined(UNICODE)
#   ifdef MSVC_RUNTIME
    size_t aux = 0;
    wcstombs_s(&aux, strOutUTF8MB->data(), len + 1, strIn, len);
#   else
    wcstombs(strOutUTF8MB->data(), strIn, len);
#   endif
#else
    // already utf8
    memcpy(strOutUTF8MB->data(), strIn, len);
#endif

    (*strOutUTF8MB)[len] = '\0';
    return len;
}

size_t UTF8MBSTR::ConvertUTF8ToString(const char* strInUTF8MB, std::vector<tchar>* strOut)
{
    ASSERT(strOut);
    size_t len;

#if defined(_WIN32) && defined(UNICODE)
    // tchar is wchar_t
    len = Str_UTF8CharCount(strInUTF8MB);
    strOut->resize(len + 1);
#ifdef MSVC_RUNTIME
    size_t aux = 0;
    mbstowcs_s(&aux, strInUTF8MB, len + 1, strInUTF8MB, len);
#else
    mbstowcs(strInUTF8MB, strInUTF8MB, len);
#endif
#else
    // tchar is char (UTF8 encoding)
    len = strlen(strInUTF8MB);
    strOut->resize(len + 1);
    memcpy(strOut->data(), strInUTF8MB, len);
#endif

    (*strOut)[len] = '\0';
    return len;
}


/*	$NetBSD: getdelim.c,v 1.2 2015/12/25 20:12:46 joerg Exp $	*/
/*	NetBSD-src: getline.c,v 1.2 2014/09/16 17:23:50 christos Exp 	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

ssize_t
//getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp)
fReadUntilDelimiter(char **buf, size_t *bufsiz, int delimiter, FILE *fp) noexcept
{
    char *ptr, *eptr;


    if (*buf == nullptr || *bufsiz == 0) {
        *bufsiz = BUFSIZ;
        if ((*buf = (char*)malloc(*bufsiz)) == nullptr)
            return -1;
    }

    for (ptr = *buf, eptr = *buf + *bufsiz;;) {
        int c = fgetc(fp);
        if (c == -1) {
            if (feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - *buf);
                if (diff != 0) {
                    *ptr = '\0';
                    return diff;
                }
            }
            return -1;
        }
        *ptr++ = (char)c;
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - *buf;
        }
        if (ptr + 2 >= eptr) {
            char *nbuf;
            size_t nbufsiz = *bufsiz * 2;
            ssize_t d = ptr - *buf;
            if ((nbuf = (char*)realloc(*buf, nbufsiz)) == nullptr)
                return -1;
            *buf = nbuf;
            *bufsiz = nbufsiz;
            eptr = nbuf + nbufsiz;
            ptr = nbuf + d;
        }
    }
    return -1;
}

ssize_t fReadUntilDelimiter_StaticBuf(char *buf, const size_t bufsiz, const int delimiter, FILE *fp) noexcept
{
    // buf: line array
    char *ptr, *eptr;

    for (ptr = buf, eptr = buf + bufsiz;;) {
        const int c = fgetc(fp);
        if (c == -1) {
            if (feof(fp)) {
                ssize_t diff = (ssize_t)(ptr - buf);
                if (diff != 0) {
                    *ptr = '\0';
                    return diff;
                }
            }
            return -1;
        }
        *ptr++ = static_cast<char>(c);
        if (c == delimiter) {
            *ptr = '\0';
            return ptr - buf;
        }
        if (ptr + 2 >= eptr) {
            return -1; // buffer too small
        }
    }
    return -1;
}

ssize_t sGetDelimiter_StaticBuf(const int delimiter, const char *ptr_string, const size_t datasize) noexcept
{
    // Returns the number of chars before the delimiter (or the end of the string).
    // buf: line array
    const char *ptr_cursor, *ptr_end;

    if (*ptr_string == '\0') {
        return -1;
    }

    for (ptr_cursor = ptr_string, ptr_end = ptr_string + datasize;; ++ptr_cursor) {
        if (*ptr_cursor == '\0') {
            if (ptr_cursor != ptr_string) {
                return ptr_cursor - ptr_string;
            }
            return -1;
        }
        if (*ptr_cursor == delimiter) {
            return ptr_cursor - ptr_string;
        }
        if (ptr_cursor + 1 > ptr_end) {
            return -1; // buffer too small
        }
    }
    return -1;
}
