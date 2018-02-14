/**
* @file CScriptProfiler.h
*
*/

#pragma once
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

//	Time measurement macros
extern llong llTimeProfileFrequency;

#define	TIME_PROFILE_INIT			llong llTicksStart = 0, llTicksEnd = 0

#ifdef _WIN32
	#define	TIME_PROFILE_START		if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&llTicksStart)))	llTicksStart = GetTickCount64()
	#define TIME_PROFILE_END		if (!QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&llTicksEnd)))	llTicksEnd = GetTickCount64()

	#define TIME_PROFILE_GET_HI		((llTicksEnd - llTicksStart) / (llTimeProfileFrequency / 1000))
	#define	TIME_PROFILE_GET_LO		((((llTicksEnd - llTicksStart) * 10000) / (llTimeProfileFrequency / 1000)) % 10000)
#else
	#define	TIME_PROFILE_START		llTicksStart = GetTickCount64()
	#define TIME_PROFILE_END		llTicksEnd = GetTickCount64();

	#define TIME_PROFILE_GET_HI		(llTicksEnd - llTicksStart)
	#define	TIME_PROFILE_GET_LO		(((llTicksEnd - llTicksStart) * 10) % 10000)
#endif


#endif //_INC_CSCRIPTPROFILER_H
