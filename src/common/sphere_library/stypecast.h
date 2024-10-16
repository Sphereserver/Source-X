#ifndef _INC_STYPECAST_H
#define _INC_STYPECAST_H

#include <cstdint>
#include <limits>
#include <type_traits>

namespace detail_stypecast
{
void LogEventWarnWrapper(const char* warn_str);
}

// Helper macros
//#define n64_narrow_n32_assert(source_val)   (n64_narrow_n32_checked(source_val, true))
//#define n64_narrow_n32_warn(source_val)     (n64_narrow_n32_checked(source_val, false))


// Helper template to work on enum types.

// Primary template for non-enum types, T will simply be itself
template<typename T, typename = void>
struct underlying_or_self
{
    using type = T;
};

// Specialization for enum types, using SFINAE to enable only if T is an enum
template<typename T>
struct underlying_or_self<T, std::enable_if_t<std::is_enum_v<T>>>
{
    using type = std::underlying_type_t<T>;
};

// Helper alias
template<typename T>
using underlying_or_self_t = typename underlying_or_self<T>::type;


// Use this as a double check, to be sure at compile time that the two variables have the same size and sign.
template <typename Tout, typename Tin>
[[nodiscard]]
constexpr Tout n_alias_cast(const Tin source_val) noexcept
{
    static_assert(std::is_arithmetic_v<Tin>,  "Input variable is not an arithmetictype.");
    static_assert(std::is_arithmetic_v<Tout>, "Output variable is not an arithmetic type.");
    static_assert(sizeof(Tin) == sizeof(Tout), "Input and output types do not have the same size.");
    static_assert(!(std::is_signed_v<Tin>  && std::is_unsigned_v<Tout>), "Casting signed to unsigned.");
    static_assert(!(std::is_signed_v<Tout> && std::is_unsigned_v<Tin> ), "Casting unsigned to signed.");
    return static_cast<Tout>(source_val);
}

template <typename Tout, typename Tin>
[[nodiscard]]
constexpr Tout enum_alias_cast(const Tin source_val) noexcept
{
    static_assert(std::is_enum_v<Tin>  || std::is_enum_v<Tout>,  "Nor input nor output variables are an enum.");
    static_assert(std::is_arithmetic_v<Tin>  || std::is_enum_v<Tin>,  "Input variable is not a numeric type.");
    static_assert(std::is_arithmetic_v<Tout> || std::is_enum_v<Tout>, "Output variable is not a numeric type.");
    static_assert(sizeof(Tin) == sizeof(Tout), "Input and output types do not have the same size.");

    static_assert(!(std::is_signed_v<underlying_or_self_t<Tin>>  && std::is_unsigned_v<underlying_or_self_t<Tout>>), "Casting signed to unsigned.");
    static_assert(!(std::is_signed_v<underlying_or_self_t<Tout>> && std::is_unsigned_v<underlying_or_self_t<Tin>> ), "Casting unsigned to signed.");

    /*
    static_assert(std::is_enum_v<Tin> ||
            !(std::is_signed_v<std::underlying_type_t<Tout>>  && std::is_unsigned_v<Tin>),
        "Casting unsigned to signed.");
    static_assert(std::is_enum_v<Tin> ||
            !(std::is_unsigned_v<std::underlying_type_t<Tout>>  && std::is_signed_v<Tin>),
        "Casting signed to unsigned.");

    static_assert(std::is_enum_v<Tout> ||
            !(std::is_signed_v<std::underlying_type_t<Tin>>  && std::is_unsigned_v<Tout>),
        "Casting signed to unsigned.");
    static_assert(std::is_enum_v<Tout> ||
            !(std::is_unsigned_v<std::underlying_type_t<Tin>>  && std::is_signed_v<Tout>),
        "Casting unsigned to signed.");
    */

    return static_cast<Tout>(source_val);
}


/* Promotion and narrowing of numbers to a same sign greater/smaller data type */

// Explicitly promote to a larger type or narrow to a smaller type, instead of inattentive raw casts.
// Function prefixes:
// - n: works regardless of the sign, it deduces and uses the right types automatically.
// - i: expects a signed number as input
// - u: expects an unsigned number as input


// Promote to the corresponding 32 bits numeric type a smaller numeric variable.
template <typename T>
[[nodiscard]]
constexpr auto n_promote_n32(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) < 4, "Input variable is not smaller than a 32 bit number.");
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype32>(source_val);
        return static_cast<int32>(source_val);
    }
    else
        return static_cast<uint32>(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum_promote_n32(const T source_val) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n_promote_n32(static_cast<std::underlying_type_t<const T>>(source_val));
}

// Promote to the corresponding 64 bits numeric type a smaller numeric variable.
template <typename T>
[[nodiscard]]
constexpr auto n_promote_n64(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) < 8, "Input variable is not smaller than a 64 bit number.");
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype64>(source_val);
        return static_cast<int64>(source_val);
    }
    else
        return static_cast<uint64>(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum_promote_n64(const T source_val) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n_promote_n64(static_cast<std::underlying_type_t<const T>>(source_val));
}

// Narrow a 64 bits number to a 32 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n64_narrow_n32(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 32 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'FFFF'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype32>(source_val & umask);
        return static_cast<int32>(source_val & umask);
    }
    else
        return static_cast<uint32>(source_val & umask);
}

// Narrow a 64 bits number to a 32 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n64_narrow_n32_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int32>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint32>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int32>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 32 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint32>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 32 bits unsigned integer will overflow.\n");
        }
    }
    return n64_narrow_n32(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum64_narrow_n32_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n64_narrow_n32_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// Narrow a 64 bits number to a 16 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n64_narrow_n16(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'0000'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype16>(source_val & umask);
        return static_cast<int16>(source_val & umask);
    }
    else
        return static_cast<uint16>(source_val & umask);
}

// Narrow a 64 bits number to a 16 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n64_narrow_n16_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int16>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint16>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int16>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 16 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint16>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 16 bits unsigned integer will overflow.\n");
        }
    }
    return n64_narrow_n16(source_val);
}


template <typename T>
[[nodiscard]]
constexpr auto enum64_narrow_n16_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n64_narrow_n16_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// Narrow a 64 bits number to a 8 a number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n64_narrow_n8(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type");
    static_assert(std::is_floating_point_v<T> == false, "Corresponding 8-bit floating point type does not exist?");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 8, "Input variable is not a 64 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 8 bytes and discard the upper ones.
    constexpr uint64 umask = 0x0000'0000'0000'00FF;
    if constexpr (std::is_signed_v<T>)
        return static_cast<int8>(source_val & umask);
    else
        return static_cast<uint8>(source_val & umask);
}

// Narrow a 64 bits number to a 8 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n64_narrow_n8_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int8>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint8>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 8 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 to 8 bits unsigned integer will overflow.\n");
        }
    }
    return n64_narrow_n8(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum64_narrow_n8_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n64_narrow_n8_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// Narrow a 32 bits number to a 16 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n32_narrow_n16(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T> || (std::is_floating_point_v<T> && std::is_signed_v<T>), "Unsigned floating point numbers are unsupported by the language standard");
    static_assert(sizeof(T) == 4, "Input variable is not a 32 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint32 umask = 0x0000'FFFF;
    if constexpr (std::is_signed_v<T>)
    {
        if constexpr (std::is_floating_point_v<T>)
            return static_cast<realtype16>(source_val & umask);
        return static_cast<int16>(source_val & umask);
    }
    else
        return static_cast<uint16>(source_val & umask);
}

// Narrow a 32 bits number to a 16 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n32_narrow_n16_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int16>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint16>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int16>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 to 16 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint16>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 to 16 bits unsigned integer will overflow.\n");
        }
    }
    return n32_narrow_n16(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum32_narrow_n16_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n32_narrow_n16_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// Narrow a 32 bits number to a 8 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n32_narrow_n8(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(sizeof(T) == 4, "Input variable is not a 32 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint32 umask = 0x0000'00FF;
    if constexpr (std::is_signed_v<T>)
        return static_cast<int8> (source_val & umask);
    else
        return static_cast<uint8>(source_val & umask);
}

// Narrow a 32 bits number to a 16 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n32_narrow_n8_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int8>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint8>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 to 8 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 to 8 bits unsigned integer will overflow.\n");
        }
    }
    return n32_narrow_n8(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum32_narrow_n8_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n32_narrow_n8_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// Narrow a 16 bits number to an 8 bits number, discarding any upper exceeding bytes.
template <typename T>
[[nodiscard]]
constexpr auto n16_narrow_n8(const T source_val) noexcept
{
    static_assert(std::is_arithmetic_v<T>, "Input variable has not a arithmetic type.");
    static_assert(std::is_integral_v<T>, "Only integral types are supported by this function.");
    static_assert(sizeof(T) == 2, "Input variable is not a 16 bit number.");

    // Since the narrowing can be implementation specific, here we decide that we take only the lower 16 bytes and discard the upper ones.
    constexpr uint16 umask = 0x00FF;
    if constexpr (std::is_signed_v<T>)
        return static_cast<int8>(source_val & umask);
    else
        return static_cast<uint8>(source_val & umask);
}

// Narrow a 32 bits number to a 16 bits number.
//  ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow, otherwise
//  otherwise just print an error if it overflows (wise to do this if the values depends from user input/scripts).
template <typename T>
[[nodiscard]] inline
    auto n16_narrow_n8_checked(const T source_val, bool should_assert)
{
    if (should_assert)
    {
        if constexpr (std::is_signed_v<T>) {
            ASSERT(source_val <= std::numeric_limits<int8>::max());
        }
        else {
            ASSERT(source_val <= std::numeric_limits<uint8>::max());
        }
    }
    else
    {
        if constexpr (std::is_signed_v<T>) {
            if (source_val > std::numeric_limits<int8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 16 to 8 bits signed integer will overflow.\n");
        }
        else {
            if (source_val > std::numeric_limits<uint8>::max())
                detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 16 to 8 bits unsigned integer will overflow.\n");
        }
    }
    return n16_narrow_n8(source_val);
}

template <typename T>
[[nodiscard]]
constexpr auto enum16_narrow_n8_checked(const T source_val, bool should_assert) noexcept
{
    static_assert(std::is_enum_v<T>, "Input variable is not an enum type.");
    return n16_narrow_n8_checked(static_cast<std::underlying_type_t<const T>>(source_val, should_assert));
}

// If size_t is bigger than a 32 bits number, narrow it to a 32 bits number discarding any upper exceeding bytes, otherwise plain return the same value..
[[nodiscard]]
constexpr uint32 usize_narrow_u32(const size_t source_val) noexcept
{
    // This doesn't work because n64_narrow_n32 static_asserts will be evaluated and fail on 32 bits compilation.
    /*
    if constexpr (sizeof(size_t) == 8)
        return n64_narrow_n32(source_val);
    else
        return a;
    */
#if SIZE_MAX == UINT64_MAX
    return n64_narrow_n32(source_val);
#elif SIZE_MAX == UINT32_MAX
    return source_val;
#else
#   error "size_t is neither 8 nor 4 bytes?"
#endif
}

// If size_t is bigger than a 32 bits number, narrow it to a 32 bits number and ASSERT (because you're reasonably sure but not absolutely certain) that it won't overflow. If size_t has 32 bits size, plain return the same value.
[[nodiscard]] inline
uint32 usize_narrow_u32_checked(const size_t source_val, bool should_assert)
{
    if (should_assert) {
        ASSERT(source_val <= std::numeric_limits<uint32>::max());
    }
    else if (source_val > std::numeric_limits<uint32>::max()) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from size_t to 32 bits unsigned integer will overflow.\n");
    }
    return usize_narrow_u32(source_val);
}


/* Unsigned (and size_t) to signed, clamping. */

[[nodiscard]] constexpr
    int8 i8_from_u8_clamping(const uint8 source_val) noexcept
{
    return (source_val > (uint8_t)std::numeric_limits<int8_t>::max()) ? std::numeric_limits<int8_t>::max() : (int8_t)source_val;
}

[[nodiscard]] constexpr
    int16 i16_from_u16_clamping(const uint16 source_val) noexcept
{
    return (source_val > (uint16_t)std::numeric_limits<int16_t>::max()) ? std::numeric_limits<int16_t>::max() : (int16_t)source_val;
}

[[nodiscard]] constexpr
    int32 i32_from_u32_clamping(const uint32 source_val) noexcept
{
    return (source_val > (uint32_t)std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max() : (int32_t)source_val;
}

[[nodiscard]] constexpr
    int64 i64_from_u64_clamping(const uint64 source_val) noexcept
{
    return (source_val > (uint64_t)std::numeric_limits<int64_t>::max()) ? std::numeric_limits<int64_t>::max() : (int64_t)source_val;
}

[[nodiscard]] constexpr
    int32 i32_from_usize_clamping(const size_t source_val) noexcept
{
    return (source_val > (size_t)std::numeric_limits<int32_t>::max()) ? std::numeric_limits<int32_t>::max() : (int32_t)source_val;
}

[[nodiscard]] constexpr
    int64 i64_from_usize_clamping(const size_t source_val) noexcept
{
    return (source_val > (size_t)std::numeric_limits<int64_t>::max()) ? std::numeric_limits<int64_t>::max() : (int64_t)source_val;
}

[[nodiscard]] constexpr
    uint32 u32_from_usize_clamping(const size_t source_val) noexcept
{
    if constexpr (sizeof(size_t) == 8)
        return (source_val > (size_t)std::numeric_limits<uint32_t>::max()) ? std::numeric_limits<uint32_t>::max() : (uint32_t)source_val;
    else
        return source_val;
}


/* Unsigned (and size_t) to signed, checked for overflows. */

[[nodiscard]] inline
    int8 i8_from_u8_checked(const uint8 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint8)std::numeric_limits<int8>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 8 bits unsigned integer to 8 bits signed integer will overflow.\n");
    }

    return static_cast<int8>(source_val);
}
template <typename T> int8 i8_from_u8_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int16 i8_from_u16_checked(const uint16 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint16)std::numeric_limits<int8>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 16 bits unsigned integer to 8 bits signed integer will overflow.\n");
    }

    return static_cast<int16>(source_val);
}
template <typename T> int16 i8_from_u16_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int16 i16_from_u16_checked(const uint16 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint16)std::numeric_limits<int16>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 16 bits unsigned integer to 16 bits signed integer will overflow.\n");
    }

    return static_cast<int16>(source_val);
}
template <typename T> int16 i16_from_u16_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int16 i16_from_u32_checked(const uint32 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint32)std::numeric_limits<int16>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 bits unsigned integer to 16 bits signed integer will overflow.\n");
    }

    return static_cast<int16>(source_val);
}
template <typename T> int16 i16_from_u32_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int16 i16_from_u64_checked(const uint64 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint64)std::numeric_limits<int16>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 bits unsigned integer to 16 bits signed integer will overflow.\n");
    }

    return static_cast<int16>(source_val);
}
template <typename T> int16 i16_from_u64_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

// Convert a 32 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
    int32 i32_from_u32_checked(const uint32 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint32)std::numeric_limits<int32>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 32 bits unsigned integer to 32 bits signed integer will overflow.\n");
    }

    return static_cast<int32>(source_val);
}
template <typename T> int32 i32_from_u32_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int32 i32_from_u64_checked(const uint64 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint64)std::numeric_limits<int64>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 bits unsigned integer to 32 bits signed integer will overflow.\n");
    }

    return static_cast<int32>(source_val);
}
template <typename T> int32 i32_from_u64_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

// Convert a 64 bits unsigned value to signed and ASSERT (because you're reasonably sure but not absolutely certain) that it will fit into its signed datatype counterpart (unsigned variables can store greater values than signed ones).
[[nodiscard]] inline
    int64 i64_from_u64_checked(const uint64 source_val, bool should_assert) // not clamping/capping
{
    const bool would_overflow = (source_val > (uint64)std::numeric_limits<int64>::max());
    if (should_assert) {
        ASSERT(!would_overflow);
    }
    else if (would_overflow) {
        detail_stypecast::LogEventWarnWrapper("[Internal] Narrowing conversion from 64 bits unsigned integer to 64 bits signed integer will overflow.\n");
    }

    return static_cast<int64>(source_val);
}
template <typename T> int64 i64_from_u64_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

// size_t conversions

[[nodiscard]] inline
    int8 i8_from_usize_checked(const size_t source_val, bool should_assert) // not clamping/capping
{
#if SIZE_MAX == UINT64_MAX
    return n64_narrow_n8_checked(source_val, should_assert);
#elif SIZE_MAX == UINT32_MAX
    return n32_narrow_n8_checked(source_val, should_assert);
#else
#   error "size_t is neither 8 nor 4 bytes?"
#endif
}
template <typename T> int8 i8_from_usize_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument


[[nodiscard]] inline
    int16 i16_from_usize_checked(const size_t source_val, bool should_assert) // not clamping/capping
{
#if SIZE_MAX == UINT64_MAX
    return n64_narrow_n16_checked(source_val, should_assert);
#elif SIZE_MAX == UINT32_MAX
    return n32_narrow_n16_checked(source_val, should_assert);
#else
#   error "size_t is neither 8 nor 4 bytes?"
#endif
}
template <typename T> int16 i16_from_usize_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int32 i32_from_usize_checked(const size_t source_val, bool should_assert) // not clamping/capping
{
#if SIZE_MAX == UINT64_MAX
    return i32_from_u32_clamping(n64_narrow_n32_checked(source_val, should_assert));
#elif SIZE_MAX == UINT32_MAX
    return i32_from_u32_checked(n_alias_cast<uint32>(source_val), should_assert);
#else
#   error "size_t is neither 8 nor 4 bytes?"
#endif
}
template <typename T> int32 i32_from_usize_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument

[[nodiscard]] inline
    int64 i64_from_usize_checked(const size_t source_val, bool should_assert) // not clamping/capping
{
#if SIZE_MAX == UINT64_MAX
    return i64_from_u64_checked(n_alias_cast<uint64>(source_val), should_assert);
#elif SIZE_MAX == UINT32_MAX
    (void)should_assert;
    return static_cast<int64>(source_val);   // For sure it will fit
#else
#   error "size_t is neither 8 nor 4 bytes?"
#endif
}
template <typename T> int64 i64_from_usize_checked(T, bool) = delete; // disable implicit type conversion for the inpuT source_valrgument




#endif // _INC_STYPECAST_H
