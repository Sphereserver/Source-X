//
// CObjBase.h
//

#ifndef _INC_COBJBASE_H
#define _INC_COBJBASE_H
#pragma once
#include "../common/CObjBaseTemplate.h"
#include "../common/CScriptObj.h"
#include "CServTime.h"
#include "CContainer.h"
#include "CBase.h"
#include "CResource.h"
//#include "CItemVendable.h"

enum MEMORY_TYPE
{
	// IT_EQ_MEMORY_OBJ
	// Types of memory a CChar has about a game object. (m_wHue)
	MEMORY_NONE			= 0,
	MEMORY_SAWCRIME		= 0x0001,	// I saw them commit a crime or i was attacked criminally. I can call the guards on them. the crime may not have been against me.
	MEMORY_IPET			= 0x0002,	// I am a pet. (this link is my master) (never time out)
	MEMORY_FIGHT		= 0x0004,	// Active fight going on now. may not have done any damage. and they may not know they are fighting me.
	MEMORY_IAGGRESSOR	= 0x0008,	// I was the agressor here. (good or evil)
	MEMORY_HARMEDBY		= 0x0010,	// I was harmed by them. (but they may have been retaliating)
	MEMORY_IRRITATEDBY	= 0x0020,	// I saw them snoop from me or someone.
	MEMORY_SPEAK		= 0x0040,	// We spoke about something at some point. (or was tamed) (NPC_MEM_ACT_TYPE)
	MEMORY_AGGREIVED	= 0x0080,	// I was attacked and was the inocent party here !
	MEMORY_GUARD		= 0x0100,	// Guard this item (never time out)
	MEMORY_ISPAWNED		= 0x0200,	// UNUSED!!!! I am spawned from this item. (never time out)
	MEMORY_GUILD		= 0x0400,	// This is my guild stone. (never time out) only have 1
	MEMORY_TOWN			= 0x0800,	// This is my town stone. (never time out) only have 1
	MEMORY_UNUSED		= 0x1000,	// UNUSED!!!! I am following this Object (never time out)
	MEMORY_WAR_TARG		= 0x2000,	// This is one of my current war targets.
	MEMORY_FRIEND		= 0x4000,	// They can command me but not release me. (not primary blame)
	MEMORY_UNUSED2		= 0x8000	// UNUSED!!!! Gump record memory (More1 = Context, More2 = Uid)
};

enum NPC_MEM_ACT_TYPE	// A simgle primary memory about the object.
{
	// related to MEMORY_SPEAK
	NPC_MEM_ACT_NONE = 0,		// we spoke about something non-specific,
	NPC_MEM_ACT_SPEAK_TRAIN,	// I am speaking about training. Waiting for money
	NPC_MEM_ACT_SPEAK_HIRE,		// I am speaking about being hired. Waiting for money
	NPC_MEM_ACT_FIRSTSPEAK,		// I attempted (or could have) to speak to player. but have had no response.
	NPC_MEM_ACT_TAMED,		// I was tamed by this person previously.
	NPC_MEM_ACT_IGNORE		// I looted or looked at and discarded this item (ignore it)
};

class PacketSend;
class PacketPropertyList;

class CObjBase : public CObjBaseTemplate, public CScriptObj
{
	// All Instances of CItem or CChar have these base attributes.
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szRefKeys[];

private:
	CServTime m_timeout;		// when does this rot away ? or other action. 0 = never, else system time
	CServTime m_timestamp;
	HUE_TYPE m_wHue;			// Hue or skin color. (CItems must be < 0x4ff or so)
	LPCTSTR m_RunningTrigger;

protected:
	CResourceRef m_BaseRef;	// Pointer to the resource that describes this type.
public:
	inline bool CallPersonalTrigger(TCHAR * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult, bool bFull);
	static const char *m_sClassName;
	CVarDefMap m_TagDefs;		// attach extra tags here.
	CVarDefMap m_BaseDefs;		// New Variable storage system
	DWORD	m_Can;
	
	WORD	m_attackBase;	// dam for weapons
	WORD	m_attackRange;	// variable range of attack damage.

	WORD	m_defenseBase;	// Armor for IsArmor items
	WORD	m_defenseRange;	// variable range of defense.
	CGrayUID m_uidSpawnItem;		// SpawnItem for this item

	CResourceRefArray m_OEvents;
	static size_t sm_iCount;	// how many total objects in the world ?
	CVarDefMap * GetTagDefs();
	virtual void DeletePrepare();
	bool IsTriggerActive(LPCTSTR trig) ;
	LPCTSTR GetTriggerActive();
	void SetTriggerActive(LPCTSTR trig = NULL); 

public:
	BYTE	RangeL() const;
	BYTE	RangeH() const;
	CServTime GetTimeStamp() const;
	void SetTimeStamp( INT64 t_time);
	LPCTSTR GetDefStr( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const;
	INT64 GetDefNum( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const;
	void SetDefNum(LPCTSTR pszKey, INT64 iVal, bool fZero = true);
	void SetDefStr(LPCTSTR pszKey, LPCTSTR pszVal, bool fQuoted = false, bool fZero = true);
	void DeleteDef(LPCTSTR pszKey);
	CVarDefCont * GetDefKey( LPCTSTR pszKey, bool fDef ) const;
	LPCTSTR GetKeyStr( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const;
	INT64 GetKeyNum( LPCTSTR pszKey, bool fZero = false, bool fDef = false ) const;
	CVarDefCont * GetKey( LPCTSTR pszKey, bool fDef ) const;
	void SetKeyNum(LPCTSTR pszKey, INT64 iVal);
	void SetKeyStr(LPCTSTR pszKey, LPCTSTR pszVal);
	void DeleteKey(LPCTSTR pszKey);

protected:
	virtual void DupeCopy( const CObjBase * pObj );

public:
	virtual bool OnTick() = 0;
	virtual int FixWeirdness() = 0;
	virtual int GetWeight(WORD amount = 0) const = 0;
	virtual bool IsResourceMatch( RESOURCE_ID_BASE rid, DWORD dwArg ) = 0;

	virtual int IsWeird() const;
	virtual void Delete(bool bforce = false);

	// Accessors

	virtual WORD GetBaseID() const = 0;
	CBaseBaseDef * Base_GetDef() const;

	void SetUID( DWORD dwVal, bool fItem );
	CObjBase* GetNext() const;
	CObjBase* GetPrev() const;
	virtual LPCTSTR GetName() const;	// resolve ambiguity w/CScriptObj
	LPCTSTR GetResourceName() const;

public:
	// Hue
	void SetHue( HUE_TYPE wHue, bool bAvoidTrigger = true, CTextConsole *pSrc = NULL, CObjBase *SourceObj = NULL, long long sound = 0 );
	HUE_TYPE GetHue() const;

protected:
	WORD GetHueAlt() const;
	void SetHueAlt( HUE_TYPE wHue );

public:
	// Timer
	virtual void SetTimeout( INT64 iDelayInTicks );
	bool IsTimerSet() const;
	INT64 GetTimerDiff() const;	// return: < 0 = in the past ( m_timeout - CServTime::GetCurrentTime() )
	bool IsTimerExpired() const;
	INT64 GetTimerAdjusted() const;
	INT64 GetTimerDAdjusted() const;

public:
	// Location
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false) = 0;	// Move to a location at top level.
	virtual bool MoveNear( CPointMap pt, WORD iSteps = 0 );
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, WORD iSteps = 0 );

	void inline SetNamePool_Fail( TCHAR * ppTitles );
	bool SetNamePool( LPCTSTR pszName );

	void Sound( SOUND_TYPE id, int iRepeat = 1 ) const; // Play sound effect from this location.
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = NULL, BYTE bspeedseconds = 5, BYTE bloop = 1, bool fexplode = false, DWORD color = 0, DWORD render = 0, WORD effectid = 0, WORD explodeid = 0, WORD explodesound = 0, DWORD effectuid = 0, BYTE type = 0) const;

	void r_WriteSafe( CScript & s );

	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	// some command on this object as a target

	void Emote(LPCTSTR pText, CClient * pClientExclude = NULL, bool fPossessive = false);
	void Emote2(LPCTSTR pText, LPCTSTR pText2, CClient * pClientExclude = NULL, bool fPossessive = false);

	virtual void Speak( LPCTSTR pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
	virtual void SpeakUTF8( LPCTSTR pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );
	virtual void SpeakUTF8Ex( const NWORD * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );
	
	void RemoveFromView( CClient * pClientExclude = NULL , bool fHardcoded = true );	// remove this item from all clients.
	void ResendOnEquip( bool fAllClients = false );	// Fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again.
	void ResendTooltip( bool bSendFull = false, bool bUseCache = false );	// force reload of tooltip for this object
	void UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = NULL ) const;
	void UpdateObjMessage( LPCTSTR pTextThem, LPCTSTR pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;

	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, LPCTSTR pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

	bool IsContainer() const;

	virtual void Update(const CClient * pClientExclude = NULL)	// send this new item to clients.
		= 0;
	virtual void Flip()
		= 0;
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem )
		= 0;

	virtual TRIGRET_TYPE Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );

public:
	explicit CObjBase( bool fItem );
	virtual ~CObjBase();
private:
	CObjBase(const CObjBase& copy);
	CObjBase& operator=(const CObjBase& other);

public:
	//	Some global object variables
	signed int m_ModAr;

#define SU_UPDATE_HITS			0x01	// update hits to others
#define SU_UPDATE_MODE			0x02	// update mode to all
#define SU_UPDATE_TOOLTIP		0x04	// update tooltip to all
	unsigned char m_fStatusUpdate;	// update flags for next tick
	virtual void OnTickStatusUpdate();

protected:
	PacketPropertyList* m_PropertyList;	// currently cached property list packet
	DWORD m_PropertyHash;				// latest property list hash
	DWORD m_PropertyRevision;			// current property list revision

public:
	PacketPropertyList* GetPropertyList(void) const { return m_PropertyList; }
	void SetPropertyList(PacketPropertyList* propertyList);
	void FreePropertyList(void);
	DWORD UpdatePropertyRevision(DWORD hash);
	void UpdatePropertyFlag(int mask);
};

enum STONEALIGN_TYPE // Types of Guild/Town stones
{
	STONEALIGN_STANDARD = 0,
	STONEALIGN_ORDER,
	STONEALIGN_CHAOS
};

enum ITRIG_TYPE
{
	// XTRIG_UNKNOWN = some named trigger not on this list.
	ITRIG_AfterClick=1,
	ITRIG_Buy,
	ITRIG_Click,
	ITRIG_CLIENTTOOLTIP, // Sending tooltip to client for this item
	ITRIG_ContextMenuRequest,
	ITRIG_ContextMenuSelect,
	ITRIG_Create,		// Item is being created.
	ITRIG_DAMAGE,		// I have been damaged in some way
	ITRIG_DCLICK,		// I have been dclicked.
	ITRIG_DESTROY,		//+I am nearly destroyed
	ITRIG_DROPON_CHAR,		// I have been dropped on this char
	ITRIG_DROPON_GROUND,		// I have been dropped on the ground here
	ITRIG_DROPON_ITEM,		// An item has been 
	ITRIG_DROPON_SELF,		// An item has been dropped upon me
	ITRIG_DROPON_TRADE,		// Droping an item in a trade window
	//ITRIG_DYE,
	ITRIG_EQUIP,		// I have been equipped.
	ITRIG_EQUIPTEST,
	ITRIG_MemoryEquip,
	ITRIG_PICKUP_GROUND,
	ITRIG_PICKUP_PACK,	// picked up from inside some container.
	ITRIG_PICKUP_SELF,	// picked up from this container
	ITRIG_PICKUP_STACK,	// picked up from a stack (ARGO)
	ITRIG_Sell,
	ITRIG_Ship_Turn,
	ITRIG_SPELLEFFECT,		// cast some spell on me.
	ITRIG_STEP,			// I have been walked on. (or shoved)
	ITRIG_TARGON_CANCEL,
	ITRIG_TARGON_CHAR,
	ITRIG_TARGON_GROUND,
	ITRIG_TARGON_ITEM,	// I am being combined with an item
	ITRIG_TIMER,		// My timer has expired.
	ITRIG_ToolTip,
	ITRIG_UNEQUIP,
	ITRIG_QTY
};

//	number of steps to remember for pathfinding
//	default to 24 steps, will have 24*4 extra bytes per char
#define MAX_NPC_PATH_STORAGE_SIZE	UO_MAP_VIEW_SIGHT*2

enum WAR_SWING_TYPE	// m_Act_War_Swing_State
{
	WAR_SWING_INVALID = -1,
	WAR_SWING_EQUIPPING = 0,	// we are recoiling our weapon.
	WAR_SWING_READY,			// we can swing at any time.
	WAR_SWING_SWINGING			// we are swinging our weapon.
};

enum CTRIG_TYPE
{
	CTRIG_AAAUNUSED		= 0,
	CTRIG_AfterClick,
	CTRIG_Attack,			// I am attacking someone (SRC)
	CTRIG_CallGuards,

	CTRIG_charAttack,		// Here starts @charXXX section
	CTRIG_charClick,
	CTRIG_charClientTooltip,
	CTRIG_charContextMenuRequest,
	CTRIG_charContextMenuSelect,
	CTRIG_charDClick,
	CTRIG_charTradeAccepted,

	CTRIG_Click,			// I got clicked on by someone.
	CTRIG_ClientTooltip,	 // Sending tooltips for me to someone
	CTRIG_CombatAdd,		// I add someone to my attacker list
	CTRIG_CombatDelete,		// delete someone from my list
	CTRIG_CombatEnd,		// I finished fighting
	CTRIG_CombatStart,		// I begin fighting
	CTRIG_ContextMenuRequest,
	CTRIG_ContextMenuSelect,
	CTRIG_Create,			// Newly created (not in the world yet)
	CTRIG_Criminal,			// Called before someone becomes 'gray' for someone
	CTRIG_DClick,			// Someone has dclicked on me.
	CTRIG_Death,			//+I just got killed.
	CTRIG_DeathCorpse,		// Corpse
	CTRIG_Destroy,			//+I am nearly destroyed
	CTRIG_Dismount,
	//CTRIG_DYE,
	CTRIG_Eat,
	CTRIG_EffectAdd,
	CTRIG_EnvironChange,	// my environment changed somehow (light,weather,season,region)
	CTRIG_ExpChange,		// EXP is going to change
	CTRIG_ExpLevelChange,	// Experience LEVEL is going to change

	CTRIG_FameChange,		// Fame chaged

	CTRIG_FollowersUpdate,	// Adding or removing CurFollowers.

	CTRIG_GetHit,			// I just got hit.
	CTRIG_Hit,				// I just hit someone. (TARG)
	CTRIG_HitCheck,
	CTRIG_HitIgnore,
	CTRIG_HitMiss,			// I just missed.
	CTRIG_HitTry,			// I am trying to hit someone. starting swing.,
	CTRIG_HouseDesignCommit,	// I committed a new house design
	CTRIG_HouseDesignExit,	// I exited house design mode

	CTRIG_itemAfterClick,
	CTRIG_itemBuy,
	CTRIG_itemClick,		// I clicked on an item
	CTRIG_itemClientTooltip,
	CTRIG_itemContextMenuRequest,
	CTRIG_itemContextMenuSelect,
	CTRIG_itemCreate,		//?
	CTRIG_itemDamage,		//?
	CTRIG_itemDCLICK,		// I have dclicked item
	CTRIG_itemDestroy,		//+Item is nearly destroyed
	CTRIG_itemDROPON_CHAR,		// I have been dropped on this char
	CTRIG_itemDROPON_GROUND,	// I dropped an item on the ground
	CTRIG_itemDROPON_ITEM,		// I have been dropped on this item
	CTRIG_itemDROPON_SELF,		// I have been dropped on this item
	CTRIG_itemDROPON_TRADE,
	CTRIG_itemEQUIP,		// I have equipped an item
	CTRIG_itemEQUIPTEST,
	CTRIG_itemMemoryEquip,
	CTRIG_itemPICKUP_GROUND,
	CTRIG_itemPICKUP_PACK,	// picked up from inside some container.
	CTRIG_itemPICKUP_SELF,	// picked up from this (ACT) container.
	CTRIG_itemPICKUP_STACK,	// was picked up from a stack
	CTRIG_itemSell,
	CTRIG_itemSPELL,		// cast some spell on the item.
	CTRIG_itemSTEP,			// stepped on an item
	CTRIG_itemTARGON_CANCEL,
	CTRIG_itemTARGON_CHAR,
	CTRIG_itemTARGON_GROUND,
	CTRIG_itemTARGON_ITEM,	// I am being combined with an item
	CTRIG_itemTimer,		//?
	CTRIG_itemToolTip,		// Did tool tips on an item
	CTRIG_itemUNEQUIP,		// i have unequipped (or try to unequip) an item

	CTRIG_Jailed,			// I'm up to be send to jail, or to be forgiven

	CTRIG_KarmaChange,			// Karma chaged

	CTRIG_Kill,				//+I have just killed someone
	CTRIG_LogIn,			// Client logs in
	CTRIG_LogOut,			// Client logs out (21)
	CTRIG_Mount,
	CTRIG_MurderDecay,		// I have decayed one of my kills
	CTRIG_MurderMark,		// I am gonna to be marked as a murder
	CTRIG_NotoSend,			// sending notoriety

	CTRIG_NPCAcceptItem,		// (NPC only) i've been given an item i like (according to DESIRES)
	CTRIG_NPCActFight,
	CTRIG_NPCActFollow,		// (NPC only) decided to follow someone
	CTRIG_NPCAction,
	CTRIG_NPCHearGreeting,		// (NPC only) i have been spoken to for the first time. (no memory of previous hearing)
	CTRIG_NPCHearUnknown,		//+(NPC only) I heard something i don't understand.
	CTRIG_NPCLookAtChar,		//
	CTRIG_NPCLookAtItem,		//
	CTRIG_NPCLostTeleport,		//+(NPC only) ready to teleport back to spawn
	CTRIG_NPCRefuseItem,		// (NPC only) i've been given an item i don't want.
	CTRIG_NPCRestock,			// (NPC only) 
	CTRIG_NPCSeeNewPlayer,		//+(NPC only) i see u for the first time. (in 20 minutes) (check memory time)
	CTRIG_NPCSeeWantItem,		// (NPC only) i see something good.
	CTRIG_NPCSpecialAction,
	
	CTRIG_PartyDisband,			//I just disbanded my party
	CTRIG_PartyInvite,			//SRC invited me to join a party, so I may chose
	CTRIG_PartyLeave,
	CTRIG_PartyRemove,			//I have ben removed from the party by SRC

	CTRIG_PersonalSpace,	//+i just got stepped on.
	CTRIG_PetDesert,
	CTRIG_Profile,			// someone hit the profile button for me.
	CTRIG_ReceiveItem,		// I was just handed an item (Not yet checked if i want it)
	CTRIG_RegenStat,		// Hits/mana/stam/food regeneration

	CTRIG_RegionEnter,
	CTRIG_RegionLeave,
	CTRIG_RegionResourceFound,	// I just discovered a resource
	CTRIG_RegionResourceGather,

	CTRIG_Rename,			// Changing my name or pets one

	CTRIG_Resurrect,		// I'm going to resurrect via function or spell.

	CTRIG_SeeCrime,			// I am seeing a crime
	CTRIG_SeeHidden,		// I'm about to see a hidden char
	CTRIG_SeeSnoop,			// I see someone Snooping something.

	// SKTRIG_QTY
	CTRIG_SkillAbort,			// SKTRIG_ABORT
	CTRIG_SkillChange,
	CTRIG_SkillFail,			// SKTRIG_FAIL
	CTRIG_SkillGain,			// SKTRIG_GAIN
	CTRIG_SkillMakeItem,
	CTRIG_SkillMemu,
	CTRIG_SkillPreStart,		// SKTRIG_PRESTART
	CTRIG_SkillSelect,			// SKTRIG_SELECT
	CTRIG_SkillStart,			// SKTRIG_START
	CTRIG_SkillStroke,			// SKTRIG_STROKE
	CTRIG_SkillSuccess,			// SKTRIG_SUCCESS
	CTRIG_SkillTargetCancel,	// SKTRIG_TARGETCANCEL
	CTRIG_SkillUseQuick,		// SKTRIG_USEQUICK
	CTRIG_SkillWait,			// SKTRIG_WAIT

	CTRIG_SpellBook,
	CTRIG_SpellCast,		//+Char is casting a spell.
	CTRIG_SpellEffect,		//+A spell just hit me.
	CTRIG_SpellFail,		// The spell failed
	CTRIG_SpellSelect,		// selected a spell
	CTRIG_SpellSuccess,		// The spell succeeded
	CTRIG_SpellTargetCancel,	//  cancelled spell target
	CTRIG_StatChange,
	CTRIG_StepStealth,		//+Made a step while being in stealth 
	CTRIG_Targon_Cancel,		// Cancel target from TARGETF
	CTRIG_ToggleFlying,
	CTRIG_ToolTip,			// someone did tool tips on me.
	CTRIG_TradeAccepted,	// Everything went well, and we are about to exchange trade items
	CTRIG_TradeClose,		// Fired when a Trade Window is being deleted, no returns
	CTRIG_TradeCreate,		// Trade window is going to be created

	// Packet related triggers
	CTRIG_UserBugReport,
	CTRIG_UserChatButton,
	CTRIG_UserExtCmd,
	CTRIG_UserExWalkLimit,
	CTRIG_UserGuildButton,
	CTRIG_UserKRToolbar,
	CTRIG_UserMailBag,
	CTRIG_UserQuestArrowClick,
	CTRIG_UserQuestButton,
	CTRIG_UserSkills,
	CTRIG_UserSpecialMove,
	CTRIG_UserStats,
	CTRIG_UserVirtue,
	CTRIG_UserVirtueInvoke,
	CTRIG_UserWarmode,

	CTRIG_QTY				// 130
};
#endif
