#include "../CLog.h"
#include "stypecast.h"

namespace detail_stypecast {

void LogEventWarnWrapper(const char* warn_str)
{
    g_Log.EventWarn(warn_str);
}

}

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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

[[nodiscard]]
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
