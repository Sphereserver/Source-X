/**
* @file CItem.h
*
*/

#ifndef _INC_CITEM_H
#define _INC_CITEM_H

#include "../../common/CRect.h"
#include "../../common/CResourceBase.h"
#include "../../common/CObjBaseTemplate.h"
#include "../../common/CServerMap.h"
#include "../../common/CUID.h"
#include "../CBase.h"
#include "../CServerConfig.h"
#include "../CObjBase.h"
#include "../CFaction.h"
#include "CItemBase.h"


enum ITC_TYPE	// Item Template commands
{
	ITC_BREAK,
	ITC_BUY,
	ITC_CONTAINER,
	ITC_FULLINTERP,
	ITC_ITEM,
	ITC_ITEMNEWBIE,
	ITC_NEWBIESWAP,
	ITC_SELL,
	ITC_QTY
};

class CItem : public CObjBase
{
	// RES_WORLDITEM
public:
	static const char *m_sClassName;
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szRefKeys[];
	static lpctstr const sm_szTrigName[ITRIG_QTY+1];
	static lpctstr const sm_szTemplateTable[ITC_QTY+1];

private:
	ITEMID_TYPE m_dwDispIndex;		// The current display type. ITEMID_TYPE
	word m_amount;		// Amount of items in pile. 64K max (or corpse type)
	IT_TYPE m_type;		// What does this item do when dclicked ? defines dynamic_cast type
	uchar m_containedGridIndex;	// Which grid have i been placed in ? (when in a container)
	dword	m_CanUse;		// Base attribute flags. can_u_all/male/female..
	word	m_weight;
    CFaction *_pSlayer; // Storing Slayer type.

public:
	byte	m_speed;
	// Attribute flags.
#define ATTR_IDENTIFIED			0x0001				// This is the identified name. ???
#define ATTR_DECAY				0x0002				// Timer currently set to decay.
#define ATTR_NEWBIE				0x0004				// Not lost on death or sellable ?
#define ATTR_MOVE_ALWAYS		0x0008				// Always movable (else Default as stored in client) (even if MUL says not movalble) NEVER DECAYS !
#define ATTR_MOVE_NEVER			0x0010				// Never movable (else Default as stored in client) NEVER DECAYS !
#define ATTR_MAGIC				0x0020				// DON'T SET THIS WHILE WORN! This item is magic as apposed to marked or markable.
#define ATTR_OWNED				0x0040				// This is owned by the town. You need to steal it. NEVER DECAYS !
#define ATTR_INVIS				0x0080				// Gray hidden item (to GM's or owners?)
#define ATTR_CURSED				0x0100
#define ATTR_CURSED2			0x0200				// cursed damned unholy
#define ATTR_BLESSED			0x0400
#define ATTR_BLESSED2			0x0800				// blessed sacred holy
#define ATTR_FORSALE			0x1000				// For sale on a vendor.
#define ATTR_STOLEN				0x2000				// The item is hot. m_uidLink = previous owner.
#define ATTR_CAN_DECAY			0x4000				// This item can decay. but it would seem that it would not (ATTR_MOVE_NEVER etc)
#define ATTR_STATIC				0x8000				// WorldForge merge marker. (used for statics saves)
#define ATTR_EXCEPTIONAL		0x10000				// Is Exceptional
#define ATTR_ENCHANTED			0x20000				// Is Enchanted
#define ATTR_IMBUED				0x40000				// Is Imbued
#define ATTR_QUESTITEM			0x80000				// Is Quest Item
#define ATTR_INSURED			0x100000			// Is Insured
#define ATTR_NODROP				0x200000			// No-drop
#define ATTR_NOTRADE			0x400000			// No-trade
#define ATTR_ARTIFACT			0x800000			// Artifact
#define ATTR_LOCKEDDOWN			0x1000000			// Is Locked Down
#define ATTR_SECURE				0x2000000			// Is Secure
#define ATTR_REFORGED			0x4000000			// Is Runic Reforged.
#define ATTR_OPENED				0x8000000			// Is Door Opened.
#define ATTR_SHARDBOUND			0x10000000
#define ATTR_ACCOUNTBOUND  		0x20000000
#define ATTR_CHARACTERBOUND		0x40000000
#define ATTR_BALANCED			0x80000000
#define ATTR_USEBESTWEAPONSKILL	0x100000000
#define ATTR_MAGEARMOR			0x200000000
#define ATTR_SPELLCHANNELING	0x400000000
#define ATTR_BANE				0x800000000
#define ATTR_BLOODDRINKER		0x1000000000
#define ATTR_BATTLELUST			0x2000000000
#define ATTR_MANAPHASE			0x4000000000
#define ATTR_SEARINGWEAPON		0x8000000000
#define ATTR_REACTIVEPARALYZE	0x10000000000		//
#define ATTR_SPELLFOCUSING		0x20000000000		// Still needs prop on char
#define ATTR_CASTINGFOCUS		0x40000000000		// Still needs prop on char
#define ATTR_RAGEFOCUS			0x80000000000		// Still needs prop on char
#define ATTR_ANTIQUE			0x100000000000
#define ATTR_BRITTLE			0x200000000000		// Can't be powdered. Defaulty 255 durability
#define ATTR_CANNOTREPAIR		0x400000000000		// No repair, no fortify
#define ATTR_MASSIVE			0x800000000000		// Increased str requirement
#define ATTR_PRIZED				0x1000000000000		// Increase Item insurance, cannot be blessed
#define ATTR_UNLUCKY			0x2000000000000		// -100 lucky (cant have lucky property)
#define ATTR_UNWIELDY			0x4000000000000		// Increased weight
#define ATTR_EPHEMERAL			0x8000000000000
#define ATTR_LAVAINFUSED		0x10000000000000	// Lava infused
#define ATTR_BARNACLECOVERED	0x20000000000000	// Barnacle covered
#define ATTR_SHIPWRECKITEM		0x40000000000000	// Recovered from shipwreck
#define ATTR_FACTIONITEM		0x80000000000000	// Faction Item (Has cliloc)
#define ATTR_VVVITEM			0x100000000000000	// VvV Item (Has CliLoc)
#define ATTR_SPLINTERINGWEAPON	0x200000000000000
	uint64	m_Attr;
	// NOTE: If this link is set but not valid -> then delete the whole object !
	CUID m_uidLink;		// Linked to this other object in the world. (owned, key, etc)

	inline bool IsTriggerActive(lpctstr trig) { return static_cast<CObjBase*>(this)->IsTriggerActive(trig); }
	inline void SetTriggerActive(lpctstr trig = NULL) { static_cast<CObjBase*>(this)->SetTriggerActive(trig); }

	// Type specific info. IT_TYPE
	union // 4(more1) + 4(more2) + 6(morep: (2 morex) (2 morey) (1 morez) (1 morem) ) = 14 bytes
	{
		// IT_NORMAL
		struct	// used only to save and restore all this junk.
		{
			dword m_more1;
			dword m_more2;
			CPointBase m_morep;
		} m_itNormal;

		// IT_CONTAINER
		// IT_CONTAINER_LOCKED
		// IT_DOOR
		// IT_DOOR_OPEN
		// IT_DOOR_LOCKED
		// IT_SHIP_SIDE
		// IT_SHIP_SIDE_LOCKED
		// IT_SHIP_HOLD
		// IT_SHIP_HOLD_LOCK
		struct	// IsTypeLockable()
		{
			CUIDBase m_UIDLock;			// more1=the lock code. normally this is the same as the uid (magic lock=non UID)
			dword m_lock_complexity;	// more2=0-1000 = How hard to pick or magic unlock. (conflict with door ?)
		} m_itContainer;

		// IT_SHIP_TILLER
		// IT_KEY
		// IT_SIGN_GUMP
		struct
		{
			CUIDBase m_UIDLock;			// more1 = the lock code. Normally this is the UID, except if uidLink is set.
		} m_itKey;

		// IT_EQ_BANK_BOX
		struct
		{
			dword m_Check_Amount;		// more1=Current amount of gold in account..
			dword m_Check_Restock;		// more2= amount to restock the bank account to
			CPointBase m_pntOpen;		// morep=point we are standing on when opened bank box.
		} m_itEqBankBox;

		// IT_EQ_VENDOR_BOX
		struct
		{
			dword m_junk1;
			dword m_junk2;
			CPointBase m_pntOpen;		// morep=point we are standing on when opened vendor box.
		} m_itEqVendorBox;

		// IT_GAME_BOARD
		struct
		{
			int m_GameType;				// more1=0=chess, 1=checkers, 2=backgammon, 3=no pieces.
		} m_itGameBoard;

		// IT_WAND
		// IT_WEAPON_*
		struct
		{
			word m_Hits_Cur;		// more1l=eqiv to quality of the item (armor/weapon).
			word m_Hits_Max;		// more1h=can only be repaired up to this level.
			int  m_spellcharges;	// more2=for a wand etc.
			word m_spell;			// morex=SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			word m_spelllevel;		// morey=level of the spell. (0-1000)
			byte m_poison_skill;	// morez=0-100 = Is the weapon poisoned ?
			byte m_nonused;			// morem
		} m_itWeapon;

		// IT_ARMOR
		// IT_ARMOR_LEATHER
		// IT_SHIELD:
		// IT_CLOTHING
		// IT_JEWELRY
		struct
		{
			word m_Hits_Cur;		// more1l= eqiv to quality of the item (armor/weapon).
			word m_Hits_Max;		// more1h= can only be repaired up to this level.
			int  m_spellcharges;	// more2 = ? spell charges ? not sure how used here..
			word m_spell;			// morex = SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			word m_spelllevel;		// morey=level of the spell. (0-1000)
		} m_itArmor;

		// IT_SPELL = a magic spell effect. (might be equipped)
		// IT_FIRE
		// IT_SCROLL
		// IT_COMM_CRYSTAL
		// IT_CAMPFIRE
		// IT_LAVA
		struct
		{
			short m_PolyStr;	// more1l=polymorph effect of this. (on strength)
			short m_PolyDex;	// more1h=polymorph effect of this. (on dex)
			int  m_spellcharges;// more2=not sure how used here..
			word m_spell;		// morex=SPELL_TYPE = The magic spell cast on this. (daemons breath)(boots of strength) etc
			word m_spelllevel;	// morey=0-1000=level of the spell.
			byte m_pattern;		// morez = light pattern - CAN_I_LIGHT LIGHT_QTY
		} m_itSpell;

		// IT_SPELLBOOK
		// IT_SPELLBOOK_NECRO
		// IT_SPELLBOOK_PALA
		// IT_SPELLBOOK_EXTRA
		// IT_SPELLBOOK_BUSHIDO
		// IT_SPELLBOOK_NINJITSU
		// IT_SPELLBOOK_ARCANIST
		// IT_SPELLBOOK_MYSTIC
		// IT_SPELLBOOK_MASTERY
		struct
		{
			dword m_spells1;	// more1=Mask of avail spells for spell book.
			dword m_spells2;	// more2=Mask of avail spells for spell book.
		} m_itSpellbook;

		// IT_POTION
		struct
		{
			SPELL_TYPE m_Type;		// more1 = potion effect type
			dword m_skillquality;	// more2 = 0-1000 Strength of the resulting spell.
			word m_tick;			// morex = countdown to explode purple.
			word m_junk4;
			byte m_ignited;
		} m_itPotion;

		// IT_MAP
		struct
		{
			word m_top;			// more1l=in world coords.
			word m_left;		// more1h=
			word m_bottom;		// more2l=
			word m_right;		// more2h=
			word m_junk3;
			word m_junk4;
			byte m_fPinsGlued;	// morez=pins are glued in place. Cannot be moved.
			byte m_map;			// morem=map
		} m_itMap;

		// IT_FRUIT
		// IT_FOOD
		// IT_FOOD_RAW
		// IT_MEAT_RAW
		struct
		{
			CResourceIDBase m_ridCook;	// more1=Cooks into this. (only if raw)
			CREID_TYPE m_MeatType;		// more2= Meat from what type of creature ?
			word m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			word m_spelllevel;			// morey=level of the spell. (0-1000)
			byte m_poison_skill;		// morez=0-100 = Is poisoned ?
			byte m_foodval;				// morem=food value to restore
		} m_itFood;

		// IT_DRINK
		struct
		{
			CResourceIDBase m_ridCook;	// more1=Cooks into this. (only if raw)
			CREID_TYPE m_MeatType;		// more2= Meat from what type of creature ?
			word m_spell;				// morex=SPELL_TYPE = The magic spell cast on this. ( effect of eating.)
			word m_spelllevel;			// morey=level of the spell. (0-1000)
			byte m_poison_skill;		// morez=0-100 = Is poisoned ?
			byte m_foodval;				// morem=food value to restore
		} m_itDrink;

		// IT_CORPSE
		struct	// might just be a sleeping person as well
		{
			dword			m_carved;		// more1 = Corpse is already carved? (0=not carved, 1=carved)
			CUIDBase		m_uidKiller;	// more2 = Who killed this corpse, carved or looted it last. sleep=self.
			CREID_TYPE		m_BaseID;		// morex,morey = The true type of the creature who's corpse this is.
			DIR_TYPE		m_facing_dir;	// morez = Corpse dir. 0x80 = on face.
											// m_amount = the body type.
											// m_uidLink = the creatures ghost.
		} m_itCorpse;

		// IT_LIGHT_LIT
		// IT_LIGHT_OUT
		// IT_WINDOW
		struct
		{
			// CAN_I_LIGHT may be set for others as well..ie.Moon gate conflict
			dword   m_junk1;
			dword	m_junk2;
			word	m_burned;	// morex = out of charges? (1=yes / 0=no)
			word	m_charges;	// morey = how long will the torch last ?
			byte	m_pattern;	// morez = light rotation pattern (LIGHT_PATTERN)
		} m_itLight;

		// IT_EQ_TRADE_WINDOW
		struct
		{
			dword	m_iGold;		// morex|morey
			dword	m_iPlatinum;	// morez		// a platinum coin is 1 million gold, we'll ever want to trade 256 millions+ currency?
			byte	m_bCheck;		// morem = Check box for trade window.
            // We can make m_iPlatinum to be a dword (the packet uses a dword for this amount),
            //  but doing so we'll increase the union size for every item... not worth it.
            // If we'll need a bigger amount for m_iPlatinum, it would be better to use a tag for gold and platinum, instead of using more bytes here.
		} m_itEqTradeWindow;

		// IT_SPAWN_ITEM
		struct
		{
			CResourceIDBase m_ItemID;	// more1=The ITEMID_* or template for items
			dword	m_pile;				// more2=The max # of items to spawn per interval.  If this is 0, spawn up to the total amount.
			word	m_TimeLoMin;		// morex=Lo time in minutes.
			word	m_TimeHiMin;		// morey=Hi time in minutes.
			byte	m_DistMax;			// morez=How far from this will it spawn?
		} m_itSpawnItem;
		// Remember that you can access the same bytes from both m_itSpawnChar and m_itSpawnItem, it doesn't matter if it's IT_SPAWN_ITEM or IT_SPAWN_CHAR.

		// IT_SPAWN_CHAR
		struct
		{
			CResourceIDBase m_CharID;	// more1=CREID_*,  or SPAWNTYPE_*,
			dword	m_unused;		// more2=used only by IT_SPAWN_ITEM, keeping it only for mantaining the structure of the union.
			word	m_TimeLoMin;		// morex=Lo time in minutes.
			word	m_TimeHiMin;		// morey=Hi time in minutes.
			byte	m_DistMax;			// morez=How far from this will they wander?
		} m_itSpawnChar;

		// IT_EXPLOSION
		struct
		{
			dword	m_junk1;
			dword	m_junk2;
			word	m_iDamage;		// morex = damage of the explosion
			word	m_wFlags;		// morey = DAMAGE_TYPE = fire,magic,etc
			byte	m_iDist;		// morez = distance range of damage.
		} m_itExplode;	// Make this asyncronous.

						// IT_BOOK
						// IT_MESSAGE
		struct
		{
			CResourceIDBase m_ResID;	// more1 = preconfigured book id from RES_BOOK or Time date stamp for the book/message creation. (if |0x80000000)
										//CServerTime   	 m_Time;	// more2= Time date stamp for the book/message creation. (Now Placed inside TIMESTAMP for int64 support)
		} m_itBook;

		// IT_DEED
		struct
		{
			ITEMID_TYPE m_Type;			// more1 = deed for what multi, item or template ?
			dword		m_dwKeyCode;	// more2 = previous key code. (dry docked ship)
		} m_itDeed;

		// IT_CROPS
		// IT_FOLIAGE - the leaves of a tree normally.
		struct
		{
			int m_Respawn_Sec;					// more1 = plant respawn time in seconds. (for faster growth plants)
			CResourceIDBase m_ridFruitOverride;	// more2 = Override for TDATA2 = What is the fruit of this plant
		} m_itCrop;

		// IT_TREE
		// ? IT_ROCK
		// ? IT_WATER
		// ? IT_GRASS
		struct	// Natural resources. tend to be statics.
		{
			CResourceIDBase m_ridRes;	// more1 = base resource type. RES_REGIONRESOURCE
		} m_itResource;

		// IT_FIGURINE
		// IT_EQ_HORSE
		struct
		{
			CREID_TYPE m_ID;	// more1 = What sort of creature will this turn into.
			CUIDBase m_UID;		// more2 = If stored by the stables. (offline creature)
		} m_itFigurine;

		// IT_RUNE
		struct
		{
			int m_Strength;			// more1 = How many uses til a rune will wear out ?
			dword m_junk2;
			CPointBase m_pntMark;	// morep = rune marked to a location or a teleport ?
		} m_itRune;

		// IT_TELEPAD
		// IT_MOONGATE
		struct
		{
			int m_fPlayerOnly;		// more1 = The gate is player only. (no npcs, xcept pets)
			int m_fQuiet;			// more2 = The gate/telepad makes no noise.
			CPointBase m_pntMark;	// morep = marked to a location or a teleport ?
		} m_itTelepad;

		// IT_EQ_MEMORY_OBJ
		struct
		{
			// m_amount = memory type mask.
			word m_Action;		// more1l = NPC_MEM_ACT_TYPE What sort of action is this memory about ? (1=training, 2=hire, etc)
			word m_Skill;		// more1h = SKILL_TYPE = training a skill ?
			dword m_junk2;		// more2 = When did the fight start or action take place ? (Now Placed inside TIMESTAMP for int64 support)
			CPointBase m_pt;	// morep = Location the memory occured.
								// m_uidLink = what is this memory linked to. (must be valid)
		} m_itEqMemory;

		// IT_MULTI
		// IT_MULTI_CUSTOM
		// IT_SHIP
		struct
		{
			CUIDBase m_UIDCreator;	// more1 = who created this house or ship ?
			byte m_fSail;			// more2.b1 = ? speed ?
			byte m_fAnchored;
			byte m_DirMove;			// DIR_TYPE
			byte m_DirFace;
			// uidLink = my IT_SHIP_TILLER or IT_SIGN_GUMP,
			CUIDBase m_Pilot;
		} m_itShip;

		// IT_SHIP_PLANK
		struct
		{
			CUIDBase m_UIDLock;			// more1 = the lock code. normally this is the same as the uid (magic lock=non UID)
			dword m_lock_complexity;	// more2=0-1000 = How hard to pick or magic unlock. (conflict with door ?)
			word m_itSideType;			// morex = type to become (IT_SHIP_SIDE or IT_SHIP_SIDE_LOCKED)
		} m_itShipPlank;

		// IT_PORTCULIS
		// IT_PORT_LOCKED
		struct
		{
			int m_z1;			// more1 = The down z height.
			int m_z2;			// more2 = The up z height.
		} m_itPortculis;

		// IT_BEE_HIVE
		struct
		{
			int m_honeycount;		// more1 = How much honey has accumulated here.
		} m_itBeeHive;

		// IT_LOOM
		struct
		{
			CResourceIDBase m_ridCloth;	// more1 = the cloth type currenctly loaded here.
			int m_ClothQty;				// more2 = IS the loom loaded with cloth ?
		} m_itLoom;

		// IT_ARCHERY_BUTTE
		struct
		{
			CResourceIDBase m_ridAmmoType;	// more1 = arrow or bolt currently stuck in it.
			int m_AmmoCount;				// more2 = how many arrows or bolts ?
		} m_itArcheryButte;

		// IT_CANNON_MUZZLE
		struct
		{
			dword m_junk1;
			dword m_Load;			// more2 = Is the cannon loaded ? Mask = 1=powder, 2=shot
		} m_itCannon;

		// IT_EQ_MURDER_COUNT
		struct
		{
			dword m_Decay_Balance;	// more1 = For the murder flag, how much time is left ?
		} m_itEqMurderCount;

		// IT_ITEM_STONE
		struct
		{
			ITEMID_TYPE m_ItemID;	// more1= generate this item or template.
			int m_iPrice;			// more2= ??? gold to purchase / sellback. (vending machine)
			word m_wRegenTime;		// morex=regen time in seconds. 0 = no regen required.
			word m_wAmount;			// morey=Total amount to deliver. 0 = infinite, 0xFFFF=none left
		} m_itItemStone;

		// IT_EQ_STUCK
		struct
		{
			// LINK = what are we stuck to ?
		} m_itEqStuck;

		// IT_WEB
		struct
		{
			dword m_Hits_Cur;	// more1 = how much damage the web can take.
		} m_itWeb;

		// IT_DREAM_GATE
		struct
		{
			int m_index;	// more1 = how much damage the web can take.
		} m_itDreamGate;

		// IT_TRAP
		// IT_TRAP_ACTIVE
		// IT_TRAP_INACTIVE
		struct
		{
			ITEMID_TYPE m_AnimID;	// more1 = What does a trap do when triggered. 0=just use the next id.
			int	m_Damage;			// more2 = Base damage for a trap.
			word m_wAnimSec;		// morex = How long to animate as a dangerous trap.
			word m_wResetSec;		// morey = How long to sit idle til reset.
			byte m_fPeriodic;		// morez = Does the trap just cycle from active to inactive ?
		} m_itTrap;

		// IT_ANIM_ACTIVE
		struct
		{
			// NOTE: This is slightly dangerous to use as it will overwrite more1 and more2
			ITEMID_TYPE m_PrevID;	// more1 = What to turn back into after the animation.
			IT_TYPE m_PrevType;		// more2 = Any item that will animate.	??? Get rid of this !!
		} m_itAnim;

		// IT_SWITCH
		struct
		{
			ITEMID_TYPE m_SwitchID;	// more1 = the next state of this switch.
			dword		m_junk2;
			word		m_fStep;	// morex = can we just step on this to activate ?
			word		m_wDelay;	// morey = delay this how long before activation.
									// uidLink = the item to use when this item is thrown or used.
		} m_itSwitch;

		// IT_SOUND
		struct
		{
			dword	m_Sound;	// more1 = SOUND_TYPE
			int		m_Repeat;	// more2 =
		} m_itSound;

		// IT_STONE_GUILD
		// IT_STONE_TOWN
		struct
		{
			STONEALIGN_TYPE m_iAlign;	// more1=Neutral, chaos, order.
			int m_iAccountGold;			// more2=How much gold has been dropped on me?
										// ATTR_OWNED = auto promote to member.
		} m_itStone;

		// IT_LEATHER
		// IT_FEATHER
		// IT_FUR
		// IT_WOOL
		// IT_BLOOD
		// IT_BONE
		struct
		{
			int m_junk1;
			CREID_TYPE m_creid;	// more2=the source creature id. (CREID_TYPE)
		} m_itSkin;

	};	// IT_QTY

protected:
	CItem( ITEMID_TYPE id, CItemBase * pItemDef );	// only created via CreateBase()
public:
	virtual ~CItem();
private:
	CItem(const CItem& copy);
	CItem& operator=(const CItem& other);

protected:
	bool SetBase( CItemBase * pItemDef );
	virtual int FixWeirdness();

public:
    CFaction *GetSlayer();
	virtual bool OnTick();
	virtual void OnHear( lpctstr pszCmd, CChar * pSrc );
	CItemBase * Item_GetDef() const;
	ITEMID_TYPE GetID() const;
	word GetBaseID() const;
	bool SetBaseID( ITEMID_TYPE id );
	bool SetID( ITEMID_TYPE id );
	ITEMID_TYPE GetDispID() const;
	bool IsSameDispID( ITEMID_TYPE id ) const;	// account for flipped types ?
	bool SetDispID( ITEMID_TYPE id );
	void SetAnim( ITEMID_TYPE id, int iTime );

	int IsWeird() const;
	char GetFixZ(CPointMap pt, dword dwBlockFlags = 0 );
	byte GetSpeed() const;
	void SetAttr(uint64 iAttr)
	{
		m_Attr |= iAttr;
	}
	void ClrAttr(uint64 iAttr)
	{
		m_Attr &= ~iAttr;
	}
	bool IsAttr(uint64 iAttr) const	// ATTR_DECAY
	{
		return((m_Attr & iAttr) ? true : false);
	}
	void SetCanUse(uint64 iCanUse)
	{
		m_CanUse |= iCanUse;
	}
	void ClrCanUse(uint64 iCanUse)
	{
		m_CanUse &= ~iCanUse;
	}
	bool IsCanUse(uint64 iCanUse) const	// CanUse_None
	{
		return ((m_CanUse & iCanUse) ? true : false);
	}

	height_t GetHeight() const;
	int64  GetDecayTime() const;
	void SetDecayTime( int64 iTime = 0 );
	SOUND_TYPE GetDropSound( const CObjBase * pObjOn ) const;
	bool IsTopLevelMultiLocked() const;
	bool IsMovableType() const;
	bool IsMovable() const;
	int GetVisualRange() const;

	bool IsStackableException() const;
	bool IsStackable( const CItem * pItem ) const;
	bool IsStackableType() const
	{
		return Can(CAN_I_PILE);
	}

	virtual bool  IsSameType( const CObjBase * pObj ) const;
	bool Stack( CItem * pItem );
	word ConsumeAmount( word iQty = 1, bool fTest = false );

	CREID_TYPE GetCorpseType() const;
	void  SetCorpseType( CREID_TYPE id );
	virtual void SetAmount( word amount );					// virtual for override in CItemSpawn
	word GetMaxAmount();
	bool SetMaxAmount( word amount );
	void SetAmountUpdate( word amount );
	virtual word GetAmount() const							// virtual for override in CItemSpawn
	{
		return m_amount;
	}

	lpctstr GetName() const;	// allowed to be default name.
	lpctstr GetNameFull( bool fIdentified ) const;

	virtual bool SetName( lpctstr pszName );

	virtual int GetWeight(word amount = 0) const;

	void SetTimeout( int64 iDelay );

	virtual void OnMoveFrom();
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false); // Put item on the ground here.
	bool MoveToUpdate(CPointMap pt, bool bForceFix = false);
	bool MoveToDecay(const CPointMap & pt, int64 iDecayTime, bool bForceFix = false);
	bool MoveToCheck( const CPointMap & pt, CChar * pCharMover = NULL );
	virtual bool MoveNearObj( const CObjBaseTemplate *pItem, word iSteps = 0 );

	CItem* GetNext() const
	{
		return static_cast <CItem*>(CObjBase::GetNext());
	}
	CItem* GetPrev() const
	{
		return static_cast <CItem*>(CObjBase::GetPrev());
	}

	CObjBase * GetContainer() const;
	CObjBaseTemplate * GetTopLevelObj() const;
	uchar GetContainedGridIndex() const;
	void SetContainedGridIndex(uchar index);

	void  Update( const CClient * pClientExclude = NULL );		// send this new item to everyone.
	void  Flip();
	bool  LoadSetContainer( CUID uid, LAYER_TYPE layer );

	void WriteUOX( CScript & s, int index );

	void r_WriteMore1( CSString & sVal );
	void r_WriteMore2( CSString & sVal );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual bool r_Load( CScript & s ); // Load an item from script
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

private:
	TRIGRET_TYPE OnTrigger( lpctstr pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
	TRIGRET_TYPE OnTriggerCreate(CTextConsole * pSrc, CScriptTriggerArgs * pArgs );

public:
	TRIGRET_TYPE OnTrigger( ITRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL );

	// Item type specific stuff.
    inline bool IsType(IT_TYPE type) const {
        return ( m_type == type );
    }
    inline IT_TYPE GetType() const {
        return m_type;
    }
	CItem * SetType( IT_TYPE type );
	bool IsTypeLit() const;
	bool IsTypeBook() const;
	bool IsTypeSpellbook() const;
	bool IsTypeArmor() const;
	bool IsTypeWeapon() const;
	bool IsTypeMulti() const;
	bool IsTypeArmorWeapon() const;
	bool IsTypeLocked() const;
	bool IsTypeLockable() const;
	bool IsTypeSpellable() const;

	bool IsResourceMatch( CResourceIDBase rid, dword dwArg );

	bool IsValidLockLink( CItem * pItemLock ) const;
	bool IsValidLockUID() const;
	bool IsKeyLockFit( dword dwLockUID ) const;

	void ConvertBolttoCloth();

	// Spells
	SKILL_TYPE GetSpellBookSkill();
	SPELL_TYPE GetScrollSpell() const;
	bool IsSpellInBook( SPELL_TYPE spell ) const;
	int GetSpellcountInBook() const;
	int  AddSpellbookScroll( CItem * pItem );
	int AddSpellbookSpell( SPELL_TYPE spell, bool fUpdate );

	//Doors
	bool IsDoorOpen() const;
	bool Use_Door( bool fJustOpen );
	bool Use_DoorNew( bool bJustOpen );
	bool Use_Portculis();
	SOUND_TYPE Use_Music( bool fWell ) const;

	bool SetMagicLock( CChar * pCharSrc, int iSkillLevel );
	void SetSwitchState();
	void SetTrapState( IT_TYPE state, ITEMID_TYPE id, int iTimeSec );
	int Use_Trap();
	bool Use_Light();
	int Use_LockPick( CChar * pCharSrc, bool fTest, bool fFail );
	lpctstr Use_SpyGlass( CChar * pUser ) const;
	lpctstr Use_Sextant( CPointMap pntCoords ) const;

	bool IsBookWritable() const;
	bool IsBookSystem() const;

	void OnExplosion();
	bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false );
	int OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType = DAMAGE_HIT_BLUNT );

	int Armor_GetRepairPercent() const;
	lpctstr Armor_GetRepairDesc() const;
	bool Armor_IsRepairable() const;
	int Armor_GetDefense() const;
	int Weapon_GetAttack(bool bGetRange = true) const;
	SKILL_TYPE Weapon_GetSkill() const;
	SOUND_TYPE Weapon_GetSoundHit() const;
	SOUND_TYPE Weapon_GetSoundMiss() const;
	void Weapon_GetRangedAmmoAnim(ITEMID_TYPE &id, dword &hue, dword &render);
	CResourceIDBase Weapon_GetRangedAmmoRes();
	CItem *Weapon_FindRangedAmmo(CResourceIDBase id);

	bool IsMemoryTypes( word wType ) const;

	bool Ship_Plank( bool fOpen );

	void Plant_SetTimer();
	bool Plant_OnTick();
	void Plant_CropReset();
	bool Plant_Use( CChar * pChar );

	virtual void DupeCopy( const CItem * pItem );
	CItem * UnStackSplit( word amount, CChar * pCharSrc = NULL );

	static CItem * CreateBase( ITEMID_TYPE id );
	static CItem * CreateHeader( tchar * pArg, CObjBase * pCont = NULL, bool fDupeCheck = false, CChar * pSrc = NULL );
	static CItem * CreateScript(ITEMID_TYPE id, CChar * pSrc = NULL);
	CItem * GenerateScript(CChar * pSrc = NULL);
	static CItem * CreateDupeItem( const CItem * pItem, CChar * pSrc = NULL, bool fSetNew = false );
	static CItem * CreateTemplate( ITEMID_TYPE id, CObjBase* pCont = NULL, CChar * pSrc = NULL );

	static CItem * ReadTemplate( CResourceLock & s, CObjBase * pCont );

	int GetAbilityFlags() const;

	virtual void Delete(bool bforce = false);
	virtual bool NotifyDelete();
};

#endif // _INC_CITEM_H
