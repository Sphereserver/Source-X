/**
* @file CCharBase.h
* 
*/

#pragma once
#ifndef _INC_CCHARBASE_H
#define _INC_CCHARBASE_H

#include "../common/sphere_library/CString.h"
#include "../common/CResourceBase.h"
#include "../common/sphereproto.h"
#include "../common/CScript.h"
#include "../common/CScriptObj.h"
#include "../common/CTextConsole.h"
#include "../CBase.h"


class CCharBase : public CBaseBaseDef // define basic info about each "TYPE" of monster/creature.
{
	// RES_CHARDEF
public:
	static const char *m_sClassName;
	ITEMID_TYPE m_trackID;	// ITEMID_TYPE what look like on tracking.
	SOUND_TYPE m_soundbase;	// sounds ( typically 5 sounds per creature, humans and birds have more.)

	CResourceQtyArray m_FoodType; // FOODTYPE=MEAT 15 (3)
	short m_MaxFood;	// Derived from foodtype...this is the max amount of food we can eat. (based on str ?)

	word  m_defense;	// base defense. (basic to body type) can be modified by armor.
	dword m_Anims;	// Bitmask of animations available for monsters. ANIM_TYPE
	HUE_TYPE m_wBloodHue;	// when damaged , what color is the blood (-1) = no blood
	HUE_TYPE m_wColor;

	short m_Str;	// Base Str for type. (in case of polymorph)
	short m_Dex;
	short m_Int;
	short m_iMoveRate;	// move rate percent

						// NPC info ----------------------------------------------------
private:
	uint m_iHireDayWage;		// if applicable. (NPC)
public:
	//SHELTER=FORESTS (P), MOUNTAINS (P)
	//AVERSIONS=TRAPS, CIVILIZATION
	CResourceQtyArray m_Aversions;
	CResourceQtyArray m_Desires;	// DESIRES= that are typical for the char class. see also m_sNeed

									// If this is an NPC.
									// We respond to what we here with this.
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
	static lpctstr const sm_szLoadKeys[];

private:
	void SetFoodType( lpctstr pszFood );
	void CopyBasic( const CCharBase * pCharDef );

public:
	explicit CCharBase( CREID_TYPE id );
	~CCharBase() {}

private:
	CCharBase(const CCharBase& copy);
	CCharBase& operator=(const CCharBase& other);

public:
	virtual void UnLink()
	{
		// We are being reloaded .
		m_Speech.RemoveAll();
		m_FoodType.RemoveAll();
		m_Desires.RemoveAll();
		CBaseBaseDef::UnLink();
	}

	CREID_TYPE GetID() const
	{
		return static_cast<CREID_TYPE>(GetResourceID().GetResIndex());
	}
	CREID_TYPE GetDispID() const
	{
		return static_cast<CREID_TYPE>(m_dwDispIndex);
	}
	bool SetDispID( CREID_TYPE id );

	uint GetHireDayWage() const { return( m_iHireDayWage ); }

	static CCharBase * FindCharBase( CREID_TYPE id );
	static bool IsValidDispID( CREID_TYPE id );
	static bool IsPlayableID( CREID_TYPE id, bool bCheckGhost = false);
	static bool IsHumanID( CREID_TYPE id, bool bCheckGhost = false );
	static bool IsElfID( CREID_TYPE id, bool bCheckGhost = false);
	static bool IsGargoyleID( CREID_TYPE id, bool bCheckGhost = false );

	bool IsFemale() const
	{
		return(( m_Can & CAN_C_FEMALE ) ? true : false );
	}

	lpctstr GetTradeName() const;

	bool r_LoadVal( CScript & s );
	bool r_WriteVal( lpctstr pszKey, CString & sVal, CTextConsole * pSrc = NULL );
	bool r_Load( CScript & s );
};

inline bool CCharBase::IsValidDispID( CREID_TYPE id ) //  static
{
	return( id > 0 && id < CREID_QTY );
}

inline bool CCharBase::IsPlayableID( CREID_TYPE id, bool bCheckGhost)
{
	return ( CCharBase::IsHumanID( id, bCheckGhost) || CCharBase::IsElfID( id, bCheckGhost) || CCharBase::IsGargoyleID( id, bCheckGhost));
}

inline bool CCharBase::IsHumanID( CREID_TYPE id, bool bCheckGhost ) // static
{
	if ( bCheckGhost == true)
		return( id == CREID_MAN || id == CREID_WOMAN || id == CREID_EQUIP_GM_ROBE  || id == CREID_GHOSTMAN || id == CREID_GHOSTWOMAN);
	else
		return( id == CREID_MAN || id == CREID_WOMAN || id == CREID_EQUIP_GM_ROBE);
}

inline bool CCharBase::IsElfID( CREID_TYPE id, bool bCheckGhost ) // static
{
	if ( bCheckGhost == true)
		return( id == CREID_ELFMAN || id == CREID_ELFWOMAN || id == CREID_ELFGHOSTMAN || id == CREID_ELFGHOSTWOMAN);
	else
		return( id == CREID_ELFMAN || id == CREID_ELFWOMAN );
}

inline bool CCharBase::IsGargoyleID( CREID_TYPE id, bool bCheckGhost ) // static
{
	if ( bCheckGhost == true)
		return( id == CREID_GARGMAN || id == CREID_GARGWOMAN || id == CREID_GARGGHOSTMAN || id == CREID_GARGGHOSTWOMAN );
	else
		return( id == CREID_GARGMAN || id == CREID_GARGWOMAN );
}


#endif // _INC_CCHARBASE_H
