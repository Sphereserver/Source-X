/**
* @file CCharBase.h
* 
*/

#ifndef _INC_CCHARBASE_H
#define _INC_CCHARBASE_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/resource/CResourceBase.h"
#include "../../common/sphereproto.h"
#include "../../common/CScriptObj.h"
#include "../../common/CTextConsole.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../CBase.h"
#include "../components/CCFaction.h"


class CScript;

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
	ushort m_MaxFood;		// Derived from foodtype...this is the max amount of food we can eat. (based on str ?)

	word   m_defense;		// base defense. (basic to body type) can be modified by armor.
	uint64 m_Anims;			// Bitmask of animations available for monsters. ANIM_TYPE
	HUE_TYPE _wBloodHue;	// when damaged , what color is the blood (-1) = no blood
	HUE_TYPE m_wColor;

	ushort m_Str;		// Base Str for type. (in case of polymorph)
	ushort m_Dex;
	ushort m_Int;

	RESDISPLAY_VERSION _iEraLimitGear;	// Don't allow to create gear newer than the given era (softcoded).
	RESDISPLAY_VERSION _iEraLimitLoot;	// Don't allow to create loot newer than the given era (softcoded).

    ushort _uiRange;

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
    virtual ~CCharBase() = default;

private:
	CCharBase(const CCharBase& copy);
	CCharBase& operator=(const CCharBase& other);

public:
	virtual void UnLink() override;

	CREID_TYPE GetID() const noexcept
	{
		return CREID_TYPE(GetResourceID().GetResIndex());
	}
	CREID_TYPE GetDispID() const noexcept
	{
		return CREID_TYPE(m_dwDispIndex);
	}
	bool SetDispID( CREID_TYPE id );

	uint GetHireDayWage() const noexcept { return m_iHireDayWage; }

    /**
    * @brief   Returns the RangeLow.
    * @return  Value.
    */
    byte GetRangeL() const noexcept;

    /**
    * @brief   Returns the RangeHigh.
    * @return  Value.
    */
    byte GetRangeH() const noexcept;


	static CCharBase * FindCharBase( CREID_TYPE id );
	static bool IsValidDispID( CREID_TYPE id ) noexcept;
	static bool IsPlayableID( CREID_TYPE id, bool fCheckGhost = false) noexcept;
	static bool IsHumanID( CREID_TYPE id, bool fCheckGhost = false ) noexcept;
	static bool IsElfID( CREID_TYPE id, bool fCheckGhost = false) noexcept;
	static bool IsGargoyleID( CREID_TYPE id, bool fCheckGhost = false ) noexcept;

	bool IsFemale() const noexcept
	{
		return (( m_Can & CAN_C_FEMALE ) ? true : false );
	}

	lpctstr GetTradeName() const;

	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Load( CScript & s ) override;
};


#endif // _INC_CCHARBASE_H