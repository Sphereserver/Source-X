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
#define	TIME_PROFILE_INIT	    llong llTicksStart = 0; llong llTicksEnd = 0

#define	TIME_PROFILE_START	    llTicksStart = CSTime::GetMonotonicSysTimeMilli();
#define TIME_PROFILE_END	    llTicksEnd = CSTime::GetMonotonicSysTimeMilli();

#define TIME_PROFILE_GET_HI	    ((llTicksEnd - llTicksStart) / 1000)            // Get only the seconds
#define	TIME_PROFILE_GET_LO	    (((llTicksEnd - llTicksStart) * 10) % 10000)    // Get milliseconds only, without the seconds part


#endif //_INC_CSCRIPTPROFILER_H
