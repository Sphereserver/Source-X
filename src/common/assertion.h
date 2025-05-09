#ifndef _INC_ASSERTION_H
#define _INC_ASSERTION_H

#ifndef STATIC_ANALYSIS_
    #if defined(__COVERITY__)
        #define STATIC_ANALYSIS_
    #endif
#endif

[[noreturn]]
extern void Assert_Fail(const char * pExp, const char *pFile, long long llLine);

#define ASSERT_ALWAYS(exp)  if ( !(exp) ) [[unlikely]] Assert_Fail(#exp, __FILE__, __LINE__)

#if defined(STATIC_ANALYSIS)
    #include <cassert>
    #define ASSERT(exp)         assert(exp)
#elif defined(_NIGHTLYBUILD) || defined(_DEBUG)
    #define ASSERT(exp)         if ( !(exp) ) [[unlikely]] Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define ASSERT(exp)         (void)0
#endif

#if defined(_DEBUG) || defined(STATIC_ANALYSIS)
    #define DEBUG_ASSERT(exp)   if ( !(exp) ) [[unlikely]] Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define DEBUG_ASSERT(exp)   (void)0
#endif


#undef STATIC_ANALYSIS_
#endif // ! _INC_ASSERTION_H

