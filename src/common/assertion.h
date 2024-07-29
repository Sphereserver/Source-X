#ifndef _INC_ASSERTION_H
#define _INC_ASSERTION_H

#ifndef STATIC_ANALYSIS
    #if defined(__COVERITY__)
        #define STATIC_ANALYSIS
    #endif
#endif

extern void Assert_Fail(const char * pExp, const char *pFile, long long llLine);

#define PERSISTANT_ASSERT(exp)  if ( !(exp) )   Assert_Fail(#exp, __FILE__, __LINE__)

#if defined(STATIC_ANALYSIS)
    #include <cassert>
    #define ASSERT(exp)         assert(exp)
#elif defined(_NIGHTLY) || defined(_DEBUG)
    #define ASSERT(exp)         if ( !(exp) )   Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define ASSERT(exp)         (void)0
#endif

#if defined(_DEBUG) || defined(STATIC_ANALYSIS)
    #define DEBUG_ASSERT(exp)   if ( !(exp) )   Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define DEBUG_ASSERT(exp)   (void)0
#endif


#undef STATIC_ANALYSIS
#endif // ! _INC_ASSERTION_H

