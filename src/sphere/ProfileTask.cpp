#include "ProfileTask.h"
#include "threads.h"

ProfileTask::ProfileTask(PROFILE_TYPE id) : m_context(nullptr), m_previousTask(PROFILE_OVERHEAD)
{
	m_context = static_cast<AbstractSphereThread *>(ThreadHolder::get()->current());
	if (m_context != nullptr)
	{
		m_previousTask = m_context->m_profile.GetCurrentTask();
		m_context->m_profile.Start(id);
	}
}

ProfileTask::~ProfileTask(void)
{
	if (m_context != nullptr)
		m_context->m_profile.Start(m_previousTask);
}
