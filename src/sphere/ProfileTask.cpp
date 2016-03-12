
#include "ProfileTask.h"

ProfileTask::ProfileTask(PROFILE_TYPE id) : m_context(NULL), m_previousTask(PROFILE_OVERHEAD)
{
	m_context = STATIC_CAST<AbstractSphereThread *>(ThreadHolder::current());
	if (m_context != NULL)
	{
		m_previousTask = m_context->m_profile.GetCurrentTask();
		m_context->m_profile.Start(id);
	}
}

ProfileTask::~ProfileTask(void)
{
	if (m_context != NULL)
		m_context->m_profile.Start(m_previousTask);
}
