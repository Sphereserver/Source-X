#include "ProfileTask.h"
#include "threads.h"

ProfileTask::ProfileTask(PROFILE_TYPE id) : m_context(nullptr), m_previousTask(PROFILE_OVERHEAD)
{
	IThread* icontext = ThreadHolder::get().current();
	if (icontext == nullptr)
	{
		// Thread was deleted, manually or by app closing signal.
		return;
	}

	m_context = static_cast<AbstractSphereThread*>(icontext);
	if (m_context != nullptr && !m_context->closing())
	{
		m_previousTask = m_context->m_profile.GetCurrentTask();
		m_context->m_profile.Start(id);
	}
}

ProfileTask::~ProfileTask(void)
{
	if (m_context != nullptr && !m_context->closing())
		m_context->m_profile.Start(m_previousTask);
}
