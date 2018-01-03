/**
* @file CChar.h
*
*/

#pragma once
#ifndef _INC_CCHAR_H
#define _INC_CCHAR_H

#include "../../common/sphereproto.h"
#include "../clients/CParty.h"
#include "../items/CItemContainer.h"
#include "../items/CItemCorpse.h"
#include "../items/CItemMemory.h"
#include "../items/CItemStone.h"
#include "../CObjBase.h"
#include "CCharBase.h"
#include "CCharPlayer.h"


class CCharNPC;


enum NPCBRAIN_TYPE	// General AI type.
{
	NPCBRAIN_NONE = 0,	// 0 = This should never really happen.
	NPCBRAIN_ANIMAL,	// 1 = can be tamed.
	NPCBRAIN_HUMAN,		// 2 = generic human.
	NPCBRAIN_HEALER,	// 3 = can res.
	NPCBRAIN_GUARD,		// 4 = inside cities
	NPCBRAIN_BANKER,	// 5 = can open your bank box for you
	NPCBRAIN_VENDOR,	// 6 = will sell from vendor boxes.
	NPCBRAIN_STABLE,	// 7 = will store your animals for you.
	NPCBRAIN_MONSTER,	// 8 = not tamable. normally evil.
	NPCBRAIN_BERSERK,	// 9 = attack closest (blades, vortex)
	NPCBRAIN_DRAGON,	// 10 = we can breath fire. may be tamable ? hirable ?
	NPCBRAIN_QTY
};

class CChar : public CObjBase, public CContainer, public CTextConsole
{
	// RES_WORLDCHAR
private:
	// Spell type effects.
#define STATF_INVUL			0x00000001	// Invulnerability
#define STATF_DEAD			0x00000002
#define STATF_Freeze		0x00000004	// Paralyzed. (spell)
#define STATF_Invisible		0x00000008	// Invisible (spell).
#define STATF_Sleeping		0x00000010	// You look like a corpse ?
#define STATF_War			0x00000020	// War mode on ?
#define STATF_Reactive		0x00000040	// have reactive armor on.
#define STATF_Poisoned		0x00000080	// Poison level is in the poison object
#define STATF_NightSight	0x00000100	// All is light to you
#define STATF_Reflection	0x00000200	// Magic reflect on.
#define STATF_Polymorph		0x00000400	// We have polymorphed to another form.
#define STATF_Incognito		0x00000800	// Dont show skill titles
#define STATF_SpiritSpeak	0x00001000	// I can hear ghosts clearly.
#define STATF_Insubstantial	0x00002000	// Ghost has not manifest. or GM hidden
#define STATF_EmoteAction	0x00004000	// The creature will emote its actions to it's owners.
#define STATF_COMM_CRYSTAL	0x00008000	// I have a IT_COMM_CRYSTAL or listening item on me.
#define STATF_HasShield		0x00010000	// Using a shield
#define STATF_ArcherCanMove	0x00020000	// Can move with archery
#define STATF_Stone			0x00040000	// turned to stone.
#define STATF_Hovering		0x00080000	// hovering (flying gargoyle)
#define STATF_Fly			0x00100000	// Flying or running ? (anim)
	//							0x00200000
#define STATF_Hallucinating	0x00400000	// eat 'shrooms or bad food.
#define STATF_Hidden		0x00800000	// Hidden (non-magical)
#define STATF_InDoors		0x01000000	// we are covered from the rain.
#define STATF_Criminal		0x02000000	// The guards will attack me. (someone has called guards)
#define STATF_Conjured		0x04000000	// This creature is conjured and will expire. (leave no corpse or loot)
#define STATF_Pet			0x08000000	// I am a pet/hirling. check for my owner memory.
#define STATF_Spawned		0x10000000	// I am spawned by a spawn item.
#define STATF_SaveParity	0x20000000	// Has this char been saved or not ?
#define STATF_Ridden		0x40000000	// This is the horse. (don't display me) I am being ridden
#define STATF_OnHorse		0x80000000	// Mounted on horseback.

	uint64 m_iStatFlag;		// Flags above

#define SKILL_VARIANCE 100		// Difficulty modifier for determining success. 10.0 %
	ushort m_Skill[SKILL_QTY];	// List of skills ( skill * 10 )

	CClient * m_pClient;	// is the char a logged in m_pPlayer ?

public:
	struct LastAttackers
	{
		int64	elapsed;
		dword	charUID;
		int64	amountDone;
		int64	threat;
		bool	ignore;
	};
	std::vector<LastAttackers> m_lastAttackers;

	struct NotoSaves
	{
		dword		charUID;	// Character viewing me
		NOTO_TYPE	color;		// Color sent on movement packets
		int64		time;		// Update timer
		NOTO_TYPE	value;		// Notoriety type
	};
	std::vector<NotoSaves> m_notoSaves;

	static const char *m_sClassName;
	CCharPlayer * m_pPlayer;	// May even be an off-line player !
	CCharNPC * m_pNPC;			// we can be both a player and an NPC if "controlled" ?
	CPartyDef * m_pParty;		// What party am i in ?
	CRegionWorld * m_pArea;		// What region are we in now. (for guarded message)
	CRegionBase * m_pRoom;		// What room we are in now.

	static lpctstr const sm_szRefKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szTrigName[CTRIG_QTY+1];
	static const LAYER_TYPE sm_VendorLayers[3];

	// Combat stuff. cached data. (not saved)
	CUID m_uidWeapon;			// current Wielded weapon.	(could just get rid of this ?)
	word m_defense;				// calculated armor worn (NOT intrinsic armor)

	height_t m_height;			// Height set in-game or under some trigger (height=) - for both items and chars

	CUID m_UIDLastNewItem;		// Last item created, used to store on this CChar the UID of the last created item via ITEM or ITEMNEWBIe in @Create and @Restock to prevent COLOR, etc properties to be called with no reference when the item was not really created, ie: ITEM=i_dagger,R5
	uint m_exp;					// character experience
	uint m_level;				// character experience level
	byte m_iVisualRange;		// Visual Range
	//DIR_TYPE m_dirClimb;		// we are standing on a CAN_I_CLIMB or UFLAG2_CLIMBABLE, DIR_QTY = not on climbable
	bool m_fClimbUpdated;		// FixClimbHeight() called?
	bool m_fIgnoreNextPetCmd;	// return 1 in speech block for this pet will make it ignore target petcmds while allowing the rest to perform them
	height_t m_zClimbHeight;	// The height at the end of the climbable.

	// Saved stuff.
	DIR_TYPE m_dirFace;			// facing this dir.
	CSString m_sTitle;			// Special title such as "the guard" (replaces the normal skill title)
	CPointMap m_ptHome;			// What is our "home" region. (towns and bounding of NPC's)
	int64 m_virtualGold;		// Virtual gold used by TOL clients

	// Speech
	FONT_TYPE m_fonttype;			// speech font to use (client send this to server, but it's not used)
	HUE_TYPE m_SpeechHue;			// speech hue to use (sent by client)
	HUE_TYPE m_SpeechHueOverride;	// speech hue to use (ignore the one sent by the client and use this)

	// In order to revert to original Hue and body.
	CREID_TYPE m_prev_id;		// Backup of body type for ghosts and poly
	HUE_TYPE m_prev_Hue;		// Backup of skin color. in case of polymorph etc.
	HUE_TYPE m_wBloodHue;		// Replicating CharDef's BloodColor on the char, or overriding it.
	bool IsTriggerActive(lpctstr trig) { return static_cast<CObjBase*>(const_cast<CChar*>(this))->IsTriggerActive(trig); }
	void SetTriggerActive(lpctstr trig = NULL) { static_cast<CObjBase*>(const_cast<CChar*>(this))->SetTriggerActive(trig); }

	// Client's local light (might be useful in the future for NPCs also? keep it here for now)
	byte m_LocalLight;

	// When events happen to the char. check here for reaction scripts.

	// Skills, Stats and health
	struct
	{
		short	m_base;
		short	m_mod;			// signed for modifier
		short	m_val;			// signed for karma
		short	m_max;			// max
		ushort m_regen;	// Tick time since last regen.
	} m_Stat[STAT_QTY];

	CServerTime m_timeLastRegen;	// When did i get my last regen tick ?
	CServerTime m_timeCreate;		// When was i created ?

	CServerTime m_timeLastHitsUpdate;
	int64 m_timeLastCallGuards;

	// Some character action in progress.
	SKILL_TYPE	m_Act_SkillCurrent;	// Currently using a skill. Could be combat skill.
	CUID		m_Act_UID;			// Current action target
	CUID		m_Fight_Targ_UID;	// Current combat target
	CUID		m_Act_Prv_UID;		// Previous target.
	int			m_Act_Difficulty;	// -1 = fail skill. (0-100) for skill advance calc.
	CPointBase  m_Act_p;			// Moving to this location. or location of forge we are working on.
	int			m_StepStealth;		// Max steps allowed to walk invisible while using Stealth skill

									// Args related to specific actions type (m_Act_SkillCurrent)
	union
	{
		// README! To access the various items with ACTARG1/2/3, each struct member must have a size of 4 bytes.

		struct
		{
			dword m_Arg1;	// "ACTARG1"
			dword m_Arg2;	// "ACTARG2"
			dword m_Arg3;	// "ACTARG3"
		} m_atUnk;

		// SKILL_MAGERY
		// SKILL_NECROMANCY
		// SKILL_CHIVALRY
		// SKILL_BUSHIDO
		// SKILL_NINJITSU
		// SKILL_SPELLWEAVING
		struct
		{
			SPELL_TYPE m_Spell;			// ACTARG1 = Currently casting spell.
			CREID_TYPE m_SummonID;		// ACTARG2 = A sub arg of the skill. (summoned type ?)
		} m_atMagery;

		// SKILL_ALCHEMY
		// SKILL_BLACKSMITHING
		// SKILL_BOWCRAFT
		// SKILL_CARPENTRY
		// SKILL_CARTOGRAPHY
		// SKILL_INSCRIPTION
		// SKILL_TAILORING
		// SKILL_TINKERING
		struct
		{
			dword m_Stroke_Count;		// ACTARG1 = For smithing, tinkering, etc. all requiring multi strokes.
			ITEMID_TYPE m_ItemID;		// ACTARG2 = Making this item.
			dword m_Amount;				// ACTARG3 = How many of this item are we making?
		} m_atCreate;

		// SKILL_LUMBERJACKING
		// SKILL_MINING
		// SKILL_FISHING
		struct
		{
			dword m_Stroke_Count;		// ACTARG1 = All requiring multi strokes.
			dword m_ridType;			// ACTARG2 = Type of item we're harvesting
			dword m_bounceItem;			// ACTARG3 = Drop item on backpack (true) or drop it on ground (false)
		} m_atResource;

		// SKILL_TAMING
		// SKILL_MEDITATION
		struct
		{
			dword m_Stroke_Count;		// ACTARG1 = All requiring multi strokes.
		} m_atTaming;

		// SKILL_ARCHERY
		// SKILL_SWORDSMANSHIP
		// SKILL_MACEFIGHTING
		// SKILL_FENCING
		// SKILL_WRESTLING
		// SKILL_THROWING
		struct
		{
			WAR_SWING_TYPE m_War_Swing_State;		// ACTARG1 = We are in the war mode swing.
			uint64 m_timeNextCombatSwing;			// (ACTARG2 << 32) | ACTARG3 = Time to wait before starting another combat swing.
		} m_atFight;

		// SKILL_ENTICEMENT
		// SKILL_DISCORDANCE
		// SKILL_PEACEMAKING
		struct
		{
			dword m_InstrumentUID;		// ACTARG1 = UID of the instrument we are playing.
		} m_atBard;

		// SKILL_PROVOCATION
		struct
		{
			dword m_InstrumentUID;		// ACTARG1 = UID of the instrument we are playing.
			dword m_Unused2;
			dword m_IsAlly;				// ACTARG3 = Is the provoked considered an ally of the target? 0/1
		} m_atProvocation;				//	If so, abort the skill. To allow always, override it to 0 in @Success via scripts.

		// SKILL_TRACKING
		struct
		{
			DIR_TYPE m_PrvDir;			// ACTARG1 = Previous direction of tracking target, used for when to notify player
		} m_atTracking;

		// NPCACT_RIDDEN
		struct
		{
			mutable dword m_FigurineUID;// ACTARG1 = This creature is being ridden by this object link. IT_FIGURINE IT_EQ_HORSE
		} m_atRidden;

		// NPCACT_TALK
		// NPCACT_TALK_FOLLOW
		struct
		{
			dword m_HearUnknown;		// ACTARG1 = Speaking NPC has no idea what u're saying.
			dword m_WaitCount;			// ACTARG2 = How long have i been waiting (xN sec)
										// m_Act_UID = who am i talking to ?
		} m_atTalk;

		// NPCACT_FLEE
		struct
		{
			dword m_iStepsMax;			// ACTARG1 = How long should it take to get there.
			dword m_iStepsCurrent;		// ACTARG2 = How long has it taken ?
										// m_Act_UID = who am i fleeing from ?
		} m_atFlee;
	};

public:
	CChar( CREID_TYPE id );
	virtual ~CChar(); // Delete character
	bool DupeFrom( CChar * pChar, bool fNewbieItems);

private:
	CChar(const CChar& copy);
	CChar& operator=(const CChar& other);

public:
	// Status and attributes ------------------------------------
	int IsWeird() const;
	char GetFixZ( CPointMap pt, uint wBlockFlags = 0);
	virtual void Delete(bool bforce = false);
	virtual bool NotifyDelete();
	bool IsStatFlag( uint64 iStatFlag ) const;
	void StatFlag_Set(uint64 iStatFlag);
	void StatFlag_Clear(uint64 iStatFlag);
	void StatFlag_Mod(uint64 iStatFlagStatFlag, bool fMod );
	bool IsPriv( word flag ) const;
	PLEVEL_TYPE GetPrivLevel() const;

	CCharBase * Char_GetDef() const;
	CRegionWorld * GetRegion() const;
	CRegionBase * GetRoom() const;
	int GetSight() const;
	void SetSight(byte newSight);

	bool Can( word wCan ) const;
	bool Can( int wCan ) const;
	bool IsResourceMatch( CResourceIDBase rid, dword dwArg );
	bool IsResourceMatch( CResourceIDBase rid, dword dwArg, dword dwArgResearch );

	bool IsSpeakAsGhost() const;
	bool CanUnderstandGhost() const;
	bool IsPlayableCharacter() const;
	bool IsHuman() const;
	bool IsElf() const;
	bool IsGargoyle() const;

	int	 GetHealthPercent() const;
	lpctstr GetTradeTitle() const; // Paperdoll title for character p (2)

	// Information about us.
	CREID_TYPE GetID() const;
	word GetBaseID() const;
	CREID_TYPE GetDispID() const;
	void SetID( CREID_TYPE id );

	lpctstr GetName() const;
	lpctstr GetNameWithoutIncognito() const;
	lpctstr GetName( bool fAllowAlt ) const;
	bool SetName( lpctstr pName );

	height_t GetHeightMount( bool fEyeSubstract = false ) const;
	height_t GetHeight() const;

	bool CanSeeAsDead( const CChar * pChar = NULL ) const;
	bool CanSeeInContainer( const CItemContainer * pContItem ) const;
	bool CanSee( const CObjBaseTemplate * pObj ) const;
	inline bool CanSeeLOS_New_Failed( CPointMap * pptBlock, CPointMap &ptNow ) const;
	bool CanSeeLOS_New( const CPointMap & pd, CPointMap * pBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, word wFlags = 0 ) const;
	bool CanSeeLOS( const CPointMap & pd, CPointMap * pBlock = NULL, int iMaxDist = UO_MAP_VIEW_SIGHT, word wFlags = 0 ) const;
	bool CanSeeLOS( const CObjBaseTemplate * pObj, word wFlags = 0  ) const;

#define LOS_NB_LOCAL_TERRAIN	0x00001 // Terrain inside a region I am standing in does not block LOS
#define LOS_NB_LOCAL_STATIC		0x00002 // Static items inside a region I am standing in do not block LOS
#define LOS_NB_LOCAL_DYNAMIC	0x00004 // Dynamic items inside a region I am standing in do not block LOS
#define LOS_NB_LOCAL_MULTI		0x00008 // Multi items inside a region I am standing in do not block LOS
#define LOS_NB_TERRAIN			0x00010 // Terrain does not block LOS at all
#define LOS_NB_STATIC			0x00020 // Static items do not block LOS at all
#define LOS_NB_DYNAMIC			0x00040 // Dynamic items do not block LOS at all
#define LOS_NB_MULTI			0x00080 // Multi items do not block LOS at all
#define LOS_NB_WINDOWS			0x00100 // Windows do not block LOS (e.g. Archery + Magery)
#define LOS_NO_OTHER_REGION		0x00200 // Do not allow LOS path checking to go out of your region
#define LOS_NC_MULTI			0x00400 // Do not allow LOS path checking to go through (no cross) a multi region (except the one you are standing in)
#define LOS_FISHING				0x00800 // Do not allow LOS path checking to go through objects or terrain which do not represent water
#define LOS_NC_WATER			0x01000	// Water does not block LOS at all.

	bool CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const;
	bool CanSeeItem( const CItem * pItem ) const;
	bool CanTouch( const CPointMap & pt ) const;
	bool CanTouch( const CObjBase * pObj ) const;
	IT_TYPE CanTouchStatic( CPointMap & pt, ITEMID_TYPE id, CItem * pItem );
	bool CanMove( CItem * pItem, bool fMsg = true ) const;
	byte GetLightLevel() const;
	bool CanUse( CItem * pItem, bool fMoveOrConsume ) const;
	bool IsMountCapable() const;

	short  Food_CanEat( CObjBase * pObj ) const;
	short  Food_GetLevelPercent() const;
	lpctstr Food_GetLevelMessage( bool fPet, bool fHappy ) const;

public:
	short	Stat_GetAdjusted( STAT_TYPE i ) const;
	void	Stat_SetBase( STAT_TYPE i, short iVal );
	short	Stat_GetBase( STAT_TYPE i ) const;
	void	Stat_AddBase( STAT_TYPE i, short iVal );
	void	Stat_AddMod( STAT_TYPE i, short iVal );
	void	Stat_SetMod( STAT_TYPE i, short iVal );
	short	Stat_GetMod( STAT_TYPE i ) const;
	void	Stat_SetVal( STAT_TYPE i, short iVal );
	short	Stat_GetVal( STAT_TYPE i ) const;
	void	Stat_SetMax( STAT_TYPE i, short iVal );
	short	Stat_GetMax( STAT_TYPE i ) const;
	short	Stat_GetSum() const;
	short	Stat_GetLimit( STAT_TYPE i ) const;
	bool Stat_Decrease( STAT_TYPE stat, SKILL_TYPE skill = (SKILL_TYPE)NULL);
	bool Stats_Regen(int64 iTimeDiff);
	ushort Stats_GetRegenVal(STAT_TYPE iStat, bool bGetTicks);
	SKILLLOCK_TYPE Stat_GetLock(STAT_TYPE stat);
	void Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state);


	// Location and movement ------------------------------------
private:
	bool TeleportToCli( int iType, int iArgs );
	bool TeleportToObj( int iType, tchar * pszArgs );
private:
	CRegionBase * CheckValidMove( CPointBase & ptDest, word * pwBlockFlags, DIR_TYPE dir, height_t * ClimbHeight, bool fPathFinding = false ) const;
	void FixClimbHeight();
	bool MoveToRegion( CRegionWorld * pNewArea, bool fAllowReject);
	bool MoveToRoom( CRegionBase * pNewRoom, bool fAllowReject);
	bool IsVerticalSpace( CPointMap ptDest, bool fForceMount = false );

public:
	CChar* GetNext() const;
	CObjBaseTemplate * GetTopLevelObj() const;

	bool IsSwimming() const;

	bool MoveToRegionReTest( dword dwType );
	bool MoveToChar(CPointMap pt, bool bForceFix = false);
	bool MoveTo(CPointMap pt, bool bForceFix = false);
	virtual void SetTopZ( char z );
	bool MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart = 1, bool bFromShip = false);
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, word iSteps = 0 );
	bool MoveNear( CPointMap pt, word iSteps = 0 );

	CRegionBase * CanMoveWalkTo( CPointBase & pt, bool fCheckChars = true, bool fCheckOnly = false, DIR_TYPE dir = DIR_QTY, bool fPathFinding = false );
	void CheckRevealOnMove();
	TRIGRET_TYPE CheckLocation( bool fStanding = false );

public:
	// Client Player specific stuff. -------------------------
	void ClientAttach( CClient * pClient );
	void ClientDetach();
	bool IsClient() const;
	CClient * GetClient() const;

	bool SetPrivLevel( CTextConsole * pSrc, lpctstr pszFlags );
	bool CanDisturb( const CChar * pChar ) const;
	void SetDisconnected();
	bool SetPlayerAccount( CAccount * pAccount );
	bool SetPlayerAccount( lpctstr pszAccount );
	bool SetNPCBrain( NPCBRAIN_TYPE NPCBrain );
	NPCBRAIN_TYPE GetNPCBrain( bool fDefault = true ) const;  // Return NPCBRAIN_ANIMAL for animals, _HUMAN for NPC human and PCs, >= _MONSTER for monsters
	void ClearNPC();
	void ClearPlayer();

public:
	void ObjMessage( lpctstr pMsg, const CObjBase * pSrc ) const;
	void SysMessage( lpctstr pMsg ) const;

	void UpdateStatsFlag() const;
	void UpdateStatVal( STAT_TYPE type, short iChange = 0, short iLimit = 0 );
	void UpdateHitsFlag();
	void UpdateModeFlag();
	void UpdateManaFlag() const;
	void UpdateStamFlag() const;
	void UpdateRegenTimers( STAT_TYPE iStat, short iVal);
	ANIM_TYPE GenerateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, byte iFrameDelay = 0, byte iAnimLen = 7);
	bool UpdateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, byte iFrameDelay = 0, byte iAnimLen = 7);

	void UpdateMode( CClient * pExcludeClient = NULL, bool fFull= false );
	void UpdateSpeedMode();
	void UpdateVisualRange();
	void UpdateMove( const CPointMap & ptOld, CClient * pClientExclude = NULL, bool bFull = false );
	void UpdateDir( DIR_TYPE dir );
	void UpdateDir( const CPointMap & pt );
	void UpdateDir( const CObjBaseTemplate * pObj );
	void UpdateDrag( CItem * pItem, CObjBase * pCont = NULL, CPointMap * pt = NULL );
	void Update(const CClient * pClientExclude = NULL);

public:
	lpctstr GetPronoun() const;	// he
	lpctstr GetPossessPronoun() const;	// his
	byte GetModeFlag( const CClient *pViewer = NULL ) const;
	byte GetDirFlag(bool fSquelchForwardStep = false) const;
	dword GetMoveBlockFlags(bool bIgnoreGM = false) const;

	int FixWeirdness();
	void CreateNewCharCheck();

private:
	// Contents/Carry stuff. ---------------------------------
	void ContentAdd( CItem * pItem );
protected:
	void OnRemoveObj( CSObjListRec* pObRec );	// Override this = called when removed from list.
public:
	bool CanCarry( const CItem * pItem ) const;
	bool CanEquipStr( CItem * pItem ) const;
	LAYER_TYPE CanEquipLayer( CItem * pItem, LAYER_TYPE layer, CChar * pCharMsg, bool fTest );
	CItem * LayerFind( LAYER_TYPE layer ) const;
	void LayerAdd( CItem * pItem, LAYER_TYPE layer = LAYER_QTY );

	TRIGRET_TYPE OnCharTrigForLayerLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, LAYER_TYPE layer );
	TRIGRET_TYPE OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, word wMemType );

	void OnWeightChange( int iChange );
	int GetWeight(word amount = 0) const;
	int GetWeightLoadPercent( int iWeight ) const;

	CItem * GetSpellbook(SPELL_TYPE iSpell = SPELL_Clumsy) const;
	CItemContainer * GetPack() const;
	CItemContainer * GetBank( LAYER_TYPE layer = LAYER_BANKBOX );
	CItemContainer * GetPackSafe();
	CItem * GetBackpackItem(ITEMID_TYPE item);
	void AddGoldToPack( int iAmount, CItemContainer * pPack=NULL );

	//private:
	virtual TRIGRET_TYPE OnTrigger( lpctstr pTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
	TRIGRET_TYPE OnTrigger( CTRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = NULL );

public:
	// Load/Save----------------------------------

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Load( CScript & s );  // Load a character from Script
	virtual bool r_WriteVal( lpctstr pszKey, CSString & s, CTextConsole * pSrc = NULL );
	virtual void r_Write( CScript & s );

	void r_WriteParity( CScript & s );

private:
	// Noto/Karma stuff. --------------------------------

	/**
	* @brief Update Karma with the given values.
	*
	* Used to increase/decrease Karma values, checks if you can have the resultant values,
	* fire @KarmaChange trigger and show a message as result of the change (if procceed).
	* Can't never be greater than g_Cfg.m_iMaxKarma or lower than g_Cfg.m_iMinKarma or iBottom.
	* @param iKarma Amount of karma to change, can be possitive and negative.
	* @param iBottom is the lower value you can have for this execution.
	* @param bMessage show message to the char or not.
	*/
	void Noto_Karma( int iKarma, int iBottom=INT32_MIN, bool bMessage = false );

	/**
	* @brief Update Fame with the given value.
	*
	* Used to increase/decrease Fame, it fires @FameChange trigger.
	* Can't never exceed g_Cfg.m_iMaxFame and can't never be lower than 0.
	* @param iFameChange is the amount of fame to change over the current one.
	*/
	void Noto_Fame( int iFameChange );

	/**
	* @brief I have a new notoriety Level? check it and show a message if so.
	*
	* @param iPriv The 'new' notoriety level, it will be checked against the current level to see if it changed.
	*/
	void Noto_ChangeNewMsg( int iPrv );

	/**
	* @brief I've become murderer or criminal, let's see a message for it.
	*
	* MSG_NOTO_CHANGE_0-8 contains the strings 'you have gained a bit/a lot/etc of', so the given iDelta is used to check wether of these MSG should be used.
	* @param iDelta Amount of Karma/Fame changed.
	* @param pszType String containing 'Karma' or 'Fame' to pass as argument to the given text.
	*/
	void Noto_ChangeDeltaMsg( int iDelta, lpctstr pszType );

public:
	// Notoriety code

	/**
	* @brief Returns what is this char to the viewer.
	*
	* This allows the noto attack check in the client.
	* Notoriety handler using std::vector, it's saved and readed here but calcs are being made in Noto_CalcFlag().
	* Actually 2 values are stored in this vectored list: Notoriety (the notoriety level) and Color (the color we are showing in the HP bar and in our character for the viewer).
	* Calls @NotoSend trigger with src = pChar, argn1 = notoriety level, argn2 = color to send.
	* @param pChar is the CChar that needs to know what I am (good, evil, criminal, neutral...) to him.
	* @param fIncog if set to true (usually because of Incognito spell), this character will be gray for the viever (pChar).
	* @param fInvul if set to true invulnerable characters will return NOTO_INVUL (yellow bar, etc).
	* @param bGetColor if set to true only the color will be returned and not the notoriety (note that they can differ if set to so in the @NotoSend trigger).
	* @return NOTO_TYPE notoriety level.
	*/
	NOTO_TYPE Noto_GetFlag( const CChar * pChar, bool fIncog = false, bool fInvul = false, bool bGetColor = false ) const;

	/**
	* @brief Notoriety calculations
	*
	* TAG.OVERRIDE.NOTO will override everything and use the value in the tag for everyone, regardless of what I really are for them.
	* If this char is a pet, check if notoriety must be inherited from it's master or do regular checks for it.
	* @param pChar is the CChar that needs to know what I am (good, evil, criminal, neutral...) to him.
	* @param fIncog if set to true (usually because of Incognito spell), this character will be gray for the viever (pChar).
	* @param fInvul if set to true invulnerable characters will return NOTO_INVUL (yellow bar, etc).
	* @return NOTO_TYPE notoriety level.
	*/
	NOTO_TYPE Noto_CalcFlag( const CChar * pChar, bool fIncog = false, bool fInvul = false ) const;

	/**
	* @brief What color should the viewer see from me?
	*
	* Used to retrieve color for character and corpse's names.
	* @param pChar is the CChar that needs to know what I am (good, evil, criminal, neutral...) to him.
	* @param fIncog if set to true (usually because of Incognito spell), this character will be gray for the viever (pChar).
	* @return HUE_TYPE my color.
	*/
	HUE_TYPE Noto_GetHue( const CChar * pChar, bool fIncog = false ) const;

	/**
	* @brief I'm neutral?
	*
	* @return true if I am.
	*/

	bool Noto_IsNeutral() const;
	/**
	* @brief I'm murderer?
	*
	* @return true if I am.
	*/
	bool Noto_IsMurderer() const;

	/**
	* @brief I'm evil?
	*
	* @return true if I am.
	*/
	bool Noto_IsEvil() const;

	/**
	* @brief I'm a criminal?
	*
	* @return true if I am.
	*/
	bool Noto_IsCriminal() const
	{
		// do the guards hate me ?
		if ( IsStatFlag( STATF_Criminal ) )
			return true;
		return Noto_IsEvil();
	}

	/**
	* @brief Notoriety level for this character.
	*
	* checks my position on g_Cfg.m_NotoFameLevels or g_Cfg.m_NotoKarmaLevels.
	* @return notoriety level.
	*/
	int Noto_GetLevel() const;

	/**
	* @brief // Lord, StatfLevel ... used for Noto_GetTitle.
	*
	* @return string with the title.
	*/
	lpctstr Noto_GetFameTitle() const;

	/**
	* @brief Paperdoll title for character.
	*
	* This checks for <tag.name.prefix> <nototitle> <name> <tag.name.suffix> and the sex to send the correct text to the paperdoll.
	* @return string with the title.
	*/
	lpctstr Noto_GetTitle() const;

	/**
	* @brief I killed someone, should I have credits? and penalties?
	*
	* Here fires the @MurderMark trigger, also gives exp if level system is enabled to give exp on killing.
	* @param pKill, the chara I killed (or participated to kill).
	* @param iTotalKillers how many characters participated in this kill.
	*/
	void Noto_Kill(CChar * pKill, int iTotalKillers = 0);

	/**
	* @brief I'm becoming criminal.
	*
	* The @Criminal trigger is fired here.
	* @param pChar: on who I performed criminal actions or saw me commiting a crime and flagged me as criminal.
	* @return true if I really became a criminal.
	*/
	bool Noto_Criminal( CChar * pChar = NULL);

	/**
	* @brief I am a murderer (it seems) (update my murder decay item).
	*
	*/
	void Noto_Murder();

	/**
	* @brief How much notoriety values do I have stored?
	*
	* @return amount of characters stored.
	*/
	int NotoSave();

	/**
	* @brief Adding someone to my notoriety list.
	*
	* @param pChar is retrieving my notoriety, I'm going to store what I have to send him on my list.
	* @param value is the notoriety value I have for him
	* @param color (if specified) is the color override sent in packets.
	*/
	void NotoSave_Add( CChar * pChar, NOTO_TYPE value, NOTO_TYPE color = NOTO_INVALID );

	/**
	* @brief Retrieving the stored notoriety for this list's entry.
	*
	* @param id is the entry we want to recover.
	* @param bGetColor if true will retrieve the Color and not the Noto value.
	* @return Value of Notoriety (or color)
	*/
	NOTO_TYPE NotoSave_GetValue( int id, bool bGetColor = false );

	/**
	* @brief Gets how much time this notoriety was stored.
	*
	* @param id the entry on the list.
	* @return time in seconds.
	*/
	int64 NotoSave_GetTime( int id );

	/**
	* @brief Clearing all notoriety data
	*/
	void NotoSave_Clear();

	/**
	* @brief Clearing notoriety and update myself so everyone checks my noto again.
	*/
	void NotoSave_Update();

	/**
	* @brief Deleting myself and sending data again for given char.
	*
	* @param id, entry of the viewer.
	*/
	void NotoSave_Resend( int id );

	/**
	* @brief Gets the entry list of the given CChar.
	*
	* @param pChar, CChar to retrieve the entry number for.
	* @return the entry number.
	*/
	int NotoSave_GetID( CChar * pChar );

	/**
	* @brief Removing stored data for pChar.
	*
	* @param pChar, the CChar I want to remove from my list.
	* @return true if successfully removed it.
	*/
	bool NotoSave_Delete( CChar * pChar );

	/**
	* @brief Removing expired notorieties.
	*/
	void NotoSave_CheckTimeout();

	/**
	* @brief We are snooping or stealing, is taking this item a crime ?
	*
	* @param pItem the item we are acting to.
	* @param ppCharMark = The character we are offending.
	* @return false = no crime.
	*/
	bool IsTakeCrime( const CItem * pItem, CChar ** ppCharMark = NULL ) const;


	/**
	* @brief We killed a character, starting exp calcs
	*
	* Main function for default Level system.
	* Triggers @ExpChange and @LevelChange if needed
	* @param delta, amount of exp gaining (or losing?)
	* @param ppCharDead from who we gained the experience.
	*/
	void ChangeExperience(int delta = 0, CChar *pCharDead = NULL);
	int GetSkillTotal(int what = 0, bool how = true);

	// skills and actions. -------------------------------------------
	static bool IsSkillBase( SKILL_TYPE skill );
	static bool IsSkillNPC( SKILL_TYPE skill );

	SKILL_TYPE Skill_GetBest( uint iRank = 0 ) const; // Which skill is the highest for character p
	SKILL_TYPE Skill_GetActive() const
	{
		return m_Act_SkillCurrent;
	}
	lpctstr Skill_GetName( bool fUse = false ) const;
	ushort Skill_GetBase( SKILL_TYPE skill ) const;
	int Skill_GetMax( SKILL_TYPE skill, bool ignoreLock = false ) const;
	int Skill_GetSum() const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	ushort Skill_GetAdjusted(SKILL_TYPE skill) const;
	SKILL_TYPE Skill_GetMagicRandom(ushort iMinValue = 0);
	SKILL_TYPE Skill_GetMagicBest();

	/**
	* @brief Checks if the given skill can be used.
	*
	* @param skill: Skill to check
	* @return if it can be used, or not....
	*/
	bool Skill_CanUse( SKILL_TYPE skill );

	void Skill_SetBase( SKILL_TYPE skill, int iValue );
	bool Skill_UseQuick( SKILL_TYPE skill, int64 difficulty, bool bAllowGain = true, bool bUseBellCurve = true );

	bool Skill_CheckSuccess( SKILL_TYPE skill, int difficulty, bool bUseBellCurve = true ) const;
	bool Skill_Wait( SKILL_TYPE skilltry );
	bool Skill_Start( SKILL_TYPE skill, int iDifficultyIncrease = 0 ); // calc skill progress.
	void Skill_Fail( bool fCancel = false );
	int Skill_Stroke( bool fResource);				// Strokes in crafting skills, calling for SkillStroke trig
	ANIM_TYPE Skill_GetAnim( SKILL_TYPE skill);
	SOUND_TYPE Skill_GetSound( SKILL_TYPE skill);
	int Skill_Stage( SKTRIG_TYPE stage );
	TRIGRET_TYPE	Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage);
	TRIGRET_TYPE	Skill_OnTrigger( SKILL_TYPE skill, SKTRIG_TYPE  stage, CScriptTriggerArgs * pArgs); //pArgs.m_iN1 will be rewritten with skill

	TRIGRET_TYPE	Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig);
	TRIGRET_TYPE	Skill_OnCharTrigger( SKILL_TYPE skill, CTRIG_TYPE ctrig, CScriptTriggerArgs * pArgs); //pArgs.m_iN1 will be rewritten with skill

	bool Skill_Mining_Smelt( CItem * pItemOre, CItem * pItemTarg );
	bool Skill_Tracking( CUID uidTarg, DIR_TYPE & dirPrv, int iDistMax = INT16_MAX );
	bool Skill_MakeItem( ITEMID_TYPE id, CUID uidTarg, SKTRIG_TYPE stage, bool fSkillOnly = false, int iReplicationQty = 1 );
	bool Skill_MakeItem_Success();
	bool Skill_Snoop_Check( const CItemContainer * pItem );
	void Skill_Cleanup();	 // may have just cancelled targetting.

							 // test for skill towards making an item
	int SkillResourceTest( const CResourceQtyArray * pResources );

	void Spell_Effect_Remove(CItem * pSpell);
	void Spell_Effect_Add( CItem * pSpell );

private:
	int Skill_Done();	 // complete skill (async)
	void Skill_Decay();
	void Skill_Experience( SKILL_TYPE skill, int difficulty );

	int Skill_NaturalResource_Setup( CItem * pResBit );
	CItem * Skill_NaturalResource_Create( CItem * pResBit, SKILL_TYPE skill );
	void Skill_SetTimeout();
	int64 Skill_GetTimeout();

	int	Skill_Scripted( SKTRIG_TYPE stage );

	int Skill_Inscription( SKTRIG_TYPE stage );
	int Skill_MakeItem( SKTRIG_TYPE stage );
	int Skill_Information( SKTRIG_TYPE stage );
	int Skill_Hiding( SKTRIG_TYPE stage );
	int Skill_Enticement( SKTRIG_TYPE stage );
	int Skill_Snooping( SKTRIG_TYPE stage );
	int Skill_Stealing( SKTRIG_TYPE stage );
	int Skill_Mining( SKTRIG_TYPE stage );
	int Skill_Lumberjack( SKTRIG_TYPE stage );
	int Skill_Taming( SKTRIG_TYPE stage );
	int Skill_Fishing( SKTRIG_TYPE stage );
	int Skill_Cartography(SKTRIG_TYPE stage);
	int Skill_DetectHidden(SKTRIG_TYPE stage);
	int Skill_Herding(SKTRIG_TYPE stage);
	int Skill_Blacksmith(SKTRIG_TYPE stage);
	int Skill_Lockpicking(SKTRIG_TYPE stage);
	int Skill_Peacemaking(SKTRIG_TYPE stage);
	int Skill_Carpentry(SKTRIG_TYPE stage);
	int Skill_Provocation(SKTRIG_TYPE stage);
	int Skill_Poisoning(SKTRIG_TYPE stage);
	int Skill_Cooking(SKTRIG_TYPE stage);
	int Skill_Healing(SKTRIG_TYPE stage);
	int Skill_Meditation(SKTRIG_TYPE stage);
	int Skill_RemoveTrap(SKTRIG_TYPE stage);
	int Skill_Begging( SKTRIG_TYPE stage );
	int Skill_SpiritSpeak( SKTRIG_TYPE stage );
	int Skill_Magery( SKTRIG_TYPE stage );
	int Skill_Tracking( SKTRIG_TYPE stage );
	int Skill_Fighting( SKTRIG_TYPE stage );
	int Skill_Musicianship( SKTRIG_TYPE stage );
	int Skill_Tailoring( SKTRIG_TYPE stage );

	int Skill_Act_Napping(SKTRIG_TYPE stage);
	int Skill_Act_Throwing(SKTRIG_TYPE stage);
	int Skill_Act_Breath(SKTRIG_TYPE stage);
	int Skill_Act_Training( SKTRIG_TYPE stage );

	void Spell_Dispel( int iskilllevel );
	CChar * Spell_Summon( CREID_TYPE id, CPointMap pt );
	bool Spell_Recall(CItem * pRune, bool fGate);
	SPELL_TYPE Spell_GetIndex(SKILL_TYPE skill = SKILL_NONE);	//gets first spell for the magic skill given.
	SPELL_TYPE Spell_GetMax(SKILL_TYPE skill = SKILL_NONE);	//gets first spell for the magic skill given.
	CItem * Spell_Effect_Create( SPELL_TYPE spell, LAYER_TYPE layer, int iSkillLevel, int iDuration, CObjBase * pSrc = NULL, bool bEquip = true );
	bool Spell_Equip_OnTick( CItem * pItem );

	void Spell_Field(CPointMap pt, ITEMID_TYPE idEW, ITEMID_TYPE idNS, uint fieldWidth, uint fieldGauge, int iSkill, CChar * pCharSrc = NULL, ITEMID_TYPE idnewEW = static_cast<ITEMID_TYPE>(NULL), ITEMID_TYPE idnewNS = static_cast<ITEMID_TYPE>(NULL), int iDuration = 0, HUE_TYPE iColor = HUE_DEFAULT);
	void Spell_Area( CPointMap pt, int iDist, int iSkill );
	bool Spell_TargCheck_Face();
	bool Spell_TargCheck();
	bool Spell_Unequip( LAYER_TYPE layer );

	int  Spell_CastStart();
	void Spell_CastFail();

public:
	inline bool Spell_SimpleEffect( CObjBase * pObj, CObjBase * pObjSrc, SPELL_TYPE &spell, int &iSkillLevel );
	bool Spell_CastDone();
	bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false );
	bool Spell_Resurrection(CItemCorpse * pCorpse = NULL, CChar * pCharSrc = NULL, bool bNoFail = false);
	bool Spell_Teleport( CPointMap pt, bool bTakePets = false, bool bCheckAntiMagic = true, bool bDisplayEffect = true, ITEMID_TYPE iEffect = ITEMID_NOTHING, SOUND_TYPE iSound = SOUND_NONE );
	bool Spell_CanCast( SPELL_TYPE &spell, bool fTest, CObjBase * pSrc, bool fFailMsg, bool fCheckAntiMagic = true );
	int	GetSpellDuration( SPELL_TYPE spell, int iSkillLevel, CChar * pCharSrc = NULL );

	// Memories about objects in the world. -------------------
	bool Memory_OnTick( CItemMemory * pMemory );
	bool Memory_UpdateFlags( CItemMemory * pMemory );
	bool Memory_UpdateClearTypes( CItemMemory * pMemory, word MemTypes );
	void Memory_AddTypes( CItemMemory * pMemory, word MemTypes );
	bool Memory_ClearTypes( CItemMemory * pMemory, word MemTypes );
	CItemMemory * Memory_CreateObj( CUID uid, word MemTypes );
	CItemMemory * Memory_CreateObj( const CObjBase * pObj, word MemTypes );

public:
	void Memory_ClearTypes( word MemTypes );
	CItemMemory * Memory_FindObj( CUID uid ) const;
	CItemMemory * Memory_FindObj( const CObjBase * pObj ) const;
	CItemMemory * Memory_AddObjTypes( CUID uid, word MemTypes );
	CItemMemory * Memory_AddObjTypes( const CObjBase * pObj, word MemTypes );
	CItemMemory * Memory_FindTypes( word MemTypes ) const;
	CItemMemory * Memory_FindObjTypes( const CObjBase * pObj, word MemTypes ) const;
	// -------- Public alias for MemoryCreateObj ------------------
	CItemMemory * Memory_AddObj( CUID uid, word MemTypes );
	CItemMemory * Memory_AddObj( const CObjBase * pObj, word MemTypes );
	// ------------------------------------------------------------

public:
	void SoundChar(CRESND_TYPE type);
	void Action_StartSpecial(CREID_TYPE id);

private:
	void OnNoticeCrime( CChar * pCriminal, const CChar * pCharMark );
public:
	bool CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, lpctstr pAction );

private:
	// Armor, weapons and combat ------------------------------------
	int	CalcFightRange( CItem * pWeapon = NULL );

	
	bool Fight_IsActive() const;
public:
	int CalcArmorDefense() const;
	
	void Memory_Fight_Retreat( CChar * pTarg, CItemMemory * pFight );
	void Memory_Fight_Start( const CChar * pTarg );
	bool Memory_Fight_OnTick( CItemMemory * pMemory );

	bool Fight_Attack( const CChar * pCharTarg, bool toldByMaster = false );
	bool Fight_Clear( const CChar * pCharTarg , bool bForced = false );
	void Fight_ClearAll();
	void Fight_HitTry();
	WAR_SWING_TYPE Fight_Hit( CChar * pCharTarg );
	bool Fight_Parry(CItem * &pItemParry);
	WAR_SWING_TYPE Fight_CanHit(CChar * pCharTarg);
	SKILL_TYPE Fight_GetWeaponSkill() const;
	int Fight_CalcDamage( const CItem * pWeapon, bool bNoRandom = false, bool bGetMax = true ) const;
	bool Fight_IsAttackable();

	// Attacker System
	enum ATTACKER_CLEAR_TYPE
	{
		ATTACKER_CLEAR_FORCED		= 0,
		ATTACKER_CLEAR_ELAPSED		= 1,
		ATTACKER_CLEAR_DISTANCE		= 2,
		ATTACKER_CLEAR_REMOVEDCHAR	= 3,
		ATTACKER_CLEAR_SCRIPT		= 4,
		//ATTACKER_CLEAR_DEATH		= 3,
	};

	int		Attacker() {
		return (int)m_lastAttackers.size();
	}
	bool	Attacker_Add(CChar * pChar, int64 threat = 0);
	CChar * Attacker_GetLast();
	bool	Attacker_Delete(size_t attackerIndex, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	bool	Attacker_Delete(std::vector<LastAttackers>::iterator &itAttacker, bool bForced, ATTACKER_CLEAR_TYPE type);
	bool	Attacker_Delete(CChar * pChar, bool bForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	void	Attacker_RemoveChar();
	void	Attacker_Clear();
	void	Attacker_CheckTimeout();
	int64	Attacker_GetDam(size_t attackerIndex);
	void	Attacker_SetDam(CChar * pChar, int64 value);
	void	Attacker_SetDam(size_t attackerIndex, int64 value);
	CChar * Attacker_GetUID(size_t attackerIndex);
	int64	Attacker_GetElapsed(size_t attackerIndex);
	void	Attacker_SetElapsed(CChar * pChar, int64 value);
	void	Attacker_SetElapsed(size_t attackerIndex, int64 value);
	int64	Attacker_GetThreat(size_t attackerIndex);
	void	Attacker_SetThreat(CChar * pChar, int64 value);
	void	Attacker_SetThreat(size_t attackerIndex, int64 value);
	bool	Attacker_GetIgnore(size_t pChar);
	bool	Attacker_GetIgnore(CChar * pChar);
	void	Attacker_SetIgnore(size_t pChar, bool fIgnore);
	void	Attacker_SetIgnore(CChar * pChar, bool fIgnore);
	int64	Attacker_GetHighestThreat();
	int		Attacker_GetID(CChar * pChar);
	int		Attacker_GetID(CUID pChar);

	//
	bool Player_OnVerb( CScript &s, CTextConsole * pSrc );
	void InitPlayer( CClient * pClient, const char * pszCharname, bool bFemale, RACE_TYPE rtRace, short wStr, short wDex, short wInt,
		PROFESSION_TYPE iProf, SKILL_TYPE skSkill1, int iSkillVal1, SKILL_TYPE skSkill2, int iSkillVal2, SKILL_TYPE skSkill3, int iSkillVal3, SKILL_TYPE skSkill4, int iSkillVal4,
		HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, int iStartLoc );
	bool ReadScriptTrig(CCharBase * pCharDef, CTRIG_TYPE trig, bool bVendor = false);
	bool ReadScript(CResourceLock &s, bool bVendor = false);
	void NPC_LoadScript( bool fRestock );
	void NPC_CreateTrigger();

	// Mounting and figurines
	bool Horse_Mount( CChar * pHorse ); // Remove horse char and give player a horse item
	bool Horse_UnMount(); // Remove horse char and give player a horse item

private:
	CItem * Horse_GetMountItem() const;
	CChar * Horse_GetMountChar() const;
public:
	CChar * Use_Figurine( CItem * pItem, bool bCheckFollowerSlots = true );
	CItem * Make_Figurine( CUID uidOwner, ITEMID_TYPE id = ITEMID_NOTHING );
	CItem * NPC_Shrink();
	bool FollowersUpdate( CChar * pChar, short iFollowerSlots = 0, bool bCheckOnly = false );

	int  ItemPickup( CItem * pItem, word amount );
	bool ItemEquip( CItem * pItem, CChar * pCharMsg = NULL, bool fFromDClick = false );
	bool ItemEquipWeapon( bool fForce );
	bool ItemEquipArmor( bool fForce );
	bool ItemBounce( CItem * pItem, bool bDisplayMsg = true );
	bool ItemDrop( CItem * pItem, const CPointMap & pt );

	void Flip();
	bool SetPoison( int iLevel, int iTicks, CChar * pCharSrc );
	bool SetPoisonCure( int iLevel, bool fExtra );
	bool CheckCorpseCrime( const CItemCorpse *pCorpse, bool fLooting, bool fTest );
	CItemCorpse * FindMyCorpse( bool ignoreLOS = false, int iRadius = 2) const;
	CItemCorpse * MakeCorpse( bool fFrontFall );
	bool RaiseCorpse( CItemCorpse * pCorpse );
	bool Death();
	bool Reveal( uint64 iFlags = 0 );
	void Jail( CTextConsole * pSrc, bool fSet, int iCell );
	void EatAnim( lpctstr pszName, short iQty );
	/**
	* @Brief I'm calling guards (Player speech)
	*
	* Looks for nearby criminals to call guards on
	* This is called from players only, since NPCs will CallGuards(OnTarget) directly.
	*/
	void CallGuards();
	/**
	* @Brief I'm calling guards on pCriminal
	*
	* @param pCriminal: character who shall be punished by guards
	*/
	void CallGuards( CChar * pCriminal );

#define DEATH_NOFAMECHANGE 0x01
#define DEATH_NOCORPSE 0x02
#define DEATH_NOLOOTDROP 0x04
#define DEATH_NOCONJUREDEFFECT 0x08
#define DEATH_HASCORPSE 0x010

	virtual void Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
	virtual void SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font= FONT_NORMAL, CLanguageID lang = 0 );
	virtual void SpeakUTF8Ex( const nword * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

	bool OnFreezeCheck();
	void DropAll( CItemContainer * pCorpse = NULL, uint64 dwAttr = 0 );
	void UnEquipAllItems( CItemContainer * pCorpse = NULL, bool bLeaveHands = false );
	void Wake();
	void SleepStart( bool fFrontFall );

	void Guild_Resign( MEMORY_TYPE memtype );
	CItemStone * Guild_Find( MEMORY_TYPE memtype ) const;
	CStoneMember * Guild_FindMember( MEMORY_TYPE memtype ) const;
	lpctstr Guild_Abbrev( MEMORY_TYPE memtype ) const;
	lpctstr Guild_AbbrevBracket( MEMORY_TYPE memtype ) const;

	void Use_EatQty( CItem * pFood, short iQty = 1 );
	bool Use_Eat( CItem * pItem, short iQty = 1 );
	bool Use_MultiLockDown( CItem * pItemTarg );
	void Use_CarveCorpse( CItemCorpse * pCorpse );
	bool Use_Repair( CItem * pItem );
	int  Use_PlayMusic( CItem * pInstrument, int iDifficultyToPlay );
	void Use_Drink(CItem *pItem);
	bool Use_Cannon_Feed( CItem * pCannon, CItem * pFeed );
	bool Use_Item_Web( CItem * pItem );
	void Use_MoonGate( CItem * pItem );
	bool Use_Kindling( CItem * pKindling );
	bool Use_BedRoll( CItem * pItem );
	bool Use_Seed( CItem * pItem, CPointMap * pPoint );
	bool Use_Key( CItem * pKey, CItem * pItemTarg );
	bool Use_KeyChange( CItem * pItemTarg );
	bool Use_Train_PickPocketDip( CItem * pItem, bool fSetup );
	bool Use_Train_Dummy( CItem * pItem, bool fSetup );
	bool Use_Train_ArcheryButte( CItem * pButte, bool fSetup );
	/**
	* Uses an item (triggers item doubleclick)
	* @param pItem item being processed
	* @param fLink true if the method is following a linked item (not targeted directly)
	* return true if the action succeeded
	*/
	bool Use_Item(CItem *pItem, bool fLink = false);
	bool Use_Obj( CObjBase * pObj, bool fTestTouch, bool fScript = false );
private:
	int Do_Use_Item(CItem * pItem, bool fLink = false);

	// NPC AI -----------------------------------------
private:
	static CREID_TYPE NPC_GetAllyGroupType(CREID_TYPE idTest);

	bool NPC_StablePetRetrieve( CChar * pCharPlayer );
	bool NPC_StablePetSelect( CChar * pCharPlayer );

	int NPC_WantThisItem( CItem * pItem ) const;
	int NPC_GetWeaponUseScore( CItem * pItem );

	int  NPC_GetHostilityLevelToward( const CChar * pCharTarg ) const;
	int	 NPC_GetAttackContinueMotivation( CChar * pChar, int iMotivation = 0 ) const;
	int  NPC_GetAttackMotivation( CChar * pChar, int iMotivation = 0 ) const;
	bool NPC_CheckHirelingStatus();
	int  NPC_GetTrainMax( const CChar * pStudent, SKILL_TYPE Skill ) const;

	bool NPC_OnVerb( CScript &s, CTextConsole * pSrc = NULL );
	void NPC_OnHirePayMore( CItem * pGold, int iWage, bool fHire = false );
public:
	bool NPC_OnHirePay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnHireHear( CChar * pCharSrc );
	int	 NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill );
	bool NPC_OnTrainPay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnTrainHear( CChar * pCharSrc, lpctstr pCmd );
	bool NPC_TrainSkill( CChar * pCharSrc, SKILL_TYPE skill, int toTrain );
    int PayGold(CChar * pCharSrc, int iGold, CItem * pGold, ePayGold iReason);
private:
	bool NPC_CheckWalkHere( const CPointBase & pt, const CRegionBase * pArea, word wBlockFlags ) const;
	void NPC_OnNoticeSnoop( CChar * pCharThief, CChar * pCharMark );

	void NPC_LootMemory( CItem * pItem );
	bool NPC_LookAtCharGuard( CChar * pChar, bool bFromTrigger = false );
	bool NPC_LookAtCharHealer( CChar * pChar );
	bool NPC_LookAtCharHuman( CChar * pChar );
	bool NPC_LookAtCharMonster( CChar * pChar );
	bool NPC_LookAtChar( CChar * pChar, int iDist );
	bool NPC_LookAtItem( CItem * pItem, int iDist );
	bool NPC_LookAround( bool fForceCheckItems = false );
	int  NPC_WalkToPoint(bool fRun = false);
	CChar * NPC_FightFindBestTarget();
	bool NPC_FightMagery(CChar * pChar);
	bool NPC_FightCast(CObjBase * &pChar ,CObjBase * pSrc, SPELL_TYPE &spell, SKILL_TYPE skill = SKILL_NONE);
	bool NPC_FightArchery( CChar * pChar );
	bool NPC_FightMayCast(bool fCheckSkill = true) const;
	void NPC_GetAllSpellbookSpells();

	bool NPC_Act_Follow( bool fFlee = false, int maxDistance = 1, bool fMoveAway = false );
	void NPC_Act_Guard();
	void NPC_Act_GoHome();
	bool NPC_Act_Talk();
	void NPC_Act_Wander();
	void NPC_Act_Fight();
	void NPC_Act_Idle();
	void NPC_Act_Looting();
	void NPC_Act_Flee();
	void NPC_Act_Goto(int iDist = 30);
	void NPC_Act_Runto(int iDist = 30);
	bool NPC_Act_Food();

	void NPC_ActStart_SpeakTo( CChar * pSrc );

	void NPC_OnTickAction();

public:
	void NPC_Pathfinding();		//	NPC thread AI - pathfinding
	void NPC_Food();			//	NPC thread AI - search for food
	void NPC_ExtraAI();			//	NPC thread AI - some general extra operations
	void NPC_AddSpellsFromBook(CItem * pBook);

	void NPC_PetDesert();
	void NPC_PetClearOwners();
	bool NPC_PetSetOwner( CChar * pChar );
	CChar * NPC_PetGetOwner() const;
	bool NPC_IsOwnedBy( const CChar * pChar, bool fAllowGM = true ) const;
	bool NPC_CanSpeak() const;

	static CItemVendable * NPC_FindVendableItem( CItemVendable * pVendItem, CItemContainer * pVend1, CItemContainer * pVend2 );

	bool NPC_IsVendor() const;
	int NPC_GetAiFlags();
	bool NPC_Vendor_Restock(bool bForce = false, bool bFillStock = false);
	int NPC_GetVendorMarkup() const;

	void NPC_OnPetCommand( bool fSuccess, CChar * pMaster );
	bool NPC_OnHearPetCmd( lpctstr pszCmd, CChar * pSrc, bool fAllPets = false );
	bool NPC_OnHearPetCmdTarg( int iCmd, CChar * pSrc, CObjBase * pObj, const CPointMap & pt, lpctstr pszArgs );
	size_t  NPC_OnHearName( lpctstr pszText ) const;
	void NPC_OnHear( lpctstr pCmd, CChar * pSrc, bool fAllPets = false );
	bool NPC_OnItemGive( CChar * pCharSrc, CItem * pItem );
	bool NPC_SetVendorPrice( CItem * pItem, int iPrice );
	bool OnTriggerSpeech(bool bIsPet, lpctstr pszText, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue = HUE_DEFAULT);

	// Outside events that occur to us.
	int  OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType, int iDmgPhysical = 0, int iDmgFire = 0, int iDmgCold = 0, int iDmgPoison = 0, int iDmgEnergy = 0 );
	void OnHarmedBy( CChar * pCharSrc );
	bool OnAttackedBy( CChar * pCharSrc, int iHarmQty, bool fPetsCommand = false, bool fShouldReveal = true );

	bool OnTickEquip( CItem * pItem );
	void OnTickFood( short iVal, int HitsHungerLoss );
	void OnTickStatusUpdate();
	bool OnTick();

	static CChar * CreateBasic( CREID_TYPE baseID );
	static CChar * CreateNPC( CREID_TYPE id );

	int GetAbilityFlags() const;

};

inline bool CChar::IsSkillBase( SKILL_TYPE skill ) // static
{
	// Is this in the base set of skills.
	return (skill > SKILL_NONE && skill < static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill));
}

inline bool CChar::IsSkillNPC( SKILL_TYPE skill )  // static
{
	// Is this in the NPC set of skills.
	return (skill >= NPCACT_FOLLOW_TARG && skill < NPCACT_QTY);
}


#endif // _INC_CCHAR_H
