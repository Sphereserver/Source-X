#include "ProfileTask.h"
#include "../common/CException.h"
#include "../game/CServerConfig.h"
#include "threads.h"


ProfileData& GetCurrentProfileData()
{
    auto cur_thread = static_cast<AbstractSphereThread*>(ThreadHolder::get().current());
    ASSERT(cur_thread);
    return cur_thread->m_profile;
}

ProfileTask::ProfileTask(PROFILE_TYPE id) :
    m_context(nullptr), m_previousTask(PROFILE_OVERHEAD)
{
    if (!IsSetEF(EF_Script_Profiler))
        return;

    auto& th = ThreadHolder::get();
    if (th.closing())
        return;

	IThread* icontext = th.current();
	if (icontext == nullptr)
	{
		// Thread was deleted, manually or by app closing signal.
		return;
	}

	m_context = static_cast<AbstractSphereThread*>(icontext);
	if (m_context != nullptr && !m_context->closing())
	{
        ProfileData& pdata = m_context->m_profile;
        const PROFILE_TYPE task = pdata.GetCurrentTask();
        /*
        if (task == PROFILE_MAP)
        {
            // Just track 1 out of 10 map actions, since those are *really* frequent
            //  and activating a task requests an accurate system timer, which is a slow operation.
            if (pdata.m_iMapTaskCounter < 10)
            {
                ++ pdata.m_iMapTaskCounter;
                return;
            }
            pdata.m_iMapTaskCounter = 0;
        }
        */

		m_previousTask = task;
		pdata.Start(id);
	}
}

ProfileTask::~ProfileTask(void) noexcept
{
    EXC_TRY("destroy profiletask");

	if (m_context != nullptr && !m_context->closing())
		m_context->m_profile.Start(m_previousTask);

    EXC_CATCH;
}
