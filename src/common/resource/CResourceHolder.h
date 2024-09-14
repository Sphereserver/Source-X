/**
* @file CResourceHolder.h
*
*/

#ifndef _INC_CResourceHolder_H
#define _INC_CResourceHolder_H

#include "../sphere_library/CSObjArray.h"
#include "../sphere_library/CSString.h"
#include "CResourceDef.h"
#include "CResourceHash.h"
#include "CResourceLink.h"


class CResourceScript;

class CResourceHolder : public CScriptObj
{
protected:
	static lpctstr const sm_szResourceBlocks[RES_QTY];

public:
	static const char *m_sClassName;
	CResourceHash m_ResHash;		// All script linked resources RES_QTY

	// INI File options.
	CSString m_sSCPBaseDir;		// if we want to get *.SCP files from elsewhere.

protected:
	CSObjArray< CResourceScript* > m_ResourceFiles;	// All resource files we need to get blocks from later.


protected:
	CResourceScript * AddResourceFile( lpctstr pszName );
	void AddResourceDir( lpctstr pszDirName );

public:
	void LoadResourcesOpen( CScript * pScript );
	bool LoadResources( CResourceScript * pScript );
	static lpctstr GetResourceBlockName( RES_TYPE restype );
	lpctstr GetName() const;
    lpctstr ResourceGetName( const CResourceID& rid ) const;
    lpctstr ResourceGetName(const CResourceIDBase& rid, const RES_TYPE iExpectedType);
	CResourceScript * GetResourceFile( size_t i );
    CResourceID ResourceGetID_EatStr( RES_TYPE restype, lpctstr &pszName, word wPage = 0, bool fCanFail = false );    // this moves forward (changes!) the ptcName pointer!
	CResourceID ResourceGetID( RES_TYPE restype, lpctstr ptcName, word wPage = 0, bool fCanFail = false);
	CResourceID ResourceGetIDType( RES_TYPE restype, lpctstr pszName, word wPage = 0 );
	int ResourceGetIndexType( RES_TYPE restype, lpctstr pszName, word wPage = 0 );
	bool ResourceLock( CResourceLock & s, const CResourceID& rid );
	bool ResourceLock( CResourceLock & s, RES_TYPE restype, lpctstr pszName )
	{
		return ResourceLock(s, ResourceGetIDType(restype, pszName));
	}

	CResourceScript * FindResourceFile( lpctstr pszTitle );
	CResourceScript * LoadResourcesAdd( lpctstr pszNewName );

	sl::smart_ptr_view<CResourceDef> ResourceGetDefRef(const CResourceID& rid) const;
	CResourceDef * ResourceGetDef( const CResourceID& rid ) const;
	sl::smart_ptr_view<CResourceDef> ResourceGetDefRefByName(RES_TYPE restype, lpctstr pszName, word wPage = 0);
	CResourceDef* ResourceGetDefByName(RES_TYPE restype, lpctstr pszName, word wPage = 0);
	virtual bool OpenResourceFind( CScript &s, lpctstr pszFilename, bool fCritical = true );
	virtual bool LoadResourceSection( CScript * pScript ) = 0;

public:
	CResourceHolder() = default;
	virtual ~CResourceHolder() = default;

	CResourceHolder(const CResourceHolder& copy) = delete;
	CResourceHolder& operator=(const CResourceHolder& other) = delete;
};

inline lpctstr CResourceHolder::GetResourceBlockName( RES_TYPE restype )	// static
{
	if ( restype < 0 || restype >= RES_QTY )
		restype = RES_UNKNOWN;
	return( sm_szResourceBlocks[restype] );
}

#endif // _INC_CResourceHolder_H
