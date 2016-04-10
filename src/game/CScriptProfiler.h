#ifndef _INC_CSCRIPTPROFILER_H
#define _INC_CSCRIPTPROFILER_H

#include "../common/common.h"
#include "../common/CTime.h"

extern struct CScriptProfiler
{
    uchar	initstate;
    dword		called;
    llong	total;
    struct CScriptProfilerFunction
    {
        tchar	name[128];	// name of the function
        dword	called;		// how many times called
        llong	total;		// total executions time
        llong	min;		// minimal executions time
        llong	max;		// maximal executions time
        llong	average;	// average executions time
        CScriptProfilerFunction *next;
    }		*FunctionsHead, *FunctionsTail;
    struct CScriptProfilerTrigger
    {
        tchar	name[128];	// name of the trigger
        dword	called;		// how many times called
        llong	total;		// total executions time
        llong	min;		// minimal executions time
        llong	max;		// maximal executions time
        llong	average;	// average executions time
        CScriptProfilerTrigger *next;
    }		*TriggersHead, *TriggersTail;
} g_profiler;

//	Time measurement macros
extern llong llTimeProfileFrequency;

#ifdef _WINDOWS

#define	TIME_PROFILE_INIT	\
	llong llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else // !_WINDOWS

#define	TIME_PROFILE_INIT	\
	llong llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif // _WINDOWS

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

#endif //_INC_CSCRIPTPROFILER_H
