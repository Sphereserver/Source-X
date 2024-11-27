/**
* @file CResourceScript.h
*
*/

#ifndef _INC_CRESOURCESCRIPT_H
#define _INC_CRESOURCESCRIPT_H

#include "../sphere_library/CSTime.h"
#include "../CScript.h"


class CResourceScript : public CScript
{
    // A script file containing resource, speech, motives or events handlers.
    // NOTE: we should check periodically if this file has been altered externally ?

private:
    int		m_iOpenCount;		    // How many CResourceLock(s) have this open ?

    // Last time it was closed. What did the file params look like ?
    dword m_dwSize;			// Compare to see if this has changed.
    CSTime m_dateChange;	// real world time/date of last change.

private:
    void _Init()
    {
        m_iOpenCount = 0;
        m_dwSize = UINT32_MAX;			// Compare to see if this has changed.
    }

public:
    static const char *m_sClassName;
    explicit CResourceScript(lpctstr pszFileName)
    {
        _Init();
        _SetFilePath(pszFileName);
    }
    CResourceScript()
    {
        _Init();
    }
    virtual ~CResourceScript() = default;

private:    bool _CheckForChange();
public:     bool CheckForChange();

public:
    CResourceScript(const CResourceScript& copy) = delete;
    CResourceScript& operator=(const CResourceScript& other) = delete;

    bool IsFirstCheck() const noexcept
    {
        return (m_dwSize == UINT32_MAX && !m_dateChange.IsTimeValid());
    }
    void ReSync();
    virtual bool Open( lpctstr pszFilename = nullptr, uint wFlags = OF_READ ) override;
    virtual void Close() override;
    virtual void CloseForce() override;
};


#endif // _INC_CRESOURCESCRIPT_H
