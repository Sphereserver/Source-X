#include "sstring.h"
#include "../../sphere/threads.h"
#include "../../sphere/ProfileTask.h"
#include "../CExpression.h"
#include "../CScript.h"


#ifndef _MSC_VER
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#include "../regex/deelx.h"
#ifndef _MSC_VER
    #pragma GCC diagnostic pop
#endif

// String utilities: Modifiers

size_t strcpylen(tchar * pDst, lpctstr pSrc)
{
    strcpy(pDst, pSrc);
    return strlen(pDst);
}

size_t strncpylen(tchar * pDst, lpctstr pSrc, size_t uiMaxSize)
{
    // it does NOT include the iMaxSize element! (just like memcpy)
    // so iMaxSize=sizeof() is ok !
    ASSERT(uiMaxSize);
    strncpy(pDst, pSrc, uiMaxSize - 1);
    pDst[uiMaxSize - 1] = '\0';	// always terminate.
    return strlen(pDst);
}

void strncpynull(tchar * pDst, lpctstr pSrc, size_t uiMaxSize)
{
    // it does NOT include the iMaxSize element! (just like memcpy)
    // so iMaxSize=sizeof() is ok !
    ASSERT(uiMaxSize);
    strncpy(pDst, pSrc, uiMaxSize - 1);
    pDst[uiMaxSize - 1] = '\0';	// always terminate.
}

lpctstr Str_GetArticleAndSpace(lpctstr pszWord)
{
    // NOTE: This is wrong many times.
    //  ie. some words need no article (plurals) : boots.

    // This function shall NOT be called if OF_NoPrefix is set!

    if (pszWord)
    {
        static const tchar sm_Vowels[] = { 'A', 'E', 'I', 'O', 'U' };
        tchar chName = static_cast<tchar>(toupper(pszWord[0]));
        for (uint x = 0; x < CountOf(sm_Vowels); ++x)
        {
            if (chName == sm_Vowels[x])
                return "an ";
        }
    }
    return "a ";
}

int Str_GetBare(tchar * pszOut, lpctstr pszInp, int iMaxOutSize, lpctstr pszStrip)
{
    // That the client can deal with. Basic punctuation and alpha and numbers.
    // RETURN: Output length.

    if (!pszStrip)
        pszStrip = "{|}~";	// client cant print these.

                            //GETNONWHITESPACE( pszInp );	// kill leading white space.

    int j = 0;
    for (int i = 0; ; ++i)
    {
        tchar ch = pszInp[i];
        if (ch)
        {
            if (ch < ' ' || ch >= 127)
                continue;	// Special format chars.

            int k = 0;
            while (pszStrip[k] && pszStrip[k] != ch)
                ++k;

            if (pszStrip[k])
                continue;

            if (j >= iMaxOutSize - 1)
                ch = '\0';
        }
        pszOut[j++] = ch;
        if (ch == 0)
            break;
    }
    return (j - 1);
}

tchar * Str_MakeFiltered(tchar * pStr)
{
    int len = (int)strlen(pStr);
    for (int i = 0; len; ++i, --len)
    {
        if (pStr[i] == '\\')
        {
            switch (pStr[i + 1])
            {
                case 'b': pStr[i] = '\b'; break;
                case 'n': pStr[i] = '\n'; break;
                case 'r': pStr[i] = '\r'; break;
                case 't': pStr[i] = '\t'; break;
                case '\\': pStr[i] = '\\'; break;
            }
            --len;
            memmove(pStr + i + 1, pStr + i + 2, len);
        }
    }
    return pStr;
}

void Str_MakeUnFiltered(tchar * pStrOut, lpctstr pStrIn, int iSizeMax)
{
    int len = (int)strlen(pStrIn);
    int iIn = 0;
    int iOut = 0;
    for (; iOut < iSizeMax && iIn <= len; ++iIn, ++iOut)
    {
        tchar ch = pStrIn[iIn];
        switch (ch)
        {
            case '\b': ch = 'b'; break;
            case '\n': ch = 'n'; break;
            case '\r': ch = 'r'; break;
            case '\t': ch = 't'; break;
            case '\\': ch = '\\'; break;
            default:
                pStrOut[iOut] = ch;
                continue;
        }

        pStrOut[iOut++] = '\\';
        pStrOut[iOut] = ch;
    }
}

int Str_TrimEndWhitespace(tchar * pStr, int len)
{
    while (len > 0)
    {
        --len;
        if (pStr[len] < 0 || !ISWHITESPACE(pStr[len]))
        {
            ++len;
            break;
        }
    }
    pStr[len] = '\0';
    return len;
}

tchar * Str_TrimWhitespace(tchar * pStr)
{
    // TODO: WARNING! Possible Memory Leak here!
    GETNONWHITESPACE(pStr);
    Str_TrimEndWhitespace(pStr, (int)strlen(pStr));
    return pStr;
}

// String utilities: String operations

int FindTable(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
    // A non-sorted table.
    for (int i = 0; i < iCount; ++i)
    {
        if (!strcmpi(*ppszTable, pszFind))
            return i;
        ppszTable = (lpctstr const *)((const byte *)ppszTable + iElemSize);
    }
    return -1;
}

int FindTableSorted(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
    // Do a binary search (un-cased) on a sorted table.
    // RETURN: -1 = not found
    int iHigh = iCount - 1;
    if (iHigh < 0)
        return -1;
    int iLow = 0;

    while (iLow <= iHigh)
    {
        int i = (iHigh + iLow) / 2;
        lpctstr pszName = *((lpctstr const *)((const byte *)ppszTable + (i*iElemSize)));
        int iCompare = strcmpi(pszFind, pszName);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;
}

static int Str_CmpHeadI(lpctstr pszFind, lpctstr pszTable)
{
    tchar ch0 = '_';
    for (int i = 0; ; ++i)
    {
        //	we should always use same case as in other places. since strcmpi lowers,
        //	we should lower here as well. fucking shit!
        tchar ch1 = static_cast<tchar>(tolower(pszFind[i]));
        tchar ch2 = static_cast<tchar>(tolower(pszTable[i]));
        if (ch2 == 0)
        {
            if ( (!iswalnum(ch1)) && (ch1 != ch0) )
                return 0;
            return (ch1 - ch2);
        }
        if (ch1 != ch2)
            return (ch1 - ch2);
    }
}

int FindTableHead(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
    for (int i = 0; i < iCount; ++i)
    {
        int iCompare = Str_CmpHeadI(pszFind, *ppszTable);
        if (!iCompare)
            return i;
        ppszTable = (lpctstr const *)((const byte *)ppszTable + iElemSize);
    }
    return -1;
}

int FindTableHeadSorted(lpctstr pszFind, lpctstr const * ppszTable, int iCount, int iElemSize)
{
    // Do a binary search (un-cased) on a sorted table.
    // RETURN: -1 = not found
    int iHigh = iCount - 1;
    if (iHigh < 0)
        return -1;
    int iLow = 0;

    while (iLow <= iHigh)
    {
        int i = (iHigh + iLow) / 2;
        lpctstr pszName = *((lpctstr const *)((const byte *)ppszTable + (i*iElemSize)));
        int iCompare = Str_CmpHeadI(pszFind, pszName);
        if (iCompare == 0)
            return i;
        if (iCompare > 0)
            iLow = i + 1;
        else
            iHigh = i - 1;
    }
    return -1;
}

bool Str_Check(lpctstr pszIn)
{
    if (pszIn == NULL)
        return true;

    lpctstr p = pszIn;
    while (*p != '\0' && (*p != 0x0A) && (*p != 0x0D))
        ++p;

    return (*p != '\0');
}

bool Str_CheckName(lpctstr pszIn)
{
    if (pszIn == NULL)
        return true;

    lpctstr p = pszIn;
    while (*p != '\0' &&
        (
        ((*p >= 'A') && (*p <= 'Z')) ||
            ((*p >= 'a') && (*p <= 'z')) ||
            ((*p >= '0') && (*p <= '9')) ||
            ((*p == ' ') || (*p == '\'') || (*p == '-') || (*p == '.'))
            ))
        ++p;

    return (*p != '\0');
}

int Str_IndexOf(tchar * pStr1, tchar * pStr2, int offset)
{
    if (offset < 0)
        return -1;

    int len = (int)strlen(pStr1);
    if (offset >= len)
        return -1;

    int slen = (int)strlen(pStr2);
    if (slen > len)
        return -1;

    tchar firstChar = pStr2[0];

    for (int i = offset; i < len; ++i)
    {
        tchar c = pStr1[i];
        if (c == firstChar)
        {
            int rem = len - i;
            if (rem >= slen)
            {
                int j = i;
                int k = 0;
                bool found = true;
                while (k < slen)
                {
                    if (pStr1[j] != pStr2[k])
                    {
                        found = false;
                        break;
                    }
                    ++j; ++k;
                }
                if (found)
                    return i;
            }
        }
    }

    return -1;
}

static MATCH_TYPE Str_Match_After_Star(lpctstr pPattern, lpctstr pText)
{
    // pass over existing ? and * in pattern
    for (; *pPattern == '?' || *pPattern == '*'; ++pPattern)
    {
        // take one char for each ? and +
        if (*pPattern == '?' &&
            !*pText++)		// if end of text then no match
            return MATCH_ABORT;
    }

    // if end of pattern we have matched regardless of text left
    if (!*pPattern)
        return MATCH_VALID;

    // get the next character to match which must be a literal or '['
    tchar nextp = static_cast<tchar>(tolower(*pPattern));
    MATCH_TYPE match = MATCH_INVALID;

    // Continue until we run out of text or definite result seen
    do
    {
        // a precondition for matching is that the next character
        // in the pattern match the next character in the text or that
        // the next pattern char is the beginning of a range.  Increment
        // text pointer as we go here
        if (nextp == tolower(*pText) || nextp == '[')
        {
            match = Str_Match(pPattern, pText);
            if (match == MATCH_VALID)
                break;
        }

        // if the end of text is reached then no match
        if (!*pText++)
            return MATCH_ABORT;

    } while (
        match != MATCH_ABORT &&
        match != MATCH_PATTERN);

    return match;	// return result
}

MATCH_TYPE Str_Match(lpctstr pPattern, lpctstr pText)
{
    // case independant

    tchar range_start;
    tchar range_end;  // start and end in range

    for (; *pPattern; ++pPattern, ++pText)
    {
        // if this is the end of the text then this is the end of the match
        if (!*pText)
            return (*pPattern == '*' && *++pPattern == '\0') ? MATCH_VALID : MATCH_ABORT;

        // determine and react to pattern type
        switch (*pPattern)
        {
            // single any character match
            case '?':
                break;
                // multiple any character match
            case '*':
                return Str_Match_After_Star(pPattern, pText);
                // [..] construct, single member/exclusion character match
            case '[':
            {
                // move to beginning of range
                ++pPattern;
                // check if this is a member match or exclusion match
                bool fInvert = false;             // is this [..] or [!..]
                if (*pPattern == '!' || *pPattern == '^')
                {
                    fInvert = true;
                    ++pPattern;
                }
                // if closing bracket here or at range start then we have a
                // malformed pattern
                if (*pPattern == ']')
                    return MATCH_PATTERN;

                bool fMemberMatch = false;       // have I matched the [..] construct?
                for (;;)
                {
                    // if end of construct then fLoop is done
                    if (*pPattern == ']')
                        break;

                    // matching a '!', '^', '-', '\' or a ']'
                    if (*pPattern == '\\')
                        range_start = range_end = static_cast<tchar>(tolower(*++pPattern));
                    else
                        range_start = range_end = static_cast<tchar>(tolower(*pPattern));

                    // if end of pattern then bad pattern (Missing ']')
                    if (!*pPattern)
                        return MATCH_PATTERN;

                    // check for range bar
                    if (*++pPattern == '-')
                    {
                        // get the range end
                        range_end = static_cast<tchar>(tolower(*++pPattern));
                        // if end of pattern or construct then bad pattern
                        if (range_end == '\0' || range_end == ']')
                            return MATCH_PATTERN;
                        // special character range end
                        if (range_end == '\\')
                        {
                            range_end = static_cast<tchar>(tolower(*++pPattern));
                            // if end of text then we have a bad pattern
                            if (!range_end)
                                return MATCH_PATTERN;
                        }
                        // move just beyond this range
                        ++pPattern;
                    }

                    // if the text character is in range then match found.
                    // make sure the range letters have the proper
                    // relationship to one another before comparison
                    tchar chText = static_cast<tchar>(tolower(*pText));
                    if (range_start < range_end)
                    {
                        if (chText >= range_start && chText <= range_end)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                    else
                    {
                        if (chText >= range_end && chText <= range_start)
                        {
                            fMemberMatch = true;
                            break;
                        }
                    }
                }	// while

                    // if there was a match in an exclusion set then no match
                    // if there was no match in a member set then no match
                if ((fInvert && fMemberMatch) ||
                    !(fInvert || fMemberMatch))
                    return MATCH_RANGE;

                // if this is not an exclusion then skip the rest of the [...]
                //  construct that already matched.
                if (fMemberMatch)
                {
                    while (*pPattern != ']')
                    {
                        // bad pattern (Missing ']')
                        if (!*pPattern)
                            return MATCH_PATTERN;
                        // skip exact match
                        if (*pPattern == '\\')
                        {
                            ++pPattern;
                            // if end of text then we have a bad pattern
                            if (!*pPattern)
                                return MATCH_PATTERN;
                        }
                        // move to next pattern char
                        ++pPattern;
                    }
                }
            }
            break;

            // must match this character (case independant) ?exactly
            default:
                if (tolower(*pPattern) != tolower(*pText))
                    return MATCH_LITERAL;
        }
    }
    // if end of text not reached then the pattern fails
    if (*pText)
        return MATCH_END;
    else
        return MATCH_VALID;
}

#ifdef _MSC_VER
    // /GL + /LTCG flags inline in linking phase this function, but probably in a wrong way, so that
    // something gets corrupted on the memory and an exception is generated later
    #pragma auto_inline(off)
#endif
bool Str_Parse(tchar * pLine, tchar ** ppArg, lpctstr pszSep)
{
    // Parse a list of args. Just get the next arg.
    // similar to strtok()
    // RETURN: true = the second arg is valid.

    if (pszSep == NULL)	// default sep.
        pszSep = "=, \t";

    // skip leading white space.
    GETNONWHITESPACE(pLine);

    tchar ch;
    // variables used to track opened/closed quotes and brackets
    bool bQuotes = false;
    int iCurly, iSquare, iRound, iAngle;
    iCurly = iSquare = iRound = iAngle = 0;

    // ignore opened/closed brackets if that type of bracket is also a separator
    bool bSepHasCurly, bSepHasSquare, bSepHasRound, bSepHasAngle;
    bSepHasCurly = bSepHasSquare = bSepHasRound = bSepHasAngle = false;
    for (uint j = 0; pszSep[j + 1] != '\0'; ++j)		// loop through each separator
    {
        const tchar & sep = pszSep[j];
        if (sep == '{' || sep == '}')
            bSepHasCurly = true;
        else if (sep == '[' || sep == ']')
            bSepHasSquare = true;
        else if (sep == '(' || sep == ')')
            bSepHasRound = true;
        else if (sep == '<' || sep == '>')
            bSepHasAngle = true;
    }

    for (; ; ++pLine)
    {
        ch = *pLine;
        if (ch == '"')	// quoted argument
        {
            bQuotes = !bQuotes;
            continue;
        }
        if (ch == '\0')	// no more args i guess.
        {
            if (ppArg != NULL)
                *ppArg = pLine;
            return false;
        }

        if (!bQuotes)
        {
            // We are not inside a quote, so let's check if the char is a bracket or a separator

            // Here we track opened and closed brackets.
            //	we'll ignore items inside brackets, if the bracket isn't a separator in the list
            if (ch == '{') {
                if (!bSepHasCurly) {
                    if (!iSquare && !iRound && !iAngle)
                        ++iCurly;
                }
            }
            else if (ch == '[') {
                if (!bSepHasSquare) {
                    if (!iCurly && !iRound && !iAngle)
                        ++iSquare;
                }
            }
            else if (ch == '(') {
                if (!bSepHasRound) {
                    if (!iCurly && !iSquare && !iAngle)
                        ++iRound;
                }
            }
            else if (ch == '<') {
                if (!bSepHasAngle) {
                    if (!iCurly && !iSquare && !iRound)
                        ++iAngle;
                }
            }
            else if (ch == '}') {
                if (!bSepHasCurly) {
                    if (iCurly)
                        --iCurly;
                }
            }
            else if (ch == ']') {
                if (!bSepHasSquare) {
                    if (iSquare)
                        --iSquare;
                }
            }
            else if (ch == ')') {
                if (!bSepHasRound) {
                    if (iRound)
                        --iRound;
                }
            }
            else if (ch == '>') {
                if (!bSepHasAngle) {
                    if (iAngle)
                        --iAngle;
                }
            }

            // separate the string when i encounter a separator, but only if at this point of the string we aren't inside an argument
            // enclosed by brackets. but, if one of the separators is a bracket, don't care if we are inside or outside, separate anyways.

            //	don't turn this if into an else if!
            //	We can choose as a separator also one of {[(< >)]} and they have to be treated as such!
            if ((iCurly<=0) && (iSquare<=0) && (iRound<=0))
            {
                if (strchr(pszSep, ch))		// if ch is a separator
                    break;
            }
        }	// end of the quotes if clause

    }	// end of the for loop

    if (*pLine == '\0')
        return false;

    *pLine = '\0';
    ++pLine;
    if (IsSpace(ch))	// space separators might have other seps as well ?
    {
        GETNONWHITESPACE(pLine);
        ch = *pLine;
        if (ch && strchr(pszSep, ch))
            ++pLine;
    }

    // skip trailing white space on args as well.
    if (ppArg != NULL)
        *ppArg = Str_TrimWhitespace(pLine);

    if (iCurly || iSquare || iRound || bQuotes)
    {
        //g_Log.EventError("Not every bracket or quote was closed.\n");
        return false;
    }

    return true;
}
#ifdef _MSC_VER
    #pragma auto_inline(on)
#endif

int Str_ParseCmds(tchar * pszCmdLine, tchar ** ppCmd, int iMax, lpctstr pszSep)
{
    int iQty = 0;
    GETNONWHITESPACE(pszCmdLine);

    if (pszCmdLine != NULL && pszCmdLine[0] != '\0')
    {
        ppCmd[0] = pszCmdLine;
        ++iQty;
        while (Str_Parse(ppCmd[iQty - 1], &(ppCmd[iQty]), pszSep))
        {
            if (++iQty >= iMax)
                break;
        }
    }
    for (int j = iQty; j < iMax; ++j)
        ppCmd[j] = NULL;	// terminate if possible.
    return iQty;
}

int Str_ParseCmds(tchar * pszCmdLine, int64 * piCmd, int iMax, lpctstr pszSep)
{
    tchar * ppTmp[256];
    if (iMax > (int)CountOf(ppTmp))
        iMax = (int)CountOf(ppTmp);

    int iQty = Str_ParseCmds(pszCmdLine, ppTmp, iMax, pszSep);
    int i;
    for (i = 0; i < iQty; ++i)
        piCmd[i] = Exp_GetVal(ppTmp[i]);
    for (; i < iMax; ++i)
        piCmd[i] = 0;

    return iQty;
}

int Str_RegExMatch(lpctstr pPattern, lpctstr pText, tchar * lastError)
{
    try
    {
        CRegexp expressionformatch(pPattern, NO_FLAG);
        MatchResult result = expressionformatch.Match(pText);
        if (result.IsMatched())
            return 1;

        return 0;
    }
    catch (const std::bad_alloc e)
    {
        strncpynull(lastError, e.what(), SCRIPT_MAX_LINE_LEN);
        CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
    catch (...)
    {
        strncpynull(lastError, "Unknown", SCRIPT_MAX_LINE_LEN);
        CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
        return -1;
    }
}

void CharToMultiByteNonNull(byte * Dest, const char * Src, int MBytes)
{
    for (int idx = 0; idx != MBytes * 2; idx += 2) {
        if (Src[idx / 2] == '\0')
            break;
        Dest[idx] = (byte)(Src[idx / 2]);
    }
}
