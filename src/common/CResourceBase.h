/**
* @file CResourceBase.h
*
*/

#ifndef _INC_CRESOURCEBASE_H
#define _INC_CRESOURCEBASE_H

#include "sphere_library/CSTime.h"
#include "../game/CServerTime.h"
#include "CScriptContexts.h"
#include "CResourceDef.h"
#include "CResourceHash.h"
#include "CResourceLink.h"
#include "CResourceQty.h"


class CResourceBase : public CScriptObj
{
protected:
	static lpctstr const sm_szResourceBlocks[RES_QTY];

	CSObjArray< CResourceScript* > m_ResourceFiles;	// All resource files we need to get blocks from later.

public:
	static const char *m_sClassName;
	CResourceHash m_ResHash;		// All script linked resources RES_QTY

	// INI File options.
	CSString m_sSCPBaseDir;		// if we want to get *.SCP files from elsewhere.

protected:
	CResourceScript * AddResourceFile( lpctstr pszName );
	void AddResourceDir( lpctstr pszDirName );

public:
	void LoadResourcesOpen( CScript * pScript );
	bool LoadResources( CResourceScript * pScript );
	static lpctstr GetResourceBlockName( RES_TYPE restype );
	lpctstr GetName() const;
	CResourceScript * GetResourceFile( size_t i );
	CResourceID ResourceGetID( RES_TYPE restype, lpctstr & pszName );
	CResourceID ResourceGetIDType( RES_TYPE restype, lpctstr pszName );
	int ResourceGetIndexType( RES_TYPE restype, lpctstr pszName );
	lpctstr ResourceGetName( CResourceIDBase rid ) const;
	CScriptObj * ResourceGetDefByName( RES_TYPE restype, lpctstr pszName )
	{
		// resolve a name to the actual resource def.
		return ResourceGetDef(ResourceGetID(restype, pszName));
	}
	bool ResourceLock( CResourceLock & s, CResourceIDBase rid );
	bool ResourceLock( CResourceLock & s, RES_TYPE restype, lpctstr pszName )
	{
		return ResourceLock(s, ResourceGetIDType(restype, pszName));
	}

	CResourceScript * FindResourceFile( lpctstr pszTitle );
	CResourceScript * LoadResourcesAdd( lpctstr pszNewName );

	virtual CResourceDef * ResourceGetDef( CResourceIDBase rid ) const;
	virtual bool OpenResourceFind( CScript &s, lpctstr pszFilename, bool bCritical = true );
	virtual bool LoadResourceSection( CScript * pScript ) = 0;

public:
	CResourceBase()
	{
	}
	virtual ~CResourceBase()
	{
	}

private:
	CResourceBase(const CResourceBase& copy);
	CResourceBase& operator=(const CResourceBase& other);
};

inline lpctstr CResourceBase::GetResourceBlockName( RES_TYPE restype )	// static
{
	if ( restype < 0 || restype >= RES_QTY )
		restype = RES_UNKNOWN;
	return( sm_szResourceBlocks[restype] );
}

#endif // _INC_CRESOURCEBASE_H
