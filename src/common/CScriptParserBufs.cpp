#include "CScriptParserBufs.h"
#include "CLog.h"


extern CScriptParserBufs g_ScriptParserBuffers;

auto CScriptParserBufs::GetCScriptTriggerArgsPtr() -> CScriptTriggerArgsPtr
{
    auto& pool = g_ScriptParserBuffers.m_poolScriptTriggerArgs;
    auto ptr = pool.acquireShared();
    if (!pool.isFromPool(ptr))
    {
        static_assert(CScriptParserBufs::sm_allow_fallback_objects);
        g_Log.EventDebug(
            "Requesting CScriptTriggerArgs from an exhausted pool (max size: %" PRIu32 "). Alive new heap-allocated fallback objects: %" PRIu32 ".\n",
            pool.sm_pool_size, pool.getFallbackCount());
    }

    ptr->Clear();
    return ptr;
}

auto CScriptParserBufs::GetScriptKeyArgBufPtr() -> CScriptKeyArgBufPtr
{
    auto& pool = g_ScriptParserBuffers.m_poolScriptKeyArgBuffers;
    auto ptr = pool.acquireUnique();
    if (!pool.isFromPool(ptr))
    {
        static_assert(CScriptParserBufs::sm_allow_fallback_objects);
        g_Log.EventDebug(
            "Requesting CScriptKeyArgBuf from an exhausted pool (max size: %" PRIu32 "). Alive new heap-allocated fallback objects: %" PRIu32 ".\n",
            pool.sm_pool_size, pool.getFallbackCount());
    }

    //ptr.get()->fill('\0');
    //memset(ptr.get(), 0, sizeof(CScriptKeyArgBuf));
    return ptr;
}
