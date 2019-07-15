/**
* @file CScriptProfiler.h
*
*/

#ifndef _INC_CSCRIPTPROFILER_H
#define _INC_CSCRIPTPROFILER_H

#include "../common/sphere_library/CSTime.h"

extern struct CScriptProfiler
{
    uchar	initstate;
    dword	called;
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

//	Time measurement macros for the profiler (use them only for the profiler!)
extern llong g_llTimeProfileFrequency;

#define	TIME_PROFILE_INIT			llong llTicksStart = 0, llTicksEnd = 0

#ifdef _WIN32
	#define	TIME_PROFILE_START		llTicksStart = GetPreciseSysTime();
	#define TIME_PROFILE_END		llTicksEnd = GetPreciseSysTime();

	#define TIME_PROFILE_GET_HI		((llTicksEnd - llTicksStart) / (g_llTimeProfileFrequency / 1000))
	#define	TIME_PROFILE_GET_LO		((((llTicksEnd - llTicksStart) * 10000) / (g_llTimeProfileFrequency / 1000)) % 10000)
#else
	#define	TIME_PROFILE_START		llTicksStart = GetSupportedTickCount()
	#define TIME_PROFILE_END		llTicksEnd = GetSupportedTickCount()

	#define TIME_PROFILE_GET_HI		(llTicksEnd - llTicksStart)
	#define	TIME_PROFILE_GET_LO		(((llTicksEnd - llTicksStart) * 10) % 10000)
#endif


#endif //_INC_CSCRIPTPROFILER_H
