#ifndef _INC_SASSERTION
#define _INC_SASSERTION


extern void Assert_Fail(const char * pExp, const char *pFile, long long llLine);

#define PERSISTANT_ASSERT(exp)	if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__)

#if defined(_NIGHTLY) || defined(_DEBUG) || defined(__COVERITY__)
    #define ASSERT(exp)		    if ( !(exp) )	Assert_Fail(#exp, __FILE__, __LINE__)
#else
    #define ASSERT(exp)			(void)0
#endif


#endif // ! _INC_SASSERTION

