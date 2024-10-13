

#include "../game/chars/CChar.h"
#include "../sphere/threads.h"
#include "sphere_library/CSAssoc.h"
#include "sphere_library/sstringobjs.h"
#include "CTextConsole.h"


extern CSStringList g_AutoComplete;

CChar * CTextConsole::GetChar() const
{
    return const_cast <CChar *>( dynamic_cast <const CChar *>( this ));
}

int CTextConsole::OnConsoleKey( CSString & sText, tchar nChar, bool fEcho )
{
    ADDTOCALLSTACK("CTextConsole::OnConsoleKey");
    // eventaully we should call OnConsoleCmd
    // RETURN:
    //  0 = dump this connection.
    //  1 = keep processing.
    //  2 = process this.

    if ( sText.GetLength() >= SCRIPT_MAX_LINE_LEN )
    {
        commandtoolong:
        SysMessage( "Command too long\n" );

        sText.Clear();
        return 0;
    }

    if ( nChar == '\r' || nChar == '\n' )
    {
        // Ignore the character if we have no text stored
        if (!sText.GetLength())
            return 1;

        if ( fEcho )
        {
            SysMessage("\n");
        }
        return 2;
    }
    else if ( nChar == 9 )			// TAB (auto-completion)
    {
        lpctstr p = nullptr;
        lpctstr tmp = nullptr;
        size_t inputLen = 0;
        bool matched = false;

        //	extract up to start of the word
        p = sText.GetBuffer() + sText.GetLength();
        while ((p >= sText.GetBuffer()) && (*p != '.') && (*p != ' ') && (*p != '/') && (*p != '='))
        {
            --p;
        }
        ++p;
        inputLen = strlen(p);

        // search in the auto-complete list for starting on P, and save coords of 1st and Last matched
        CSStringListRec	*firstmatch = nullptr, *lastmatch = nullptr;
        CSStringListRec	*curmatch = nullptr, *nextmatch = nullptr;	// the one that should be set
        for ( curmatch = g_AutoComplete.GetHead(); curmatch != nullptr; curmatch = nextmatch )
        {
            nextmatch = curmatch->GetNext();
            if ( !strnicmp(curmatch->GetBuffer(), p, inputLen) )	// matched
            {
                if ( firstmatch == nullptr )
                    firstmatch = lastmatch = curmatch;
                else
                    lastmatch = curmatch;
            }
            else if ( lastmatch )   // if no longer matches - save time by instant quit
                break;
        }

        if (( firstmatch != nullptr ) && ( firstmatch == lastmatch ))	// there IS a match and the ONLY
        {
            tmp = firstmatch->GetBuffer() + inputLen;
            matched = true;
        }
        else if ( firstmatch != nullptr )						// also make SE (if SERV/SERVER in dic) to become SERV
        {
            p = tmp = firstmatch->GetBuffer();
            tmp += inputLen;
            inputLen = strlen(p);
            matched = true;
            for ( curmatch = firstmatch->GetNext(); curmatch != lastmatch->GetNext(); curmatch = curmatch->GetNext() )
            {
                if (strnicmp(curmatch->GetBuffer(), p, inputLen) != 0)	// mismatched
                {
                    matched = false;
                    break;
                }
            }
        }

        if ( matched )
        {
            if ( fEcho )
                SysMessage(tmp);

            sText += tmp;
            if ( sText.GetLength() > SCRIPT_MAX_LINE_LEN )
                goto commandtoolong;
        }
        return 1;
    }

    if ( fEcho )
    {
        // Echo
        tchar szTmp[2];
        szTmp[0] = nChar;
        szTmp[1] = '\0';
        SysMessage( szTmp );
    }

    if ( nChar == 8 )
    {
        if ( sText.GetLength())	// back key
        {
            sText.Resize( sText.GetLength() - 1 );
        }
        return 1;
    }

    sText += nChar;
    return 1;
}

void CTextConsole::VSysMessage( lpctstr pszFormat, va_list args ) const
{
    TemporaryString tsTemp;
    vsnprintf(tsTemp.buffer(), tsTemp.capacity(), pszFormat, args );
    SysMessage(tsTemp.buffer());
}

void CTextConsole::SysMessagef( lpctstr pszFormat, ... ) const
{
    va_list vargs;
    va_start( vargs, pszFormat );
    VSysMessage( pszFormat, vargs );
    va_end( vargs );
}
