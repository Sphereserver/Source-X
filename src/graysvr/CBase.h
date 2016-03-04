//
// CBase.h
//

#ifndef _INC_CBASE_H
#define _INC_CBASE_H
#pragma once

#include "../common/CResourceBase.h"
#include "../common/CString.h"
#include "../common/CVarDefMap.h"
#include "../common/graymul.h"
#include "graysvr.h"

struct CBaseBaseDef : public CResourceLink
{
	// Minimal amount of common info to define RES_ITEMDEF or RES_CHARDEF, (it might just be a DUPE)
	// The unique index id.	(WILL not be the same as artwork if outside artwork range)

	// TAGS
	static LPCTSTR const sm_szLoadKeys[];
	// Base type of both CItemBase and CCharBase
protected:
	DWORD m_dwDispIndex;	// The base artwork id. (may be the same as GetResourceID() in base set.) but can also be "flipped"
	CGString m_sName;		// default type name. (ei, "human" vs specific "Dennis")
private:
	height_t	m_Height;
	// -------------- ResLevel -------------
	BYTE	m_ResLevel;
	HUE_TYPE 	m_ResDispDnHue;
	WORD		m_ResDispDnId;
	// -------------------------------------
	
public:
	CVarDefMap		m_TagDefs;			// TAGs
	CVarDefMap		m_BaseDefs;			// New Variable storage system

	// When events happen to the char. check here for reaction.
	CResourceRefArray	m_TEvents;			// Action or motivation type indexes. (NPC only)

	CResourceQtyArray	m_BaseResources;	// RESOURCES=10 MEAT (What is this made of)
	//WORD				m_range;

	WORD    m_attackBase;	// base attack for weapons/chars. not magic plus
	WORD	m_attackRange;	// variable range of attack damage.

	WORD	m_defenseBase;
	WORD	m_defenseRange;

	DWORD	m_Can;			// Base attribute flags. CAN_C_GHOST

public:
	LPCTSTR GetDefStr( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( pszKey, fZero );
	}

	INT64 GetDefNum( LPCTSTR pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyNum( pszKey, fZero );
	}

	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(LPCTSTR pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

// Map Movement flags.
#define CAN_C_GHOST			0x00001	// Moves thru doors etc.
#define CAN_C_SWIM			0x00002	// dolphin, elemental or is water
#define CAN_C_WALK			0x00004	// Can walk on land, climbed on walked over else Frozen by nature(Corpser) or can just swim
#define CAN_C_PASSWALLS		0x00008	// Walk thru walls.
#define CAN_C_FLY			0x00010	// Mongbat etc.
#define CAN_C_FIRE_IMMUNE	0x00020	// Has some immunity to fire ? (will walk into it (lava))
#define CAN_C_INDOORS		0x00040	// Can go under roof. Not really used except to mask.
#define CAN_C_HOVER			0x00080	// Can hover.

#define CAN_I_DOOR			0x0001	// Is a door UFLAG4_DOOR
#define CAN_I_WATER			0x0002	// Need to swim in it. UFLAG1_WATER
#define CAN_I_PLATFORM		0x0004	// we can walk on top of it. (even tho the item itself might block) UFLAG2_PLATFORM
#define CAN_I_BLOCK			0x0008	// need to walk thru walls or fly over. UFLAG1_BLOCK
#define CAN_I_CLIMB			0x0010	// step up on it, UFLAG2_CLIMBABLE
#define CAN_I_FIRE			0x0020	// Is a fire. Ussually blocks as well. UFLAG1_DAMAGE
#define CAN_I_ROOF			0x0040	// We are under a roof. can't rain on us. UFLAG4_ROOF
#define CAN_I_HOVER			0x0080	// We are hovering. UFLAG4_HOVEROVER

// masks for movement-affecting flags
#define CAN_C_MOVEMASK		(CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_INDOORS|CAN_C_HOVER)
#define CAN_I_MOVEMASK		(CAN_I_DOOR|CAN_I_WATER|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF|CAN_I_HOVER)

	// CItemBase specific defs.
#define CAN_I_PILE			0x0100		// Can item be piled UFLAG2_STACKABLE (*.mul)
#define CAN_I_DYE			0x0200		// Can item be dyed UFLAG3_CLOTH? (sort of)
#define CAN_I_FLIP			0x0400		// will flip by default.
#define CAN_I_LIGHT			0x0800		// UFLAG3_LIGHT
#define CAN_I_REPAIR		0x1000		// Is it repairable (difficulty based on value)
#define CAN_I_REPLICATE		0x2000		// Things like arrows are pretty much all the same.
#define CAN_I_DCIGNORELOS	0x4000		// when dclicked, ignore LOS checks
#define CAN_I_DCIGNOREDIST	0x8000		// when dclicked, ignore distance checks
#define CAN_I_BLOCKLOS		0x10000		// blocks LOS without blocking walkchecks
#define CAN_I_EXCEPTIONAL	0x20000		// can items be exceptional
#define CAN_I_MAKERSMARK	0x40000		// can items hold makers mark
#define CAN_I_RETAINCOLOR	0x80000		// can items retain material colors
#define CAN_I_ENCHANT		0x100000	// can items be enchanted (runic)
#define CAN_I_IMBUE			0x200000	// can items be imbued (imbuing)
#define CAN_I_RECYCLE		0x400000	// Can items be recycled.
#define CAN_I_REFORGE		0x800000	// Can items be Runic Reforged.
#define CAN_I_FORCEDC		0x1000000	// Can force DClick skipping other checks (LOS,Distance, Cont...).

#define CAN_U_ALL			0x000
#define CAN_U_MALE			0x001
#define CAN_U_FEMALE		0x002
#define CAN_U_HUMAN			0x004
#define CAN_U_ELF			0x008
#define CAN_U_GARGOYLE		0x010
#define CAN_U_NONE			0x020

	// CCharBase specific defs.
#define CAN_C_EQUIP			0x00100	// Can equip stuff. (humanoid)
#define CAN_C_USEHANDS		0x00200	// Can wield weapons (INT dependant), open doors ?, pick up/manipulate things
#define CAN_C_MOUNT			0x00400	// can mount rides
#define CAN_C_FEMALE		0x00800	// It is female by nature.
#define CAN_C_NONHUMANOID	0x01000	// Body type for combat messages.
#define CAN_C_RUN			0x02000	// Can run (not needed if they can fly)
#define CAN_C_DCIGNORELOS	0x04000	// when dclicking sth., ignore LOS checks
#define CAN_C_DCIGNOREDIST	0x08000	// when dclicking sth., ignore distance checks
#define CAN_C_NONMOVER		0x10000 // Just stay in place, avoid movement actions.

public:
	CBaseBaseDef( RESOURCE_ID id );
	virtual ~CBaseBaseDef();

private:
	CBaseBaseDef(const CBaseBaseDef& copy);
	CBaseBaseDef& operator=(const CBaseBaseDef& other);

public:
	LPCTSTR GetTypeName() const;
	virtual LPCTSTR GetName() const;
	bool HasTypeName() const;
	virtual void SetTypeName( LPCTSTR pszName );
	bool Can( WORD wCan ) const;
	virtual void UnLink();

	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc = NULL );
	virtual bool r_LoadVal( CScript & s );

	bool IsValid() const;

	BYTE	RangeL() const;

	BYTE	RangeH() const;

	height_t GetHeight() const;
	void SetHeight( height_t Height );
	
	// -------------- ResLevel -------------
	BYTE GetResLevel() const;
	bool SetResLevel( BYTE ResLevel );
	HUE_TYPE GetResDispDnHue() const;
	void SetResDispDnHue( HUE_TYPE ResDispDnHue );
	WORD GetResDispDnId() const;
	void SetResDispDnId( WORD ResDispDnId );
	// -------------------------------------

	void CopyBasic( const CBaseBaseDef * pSrc );
	void CopyTransfer( CBaseBaseDef * pSrc );
};

#endif
