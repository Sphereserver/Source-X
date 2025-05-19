#include "CScriptParserBufs.h"
#include "CScriptTriggerArgs.h"
#include "CLog.h"
#include "sphere_library/sobjpool.h"
//#include <cstdlib> //for memset


struct CScriptParserBufsImpl
{
    inline static CScriptParserBufsImpl& Get()
    {
        static auto inst = std::make_unique<CScriptParserBufsImpl>();
        return *inst.get();
    }

    static constexpr bool sm_allow_fallback_objects = true;

    // We stack allocate this POD struct, so we don't need to keep them in a pool.
    //sl::ObjectPool<CScriptSubExprState,    10'000, sm_allow_fallback_objects>
        //m_poolCScriptSubExprState;

    // This is even more expensive to construct, so will definitely benefit a lot from having allocated, cached instances.
    sl::ObjectPool<CScriptTriggerArgs,     10'000, sm_allow_fallback_objects>
        m_poolScriptTriggerArgs;
};

auto CScriptParserBufs::GetCScriptTriggerArgsPtr() -> CScriptTriggerArgsPtr
{
    auto& pool = CScriptParserBufsImpl::Get().m_poolScriptTriggerArgs;
    auto ptr = pool.acquireShared();
    if (!pool.isFromPool(ptr))
    {
        static_assert(CScriptParserBufsImpl::sm_allow_fallback_objects);
        g_Log.EventDebug(
            "Requesting CScriptTriggerArgs from an exhausted pool (max size: %" PRIuSIZE_T "). Alive new heap-allocated fallback objects: %" PRIuSIZE_T ".\n",
            pool.sm_pool_size, pool.getFallbackCount());
    }

    ptr->Clear();
    return ptr;
}
