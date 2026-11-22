#include "ProfileTask.h"
//#include "../common/CException.h" // included in the precompiled header
#include "../game/CServerConfig.h"
#include "threads.h"


ProfileData& GetCurrentProfileData() noexcept
{
    auto cur_thread = static_cast<AbstractSphereThread*>(ThreadHolder::get().current());
    if (!cur_thread)
    {
        // It's called also by the exception handlers (EXC_CATCH...), which can run on a thread that isn't (or isn't anymore)
        //  registered, for example during the shutdown. Don't abort there: count on a per-thread placeholder, not shown anywhere.
        static thread_local ProfileData s_unregisteredThreadProfile;
        static thread_local bool s_fLogged = false;
        if (!s_fLogged)
        {
            s_fLogged = true;
            reserved_stderr_log("ERROR: [THREADING] Can't get ProfileData for the current thread: it isn't registered.\n");
        }
        return s_unregisteredThreadProfile;
    }
    return cur_thread->m_profile;
}

ProfileTask::ProfileTask(PROFILE_TYPE id) :
    m_context(nullptr), m_previousTask(PROFILE_OVERHEAD)
{
    if (!IsSetEF(EF_Script_Profiler))
        return;

    auto& th = ThreadHolder::get();
    if (th.isServClosing())
        return;

	AbstractThread* icontext = th.current();
	if (icontext == nullptr)
	{
        // Thread was deleted, manually or by app closing signal.
		return;
	}

	m_context = static_cast<AbstractSphereThread*>(icontext);
    if (m_context != nullptr && !m_context->isClosing())
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

    if (m_context != nullptr && !m_context->isClosing())
		m_context->m_profile.Start(m_previousTask);

    EXC_CATCH;
}
