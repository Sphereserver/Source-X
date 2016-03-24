
#pragma once
#ifndef _INC_GRAYSVR_H
#define _INC_GRAYSVR_H

#if defined(_WIN32) && !defined(_MTNETWORK)
	// _MTNETWORK enabled via makefile for other systems
	#define _MTNETWORK
#endif

//#define DEBUGWALKSTUFF 1
//#ifdef _DEBUG
#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif
#include "CWorld.h"

///////////////////////////////////////////////


// Text mashers.

extern lpctstr GetTimeMinDesc( int dwMinutes );
extern size_t FindStrWord( lpctstr pTextSearch, lpctstr pszKeyWord );

#include "CLog.h"


////////////////////////////////////////////////////////////////////////////////////

class Main : public AbstractSphereThread
{
public:
	Main();
	virtual ~Main() { };

private:
	Main(const Main& copy);
	Main& operator=(const Main& other);

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

extern lpctstr g_szServerDescription;
extern int g_szServerBuild;
extern lpctstr const g_Stat_Name[STAT_QTY];
extern CGStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

// ---------------------------------------------------------------------------------------------


struct TScriptProfiler
{
	uchar	initstate;
	dword		called;
	llong	total;
	struct TScriptProfilerFunction
	{
		tchar	name[128];	// name of the function
		dword	called;		// how many times called
		llong	total;		// total executions time
		llong	min;		// minimal executions time
		llong	max;		// maximal executions time
		llong	average;	// average executions time
		TScriptProfilerFunction *next;
	}		*FunctionsHead, *FunctionsTail;
	struct TScriptProfilerTrigger
	{
		tchar	name[128];	// name of the trigger
		dword	called;		// how many times called
		llong	total;		// total executions time
		llong	min;		// minimal executions time
		llong	max;		// maximal executions time
		llong	average;	// average executions time
		TScriptProfilerTrigger *next;
	}		*TriggersHead, *TriggersTail;
};
extern TScriptProfiler g_profiler;

//	Time measurement macros
extern llong llTimeProfileFrequency;

#ifdef _WIN32

#define	TIME_PROFILE_INIT	\
	llong llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else // !_WIN32

#define	TIME_PROFILE_INIT	\
	llong llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif // _WIN32

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)


#endif // _INC_GRAYSVR_H
