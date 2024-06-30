#ifndef _INC_ASSERTION_H
#define _INC_ASSERTION_H


extern void Assert_Fail(const char * pExp, const char *pFile, long long llLine);

#define PERSISTANT_ASSERT(exp)	if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__)

#if defined(_NIGHTLY) || defined(_DEBUG) || defined(__COVERITY__)
    #define ASSERT(exp)		    if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define ASSERT(exp)			(void)0
#endif

#if defined(_DEBUG) || defined(__COVERITY__)
    #define DEBUG_ASSERT(exp)		    if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define DEBUG_ASSERT(exp)			(void)0
#endif


#endif // ! _INC_ASSERTION_H

