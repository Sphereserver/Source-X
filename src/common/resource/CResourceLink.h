/**
* @file CResourceLink.h
*
*/

#ifndef _INC_CRESOURCELINK_H
#define _INC_CRESOURCELINK_H

#include "../CScriptContexts.h"
#include "CResourceDef.h"

class CResourceScript;


// Each array can store 8 bytes * 4 bits per byte = 32 bits, or 32 values for 32 triggers.
#define MAX_TRIGGERS_ARRAY	10

class CResourceLink : public CResourceDef
{
    // A single resource object that also has part of itself remain in resource file.
    // A pre-indexed link into a script file.
    // This is a CResourceDef not fully read into memory at index time.
    // We are able to lock it and read it as needed
private:
    CResourceScript * m_pScript;	// we already found the script.
    CScriptLineContext m_Context;

    dword _dwRefInstances;	// How many CResourceRef objects refer to this ?

public:
    dword m_dwOnTriggers[MAX_TRIGGERS_ARRAY];
    static const char *m_sClassName;

#define XTRIG_UNKNOWN 0	// bit 0 is reserved to say there are triggers here that do not conform.

public:
    inline dword GetRefInstances() const noexcept
    {
        return _dwRefInstances;
    }

    inline void AddRefInstance() noexcept
    {
        ++_dwRefInstances;
    }
    
    void DelRefInstance();

    bool IsLinked() const;	// been loaded from the scripts ?
    CResourceScript * GetLinkFile() const;
    int GetLinkOffset() const;
    void SetLink( CResourceScript * pScript );
    void CopyTransfer( CResourceLink * pLink );
    void ScanSection( RES_TYPE restype );
    void ClearTriggers();
    void SetTrigger( int i );
    bool HasTrigger( int i ) const;
    bool ResourceLock( CResourceLock & s );

public:
    CResourceLink(const CResourceID& rid, const CVarDefContNum * pDef = nullptr);
    virtual ~CResourceLink() = default;

private:
    CResourceLink(const CResourceLink& copy);
    CResourceLink& operator=(const CResourceLink& other);
};

#endif // _INC_CRESOURCELINK_H
