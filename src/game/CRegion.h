/**
* @file CRegion.h
* @brief 
*/

#ifndef _INC_CREGION_H
#define _INC_CREGION_H

#include "../common/resource/CResourceBase.h"
#include "../common/resource/CResourceRef.h"
#include "../common/CVarDefMap.h"
#include "CRegionBase.h"

class CItemMulti;

enum RTRIG_TYPE
{
	// XTRIG_UNKNOWN	= some named trigger not on this list.
	RTRIG_CLIPERIODIC=1,		// happens to each client.
	RTRIG_ENTER,
	RTRIG_EXIT,
	RTRIG_REGPERIODIC,	// regional periodic. Happens if just 1 or many clients)
	RTRIG_STEP,
	RTRIG_QTY
};

class CRegion : public CResourceDef, public CRegionBase
{
	// region of the world of arbitrary size and location.
	// made of (possibly multiple) rectangles.
	// RES_ROOM or base for RES_AREA
private:
	CSString	m_sName;	// Name of the region.
	CSString	m_sGroup;

#define REGION_ANTIMAGIC_ALL		0x000001	// All magic banned here.
#define REGION_ANTIMAGIC_RECALL_IN	0x000002	// Teleport,recall in to this, and mark
#define REGION_ANTIMAGIC_RECALL_OUT	0x000004	// can't recall out of here.
#define REGION_ANTIMAGIC_GATE		0x000008
#define REGION_ANTIMAGIC_TELEPORT	0x000010	// Can't teleport into here.
#define REGION_ANTIMAGIC_DAMAGE		0x000020	// just no bad magic here

#define REGION_FLAG_SHIP			0x000040	// This is a ship region. ship commands
#define REGION_FLAG_NOBUILDING		0x000080	// No building in this area

#define REGION_FLAG_ANNOUNCE		0x000200	// Announce to all who enter.
#define REGION_FLAG_INSTA_LOGOUT	0x000400	// Instant Log out is allowed here. (hotel)
#define REGION_FLAG_UNDERGROUND		0x000800	// dungeon type area. (no weather)
#define REGION_FLAG_NODECAY			0x001000	// Things on the ground don't decay here.

#define REGION_FLAG_SAFE			0x002000	// This region is safe from all harm.
#define REGION_FLAG_GUARDED			0x004000	// try TAG.GUARDOWNER
#define REGION_FLAG_NO_PVP			0x008000	// Players cannot directly harm each other here.
#define REGION_FLAG_ARENA			0x010000	// Anything goes. no murder counts or crimes.

#define REGION_FLAG_NOMINING            0x020000// Can't use mining in this region.
#define REGION_FLAG_WALK_NOBLOCKHEIGHT  0x040000// Ignore char height for walkchecks inside this area.

#define REGION_FLAG_INHERIT_PARENT_EVENTS	0x0100000
#define REGION_FLAG_INHERIT_PARENT_FLAGS	0x0200000
#define REGION_FLAG_INHERIT_PARENT_TAGS		0x0400000

	dword m_dwFlags;

public:
	static const char *m_sClassName;
	CPointMap m_pt;			// safe point in the region. (for teleporting to)
	int m_iLinkedSectors;	// just for statistics tracking. How many sectors are linked ?
	int m_iModified;

	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szTrigName[RTRIG_QTY+1];
	static lpctstr const sm_szVerbKeys[];

	CResourceRefArray		m_Events;	// trigger [REGION x] when entered or exited RES_REGIONTYPE
	CVarDefMap				m_TagDefs;		// attach extra tags here.
	CVarDefMap				m_BaseDefs;		// New Variable storage system
    CItemMulti* _pMultiLink;    // Does this region belong to a multi?

	TRIGRET_TYPE OnRegionTrigger( CTextConsole * pChar, RTRIG_TYPE trig );

public:
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( ptcKey, fZero );
	}

	int64 GetDefNum( lpctstr ptcKey ) const
	{
		return m_BaseDefs.GetKeyNum( ptcKey );
	}

	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(ptcKey, iVal, fZero);
	}

	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(ptcKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(lpctstr ptcKey)
	{
		m_BaseDefs.DeleteKey(ptcKey);
	}

private:
	bool SendSectorsVerb( lpctstr pszVerb, lpctstr pszArgs, CTextConsole * pSrc ); // distribute to the CSectors

public:
	virtual bool RealizeRegion();
	void UnRealizeRegion();
#define REGMOD_FLAGS	0x0001
#define REGMOD_EVENTS	0x0002
#define REGMOD_TAGS		0x0004
#define REGMOD_NAME		0x0008
#define REGMOD_GROUP	0x0010

	void SetModified( int iModFlag ) noexcept;
	void SetName( lpctstr pszName );
	virtual lpctstr GetName() const override
	{
		return m_sName.GetBuffer();
	}
	const CSString & GetNameStr() const noexcept
	{
		return m_sName;
	}

	void r_WriteBase( CScript & s );

	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual void r_WriteBody( CScript & s, lpctstr pszPrefix );
	virtual void r_WriteModified( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute command from script
	virtual void r_Write( CScript & s );

	virtual bool AddRegionRect( const CRectMap & rect );
	bool SetRegionRect( const CRectMap & rect )
	{
		EmptyRegion();
		return AddRegionRect( rect );
	}
	inline dword GetRegionFlags() const noexcept
	{
		return m_dwFlags;
	}
	bool IsFlag( dword dwFlags ) const noexcept
	{
		return (( m_dwFlags & dwFlags ) ? true : false );
	}
	bool IsGuarded() const;
	void SetRegionFlags( dword dwFlags ) noexcept
	{
		m_dwFlags |= dwFlags;
	}
	void TogRegionFlags( dword dwFlags, bool fSet ) noexcept
	{
		if ( fSet )
			m_dwFlags |= dwFlags;
		else
			m_dwFlags &= ~dwFlags;
		SetModified( REGMOD_FLAGS );
	}

	bool CheckAntiMagic( SPELL_TYPE spell ) const;
	virtual bool IsValid() const noexcept
	{
		return m_sName.IsValid();
	}

	bool MakeRegionDefname();

public:
	explicit CRegion( CResourceID rid, lpctstr pszName = nullptr );
	virtual ~CRegion();

private:
	CRegion(const CRegion& copy);
	CRegion& operator=(const CRegion& other);
};

class CRandGroupDef;

class CRegionWorld : public CRegion
{
	// A region with extra properties.
	// [AREA] = RES_AREA
public:
	static const char *m_sClassName;
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

public:
	const CRandGroupDef * FindNaturalResource( int /* IT_TYPE */ type ) const;

public:
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false) override;
	virtual void r_WriteBody( CScript &s, lpctstr pszPrefix ) override;
	virtual void r_WriteModified( CScript &s ) override;
	virtual void r_Write( CScript & s ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute command from script

public:
	explicit CRegionWorld( CResourceID rid, lpctstr pszName = nullptr );
	virtual ~CRegionWorld();

private:
	CRegionWorld(const CRegionWorld& copy);
	CRegionWorld& operator=(const CRegionWorld& other);
};

class CTeleport : public CPointSort	// The static world teleporters.
{
	// Put a built in trigger here ? can be Array sorted by CPointMap.
public:
	static const char *m_sClassName;
	bool _fNpc;
	CPointMap _ptDst;

public:
	explicit CTeleport( const CPointMap & pt ) : CPointSort(pt)
	{
		ASSERT( pt.IsValidPoint());
		_ptDst = pt;
		_fNpc = false;
	}

	explicit CTeleport( tchar * pszArgs );

	virtual ~CTeleport() = default;

private:
	CTeleport(const CTeleport& copy);
	CTeleport& operator=(const CTeleport& other);

public:
	bool RealizeTeleport();
};

class CStartLoc		// The start locations for creating a new char.
{
public:
	static const char *m_sClassName;
	CSString m_sArea;	// Area/City Name = Britain or Occlo
	CSString m_sName;	// Place name = Castle Britannia or Docks
	CPointMap m_pt;
	int iClilocDescription; //Only for clients 7.00.13 +

public:
	explicit CStartLoc( lpctstr pszArea )
	{
		m_sArea = pszArea;
		iClilocDescription = 1149559;
	}

private:
	CStartLoc(const CStartLoc& copy);
	CStartLoc& operator=(const CStartLoc& other);
};

#endif // _INC_CREGION_H
