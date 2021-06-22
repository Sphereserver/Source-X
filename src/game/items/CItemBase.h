/**
* @file CItemBase.h
*
*/

#ifndef _INC_CITEMBASE_H
#define _INC_CITEMBASE_H

#include "../../common/resource/CResourceBase.h"
#include "../uo_files/uofiles_enums_itemid.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../uo_files/CUOItemTypeRec.h"
#include "../CBase.h"
#include "../CServerConfig.h"
#include "../components/CCFaction.h"
#include "item_types.h"


class CItemBase : public CBaseBaseDef
{
	// RES_ITEMDEF
	// Describe basic stuff about all items.
	// Partly based on CUOItemTypeRec/CUOItemTypeRec_HS
private:
	CSTypedArray<ITEMID_TYPE> m_flip_id;	//  can be flipped to make these display ids.
	CValueRangeDef m_values;	// range of values given a quality skill
	IT_TYPE	m_type;				// default double click action type. (if any)
	uint64  m_qwFlags;			//  UFLAG4_DOOR from CUOItemTypeRec/CUOItemTypeRec_HS
	word	m_weight;			// weight in WEIGHT_UNITS (UINT16_MAX=not movable) defaults from the .MUL file.
	byte    m_layer;			// Is this item equippable on paperdoll? LAYER=LAYER_TYPE defaults from the .MUL file.
	byte	m_speed;

public:
	static const char *m_sClassName;

	SKILL_TYPE m_iSkill;
	dword	m_CanUse;		// CanUse flags.
							// Not applicable to all.
	CResourceQtyArray m_SkillMake;	// what skills to create this ? (and non-consumed items)

	union
	{
		// IT_NORMAL
		struct	// used only to script ref all this junk.
		{
			dword m_tData1;	// TDATA1=
			dword m_tData2;	// TDATA2=
			dword m_tData3;	// TDATA3=
			dword m_tData4;	// TDATA4=
		} m_ttNormal;

		// IT_WAND
		// IT_WEAPON_*
		// IT_ARMOR
		// IT_ARMOR_LEATHER
		// IT_SHIELD
		// IT_CLOTHING
		// IT_SPELLBOOK
		// IT_JEWELRY
		// IT_EQ_SCRIPT
		// Container pack is the only exception here. IT_CONTAINER
		struct	// ALL equippable items ex. Weapons and armor
		{
			dword m_junk1;
			dword m_iStrReq;	// TDATA2 = REQSTR = Strength required to weild weapons/armor. Do not overwrite TDATA2 on these items!
		} m_ttEquippable;

		// IT_LIGHT_OUT
		// IT_LIGHT_LIT
		struct
		{
			dword m_junk1;
			dword m_junk2;
            CResourceIDBase m_ridLight;	    // TDATA3=Change light state to on/off
		} m_ttLightSource;

		// IT_WEAPON_BOW
		// IT_WEAPON_XBOW
		// IT_WEAPON_THROWING
		struct	// ALL equippable items ex. Weapons and armor
		{
			dword		    m_junk1;		// TDATA1= Sound it makes ?
			dword		    m_iStrReq;		// TDATA2= REQSTR= Strength required to weild weapons/armor.
            CResourceIDBase m_ridAmmo;		// TDATA3= required source ammo.
            CResourceIDBase m_ridAmmoX;		// TDATA4= fired ammo fx.
		} m_ttWeaponBow;

		// IT_CONTAINER
		// IT_SIGN_GUMP
		// IT_SHIP_HOLD
		// IT_BBOARD
		// IT_CORPSE
		// IT_TRASH_CAN
		// IT_GAME_BOARD
		// IT_EQ_BANK_BOX
		// IT_EQ_VENDOR_BOX
		// IT_KEYRING
		struct
		{
			dword m_junk1;
			GUMP_TYPE m_idGump;	// TDATA2= the gump that comes up when this container is opened.
			dword m_dwMinXY;	// TDATA3= Gump size used.
			dword m_dwMaxXY;	// TDATA4=
		} m_ttContainer;

		// IT_FIGURINE
		// IT_EQ_HORSE
		struct
		{
			dword m_junk1;
			dword m_iStrReq;			// TDATA2= REQSTR= Strength required to mount
            CResourceIDBase m_idChar;	// TDATA3= (CREID_TYPE)
		} m_ttFigurine;

		// IT_SPELLBOOK
		// IT_SPELLBOOK_NECRO
		// IT_SPELLBOOK_PALA
		// IT_SPELLBOOK_BUSHIDO
		// IT_SPELLBOOK_NINJITSU
		// IT_SPELLBOOK_ARCANIST
		// IT_SPELLBOOK_MYSTIC
		// IT_SPELLBOOK_MASTERY
		struct
		{
			dword m_junk1;
			dword m_junk2;
			dword m_iOffset;		// TDATA3= First spell number of this book type
			dword m_iMaxSpells;		// TDATA4= Max spells that this book type can handle
		} m_ttSpellbook;

		// IT_MUSICAL
		struct
		{
			dword m_iSoundGood;	// TDATA1= SOUND_TYPE if played well.
			dword m_iSoundBad;	// TDATA2= sound if played poorly.
		} m_ttMusical;

		// IT_ORE
		struct
		{
			ITEMID_TYPE m_idIngot;	// tdata1= what ingot is this to be made into.
		} m_ttOre;

		// IT_INGOT
		struct
		{
			dword m_iSkillMin;	// tdata1= what is the lowest skill
			dword m_iSkillMax;	// tdata2= what is the highest skill for max yield
		} m_ttIngot;

		// IT_DOOR
		// IT_DOOR_OPEN
		struct
		{
			SOUND_TYPE m_iSoundClose;	// tdata1(low)= sound to close. SOUND_TYPE
			SOUND_TYPE m_iSoundOpen;	// tdata1(high)= sound to open. SOUND_TYPE
            CResourceIDBase m_ridSwitch;// tdata2 ID to change into when open/close
			dword m_iXChange;			// tdata3= X position to change
			dword m_iYChange;			// tdata4= Y position to change
		} m_ttDoor;

		// IT_GAME_PIECE
		struct
		{
			dword m_iStartPosX;	// tdata1=
			dword m_iStartPosY;	// tdata2=
		} m_ttGamePiece;

		// IT_BED
		struct
		{
			dword m_iDir;		// tdata1= direction of the bed. DIR_TYPE
		} m_ttBed;

		// IT_FOLIAGE - is not consumed on reap (unless eaten then will regrow invis)
		// IT_CROPS	- is consumed and will regrow invis.
		struct
		{
            CResourceIDBase m_ridReset;	// tdata1= what will it be reset to regrow from ? 0=nothing
            CResourceIDBase m_ridGrow;	// tdata2= what will it grow further into ? 0=fully mature.
            CResourceIDBase m_ridFruit;	// tdata3= what can it be reaped for ? 0=immature can't be reaped
		} m_ttCrops;

		// IT_SEED
		// IT_FRUIT
		struct
		{
            CResourceIDBase m_ridReset;	// tdata1= what will it be reset to regrow from ? 0=nothing
            CResourceIDBase m_ridSeed;	// tdata2= what does the seed look like ? Copper coin = default.
		} m_ttFruit;

		// IT_DRINK
		// IT_BOOZE
		// IT_POTION
		// IT_PITCHER
		// IT_WATER_WASH
		struct
		{
            CResourceIDBase m_ridEmpty;	// tdata1= the empty container. IT_POTION_EMPTY IT_PITCHER_EMPTY
		} m_ttDrink;

		// IT_SHIP_PLANK
		// IT_SHIP_SIDE
		// IT_SHIP_SIDE_LOCKED
		struct
		{
            CResourceIDBase m_ridState;	// tdata1= next state open/close for the Ship Side
		} m_ttShipPlank;

		// IT_MAP
		struct
		{
			dword m_iGumpWidth;	// tdata1= map gump width
			dword m_iGumpHeight;// tdata2= map gump height
		} m_ttMap;
	};

	static lpctstr const sm_szLoadKeys[];

private:
	static CItemBase * MakeDupeReplacement( CItemBase * pBase, ITEMID_TYPE iddupe );
	int CalculateMakeValue( int iSkillLevel ) const;
protected:
	static void ReplaceItemBase( CItemBase * pOld, CResourceDef * pNew );
public:
	static void GetItemTiledataFlags( dword *pdwCanFlags, ITEMID_TYPE id );
	static height_t GetItemHeightFlags( const CUOItemTypeRec_HS & tile, dword *pdwCanFlags );
	static void GetItemSpecificFlags( const CUOItemTypeRec_HS & tile, dword *pdwCanFlags, IT_TYPE type, ITEMID_TYPE id );
	static bool IsTypeArmor( IT_TYPE type ) noexcept;
	static bool IsTypeWeapon( IT_TYPE type ) noexcept;
	static bool IsTypeSpellbook( IT_TYPE type ) noexcept;
	static bool IsTypeMulti( IT_TYPE type ) noexcept;
	static bool IsTypeEquippable(IT_TYPE type, LAYER_TYPE layer) noexcept;
	bool IsTypeEquippable() const noexcept;
	GUMP_TYPE IsTypeContainer() const noexcept;
	static IT_TYPE GetTypeBase( ITEMID_TYPE id, const CUOItemTypeRec_HS &tile );
	word GetMaxAmount();
	bool SetMaxAmount(word amount);

	static CItemBase * FindItemBase( ITEMID_TYPE id );
	inline static bool IsValidDispID( ITEMID_TYPE id ) noexcept;

	// NOTE: ??? All this stuff should be moved to scripts !
	// Classify item by ID
	static bool IsID_Multi( ITEMID_TYPE id ) noexcept;
    static bool IsID_House( ITEMID_TYPE id ) noexcept;
	static int	IsID_Door( ITEMID_TYPE id ) noexcept;
	static bool IsID_DoorOpen( ITEMID_TYPE id ) noexcept;
    static bool IsID_Ship( ITEMID_TYPE id ) noexcept;
    static bool IsID_GamePiece( ITEMID_TYPE id ) noexcept;
    static bool IsID_Track( ITEMID_TYPE id ) noexcept;
    static bool IsID_WaterFish( ITEMID_TYPE id ) noexcept;
    static bool IsID_WaterWash( ITEMID_TYPE id ) noexcept;
    static bool IsID_Chair( ITEMID_TYPE id ) noexcept;

	inline static bool IsVisibleLayer( LAYER_TYPE layer ) noexcept;

	static tchar * GetNamePluralize( lpctstr pszNameBase, bool fPluralize );
	static bool GetItemData( ITEMID_TYPE id, CUOItemTypeRec_HS * ptile );
	static height_t GetItemHeight( ITEMID_TYPE id, dword *pdwBlockFlags );

	static CREID_TYPE FindCharTrack( ITEMID_TYPE trackID );

	IT_TYPE GetType() const
	{
		return m_type;
	}
	bool IsType( IT_TYPE type ) const
	{
		return ( type == m_type );
	}
    void SetType(IT_TYPE type);

	void SetTypeName( lpctstr pszName );

	LAYER_TYPE GetEquipLayer() const
	{
		// Is this item really equippable ?
		return (LAYER_TYPE)m_layer;
	}

	lpctstr GetName() const;
	lpctstr GetArticleAndSpace() const;

	ITEMID_TYPE GetID() const noexcept
	{
		return (ITEMID_TYPE)(GetResourceID().GetResIndex());
	}
	ITEMID_TYPE GetDispID() const noexcept
	{
		return (ITEMID_TYPE)(m_dwDispIndex);
	}
	uint64 GetTFlags() const noexcept
	{
		return m_qwFlags;
	}
	bool IsSameDispID( ITEMID_TYPE id ) const;
	bool IsDupedItem( ITEMID_TYPE id ) const;
	ITEMID_TYPE GetNextFlipID( ITEMID_TYPE id ) const;

	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;

	bool IsMovableType() const noexcept
	{
		return ( m_weight != UINT16_MAX );
	}
	bool IsStackableType() const noexcept
	{
		return Can( CAN_I_PILE );
	}
#define WEIGHT_UNITS 10
	word GetWeight() const noexcept; // Get weight in tenths of a stone.
	byte GetSpeed() const;

    /**
    * @fn  byte GetRangeL() const;
    * @brief   Returns the RangeLow.
    * @return  Value.
    */
    byte GetRangeL() const noexcept;

    /**
    * @fn  byte GetRangeH() const;
    * @brief   Returns the RangeHigh.
    * @return  Value.
    */
    byte GetRangeH() const noexcept;

	word GetVolume() const noexcept
	{
		return ( m_weight / WEIGHT_UNITS );
	}

	int GetMakeValue( int iSkillLevel );
	void ResetMakeValue();
	void Restock();

	virtual void UnLink() override;

	void CopyBasic( const CItemBase * pBase );
	void CopyTransfer( CItemBase * pBase );

public:
	explicit CItemBase( ITEMID_TYPE id );

	// These don't really get destroyed til the server is shut down but keep this around anyhow.
	virtual ~CItemBase() = default;

private:
	CItemBase(const CItemBase& copy);
	CItemBase& operator=(const CItemBase& other);
};

class CItemBaseDupe : public CResourceDef
{
	// RES_ITEMDEF
	CResourceRef m_MasterItem;	// What is the "master" item ?
	uint64		m_qwFlags;		//  UFLAG4_DOOR from CUOItemTypeRec/CUOItemTypeRec_HS
	height_t	m_Height;

public:
	dword		m_Can;

	static const char *m_sClassName;
	CItemBaseDupe(ITEMID_TYPE id, CItemBase* pMasterItem);
	virtual	~CItemBaseDupe() = default;

private:
	CItemBaseDupe(const CItemBaseDupe& copy);
	CItemBaseDupe& operator=(const CItemBaseDupe& other);

public:
	static CItemBaseDupe* GetDupeRef(ITEMID_TYPE id);

	virtual void UnLink() override;
	CItemBase* GetItemDef() const;	

	inline uint64 GetTFlags() const noexcept
	{
		return( m_qwFlags );
	}
	inline height_t GetHeight() const noexcept
	{
		return( m_Height );
	}
	void SetTFlags( uint64 Flags ) noexcept
	{
		m_qwFlags = Flags;
	}
	void SetHeight( height_t Height) noexcept
	{
		m_Height = Height;
	}
};

class CItemBaseMulti : public CItemBase
{
	// This item is really a multi with other items associated.
	// define the list of objects it is made of.
	// NOTE: It does not have to be a true multi item ITEMID_MULTI
	static lpctstr const sm_szLoadKeys[];
public:
	static const char *m_sClassName;
	struct CMultiComponentItem	// a component item of a multi.
	{
		ITEMID_TYPE m_id;
		short m_dx;
		short m_dy;
		char m_dz;
	};
	struct ShipSpeed // speed of a ship
	{
		ushort period;	// time between movement
		uchar tiles;	// distance to move
	};

	struct MultiOffset
	{
		short m_dx;
		short m_dy;
		char m_dz;
	};

	std::vector<CMultiComponentItem> m_Components;
	ShipSpeed _shipSpeed; // Speed of ships (IT_SHIP)
	ShipMovementSpeed m_SpeedMode;
	CRect m_rect;		// my region.
	MultiOffset m_Offset;
	dword m_dwRegionFlags;	// Base region flags (REGION_FLAG_GUARDED etc)
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
    uint16 _iBaseStorage;
    uint8 _iBaseVendors;
    uint8 _iLockdownsPercent;
    uint8 _iMultiCount;     // Count towards char House's count, 0 = no count, 3 = count like 3 houses.

public:
	explicit CItemBaseMulti( CItemBase* pBase );
	virtual ~CItemBaseMulti() = default;

private:
	CItemBaseMulti(const CItemBaseMulti& copy);
	CItemBaseMulti& operator=(const CItemBaseMulti& other);

public:
	int GetDistanceMax() const;
	int GetDistanceDir(DIR_TYPE dir) const;

	bool AddComponent( ITEMID_TYPE id, short dx, short dy, char dz );
	bool AddComponent( tchar * pArgs );
	void SetMultiRegion( tchar * pArgs );
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pChar = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;

	static CItemBase * MakeMultiRegion( CItemBase * pBase, CScript & s );
};


/* Inline Methods Definitions */

bool CItemBase::IsVisibleLayer( LAYER_TYPE layer ) noexcept // static
{
	return ((layer > LAYER_NONE) && (layer <= LAYER_HORSE) );
}

bool CItemBase::IsValidDispID( ITEMID_TYPE id ) noexcept // static
{
	// Is this id in the base artwork set ? tile or multi.
	return ( id > ITEMID_NOTHING && id < ITEMID_MULTI_MAX );
}


#endif // _INC_CITEMBASE_H
