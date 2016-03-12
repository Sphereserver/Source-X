#ifndef _INC_PROFILETASK_H
#define _INC_PROFILETASK_H
#pragma once

#include "threads.h"
#include "ProfileData.h"

#define CurrentProfileData static_cast<AbstractSphereThread *>(ThreadHolder::current())->m_profile

class ProfileTask
{
private:
	AbstractSphereThread* m_context;
	PROFILE_TYPE m_previousTask;

public:
	explicit ProfileTask(PROFILE_TYPE id);
	~ProfileTask(void);

private:
	ProfileTask(const ProfileTask& copy);
	ProfileTask& operator=(const ProfileTask& other);
};

#endif // _INC_PROFILETASK_H
