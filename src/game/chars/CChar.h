/**
* @file CChar.h
*
*/

#ifndef _INC_CCHAR_H
#define _INC_CCHAR_H

#include "../../common/sphereproto.h"
#include "../components/CCFaction.h"
#include "../clients/CParty.h"
#include "../items/CItemContainer.h"
#include "../items/CItemCorpse.h"
#include "../items/CItemMemory.h"
#include "../items/CItemStone.h"
#include "../game_macros.h"
#include "../CObjBase.h"
#include "../CTimedObject.h"
#include "CCharBase.h"
#include "CCharPlayer.h"


class CWorldTicker;
class CCharNPC;
class CMapBlockingState;


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


enum WAR_SWING_TYPE	: int // m_Act_War_Swing_State
{
    WAR_SWING_INVALID = -1,
    WAR_SWING_EQUIPPING = 0,	// we are recoiling our weapon.
    WAR_SWING_READY,			// we can swing at any time.
    WAR_SWING_SWINGING,			// we are swinging our weapon.
    //--
    WAR_SWING_EQUIPPING_NOWAIT = 10 // Special return value for CChar::Fight_Hit, DON'T USE IT IN SCRIPTS!
};

class CChar : public CObjBase, public CContainer, public CTextConsole
{
	// RES_WORLDCHAR

	friend class CWorldTicker;

    // MT_CMUTEX_DEF; // It inherits from CObjBase which inherits CTimedObject, which already has a class mutex.

private:
	// Spell type effects.
#define STATF_INVUL			0x00000001	// Invulnerability
#define STATF_DEAD			0x00000002
#define STATF_FREEZE		0x00000004	// Paralyzed. (spell)
#define STATF_INVISIBLE		0x00000008	// Invisible (spell).
#define STATF_SLEEPING		0x00000010	// You look like a corpse ?
#define STATF_WAR			0x00000020	// War mode on ?
#define STATF_REACTIVE		0x00000040	// have reactive armor on.
#define STATF_POISONED		0x00000080	// Poison level is in the poison object
#define STATF_NIGHTSIGHT	0x00000100	// All is light to you
#define STATF_REFLECTION	0x00000200	// Magic reflect on.
#define STATF_POLYMORPH		0x00000400	// We have polymorphed to another form.
#define STATF_INCOGNITO		0x00000800	// Dont show skill titles
#define STATF_SPIRITSPEAK	0x00001000	// I can hear ghosts clearly.
#define STATF_INSUBSTANTIAL	0x00002000	// Ghost has not manifest. or GM hidden
#define STATF_EMOTEACTION	0x00004000	// The creature will emote its actions to it's owners.
#define STATF_COMM_CRYSTAL	0x00008000	// I have a IT_COMM_CRYSTAL or listening item on me.
#define STATF_HASSHIELD		0x00010000	// Using a shield
#define STATF_ARCHERCANMOVE	0x00020000	// Can move with archery
#define STATF_STONE			0x00040000	// turned to stone.
#define STATF_HOVERING		0x00080000	// hovering (flying gargoyle)
#define STATF_FLY			0x00100000	// Flying or running ? (anim)
//							0x00200000
#define STATF_HALLUCINATING	0x00400000	// eat 'shrooms or bad food.
#define STATF_HIDDEN		0x00800000	// Hidden (non-magical)
#define STATF_INDOORS		0x01000000	// we are covered from the rain.
#define STATF_CRIMINAL		0x02000000	// The guards will attack me. (someone has called guards)
#define STATF_CONJURED		0x04000000	// This creature is conjured and will expire. (leave no corpse or loot)
#define STATF_PET			0x08000000	// I am a pet/hirling. check for my owner memory.
#define STATF_SPAWNED		0x10000000	// I am spawned by a spawn item.
#define STATF_SAVEPARITY	0x20000000	// Has this char been saved or not ?
#define STATF_RIDDEN		0x40000000	// This is the horse. (don't display me) I am being ridden
#define STATF_ONHORSE		0x80000000	// Mounted on horseback.

	uint64 _uiStatFlag;		// Flags above

	CClient * m_pClient;	// is the char a currently logged in m_pPlayer ?

public:
	struct LastAttackers
	{
		int64	elapsed;
		dword	charUID;
		int		amountDone;
		int		threat;
		bool	ignore;
	};
	std::vector<LastAttackers> m_lastAttackers;

	struct NotoSaves
	{
		dword		charUID;	// Character viewing me
		int64		time;		// Update timer
		NOTO_TYPE	color;		// Color sent on movement packets
		NOTO_TYPE	value;		// Notoriety type
	};
	std::vector<NotoSaves> m_notoSaves;

	static const char *m_sClassName;

	CCharPlayer * m_pPlayer;	// May even be an off-line player !
	CCharNPC * m_pNPC;			// we can be both a player and an NPC if "controlled" ?
	CPartyDef * m_pParty;		// What party am i in ?
	CRegionWorld * m_pArea;		// What region are we in now. (for guarded message)
	CRegion * m_pRoom;		// What room we are in now.

	static lpctstr const sm_szRefKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szTrigName[CTRIG_QTY+1];
	static const LAYER_TYPE sm_VendorLayers[3];

	// Combat stuff. cached data. (not saved)
	CUID m_uidWeapon;			// current Wielded weapon.	(could just get rid of this ?)
	word m_defense;				// calculated armor worn (NOT intrinsic armor)
    ushort _uiRange;

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
	HUE_TYPE m_SpeechHueOverride;	// speech hue to use (ignore the one sent by the client, if a player, and from the defname and use this)
	HUE_TYPE m_EmoteHueOverride;	// emote hue to use

	// In order to revert to original Hue and body.
	CREID_TYPE _iPrev_id;		// Backup of body type for ghosts and poly
	HUE_TYPE _wPrev_Hue;		// Backup of skin color. in case of polymorph etc.
	HUE_TYPE _wBloodHue;		// Replicating CharDef's BloodColor on the char, or overriding it.

	CREID_TYPE m_dwDispIndex; //To change the DispID instance

	// Skills, Stats and health
	ushort m_Skill[SKILL_QTY];	// List of skills ( skill * 10 )

	struct
	{
		ushort  m_base;      // Base stat: STR, INT, DEX
		int     m_mod;	     // Modifier to base stat: ModSTR, ModINT, ModDex (signed to allow negative modifiers). Accepted values between -UINT16_MAX and +UINT16_MAX
		ushort  m_val;       // Hits, Mana, Stam
		ushort  m_max;		 // MaxVal: MaxHits, MaxMana, MaxStam
        int     m_maxMod;    // Modifier to MaxVal: ModMaxHits, ModMaxMana, ModMaxStam
		int64   m_regenRate; // Regen each this much milliseconds.
        int64   m_regenLast; // Time of the last regen.
        ushort  m_regenVal;  // Amount of Stat to gain at each regen
	} m_Stat[STAT_QTY];

    short m_iKarma;
    ushort m_uiFame;

	int64  _iTimeCreate;	    // When was i created ?
	int64  _iTimePeriodicTick;
	int64  _iTimeNextRegen;	    // When did i get my last regen tick ?
    ushort _iRegenTickCount;    // ticks until next regen.

	int64 _iTimeLastHitsUpdate;
	int64 _iTimeLastCallGuards;

	// Some character action in progress.
	SKILL_TYPE	m_Act_SkillCurrent;	// Currently using a skill. Could be combat skill.
	CUID		m_Act_UID;			// Current action target
	CUID		m_Fight_Targ_UID;	// Current combat target
	CUID		m_Act_Prv_UID;		// Previous target.
	int			m_Act_Difficulty;	// -1 = fail skill. (0-100) for skill advance calc.
	int			m_Act_Effect;
	CPointMap   m_Act_p;			// Moving to this location. or location of forge we are working on.
	int			m_StepStealth;		// Max steps allowed to walk invisible while using Stealth skill

    std::vector<CUID> m_followers;

	// Args related to specific actions type (m_Act_SkillCurrent)
	union
	{
		// README! To access the various items with ACTARG1/2/3, each struct member must have a size of 4 bytes.

		struct
		{
			dword m_dwArg1;	// "ACTARG1"
			dword m_dwArg2;	// "ACTARG2"
			dword m_dwArg3;	// "ACTARG3"
		} m_atUnk;

		// SKILL_MAGERY
		// SKILL_NECROMANCY
		// SKILL_CHIVALRY
		// SKILL_BUSHIDO
		// SKILL_NINJITSU
		// SKILL_SPELLWEAVING
		struct
		{
			SPELL_TYPE m_iSpell;			// ACTARG1 = Currently casting spell.
			CREID_TYPE m_iSummonID;		// ACTARG2 = A sub arg of the skill. (summoned type ?)
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
			dword m_dwStrokeCount;		// ACTARG1 = For smithing, tinkering, etc. all requiring multi strokes.
			ITEMID_TYPE m_iItemID;		// ACTARG2 = Making this item.
			dword m_dwAmount;				// ACTARG3 = How many of this item are we making?
		} m_atCreate;

		// SKILL_LUMBERJACKING
		// SKILL_MINING
		// SKILL_FISHING
		struct
		{
			CResourceIDBase m_ridType;	// ACTARG1 = Type of item we're harvesting
			dword m_dwBounceItem;			// ACTARG2 = Drop item on backpack (true) or drop it on ground (false)
            dword m_dwStrokeCount;		// ACTARG3 = All requiring multi strokes.
		} m_atResource;

		// SKILL_TAMING
		// SKILL_MEDITATION
		struct
		{
			dword m_dwStrokeCount;		// ACTARG1 = All requiring multi strokes.
		} m_atTaming;

		// SKILL_ARCHERY
		// SKILL_SWORDSMANSHIP
		// SKILL_MACEFIGHTING
		// SKILL_FENCING
		// SKILL_WRESTLING
		// SKILL_THROWING
		struct
		{
			WAR_SWING_TYPE m_iWarSwingState;    // ACTARG1 = We are in the war mode swing.
			int16 m_iRecoilDelay;		        // ACTARG2 & 0x0000FFFF = Duration (in tenth of secs) of the previous swing recoil time.
            int16 m_iSwingAnimationDelay;       // ACTARG2 & 0xFFFF0000 = Duration (in tenth of secs) of the previous swing animation duration.
            int16 m_iSwingAnimation;            // ACTARG3 & 0x0000FFFF = hit animation id.
            int16 m_iSwingIgnoreLastHitTag;     // ACTARG3 & 0xFFFF0000. Internally used by PreHit. If == 1 (which happens only for the hit after the first instahit), for this hit TAG.LastHit delay checking will be ignored.
		} m_atFight;

		// SKILL_ENTICEMENT
		// SKILL_DISCORDANCE
		// SKILL_PEACEMAKING
		struct
		{
			dword m_dwInstrumentUID;		// ACTARG1 = UID of the instrument we are playing.
		} m_atBard;

		// SKILL_PROVOCATION
		struct
		{
			dword m_dwInstrumentUID;    // ACTARG1 = UID of the instrument we are playing.
			dword m_Unused2;
			dword m_dwIsAlly;			// ACTARG3 = Is the provoked considered an ally of the target? 0/1
		} m_atProvocation;				//	If so, abort the skill. To allow always, override it to 0 in @Success via scripts.

		// SKILL_TRACKING
		struct
		{
			DIR_TYPE m_iPrvDir;			// ACTARG1 = Previous direction of tracking target, used for when to notify player
			dword m_dwDistMax;			// ACTARG2 = Maximum distance when starting and continuing to use the Tracking skill.
		} m_atTracking;

		// NPCACT_RIDDEN
		struct
		{
			CUID m_uidFigurine;     // ACTARG1 = This creature is being ridden by this object link. IT_FIGURINE IT_EQ_HORSE
		} m_atRidden;

		// NPCACT_TALK
		// NPCACT_TALK_FOLLOW
		struct
		{
			dword m_dwHearUnknown;		// ACTARG1 = Speaking NPC has no idea what u're saying.
			dword m_dwWaitCount;		// ACTARG2 = How long have i been waiting (xN sec)
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
	bool DupeFrom(const CChar * pChar, bool fNewbieItems);

	CChar(const CChar& copy) = delete;
	CChar& operator=(const CChar& other) = delete;

protected:
	void DeleteCleanup(bool fForce);	// Not virtual!
	virtual void DeletePrepare() override;
public:
	bool NotifyDelete(bool fForce);
	virtual bool Delete(bool fForce = false) override;

	// Status and attributes ------------------------------------
	virtual int IsWeird() const override;

#if MT_ENGINES
//protected:	bool _IsStatFlag(uint64 uiStatFlag) const noexcept;
public:		bool  IsStatFlag(uint64 uiStatFlag) const noexcept;
#else
public:
    // called very frequently, it's wise to inline it if we can
    inline bool IsStatFlag(uint64 uiStatFlag) const noexcept
    {
        return (_uiStatFlag & uiStatFlag);
    }
#endif

//protected:	void _StatFlag_Set(uint64 uiStatFlag) noexcept;
public:		void  StatFlag_Set(uint64 uiStatFlag) noexcept;

//protected:	void _StatFlag_Clear(uint64 uiStatFlag) noexcept;
public:		void  StatFlag_Clear(uint64 uiStatFlag) noexcept;

//protected:	void _StatFlag_Mod(uint64 uiStatFlag, bool fMod) noexcept;
public:		void  StatFlag_Mod(uint64 uiStatFlag, bool fMod) noexcept;

	char GetFixZ(const CPointMap& pt, uint64 uiBlockFlags = 0);
	bool IsPriv( word flag ) const;
	virtual PLEVEL_TYPE GetPrivLevel() const override;

	CCharBase * Char_GetDef() const;
	CRegionWorld * GetRegion() const;
	CRegion * GetRoom() const;
	virtual int GetVisualRange() const override;
	void SetVisualRange(byte newSight);

	virtual bool IsResourceMatch( const CResourceID& rid, dword dwArg ) const override;
	bool IsResourceMatch( const CResourceID& rid, dword dwArg, dword dwArgResearch ) const;

	bool IsSpeakAsGhost() const;
	bool CanUnderstandGhost() const;
	bool IsPlayableCharacter() const;
	bool IsHuman() const;
	bool IsElf() const;
	bool IsGargoyle() const;

	int GetStatPercent(STAT_TYPE i) const;
	lpctstr GetTradeTitle() const; // Paperdoll title for character p (2)

	// Information about us.
	CREID_TYPE GetID() const;
	virtual word GetBaseID() const override;
	CREID_TYPE GetDispID() const;
	bool SetDispID(CREID_TYPE id);
	void SetID( CREID_TYPE id );

	virtual lpctstr GetName() const override;
	lpctstr GetNameWithoutIncognito() const;
	lpctstr GetName( bool fAllowAlt ) const;
	virtual bool SetName( lpctstr pName ) override;

	height_t GetHeightMount( bool fEyeSubstract = false ) const;
	height_t GetHeight() const;

	bool CanSeeAsDead( const CChar * pChar = nullptr ) const;
	bool CanSeeInContainer( const CItemContainer * pContItem ) const;
	bool CanSee( const CObjBaseTemplate * pObj ) const;
	bool CanSeeLOS_New_Failed( CPointMap * pptBlock, const CPointMap &ptNow ) const;
	bool CanSeeLOS_New( const CPointMap & pd, CPointMap * pBlock = nullptr, int iMaxDist = UO_MAP_VIEW_SIGHT, word wFlags = 0, bool bCombatCheck = false ) const;
	bool CanSeeLOS( const CPointMap & pd, CPointMap * pBlock = nullptr, int iMaxDist = UO_MAP_VIEW_SIGHT, word wFlags = 0, bool bCombatCheck = false ) const;
	bool CanSeeLOS( const CObjBaseTemplate * pObj, word wFlags = 0, bool bCombatCheck = false) const;

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
	IT_TYPE CanTouchStatic( CPointMap * pPt, ITEMID_TYPE id, const CItem * pItem ) const;
	bool CanMoveItem( const CItem * pItem, bool fMsg = true ) const;
	byte GetLightLevel() const;
	bool CanUse( const CItem * pItem, bool fMoveOrConsume ) const;
	bool IsMountCapable() const;

	ushort  Food_CanEat( CObjBase * pObj ) const;
	short   Food_GetLevelPercent() const;
	lpctstr Food_GetLevelMessage( bool fPet, bool fHappy ) const;

public:
	ushort	Stat_GetAdjusted( STAT_TYPE i ) const;
	void	Stat_SetBase( STAT_TYPE i, ushort uiVal );
	ushort	Stat_GetBase( STAT_TYPE i ) const;
	void	Stat_AddBase( STAT_TYPE i, int iVal );
	void	Stat_AddMod( STAT_TYPE i, int iVal );
	void	Stat_SetMod( STAT_TYPE i, int iVal );
    int	    Stat_GetMod( STAT_TYPE i ) const;
	void	Stat_SetVal( STAT_TYPE i, ushort uiVal );
    void	Stat_AddVal( STAT_TYPE i, int iVal );
	ushort	Stat_GetVal( STAT_TYPE i ) const;
	void	Stat_SetMax( STAT_TYPE i, ushort uiVal );
	ushort	Stat_GetMax( STAT_TYPE i ) const;
    void	Stat_SetMaxMod( STAT_TYPE i, int iVal );
    void	Stat_AddMaxMod( STAT_TYPE i, int iVal );
    int	    Stat_GetMaxMod( STAT_TYPE i ) const;
    ushort	Stat_GetMaxAdjusted( STAT_TYPE i ) const;
	uint    Stat_GetSum() const;
	ushort	Stat_GetLimit( STAT_TYPE i ) const;     // Or STATCAP
    uint	Stat_GetSumLimit() const;               // Or STATSUM (intended as the maximum value allowed for the sum of the stats)
	bool Stat_Decrease( STAT_TYPE stat, SKILL_TYPE skill = SKILL_NONE);
	bool Stats_Regen();
	ushort Stats_GetRegenVal(STAT_TYPE iStat);
    void Stats_SetRegenVal(STAT_TYPE iStat, ushort uiVal);
    void Stats_AddRegenVal(STAT_TYPE iStat, int iVal);
    int64 Stats_GetRegenRate(STAT_TYPE iStat);  // return value is in milliseconds
    void Stats_SetRegenRate(STAT_TYPE iStat, int64 iRateMilliseconds);
	SKILLLOCK_TYPE Stat_GetLock(STAT_TYPE stat);
	void Stat_SetLock(STAT_TYPE stat, SKILLLOCK_TYPE state);
    short GetKarma() const;
    void SetKarma(short iNewKarma, CChar* pNPC = nullptr);
    ushort GetFame() const;
    void SetFame(ushort uiNewFame, CChar* pNPC = nullptr);

	void Stat_StrCheckEquip();

	// Location and movement ------------------------------------
private:
	bool TeleportToCli( int iType, int iArgs );
	bool TeleportToObj( int iType, tchar * pszArgs );
private:
	CRegion * CheckValidMove( CPointMap & ptDest, uint64 * uiBlockFlags, DIR_TYPE dir, height_t * ClimbHeight, bool fPathFinding = false ) const;
	void FixClimbHeight();
	bool MoveToRoom( CRegion * pNewRoom, bool fAllowReject);
	bool IsVerticalSpace( const CPointMap& ptDest, bool fForceMount = false ) const;

public:
	virtual CObjBaseTemplate* GetTopLevelObj() override;
	virtual const CObjBaseTemplate* GetTopLevelObj() const override;

	bool IsSwimming() const;
	bool MoveToRegion(CRegionWorld* pNewArea, bool fAllowReject);

	bool MoveToRegionReTest( dword dwType );
	bool MoveToChar(const CPointMap& pt, bool fStanding = true, bool fCheckLocationEffects = true, bool fForceFix = false, bool fAllowReject = true);
	virtual bool MoveTo(const CPointMap& pt, bool fForceFix = false) override;
	virtual void SetTopZ( char z ) override;
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, ushort iSteps = 0 ) override;
	bool MoveToValidSpot(DIR_TYPE dir, int iDist, int iDistStart = 1, bool fFromShip = false);
	bool MoveToNearestShore(bool fNoMsg = false);

    bool CanMove(bool fCheckOnly) const;
    bool ShoveCharAtPosition(CPointMap const& ptDst, ushort *uiStaminaRequirement, bool fPathfinding);
    bool CanStandAt(CPointMap *ptDest, const CRegion* pArea, uint64 uiMyMovementFlags, height_t uiMyHeight, CServerMapBlockingState* blockingState, bool fPathfinding) const;
	CRegion * CanMoveWalkTo( CPointMap & pt, bool fCheckChars = true, bool fCheckOnly = false, DIR_TYPE dir = DIR_QTY, bool fPathFinding = false );
	void CheckRevealOnMove();
	TRIGRET_TYPE CheckLocationEffects(bool fStanding);

public:
	// Client Player specific stuff. -------------------------
	bool IsPlayer() const noexcept			{ return nullptr != m_pPlayer; }
	bool IsClientActive() const noexcept	{ return nullptr != m_pClient; }
	bool IsClientType() const noexcept;
	CClient* GetClientActive() const noexcept { return m_pClient; }

	void ClientAttach( CClient * pClient );
	void ClientDetach();

	bool SetPrivLevel( CTextConsole * pSrc, lpctstr pszFlags );
	bool CanDisturb( const CChar * pChar ) const;
	void SetDisconnected( CSector *pNewSector = nullptr );
	bool SetPlayerAccount( CAccount * pAccount );
	bool SetPlayerAccount( lpctstr pszAccount );

	void ClearPlayer();

    bool IsNPC() const;
	bool SetNPCBrain( NPCBRAIN_TYPE NPCBrain );
	NPCBRAIN_TYPE GetNPCBrain() const;
    NPCBRAIN_TYPE GetNPCBrainGroup() const noexcept;	// Return NPCBRAIN_ANIMAL for animals, _HUMAN for NPC human and PCs, >= _MONSTER for monsters
	NPCBRAIN_TYPE GetNPCBrainAuto() const noexcept;	// Guess default NPC brain
	void ClearNPC();


public:
	void ObjMessage( lpctstr pMsg, const CObjBase * pSrc ) const;
	virtual void SysMessage( lpctstr pMsg ) const override;

	void UpdateStatsFlag() const;
	void UpdateStatVal( STAT_TYPE type, int iChange = 0, ushort uiLimit = 0 );
	void UpdateHitsFlag();
	void UpdateModeFlag();
	void UpdateManaFlag() const;
	void UpdateStamFlag() const;
	ANIM_TYPE GenerateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, byte iFrameDelay = 0, byte iAnimLen = 7);
	bool UpdateAnimate(ANIM_TYPE action, bool fTranslate = true, bool fBackward = false, byte iFrameDelay = 0, byte iAnimLen = 7);

	void UpdateMode( CClient * pExcludeClient = nullptr, bool fFull= false );
	void UpdateSpeedMode();
	void UpdateVisualRange();
	void UpdateMove( const CPointMap & ptOld, CClient * pClientExclude = nullptr, bool bFull = false );
	void UpdateDir( DIR_TYPE dir );
	void UpdateDir( const CPointMap & pt );
	void UpdateDir( const CObjBaseTemplate * pObj );
	void UpdateDrag( CItem * pItem, CObjBase * pCont = nullptr, CPointMap * pt = nullptr );

public:
	lpctstr GetPronoun() const;	// he
	lpctstr GetPossessPronoun() const;	// his
	byte GetModeFlag( const CClient *pViewer = nullptr ) const;
	byte GetDirFlag(bool fSquelchForwardStep = false) const;
	uint64 GetCanMoveFlags(uint64 uiCanFlags, bool fIgnoreGM = false) const;

	virtual int FixWeirdness() override;
	void CreateNewCharCheck();

	// Contents/Carry stuff. ---------------------------------
private:
	virtual void ContentAdd( CItem * pItem, bool bForceNoStack = false ) override;
protected:
	virtual void OnRemoveObj( CSObjContRec* pObRec ) override;	// Override this = called when removed from list.

public:
	bool CanCarry( const CItem * pItem ) const;
	bool CanEquipStr( CItem * pItem ) const;
	LAYER_TYPE CanEquipLayer( CItem * pItem, LAYER_TYPE layer, CChar * pCharMsg, bool fTest );
	CItem * LayerFind( LAYER_TYPE layer ) const;
	void LayerAdd( CItem * pItem, LAYER_TYPE layer = LAYER_QTY );

	TRIGRET_TYPE OnCharTrigForLayerLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, LAYER_TYPE layer );
	TRIGRET_TYPE OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, word wMemType );

	virtual void OnWeightChange( int iChange ) override;
	virtual int GetWeight(word amount = 0) const override;
	int GetWeightLoadPercent( int iWeight ) const;

	CItem * GetSpellbook(SPELL_TYPE iSpell = SPELL_Clumsy) const;
	CItem * GetSpellbookLayer() const; //Search for a Spellbook in layer 1 or 2.
	CItemContainer * GetPack() const;
	CItemContainer * GetBank( LAYER_TYPE layer = LAYER_BANKBOX );
	CItemContainer * GetPackSafe();
	CItem * GetBackpackItem(ITEMID_TYPE item);
	void AddGoldToPack( int iAmount, CItemContainer * pPack = nullptr, bool fForceNoStack = true );

public:
    /**
    * @brief   Queries if a trigger is active ( m_RunningTrigger ) .
    * @param   trig    The trig.
    * @return  true if the trigger is active, false if not.
    */
    bool IsTriggerActive(lpctstr trig) const;

    /**
    * @brief   Sets trigger active ( m_RunningTrigger ).
    * @param   trig    The trig.
    */
    void SetTriggerActive(lpctstr trig = nullptr);

	virtual TRIGRET_TYPE OnTrigger( lpctstr pTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs ) override;
	TRIGRET_TYPE OnTrigger( CTRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs = nullptr );

public:
	// Load/Save----------------------------------

	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_Load( CScript & s ) override;  // Load a character from Script
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & s, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual void r_Write( CScript & s ) override;

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
	void Noto_Karma( int iKarmaChange, int iBottom = INT32_MIN, bool fMessage = false, CChar* pNPC = nullptr );

	/**
	* @brief Update Fame with the given value.
	*
	* Used to increase/decrease Fame, it fires @FameChange trigger.
	* Can't never exceed g_Cfg.m_iMaxFame and can't never be lower than 0.
	* @param iFameChange is the amount of fame to change over the current one.
	*/
	void Noto_Fame( int iFameChange, CChar* pNPC = nullptr );

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
	* @param fIncog if set to true and he has STATF_INCOGNITO, this character will be gray for the viever (pChar).
	* @param fInvul if set to true invulnerable characters will return NOTO_INVUL (yellow bar, etc).
	* @param fGetColor if set to true only the color will be returned and not the notoriety (note that they can differ if set to so in the @NotoSend trigger).
	* @return NOTO_TYPE notoriety level.
	*/
	NOTO_TYPE Noto_GetFlag( const CChar * pChar, bool fIncog = true, bool fInvul = false, bool fGetColor = false ) const;

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
	bool Noto_IsMurderer() const noexcept;

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
	bool Noto_IsCriminal() const;


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
	* @param pCharViewer: on who I performed criminal actions or saw me commiting a crime and flagged me as criminal.
    * @param fFromSawCrime: making it criminal because of MEMORY_SAWCRIME?
	* @return true if I really became a criminal.
	*/
	bool Noto_Criminal( CChar * pCharViewer = nullptr, bool fFromSawCrime = false);

	/**
	* @brief I am a murderer (it seems) (update my murder decay item).
	*
	*/
	void Noto_Murder();

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
	* @param pChar, the CChar* of the char of which we want to resend the noto.
	*/
	void NotoSave_Resend( CChar *pChar );

	/**
	* @brief Gets the entry list of the given CChar.
	*
	* @param pChar, CChar to retrieve the entry number for.
	* @return the entry number.
	*/
	int NotoSave_GetID( CChar * pChar ) const;

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
	bool IsTakeCrime( const CItem * pItem, CChar ** ppCharMark = nullptr ) const;


	/**
	* @brief We killed a character, starting exp calcs
	*
	* Main function for default Level system.
	* Triggers @ExpChange and @LevelChange if needed
	* @param delta, amount of exp gaining (or losing?)
	* @param ppCharDead from who we gained the experience.
	*/
	void ChangeExperience(llong delta = 0, CChar *pCharDead = nullptr);
	uint GetSkillTotal(int what = 0, bool how = true);

    /*
    * @brief Check the pItem stack has amount more than iQty. If not, searches character's backpack to check if character has enough item to consume.
    */
    bool CanConsume(CItem* pItem, word iQty);

    /*
    * @brief Consumes iQty amount of the item from your stack. If the stack has less amount than iQty, searches character's backpack to consume from other stacks.
    */
    bool ConsumeFromPack(CItem* pItem, word iQty);

	// skills and actions. -------------------------------------------
	static bool IsSkillBase( SKILL_TYPE skill ) noexcept;
	static bool IsSkillNPC( SKILL_TYPE skill ) noexcept;

	SKILL_TYPE Skill_GetBest( uint iRank = 0 ) const; // Which skill is the highest for character p
	SKILL_TYPE Skill_GetActive() const noexcept
	{
		return m_Act_SkillCurrent;
	}
	lpctstr Skill_GetName( bool fUse = false ) const;
	ushort Skill_GetBase( SKILL_TYPE skill ) const;
	ushort Skill_GetMax( SKILL_TYPE skill, bool ignoreLock = false ) const;
    uint Skill_GetSum() const;
    uint Skill_GetSumMax() const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	ushort Skill_GetAdjusted(SKILL_TYPE skill) const;
	SKILL_TYPE Skill_GetMagicRandom(ushort uiMinValue = 0);
	SKILL_TYPE Skill_GetMagicBest();

	/**
	* @brief Checks if the given skill can be used.
	*
	* @param skill: Skill to check
	* @return if it can be used, or not....
	*/
	bool Skill_CanUse( SKILL_TYPE skill );

	void Skill_SetBase( SKILL_TYPE skill, ushort uiValue );
    void Skill_AddBase( SKILL_TYPE skill, int iChange );
	bool Skill_UseQuick( SKILL_TYPE skill, int64 difficulty, bool bAllowGain = true, bool bUseBellCurve = true, bool bForceCheck = false);

	bool Skill_CheckSuccess( SKILL_TYPE skill, int difficulty, bool bUseBellCurve = true ) const;
	bool Skill_Wait( SKILL_TYPE skilltry );
	bool Skill_Start( SKILL_TYPE skill, int iDifficultyIncrease = 0 ); // calc skill progress.
	void Skill_Fail( bool fCancel = false );
	int Skill_Stroke();				// Strokes in crafting skills, calling for SkillStroke trig
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
	int Skill_Focus(STAT_TYPE stat); //The focus skill is used passively, so it doesn't have any skill stage.

	int Skill_Act_Napping(SKTRIG_TYPE stage);
	int Skill_Act_Throwing(SKTRIG_TYPE stage);
	int Skill_Act_Breath(SKTRIG_TYPE stage);
	int Skill_Act_Training( SKTRIG_TYPE stage );

	void Spell_Dispel( int iskilllevel );
	CChar * Spell_Summon_Place( CChar * pChar, CPointMap ptTarg, int64 iDuration = 0);
	bool Spell_Recall(CItem * pRune, bool fGate);
    CItem * Spell_Effect_Create( SPELL_TYPE spell, LAYER_TYPE layer, int iEffect, int64 iDurationInTenths, CObjBase * pSrc = nullptr, bool bEquip = true );
	SPELL_TYPE Spell_GetIndex(SKILL_TYPE skill = SKILL_NONE);	//gets first spell for the magic skill given.
	SPELL_TYPE Spell_GetMax(SKILL_TYPE skill = SKILL_NONE);	//gets first spell for the magic skill given.
	bool Spell_Equip_OnTick( CItem * pItem );

	void Spell_Field(CPointMap pt, ITEMID_TYPE idEW, ITEMID_TYPE idNS, uint fieldWidth, uint fieldGauge, int iSkill,
        CChar * pCharSrc = nullptr, ITEMID_TYPE idnewEW = (ITEMID_TYPE)0, ITEMID_TYPE idnewNS = (ITEMID_TYPE)0,
        int64 iDuration = 0, HUE_TYPE iColor = HUE_DEFAULT);
	void Spell_Area( CPointMap pt, int iDist, int iSkill, int64 iDuration = 0);
	bool Spell_TargCheck_Face();
	bool Spell_TargCheck();
	bool Spell_Unequip( LAYER_TYPE layer );

	int  Spell_CastStart();
	void Spell_CastFail( bool fAbort = false );

public:
    bool Spell_Resurrection(CItemCorpse * pCorpse = nullptr, CChar * pCharSrc = nullptr, bool fNoFail = false);
    bool Spell_Teleport( CPointMap ptDest, bool fTakePets = false, bool fCheckAntiMagic = true, bool fDisplayEffect = true,
        ITEMID_TYPE iEffect = ITEMID_NOTHING, SOUND_TYPE iSound = SOUND_NONE );
    bool Spell_CreateGate(CPointMap ptDest, bool fCheckAntiMagic = true);
	bool Spell_SimpleEffect( CObjBase * pObj, CObjBase * pObjSrc, SPELL_TYPE &spell, int &iSkillLevel, int64 iDuration = 0);
	bool Spell_CastDone();
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool fReflecting = false, int64 iDuration = 0) override;
	bool Spell_CanCast( SPELL_TYPE &spellRef, bool fTest, CObjBase * pSrc, bool fFailMsg, bool fCheckAntiMagic = true );
	CChar * Spell_Summon_Try(SPELL_TYPE spell, CPointMap ptTarg, CREID_TYPE iC1);
	int64 GetSpellDuration( SPELL_TYPE spell, int iSkillLevel, CChar * pCharSrc = nullptr ); // in tenths of second

	// Memories about objects in the world. -------------------
	bool Memory_OnTick( CItemMemory * pMemory );
	bool Memory_UpdateFlags( CItemMemory * pMemory );
	bool Memory_UpdateClearTypes( CItemMemory * pMemory, word wMemTypes );
	void Memory_AddTypes( CItemMemory * pMemory, word wMemTypes );
	bool Memory_ClearTypes( CItemMemory * pMemory, word wMemTypes );
	CItemMemory * Memory_CreateObj( const CUID& uid, word wMemTypes );
	CItemMemory * Memory_CreateObj( const CObjBase * pObj, word wMemTypes );

public:
	void Memory_ClearTypes( word wMemTypes );
	CItemMemory * Memory_FindObj(const CUID& uid ) const;
	CItemMemory * Memory_FindObj( const CObjBase * pObj ) const;
	CItemMemory * Memory_AddObjTypes(const CUID& uid, word wMemTypes );
	CItemMemory * Memory_AddObjTypes( const CObjBase * pObj, word wMemTypes );
	CItemMemory * Memory_FindTypes( word wMemTypes ) const;
	CItemMemory * Memory_FindObjTypes( const CObjBase * pObj, word wMemTypes ) const;
	// -------- Public alias for MemoryCreateObj ------------------
	CItemMemory * Memory_AddObj( const CUID& uid, word wMemTypes );
	CItemMemory * Memory_AddObj( const CObjBase * pObj, word wMemTypes );
	// ------------------------------------------------------------

public:
	void SoundChar(CRESND_TYPE type);
	void Action_StartSpecial(CREID_TYPE id);

private:
	void OnNoticeCrime( CChar * pCriminal, CChar * pCharMark );
public:
	bool CheckCrimeSeen( SKILL_TYPE SkillToSee, CChar * pCharMark, const CObjBase * pItem, lpctstr pAction );

private:
	// Armor, weapons and combat ------------------------------------

    /**
    * @fn  byte CChar::GetRangeL() const;
    * @brief   Returns Range Lowest byte.
    * @return  The Value.
    */
    byte GetRangeL() const;

    /**
    * @fn  byte CChar::GetRangeH() const;
    * @brief   Returns Range Highest byte.
    * @return  The Value.
    */
    byte GetRangeH() const;

	int	Fight_CalcRange( CItem * pWeapon = nullptr ) const;
    void Fight_SetDefaultSwingDelays();

	bool Fight_IsActive() const;
public:
	int CalcArmorDefense() const;
	static int CalcPercentArmorDefense(LAYER_TYPE layer);
	void Memory_Fight_Retreat( CChar * pTarg, CItemMemory * pFight );
	void Memory_Fight_Start( const CChar * pTarg );
	bool Memory_Fight_OnTick( CItemMemory * pMemory );

	bool Fight_Attack( CChar * pCharTarg, bool fToldByMaster = false );
	bool Fight_Clear( CChar * pCharTarg , bool fForced = false );
	void Fight_ClearAll();
	void Fight_HitTry();
	WAR_SWING_TYPE Fight_Hit( CChar * pCharTarg );
	WAR_SWING_TYPE Fight_CanHit(CChar * pCharTarg, bool fSwingNoRange = false);
	SKILL_TYPE Fight_GetWeaponSkill() const;
    DAMAGE_TYPE Fight_GetWeaponDamType(const CItem* pWeapon = nullptr) const;
	int Fight_CalcDamage( const CItem * pWeapon, bool bNoRandom = false, bool bGetMax = true ) const;
	bool Fight_IsAttackableState();

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
    #define ATTACKER_THREAT_TOLDBYMASTER 1000

	inline int GetAttackersCount() {
		return (int)m_lastAttackers.size();
	}
	bool	Attacker_Add(CChar * pChar, int threat = 0);
	CChar * Attacker_GetLast() const;
	bool	Attacker_Delete(std::vector<LastAttackers>::iterator &itAttacker, bool fForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	bool	Attacker_Delete(int attackerIndex, bool fForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	bool	Attacker_Delete(const CChar * pChar, bool fForced = false, ATTACKER_CLEAR_TYPE type = ATTACKER_CLEAR_FORCED);
	void	Attacker_RemoveChar();
	void	Attacker_Clear();
	void	Attacker_CheckTimeout();
	int		Attacker_GetDam(int attackerIndex) const;
	void	Attacker_SetDam(const CChar * pChar, int value);
	void	Attacker_SetDam(int attackerIndex, int value);
	CChar * Attacker_GetUID(int attackerIndex) const;
	int64	Attacker_GetElapsed(int attackerIndex) const;
	void	Attacker_SetElapsed(const CChar * pChar, int64 value);
	void	Attacker_SetElapsed(int attackerIndex, int64 value);
	int		Attacker_GetThreat(int attackerIndex) const;
	void	Attacker_SetThreat(const CChar * pChar, int value);
	void	Attacker_SetThreat(int attackerIndex, int value);
	bool	Attacker_GetIgnore(int iChar) const;
	bool	Attacker_GetIgnore(const CChar * pChar) const;
	void	Attacker_SetIgnore(int iChar, bool fIgnore);
	void	Attacker_SetIgnore(const CChar * pChar, bool fIgnore);
	int		Attacker_GetHighestThreat() const;
	int		Attacker_GetID(const CChar * pChar) const;
	int		Attacker_GetID(const CUID& pChar) const;

	//
	bool Player_OnVerb( CScript &s, CTextConsole * pSrc );
	void InitPlayer( CClient * pClient, const char * pszCharname, bool fFemale, RACE_TYPE rtRace, ushort wStr, ushort wDex, ushort wInt,
		PROFESSION_TYPE iProf, SKILL_TYPE skSkill1, ushort uiSkillVal1, SKILL_TYPE skSkill2, ushort uiSkillVal2, SKILL_TYPE skSkill3, ushort uiSkillVal3, SKILL_TYPE skSkill4, ushort uiSkillVal4,
		HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, ITEMID_TYPE idFace, int iStartLoc );
	bool ReadScriptReducedTrig(CCharBase * pCharDef, CTRIG_TYPE trig, bool fVendor = false);
	bool ReadScriptReduced(CResourceLock &s, bool fVendor = false);
	void NPC_LoadScript( bool fRestock );
	void NPC_CreateTrigger();

	// Mounting and figurines
	ITEMID_TYPE Horse_GetMountItemID() const;
	bool Horse_Mount( CChar * pHorse ); // Remove horse char and give player a horse item
	bool Horse_UnMount(); // Remove horse char and give player a horse item

private:
	CItem* Horse_GetMountItem() const;
    CChar* Horse_GetMountChar() const;
    CItem* Horse_GetValidMountItem();
    CChar* Horse_GetValidMountChar();

public:
    bool CanDress(const CChar* pChar) const;
	bool IsOwnedBy( const CChar * pChar, bool fAllowGM = true ) const;
	CChar * GetOwner() const;
	CChar * Use_Figurine( CItem * pItem, bool fCheckFollowerSlots = true );
	CItem * Make_Figurine( const CUID &uidOwner, ITEMID_TYPE id = ITEMID_NOTHING );
	CItem * NPC_Shrink();
	bool FollowersUpdate( CChar * pChar, short iFollowerSlots = 0, bool fCheckOnly = false );

	int  ItemPickup( CItem * pItem, word amount );
	bool ItemEquip( CItem * pItem, CChar * pCharMsg = nullptr, bool fFromDClick = false );
	bool ItemEquipWeapon( bool fForce );
	bool ItemEquipArmor( bool fForce );
	bool ItemBounce( CItem * pItem, bool fDisplayMsg = true );
	bool ItemDrop( CItem * pItem, const CPointMap & pt );

	virtual void Update(const CClient* pClientExclude = nullptr) override;
	virtual void Flip() override;
	bool SetPoison( int iSkill, int iHits, CChar * pCharSrc );
	bool SetPoisonCure( bool fExtra );
	bool CheckCorpseCrime( CItemCorpse *pCorpse, bool fLooting, bool fTest );
	CItemCorpse * FindMyCorpse( bool fIgnoreLOS = false, int iRadius = 2) const;
	CItemCorpse * MakeCorpse( bool fFrontFall );
	bool RaiseCorpse( CItemCorpse * pCorpse );
	bool Death();
	bool Reveal( uint64 iFlags = 0 );
	void Jail( CTextConsole * pSrc, bool fSet, int iCell );
	void EatAnim(CItem* pItem, ushort uiQty);
	/**
	* @Brief I'm calling guards (Player speech)
	*
	* Looks for nearby criminals to call guards on, and marks them to Criminal.
	* This is called from players only, since NPCs will CallGuards(OnTarget) directly.
	*/
	void CallGuards();
	/**
	* @Brief I'm calling guards on pCriminal
	*
	* @param pCriminal: character who shall be punished by guards. Should already be marked as Criminal.
    * @return true if the call succeed
	*/
	bool CallGuards( CChar * pCriminal );

#define DEATH_NOFAMECHANGE 0x01
#define DEATH_NOCORPSE 0x02
#define DEATH_NOLOOTDROP 0x04
#define DEATH_NOCONJUREDEFFECT 0x08
#define DEATH_HASCORPSE 0x010

    void Speak_RevealCheck(TALKMODE_TYPE mode);
	virtual void Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL ) override;
	virtual void SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 ) override;
	virtual void SpeakUTF8Ex( const nachar * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang ) override;

	bool OnFreezeCheck() const;
    bool IsStuck(bool fFreezeCheck);

	void DropAll(CItemContainer * pCorpse = nullptr, uint64 uiAttr = 0);
	void UnEquipAllItems( CItemContainer * pCorpse = nullptr, bool fLeaveHands = false );
	void Wake();
	void SleepStart( bool fFrontFall );

	void Guild_Resign( MEMORY_TYPE memtype );
	CItemStone * Guild_Find( MEMORY_TYPE memtype ) const;
	CStoneMember * Guild_FindMember( MEMORY_TYPE memtype ) const;
	lpctstr Guild_Abbrev( MEMORY_TYPE memtype ) const;
	lpctstr Guild_AbbrevBracket( MEMORY_TYPE memtype ) const;

	void Use_EatQty( CItem * pFood, ushort uiQty = 1 );
	bool Use_Eat( CItem * pItem, ushort uiQty = 1 );
	bool Use_MultiLockDown( CItem * pItemTarg );
	void Use_CarveCorpse( CItemCorpse * pCorpse, CItem * pItemCarving );
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
	ushort NPC_GetTrainMax( const CChar * pStudent, SKILL_TYPE Skill ) const;

	bool NPC_OnVerb( CScript &s, CTextConsole * pSrc = nullptr );
	void NPC_OnHirePayMore( CItem * pGold, uint uiWage, bool fHire = false );
public:
	bool NPC_OnHirePay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnHireHear( CChar * pCharSrc );
	ushort NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill );
	bool NPC_OnTrainPay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold );
	bool NPC_OnTrainHear( CChar * pCharSrc, lpctstr pCmd );
	bool NPC_TrainSkill( CChar * pCharSrc, SKILL_TYPE skill, ushort uiAmountToTrain );
    int64 PayGold(CChar * pCharSrc, int64 iGold, CItem * pGold, ePayGold iReason);
private:
	bool NPC_CheckWalkHere( const CPointMap & pt, const CRegion * pArea ) const;
	void NPC_OnNoticeSnoop( const CChar * pCharThief, const CChar * pCharMark );

	void NPC_LootMemory( CItem * pItem );
	bool NPC_LookAtCharGuard( CChar * pChar, bool bFromTrigger = false );
	bool NPC_LookAtCharHealer( CChar * pChar );
	bool NPC_LookAtCharHuman( CChar * pChar );
	bool NPC_LookAtCharMonster( CChar * pChar );
	bool NPC_LookAtChar( CChar * pChar, int iDist );
	bool NPC_LookAtItem( CItem * pItem, int iDist );
	bool NPC_LookAround( bool fForceCheckItems = false );
	int  NPC_WalkToPoint(bool fRun = false);
	CChar * NPC_FightFindBestTarget(const std::vector<CChar*> * pvExcludeList = nullptr);
	bool NPC_FightMagery(CChar * pChar);
	bool NPC_FightCast(CObjBase * &pChar ,CObjBase * pSrc, SPELL_TYPE &spell, int &skill, int iHealThreshold, bool bIgnoreAITargetChoice = false);
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
	bool NPC_Act_Flee();
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

	void NPC_PetRelease();
	void NPC_PetDesert();
	void NPC_PetClearOwners();
	bool NPC_PetSetOwner( CChar * pChar );
	CChar * NPC_PetGetOwner() const;			// find my master
	CChar * NPC_PetGetOwnerRecursive() const;	// find the owner of the owner of the owner...
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
	int  OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType, int iDmgPhysical = 0, int iDmgFire = 0, int iDmgCold = 0, int iDmgPoison = 0, int iDmgEnergy = 0, SPELL_TYPE spell = SPELL_NONE );
	void OnTakeDamageInflictArea(int iDmg, CChar* pSrc, DAMAGE_TYPE uType, int iDmgPhysical = 0, int iDmgFire = 0, int iDmgCold = 0, int iDmgPoison = 0, int iDmgEnergy = 0, HUE_TYPE effectHue = HUE_DEFAULT, SOUND_TYPE effectSound = SOUND_NONE);
	void OnHarmedBy( CChar * pCharSrc );
	bool OnAttackedBy( CChar * pCharSrc, bool fPetsCommand = false, bool fShouldReveal = true );

protected:
	virtual void _GoAwake() override final;
	virtual void _GoSleep() override final;

	virtual bool _CanTick(bool fParentGoingToSleep = false) const override final;

protected:	virtual bool _OnTick() override final;  // _OnTick timeout for skills, AI, etc
//public:	virtual bool  _OnTick() override final;

public:
	bool OnTickEquip( CItem * pItem );
	void OnTickFood( ushort uiVal, int HitsHungerLoss );

	virtual void OnTickStatusUpdate() override;
	bool OnTickPeriodic();  // Periodic tick calls (update stats, status bar, notoriety & attackers, death check, etc)

	void OnTickSkill(); // _OnTick timeout specific for the skill behavior

	static CChar * CreateBasic( CREID_TYPE baseID );
	static CChar * CreateNPC( CREID_TYPE id );
};


inline bool CChar::IsSkillBase( SKILL_TYPE skill ) noexcept // static
{
	// Is this in the base set of skills.
	return (skill > SKILL_NONE && skill < (SKILL_TYPE)(g_Cfg.m_iMaxSkill));
}

inline bool CChar::IsSkillNPC( SKILL_TYPE skill ) noexcept  // static
{
	// Is this in the NPC set of skills.
	return (skill >= NPCACT_FOLLOW_TARG && skill < NPCACT_QTY);
}


#endif // _INC_CCHAR_H
