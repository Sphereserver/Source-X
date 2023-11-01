/**
* @file spheresvr.h
*
*/

#ifndef _INC_SPHERESVR_H
#define _INC_SPHERESVR_H

#include "../common/common.h"
#include "../sphere/threads.h"


class GlobalInitializer
{
public:
    GlobalInitializer();
};

////////////////////////////////////////////////////////////////////////////////////

class MainThread : public AbstractSphereThread
{
public:
	MainThread();
	virtual ~MainThread() { };

private:
	MainThread(const MainThread& copy);
	MainThread& operator=(const MainThread& other);

public:
	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
	virtual bool shouldExit();
};

//////////////////////////////////////////////////////////////

extern std::string g_sServerDescription;
extern CSStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );


#endif // _INC_SPHERESVR_H
