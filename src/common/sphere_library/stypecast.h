#ifndef _INC_STYPECAST_H
#define _INC_STYPECAST_H

#include <cstdint>
#include <type_traits>

//-- Explicitly promote to a larger type or narrow to a smaller type, instead of inattentive casts.

// Promote to the corresponding 32 bits numeric type a smaller numeric variable.
template <typename T>
[[nodiscard]]
constexpr auto i_promote32(const T a) noexcept
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
constexpr auto i_promote64(const T a) noexcept
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

template <typename T>
[[nodiscard]]
constexpr auto i_narrow32(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) >= 4, "Input variable is smaller than a 32 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype32>(static_cast<uint64>(a) & umask);
        return static_cast<int32>(static_cast<uint64>(a) & umask);
    }
    else
        return static_cast<uint32>(static_cast<uint64>(a) & umask);
}

template <typename T>
[[nodiscard]]
constexpr auto i_narrow16(const T a) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable is not an arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) >= 2, "Input variable is smaller than a 16 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype16>(static_cast<uint64>(a) & umask);
        return static_cast<int16>(static_cast<uint64>(a) & umask);
    }
    else
        return static_cast<uint16>(static_cast<uint64>(a) & umask);
}

template <typename T>
[[nodiscard]]
constexpr auto i64_narrow32(const T a) noexcept
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

template <typename T>
[[nodiscard]]
constexpr auto i64_narrow16(const T a) noexcept
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

template <typename T>
[[nodiscard]]
constexpr auto i32_narrow16(const T a) noexcept
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

template <typename T>
[[nodiscard]]
constexpr auto i16_narrow8(const T a) noexcept
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


[[nodiscard]]
inline int8 i8_from_u8_checked(const uint8 a) // not clamping/capping
{
    ASSERT(a <= INT8_MAX);
    return static_cast<int8>(a);
}
template <typename T> int8 i8_from_u8_checked(T) = delete;

[[nodiscard]]
inline int16 i16_from_u16_checked(const uint16 a) // not clamping/capping
{
    ASSERT(a <= INT16_MAX);
    return static_cast<int16>(a);
}
template <typename T> int16 i16_from_u16_checked(T) = delete;

[[nodiscard]]
inline int32 i32_from_u32_checked(const uint32 a) // not clamping/capping
{
    ASSERT(a <= INT32_MAX);
    return static_cast<int32>(a);
}
template <typename T> int32 i32_from_u32_checked(T) = delete;

[[nodiscard]]
inline int64 i64_from_u64_checked(const uint64 a) // not clamping/capping
{
    ASSERT(a <= INT64_MAX);
    return static_cast<int64>(a);
}
template <typename T> int64 i64_from_u64_checked(T) = delete;


[[nodiscard]] constexpr
inline int8 i8_from_u8_clamping(const uint8 a) noexcept
{
    return (a > (uint8)INT8_MAX) ? INT8_MAX : (int8)a;
}

[[nodiscard]] constexpr
inline int16 i16_from_u16_clamping(const uint16 a) noexcept
{
    return (a > (uint16)INT16_MAX) ? INT16_MAX : (int16)a;
}

[[nodiscard]] constexpr
inline int32 i32_from_u32_clamping(const uint32 a) noexcept
{
    return (a > (uint32)INT32_MAX) ? INT32_MAX : (int32)a;
}

[[nodiscard]] constexpr
inline int64 i64_from_u64_clamping(const uint64 a) noexcept
{
    return (a > (uint64)INT64_MAX) ? INT64_MAX : (int64)a;
}

[[nodiscard]] constexpr
inline int32 i32_from_usize_clamping(const size_t a) noexcept
{
    return (a > (size_t)INT32_MAX) ? INT32_MAX : (int32)a;
}

[[nodiscard]] constexpr
inline int64 i64_from_usize_clamping(const size_t a) noexcept
{
    return (a > (size_t)INT64_MAX) ? INT64_MAX : (int64)a;
}

[[nodiscard]] constexpr
inline uint32 u32_from_usize_clamping(const size_t a) noexcept
{
    if constexpr (sizeof(size_t) == 8)
        return (a > (size_t)UINT32_MAX) ? UINT32_MAX : (uint32)a;
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
