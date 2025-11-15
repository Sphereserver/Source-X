#ifndef _INC_CSCRIPTPARSERBUFS_H
#define _INC_CSCRIPTPARSERBUFS_H

#include "sphere_library/sobjpool.h"
#include "CScriptTriggerArgs.h"


/*
class CScriptTriggerArgsPtr : public std::shared_ptr<CScriptTriggerArgs>
{
    CScriptTriggerArgsPtr(std::nullptr_t) = delete;
}
*/

using CScriptKeyArgBuf = std::array<tchar, SCRIPT_MAX_LINE_LEN>;

struct CScriptParserBufs
{
    CScriptParserBufs() noexcept = default;
    ~CScriptParserBufs() noexcept = default;

    static constexpr bool sm_allow_fallback_objects = true;

    // This is even more expensive to construct, so will definitely benefit a lot from having allocated, cached instances.
    using PoolScriptTriggerArgs_t = sl::ObjectPool<CScriptTriggerArgs,  1'000,   sm_allow_fallback_objects>;

    // Not expensive to construct but frequent allocations and deallocations (like classic strings).
    using PoolScriptKeyArgBuf_t =   sl::ObjectPool<CScriptKeyArgBuf,    1'000,  sm_allow_fallback_objects>;

    PoolScriptTriggerArgs_t m_poolScriptTriggerArgs;
    PoolScriptKeyArgBuf_t   m_poolScriptKeyArgBuffers;

    using CScriptTriggerArgsPtr = PoolScriptTriggerArgs_t::SharedPtr_t;
    using CScriptKeyArgBufPtr   = PoolScriptKeyArgBuf_t::UniquePtr_t;

public:
    [[nodiscard]] static CScriptTriggerArgsPtr GetCScriptTriggerArgsPtr();
    [[nodiscard]] static CScriptKeyArgBufPtr   GetScriptKeyArgBufPtr();
};

using CScriptTriggerArgsPtr = CScriptParserBufs::CScriptTriggerArgsPtr;
using CScriptKeyArgBufPtr   = CScriptParserBufs::CScriptKeyArgBufPtr;



#endif // _INC_CSCRIPTPARSERBUFS_H
