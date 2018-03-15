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
extern llong llTimeProfileFrequency;

#define	TIME_PROFILE_INIT			llong llTicksStart = 0, llTicksEnd = 0

#ifdef _WIN32
	// From Windows documentation:
	//	On systems that run Windows XP or later, the function will always succeed and will thus never return zero.
	// Since i think no one will run Sphere on a pre XP os, we can avoid checking for overflows, in case QueryPerformanceCounter fails.
	#define	TIME_PROFILE_START		\
		LARGE_INTEGER liQPCStart;	\
		if (!QueryPerformanceCounter(&liQPCStart))		{	\
			llTicksStart = GetSupportedTickCount();		}	\
		else											{	\
			llTicksStart = liQPCStart.QuadPart;			}
		
	#define TIME_PROFILE_END		\
		LARGE_INTEGER liQPCEnd;		\
		if (!QueryPerformanceCounter(&liQPCEnd))		{	\
			llTicksEnd = GetSupportedTickCount();		}	\
		else											{	\
			llTicksEnd = liQPCEnd.QuadPart;				}

	#define TIME_PROFILE_GET_HI		((llTicksEnd - llTicksStart) / (llTimeProfileFrequency / 1000))
	#define	TIME_PROFILE_GET_LO		((((llTicksEnd - llTicksStart) * 10000) / (llTimeProfileFrequency / 1000)) % 10000)
#else
	#define	TIME_PROFILE_START		llTicksStart = GetSupportedTickCount()
	#define TIME_PROFILE_END		llTicksEnd = GetSupportedTickCount()

	#define TIME_PROFILE_GET_HI		(llTicksEnd - llTicksStart)
	#define	TIME_PROFILE_GET_LO		(((llTicksEnd - llTicksStart) * 10) % 10000)
#endif


#endif //_INC_CSCRIPTPROFILER_H
