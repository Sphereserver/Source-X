/**
* @file ProfileTask.h
*
*/

#ifndef _INC_PROFILETASK_H
#define _INC_PROFILETASK_H

#include "ProfileData.h"

class AbstractSphereThread;

ProfileData& GetCurrentProfileData();


class ProfileTask
{
private:
	AbstractSphereThread* m_context;
	PROFILE_TYPE m_previousTask;

public:
    static const char *m_sClassName;

	explicit ProfileTask(PROFILE_TYPE id);
	~ProfileTask(void) noexcept;

	ProfileTask(const ProfileTask& copy) = delete;
	ProfileTask& operator=(const ProfileTask& other) = delete;
};


#endif // _INC_PROFILETASK_H
