#ifndef _INC_STYPECAST_H
#define _INC_STYPECAST_H

#include <climits>
#include <limits>
#include <type_traits>


// Explicitly promote to a larger type or narrow to a smaller type, instead of inattentive raw casts.
// Function prefixes:
// - n: works regardless of the sign, it deduces and uses the right types automatically.
// - i: expects a signed number as input
// - u: expects an unsigned number as input


// Promote to the corresponding 32 bits numeric type a smaller numeric variable.
template <typename T>
[[nodiscard]]
constexpr auto n_promote32(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) < 4, "Input variable is not smaller than a 32 bit number.");
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype32>(a);
        return static_cast<int32>(a);
    }
    else
        return static_cast<uint32>(a);
}

// Promote to the corresponding 64 bits numeric type a smaller numeric variable.
template <typename T>
[[nodiscard]]
constexpr auto n_promote64(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) < 8, "Input variable is not smaller than a 64 bit number.");
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype64>(a);
        return static_cast<int64>(a);
    }
    else
        return static_cast<uint64>(a);
}

// Narrow a 64 bits number to a 32 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n64_narrow32(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype32>(a & umask);
        return static_cast<int32>(a & umask);
    }
    else
        return static_cast<uint32>(a & umask);
}

// Narrow a 64 bits number to a 32 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow.
template <typename T>
[[nodiscard]] inline
auto n64_narrow32_checked(const T a)
{
    ASSERT(a < std::numeric_limits<int32>::max());
    return n64_narrow32(a);
}

// Narrow a 64 bits number to a 16 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n64_narrow16(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'0000'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype16>(a & umask);
        return static_cast<int16>(a & umask);
    }
    else
        return static_cast<uint16>(a & umask);
}

// Narrow a 64 bits number to a 16 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow.
template <typename T>
[[nodiscard]] inline
auto n64_narrow16_checked(const T a)
{
    ASSERT(a < std::numeric_limits<int16>::max());
    return n64_narrow16(a);
}

// Narrow a 32 bits number to a 16 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n32_narrow16(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 4, "Input variable is not a 32 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint32 umask = 0x0000'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype16>(a & umask);
        return static_cast<int16>(a & umask);
    }
    else
        return static_cast<uint16>(a & umask);
}

// Narrow a 32 bits number to a 16 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow.
template <typename T>
[[nodiscard]] inline
auto n32_narrow16_checked(const T a)
{
    ASSERT(a < std::numeric_limits<int16>::max());
    return n32_narrow16(a);
}

// Narrow a 32 bits number to a 8 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n32_narrow8(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(sizeof(T) == 4, "Input variable is not a 32 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint32 umask = 0x0000'00FF;
    if constexpr (std::is_signed_v<T>)
        return static_cast<int8> (a & umask);
    else
        return static_cast<uint8>(a & umask);
}

// Narrow a 32 bits number to an 8 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow.
template <typename T>
[[nodiscard]] inline
auto n32_narrow8_checked(const T a)
{
    ASSERT(a < std::numeric_limits<int8>::max());
    return n32_narrow8(a);
}

// Narrow a 16 bits number to an 8 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n16_narrow8(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T>, "Only integral types are supported by this function.");
    static_assert(sizeof(T) == 2, "Input variable is not a 16 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint16 umask = 0x00FF;
    if constexpr (std::is_signed_v<T>)
        return static_cast<int8>(a & umask);
    else
        return static_cast<uint8>(a & umask);
}

// Narrow a 16 bits number to an 8 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow.
template <typename T>
[[nodiscard]] inline
auto n16_narrow8_checked(const T a)
{
    ASSERT(a < std::numeric_limits<int8>::max());
    return n16_narrow8(a);
}

// If size_t is bigger than a 32 bits number, narrow it to a 32 bits number discarding any upper exceeding bytes, otherwise plain return the same value..
[[nodiscard]]
constexpr uint32 usize_narrow32(const size_t a) noexcept
{
    // This doesn't work because n64_narrow32 static_asserts will be evaluated and fail on 32 bits compilation.
    /*
    if constexpr (sizeof(size_t) == 8)
        return n64_narrow32(a);
    else
        return a;
    */
#if SIZE_MAX == ULLONG_MAX
    // ptr size is 8 -> 64 bits
    return n64_narrow32(a);
#elif SIZE_MAX == UINT_MAX
    // ptr size is 4 -> 32 bits
    return a;
#else
#   error "Pointer size is neither 8 nor 4?"
#endif
}

// If size_t is bigger than a 32 bits number, narrow it to a 32 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow. If size_t has 32 bits size, plain return the same value.
[[nodiscard]] inline
uint32 usize_narrow32_checked(const size_t a)
{
    ASSERT(a < std::numeric_limits<uint32>::max());
    return usize_narrow32(a);
}


// Unsigned (and size_t) to signed.

// Convert an 8 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
int8 i8_from_u8_checked(const uint8 a) // not clamping/capping
{
    ASSERT(a <= (uint8_t)std::numeric_limits<int8_t>::max());
    return static_cast<int8>(a);
}
template <typename T> int8 i8_from_u8_checked(T) = delete;

// Convert a 16 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
int16 i16_from_u16_checked(const uint16 a) // not clamping/capping
{
    ASSERT(a <= (uint16_t)std::numeric_limits<int16_t>::max());
    return static_cast<int16>(a);
}
template <typename T> int16 i16_from_u16_checked(T) = delete;

// Convert a 32 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
int32 i32_from_u32_checked(const uint32 a) // not clamping/capping
{
    ASSERT(a <= (uint32_t)std::numeric_limits<int32_t>::max());
    return static_cast<int32>(a);
}
template <typename T> int32 i32_from_u32_checked(T) = delete;

// Convert a 64 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
int64 i64_from_u64_checked(const uint64 a) // not clamping/capping
{
    ASSERT(a <= (uint64_t)std::numeric_limits<int64_t>::max());
    return static_cast<int64>(a);
}
template <typename T> int64 i64_from_u64_checked(T) = delete;


[[nodiscard]] constexpr
int8 i8_from_u8_clamping(const uint8 a) noexcept
{
    return (a > (uint8_t)std::numeric_limits<int8_t>::max()) ? std::numeric_limits<int8_t>::max() : (int8_t)a;
}

[[nodiscard]] constexpr
int16 i16_from_u16_clamping(const uint16 a) noexcept
{
    return (a > (uint16_t)std::numeric_limits<int16_t>::max()) ? (uint16_t)std::numeric_limits<int16_t>::max() : (int16_t)a;
}

[[nodiscard]] constexpr
int32 i32_from_u32_clamping(const uint32 a) noexcept
{
    return (a > (uint32_t)std::numeric_limits<int32_t>::max()) ? (uint32_t)std::numeric_limits<int32_t>::max() : (int32_t)a;
}

[[nodiscard]] constexpr
int64 i64_from_u64_clamping(const uint64 a) noexcept
{
    return (a > (uint64_t)std::numeric_limits<int64_t>::max()) ? (uint64_t)std::numeric_limits<int64_t>::max() : (int64_t)a;
}

[[nodiscard]] constexpr
int32 i32_from_usize_clamping(const size_t a) noexcept
{
    return (a > (size_t)std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max() : (int32_t)a;
}

[[nodiscard]] constexpr
int64 i64_from_usize_clamping(const size_t a) noexcept
{
    return (a > (size_t)std::numeric_limits<int64_t>::max()) ? std::numeric_limits<int64_t>::max() : (int64_t)a;
}

[[nodiscard]] constexpr
uint32 u32_from_usize_clamping(const size_t a) noexcept
{
    if constexpr (sizeof(size_t) == 8)
        return (a > (size_t)std::numeric_limits<uint32_t>::max()) ? std::numeric_limits<uint32_t>::max() : (uint32_t)a;
    else
        return a;
}


// Use this as a double check, to be sure at compile time that the two variables have the same size and sign.
template <typename Tout, typename Tin>
[[nodiscard]]
constexpr Tout num_alias_cast(const Tin a) noexcept
{
    static_assert(std::is_arithmetic_v<Tin>  || std::is_enum_v<Tin>,  "Input variable is not a numeric type.");
    static_assert(std::is_arithmetic_v<Tout> || std::is_enum_v<Tout>, "Output variable is not a numeric type.");
    static_assert(sizeof(Tin) == sizeof(Tout), "Input and output types do not have the same size.");
    static_assert(!(std::is_signed_v<Tin>  && std::is_unsigned_v<Tout>), "Casting signed to unsigned.");
    static_assert(!(std::is_signed_v<Tout> && std::is_unsigned_v<Tin> ), "Casting unsigned to signed.");
    return static_cast<Tout>(a);
}


#endif // _INC_STYPECAST_H
