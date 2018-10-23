/**
* @file CCharBase.h
* 
*/

#ifndef _INC_CCHARBASE_H
#define _INC_CCHARBASE_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/CResourceBase.h"
#include "../../common/sphereproto.h"
#include "../../common/CScript.h"
#include "../../common/CScriptObj.h"
#include "../../common/CTextConsole.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../CBase.h"
#include "../components/CCFaction.h"


class CCharBase : public CBaseBaseDef // define basic info about each "TYPE" of monster/creature.
{
	// RES_CHARDEF
public:
	static const char *m_sClassName;
	ITEMID_TYPE m_trackID;	// ITEMID_TYPE what look like on tracking.
	SOUND_TYPE m_soundBase;	// first sound ID for the creature ( typically 5 sounds per creature, humans and birds have more.)
	SOUND_TYPE m_soundIdle, m_soundNotice;				// Overrides for each action, or simply some creatures doesn't have a soundBase.
	SOUND_TYPE m_soundHit, m_soundGetHit, m_soundDie;	// If != 0 use these, otherwise use soundBase;

	CResourceQtyArray m_FoodType; // FOODTYPE=MEAT 15 (3)
	short m_MaxFood;		// Derived from foodtype...this is the max amount of food we can eat. (based on str ?)

	word  m_defense;		// base defense. (basic to body type) can be modified by armor.
	dword m_Anims;			// Bitmask of animations available for monsters. ANIM_TYPE
	HUE_TYPE m_wBloodHue;	// when damaged , what color is the blood (-1) = no blood
	HUE_TYPE m_wColor;

	short m_Str;		// Base Str for type. (in case of polymorph)
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
    virtual ~CCharBase();

private:
	CCharBase(const CCharBase& copy);
	CCharBase& operator=(const CCharBase& other);

public:
	virtual void UnLink();

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
	bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc = NULL );
	bool r_Load( CScript & s );
};


#endif // _INC_CCHARBASE_H