/**
* @file CResourceLock.cpp
*
*/

#include "../../sphere/threads.h"
#include "CResourceScript.h"
#include "CResourceLock.h"


bool CResourceLock::_Open(lpctstr ptcUnused, uint uiUnused)
{
    ADDTOCALLSTACK("CResourceLock::_Open");
    UnreferencedParameter(ptcUnused);
    UnreferencedParameter(uiUnused);
    ASSERT(m_pLock);

    if ( m_pLock->_IsFileOpen() )
        m_PrvLockContext = m_pLock->_GetContext();

    ASSERT(m_pLock->_HasCache());

    // Open a seperate copy of an already opened file.
    _pStream = m_pLock->_pStream;
    _fileDescriptor = m_pLock->_fileDescriptor;
    CCacheableScriptFile::_dupeFrom(m_pLock);
    // Assume this is the new error context !
    m_PrvScriptContext._OpenScript( this );
    return true;
}
bool CResourceLock::Open(lpctstr ptcUnused, uint uiUnused)
{
    ADDTOCALLSTACK("CResourceLock::Open");
    MT_UNIQUE_LOCK_RETURN(CResourceLock::_Open(ptcUnused, uiUnused));
}

void CResourceLock::_Close()
{
    ADDTOCALLSTACK("CResourceLock::_Close");

    if (!m_pLock)
        return;

    _pStream = nullptr;

    // Assume this is not the context anymore.
    m_PrvScriptContext._Close();

    if ( m_PrvLockContext.IsValid())
        m_pLock->_SeekContext(m_PrvLockContext);	// no need to set the line number as it should not have changed.

                                                    // Restore old position in the file (if there was one)
    m_pLock->_Close();	// decrement open count on the orig.

    if ( _IsWriteMode() || ( _GetFullMode() & OF_DEFAULTMODE ))
        _Init();
}
void CResourceLock::Close()
{
    ADDTOCALLSTACK("CResourceLock::Close");
    MT_UNIQUE_LOCK_SET;
    CResourceLock::_Close();
}

bool CResourceLock::_ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
    // This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive if called
    // this much often, so here it's to be preferred ADDTOCALLSTACK_DEBUG, even if we'll lose stack trace precision.
    ADDTOCALLSTACK("CResourceLock::_ReadTextLine");
    // ARGS:
    // fRemoveBlanks = Don't report any blank lines, (just keep reading)

    ASSERT(m_pLock);
    ASSERT( ! IsBinaryMode() );

    tchar* ptcBuf = _GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN);
    while ( CCacheableScriptFile::_ReadString( ptcBuf, SCRIPT_MAX_LINE_LEN ))
    {
        m_pLock->m_iLineNum = ++m_iLineNum;	// share this with original open.
        if ( fRemoveBlanks )
        {
            if ( ParseKeyEnd() <= 0 )
                continue;
        }
        return true;
    }

    m_pszKey[0] = '\0';
    return false;
}
bool CResourceLock::ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
    ADDTOCALLSTACK_DEBUG("CResourceLock::ReadTextLine");
    MT_UNIQUE_LOCK_RETURN(CResourceLock::_ReadTextLine(fRemoveBlanks));
}

int CResourceLock::OpenLock( CResourceScript * pLock, CScriptLineContext context )
{
    ADDTOCALLSTACK("CResourceLock::OpenLock");
    // ONLY called from CResourceLink
    ASSERT(pLock);

    Close();
    m_pLock = pLock;

    if ( ! Open() )	    // open my copy.
        return -2;

    if (!SeekContext(context))
        return -3;

    // Propagate m_iResourceFileIndex from the CResourceScript to this CResourceLock
    m_iResourceFileIndex = m_pLock->m_iResourceFileIndex;

    return 0;
}

void CResourceLock::AttachObj( const CScriptObj * pObj )
{
    m_PrvObjectContext.OpenObject(pObj);
}

