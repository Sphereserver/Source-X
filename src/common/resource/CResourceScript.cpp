/**
* @file CResourceScript.cpp
*
*/

#include "../../sphere/threads.h"
#include "../../game/CServer.h"
#include "../../game/CServerConfig.h"
#include "../sphere_library/CSFileList.h"
#include "../CLog.h"
#include "CResourceScript.h"


bool CResourceScript::_CheckForChange()
{
    ADDTOCALLSTACK("CResourceScript::_CheckForChange");
    // Get Size/Date info on the file to see if it has changed.
    time_t dateChange;
    dword dwSize;
    lpctstr ptcFilePath = _GetFilePath();

    if ( !CSFileList::ReadFileInfo(ptcFilePath, dateChange, dwSize) )
    {
        g_Log.EventError( "Can't get stats info for file '%s'\n", ptcFilePath );
        return false;
    }

    bool fChange = false;

    // See If the script has changed
    if ( ! IsFirstCheck() )
    {
        if ( (m_dwSize != dwSize) || (m_dateChange != dateChange) )
        {
            g_Log.Event(LOGL_WARN, "Resource '%s' changed, resync.\n", ptcFilePath);
            fChange = true;
        }
    }

    m_dwSize = dwSize;			// Compare to see if this has changed.
    m_dateChange = dateChange;	// real world time/date of last change.
    return fChange;
}
bool CResourceScript::CheckForChange()
{
    ADDTOCALLSTACK("CResourceScript::CheckForChange");
    MT_UNIQUE_LOCK_RETURN(CResourceScript::_CheckForChange());
}

void CResourceScript::ReSync()
{
    ADDTOCALLSTACK("CResourceScript::ReSync");
    if ( !IsFirstCheck() && !CheckForChange() )
        return;
    _fCacheToBeUpdated = true;
    if ( !Open() )
        return;
    g_Cfg.LoadResourcesOpen( this );
    Close();
}

bool CResourceScript::Open( lpctstr pszFilename, uint wFlags )
{
    ADDTOCALLSTACK("CResourceScript::Open");
    // Open the file if it is not already open for use.

    if ( !IsFileOpen() && !(wFlags & OF_READWRITE))
    {
        if (_fCacheToBeUpdated || (pszFilename && _strFileName.Compare(pszFilename)) || !HasCache())
        {
            if ( ! CScript::Open( pszFilename, wFlags|OF_SHARE_DENY_WRITE))	// OF_READ
                return false;
        }
        if ( CheckForChange() )
        {
            //  what should we do about it ? reload it of course !
            g_Serv.SetServerMode(SERVMODE_ResyncLoad);
            g_Cfg.LoadResourcesOpen( this );
            g_Serv.SetServerMode(SERVMODE_Run);
        }
    }
    ASSERT(HasCache());

    ++m_iOpenCount;
    return true;
}

void CResourceScript::CloseForce()
{
    ADDTOCALLSTACK("CResourceScript::CloseForce");
    m_iOpenCount = 0;
    CScript::CloseForce();
}

void CResourceScript::Close()
{
    ADDTOCALLSTACK("CResourceScript::Close");
    // Don't close the file yet.
    // Close it later when we know it has not been used for a bit.
    if ( ! IsFileOpen())
        return;
    --m_iOpenCount;

    if ( ! m_iOpenCount )
    {
        // Just leave it open for caching purposes
        //CloseForce();
    }
}
