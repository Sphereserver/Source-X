/**
* @file CServerConfig.h
*
*/

#ifndef _INC_CSERVERCONFIG_H
#define _INC_CSERVERCONFIG_H

#include "../common/sphere_library/CSAssoc.h"
#include "../common/resource/blocks/CSkillDef.h"
#include "../common/resource/blocks/CSpellDef.h"
#include "../common/resource/blocks/CWebPageDef.h"
#include "../common/resource/CResourceBase.h"
#include "../common/resource/CResourceRef.h"
#include "../common/resource/CResourceScript.h"
#include "../common/resource/CResourceSortedArrays.h"
#include "../common/resource/CValueDefs.h"
#include "../common/CExpression.h"
#include "../common/CServerMap.h"
#include "../common/sphereproto.h"
#include "CRegion.h"
#include "game_enums.h"


class CAccount;
class CClient;
class CServerDef;
using CServerRef = CServerDef*;


/**
 * @enum    OF_TYPE
 *
 * @brief   OptionFlags (sphere.ini)
 */
enum OF_TYPE
{
	OF_NoDClickTarget			= 0x0000001,    // Weapons won't open a target in the cursor after DClicking them for equip.
	OF_NoSmoothSailing			= 0x0000002,    // Deactivate Smooth Sailing for clients >= 7.0.8.13.
	OF_ScaleDamageByDurability	= 0x0000004,    // Weapons/armors will lose DAM/AR effect based on it's current durability.
	OF_Command_Sysmsgs			= 0x0000008,    // Shows status of hearall, allshow, allmove... commands after toggling them.
	OF_PetSlots					= 0x0000010,    // Enable AOS pet follower slots on chars. If enabled, all players must have MAXFOLLOWER property set (default=5).
	OF_OSIMultiSight			= 0x0000020,    // Only send items inside multis when the player enter on the multi area.
	OF_Items_AutoName			= 0x0000040,    // Auto rename potions/scrolls to match its spell name
	OF_FileCommands				= 0x0000080,    // Enable the usage of FILE commands
	OF_NoItemNaming				= 0x0000100,    // Disable the DEFMSG."grandmaster_mark" in crafted items
	OF_NoHouseMuteSpeech		= 0x0000200,    // Players outside multis won't hear what is told inside
	OF_NoContextMenuLOS			= 0x0000400,    // Disable LOS check to use context menus on chars
	OF_MapBoundarySailing		= 0x0000800,    // Boats will move to the other side of the map when reach map boundary
	OF_Flood_Protection			= 0x0001000,    // Prevent the server send messages to client if its the same message as the last already sent
	OF_Buffs					= 0x0002000,    // Enable the buff/debuff bar on ML clients >= 5.0.2b
	OF_NoPrefix					= 0x0004000,    // Don't show "a" and "an" prefix on item names
	OF_DyeType					= 0x0008000,    // Allow use i_dye on all items with t_dye_vat typedef instead only on i_dye_tub itemdef
	OF_DrinkIsFood				= 0x0010000,    // Typedef t_drink will increase food level like t_food
	OF_NoDClickTurn				= 0x0020000,    // Don't turn the player when DClick something
	OF_NoPaperdollTradeTitle	= 0x0040000,	// Don't show the trade title on the paperdoll
    OF_NoTargTurn				= 0x0080000,    // Don't turn the player when targetting something
    OF_StatAllowValOverMax      = 0x0100000,    // Allow stats value above their maximum value (i.e. allow hits value > maxhits).
    OF_GuardOutsideGuardedArea  = 0x0200000,    // Allow guards to walk in unguarded areas, instead of being teleported back to their home point.
    OF_OWNoDropCarriedItem      = 0x0400000     // When overweighted, don't drop items on ground when moving them (or using BOUNCE) and checking if you can carry them.
};

/**
 * @enum    EF_TYPE
 *
 * @brief   ExperimentalFlags (sphere.ini)
 */
enum EF_TYPE
{
	EF_NoDiagonalCheckLOS			= 0x0000001,    // Disable LOS checks on diagonal directions.
	EF_Dynamic_Backsave				= 0x0000002,    // This will enable, if necessary, if a backgroundsave tick is triggered to save more than only one Sector.
	EF_ItemStacking					= 0x0000004,    // Enable item stacking feature when drop items on ground.
	EF_ItemStackDrop				= 0x0000008,    // The item stack will drop when an item got removed from the stack.
	EF_FastWalkPrevention			= 0x0000010,    // Enable client fastwalk prevention (INCOMPLETE YET).
	EF_Intrinsic_Locals				= 0x0000020,    // Disables the needing of 'local.', 'tag.', etc. Be aware of not creating variables with the same name of already-existing functions.
	EF_Item_Strict_Comparison		= 0x0000040,    // Don't consider log/board and leather/hide as the same resource type.
	EF_AllowTelnetPacketFilter		= 0x0000200,    // Enable packet filtering for telnet connections as well.
	EF_Script_Profiler				= 0x0000400,    // Record all functions/triggers execution time statistics (it can be viewed pressing P on console).
	EF_DamageTools					= 0x0002000,    // Damage tools (and fire @damage on them) while mining or lumberjacking
	EF_UsePingServer				= 0x0008000,    // Enable the experimental Ping Server (for showing pings on the server list, uses UDP port 12000)
	EF_FixCanSeeInClosedConts		= 0x0020000,    // Change CANSEE to return 0 for items inside containers that a client hasn't opened
#ifndef _MTNETWORK
	EF_NetworkOutThread				= 0x0800000     //
#endif
};

///////////////////////////////////////


/**
 * @class   CServerConfig
 *
 * @brief   Script defined resources (not saved in world file) (sphere.ini)
 */
extern class CServerConfig : public CResourceBase
{
	//
	static const CAssocReg sm_szLoadKeys[];

public:
	static const char *m_sClassName;
	int64 m_timePeriodic; // When to perform the next periodic update

	// Begin INI file options.
	bool m_fUseNTService;       // Start this as a system service on Win2000, XP, NT
	int	 m_fUseHTTP;            // Use the built in http server
	bool m_fUseAuthID;          // Use the OSI AuthID to avoid possible hijack to game server.
	int64  m_iMapCacheTime;     // Time in sec to keep unused map data..
	int	 _iSectorSleepDelay;    // The mask for how long sectors will sleep.
	bool m_fUseMapDiffs;        // Whether or not to use map diff files.

	CSString m_sWorldBaseDir;   // save\" = world files go here.
	CSString m_sAcctBaseDir;    // Where do the account files go/come from ?

	bool m_fSecure;             // Secure mode. (will trap exceptions)
	int64  m_iFreezeRestartTime;  // # seconds before restarting.
#define DEBUGF_NPC_EMOTE		0x0001  // NPCs emote their actions.
#define DEBUGF_ADVANCE_STATS	0x0002  // prints stat % skill changes (only for _DEBUG builds).
#define DEBUGF_EXP				0x0200  // experience gain/loss.
#define DEBUGF_LEVEL			0x0400  // experience level changes.
#define	DEBUGF_SCRIPTS			0x0800  // debug flags for scripts.
#define DEBUGF_LOS				0x1000  // debug flags for AdvancedLOS.
#define DEBUGF_WALK				0x2000  // debug flags for Walking stuff.
#define DEBUGF_PACKETS			0x4000  // log packets to file.
#define DEBUGF_NETWORK			0x8000  // debug flags for networking.
	uint m_iDebugFlags;         // DEBUG In game effects to turn on and off.

	// Decay
	int64  m_iDecay_Item;         // Base decay time in minutes.
	int64  m_iDecay_CorpsePlayer; // Time in minutes for a playercorpse to decay.
	int64  m_iDecay_CorpseNPC;    // Time in minutes for a NPC corpse to decay.

	// Save
	int  m_iSaveNPCSkills;			// Only save NPC skills above this
	int64 m_iSavePeriod;			// Minutes between saves.
	int  m_iSaveBackupLevels;		// How many backup levels.
	int64 m_iSaveBackgroundTime;	// Speed of the background save in minutes.
	uint m_iSaveSectorsPerTick;		// max number of sectors per dynamic background save step
	uint m_iSaveStepMaxComplexity;	// maximum "number of items+characters" saved at once during dynamic background save
	bool m_fSaveGarbageCollect;		// Always force a full garbage collection.

	// Account
	int64 m_iDeadSocketTime;    // Disconnect inactive socket in x min.
	int	 m_iArriveDepartMsg;    // General switch to turn on/off arrival/depart messages.
	uint m_iClientsMax;         // Maximum (FD_SETSIZE) open connections to server
	int  m_iClientsMaxIP;       // Maximum (FD_SETSIZE) open connections to server per IP
	int  m_iConnectingMax;      // max clients connecting
	int  m_iConnectingMaxIP;    // max clients connecting

	int  m_iGuestsMax;          // Allow guests who have no accounts ?
	int64 m_iClientLingerTime;   // How long logged out clients linger in seconds.
	int64 m_iMinCharDeleteTime; // How old must a char be ? (seconds).
	byte m_iMaxCharsPerAccount; // Maximum characters allowed on an account.
	bool m_fLocalIPAdmin;       // The local ip is the admin ?
	bool m_fMd5Passwords;       // Should MD5 hashed passwords be used?
    uint8 _iMaxHousesAccount;   // Max houses per account.
    uint8 _iMaxHousesPlayer;    // Max houses per player.
    uint8 _iMaxShipsAccount;    // Max ships per account.
    uint8 _iMaxShipsPlayer;     // Max ships per player.
    uint8 _iMaxHousesGuild;     // Max houses per guild.
    uint8 _iMaxShipsGuild;      // Max ships per guild.

	// Magic
	bool m_fReagentsRequired;   // Do spells require reagents to be casted?
	int  m_iWordsOfPowerColor;  // Color used for Words Of Power.
	int  m_iWordsOfPowerFont;   // Font used for Words Of Power.
	bool m_fWordsOfPowerPlayer; // Words of Power for players.
	bool m_fWordsOfPowerStaff;  // Words of Power for staff.
	bool m_fEquippedCast;       // Allow casting while equipped.
    bool m_fManaLossFail;    // Lose mana when spell casting failed.
	bool m_fReagentLossFail;    // Lose reagents when spell casting failed.
	int  m_iMagicUnlockDoor;    // 1 in N chance of magic unlock working on doors -- 0 means never.
	ITEMID_TYPE m_iSpell_Teleport_Effect_NPC;       // ID of the item shown when a NPC teleports.
	SOUND_TYPE  m_iSpell_Teleport_Sound_NPC;        // Sound sent when a NPC teleports.
	ITEMID_TYPE m_iSpell_Teleport_Effect_Players;   // ID of the item shown when a Player teleports.
	SOUND_TYPE  m_iSpell_Teleport_Sound_Players;    // Sound sent when a Player teleports.
	ITEMID_TYPE m_iSpell_Teleport_Effect_Staff;     // ID of the item shown when a Staff member teleports.
	SOUND_TYPE  m_iSpell_Teleport_Sound_Staff;      // Sound sent when a Staff member teleports.
	int64 m_iSpellTimeout; // Timeout for spell targeting in seconds.

	// In Game Effects
	int	 m_iLightDungeon;			// InDungeon light level.
	int  m_iLightDay;				// Outdoor light level.
	int  m_iLightNight;				// Outdoor light level.
	int64  m_iGameMinuteLength;		// Length of the game world minute in (real world) seconds.
	bool m_fNoWeather;				// Turn off all weather.
	bool m_fCharTags;				// Show [NPC] tags over chars.
	bool m_fVendorTradeTitle;		// Show job title on vendor names.
	bool m_fFlipDroppedItems;		// Flip dropped items.
	int  m_iItemsMaxAmount;			// Max amount allowed for stackable items.
	bool m_fCanUndressPets;			// Can players undress their pets?
	bool m_fMonsterFight;			// Will creatures fight amoung themselves.
	bool m_fMonsterFear;			// will they run away if hurt ?
    uint m_iContainerMaxItems;      // Maximum number of items allowed in a container item.
	int	 m_iBankIMax;				// Maximum number of items allowed in bank.
	int  m_iBankWMax;				// Maximum weight in WEIGHT_UNITS stones allowed in bank.
	int  m_iVendorMaxSell;			// Max things a vendor will sell in one shot.
	uint m_iMaxCharComplexity;		// How many chars per sector.
	uint m_iMaxItemComplexity;		// How many items per meter.
	uint m_iMaxSectorComplexity;	// How many items per sector.
	bool m_fGenericSounds;			// Do players receive generic (not them-devoted) sounds.
    bool m_fAutoNewbieKeys;			// Are house and boat keys newbied automatically?
    bool _fAutoHouseKeys;			// Do houses create keys automatically?
    bool _fAutoShipKeys;			// Do ships create keys automatically?
	int  m_iStamRunningPenalty;		// Weight penalty for running (+N% of max carry weight)
	int  m_iStaminaLossAtWeight;	// %Weight at which characters begin to lose stamina.
	int  m_iHitpointPercentOnRez;	// How many hitpoints do they get when they are rez'd?
	int  m_iHitsHungerLoss;			// How many % of HP will loose char on starving.
	int  m_iMaxBaseSkill;			// Maximum value for base skills at char creation.
	int	 m_iTrainSkillCost;			// GP cost of each 0.1 skill trained.
	int	 m_iTrainSkillMax;			// Max skill value that can be reached from training.
	int  m_iTrainSkillPercent;		// How much can NPC's train up to ?
	int  m_fDeadCannotSeeLiving;	// Can dead Players see living beins?
	int  m_iMediumCanHearGhosts;	// At this Spirit Speak skill level players can understand ghosts speech instead hear 'oOOoO ooO'
	bool m_iMountHeight;			// Do not allow entering under roof being on horse?
	int	 m_iMoveRate;				// The percent rate of NPC movement.
	int  m_iArcheryMaxDist;			// Max distance allowed for archery.
	int  m_iArcheryMinDist;			// Min distance required for archery.
	int64 m_iHitsUpdateRate;			// how often (seconds) send my hits updates to visible clients..
	int  m_iCombatArcheryMovementDelay; // If COMBAT_ARCHERYCANMOVE is not enabled, wait this much tenth of seconds (minimum=0) after the player stopped moving before starting a new attack..
	int  m_iCombatDamageEra;		// define damage formula to use on physical combat.
	int  m_iCombatHitChanceEra;		// define hit chance formula to use on physical combat.
	int  m_iCombatSpeedEra;			// define swing speed formula to use on physical combat.
	int  m_iSpeedScaleFactor;		// fight skill delay = m_iSpeedScaleFactor / ( (dex + 100) * Weapon Speed ).
    uint m_iCombatParryingEra;     // Parrying behaviour flags

	int  m_iSkillPracticeMax;		// max skill level a player can practice on dummies/targets upto.
	bool m_iPacketDeathAnimation;	// packet 02c

	// Flags for controlling pvp/pvm behaviour of players
	uint m_iCombatFlags;   // combat flags
	uint m_iMagicFlags;    // magic flags
	uint m_iRacialFlags;   // racial traits flags
	uint m_iRevealFlags;   // reveal flags used for SPELL_REVEAL (mostly for backwards).

	// Criminal/Karma
	bool m_fAttackingIsACrime;		// Is attacking (even before hitting) a crime?
	bool m_fGuardsInstantKill;		// Will guards kill instantly or follow normal combat rules?
	bool m_fGuardsOnMurderers;		// should guards be only called on criminals ?
	int64 m_iGuardLingerTime;		// How many seconds do guards linger about.
	int  m_iSnoopCriminal;			// 1 in # chance of getting criminal flagged when succesfully snooping.
	bool m_iTradeWindowSnooping;	// 1 means opening a container in trade window needs to use snooping, 0 direct open.
	int  m_iMurderMinCount;			// amount of murders before we get title.
	int64 m_iMurderDecayTime;		// (minutes) Roll murder counts off this often.
	bool m_fHelpingCriminalsIsACrime;// If I help (rez, heal, etc) a criminal, do I become one too?
	bool m_fLootingIsACrime;		// Looting a blue corpse is bad.
	int64  m_iCriminalTimer;		// How many minutes are criminals flagged for?.
	int	 m_iPlayerKarmaNeutral;		// How much bad karma makes a player neutral?
	int	 m_iPlayerKarmaEvil;		// How much bad karma makes a player evil?
	int  m_iMinKarma;				// Minimum karma level
	int  m_iMaxKarma;				// Maximum karma level
	int  m_iMaxFame;				// Maximum fame level

	// other
	
    int  m_iAutoProcessPriority;
	uint m_iExperimentalFlags;	// Experimental Flags.
	uint m_iOptionFlags;		// Option Flags.
    bool m_fNoResRobe;          // Adding resurrection robe to resurrected players or not.
    int	 m_iLostNPCTeleport;    // if Distance from HOME is greater than this, NPC will teleport to it instead of walking.
	int64 m_iWoolGrowthTime;     // how long till wool grows back on sheared sheep, in minutes.
	uint m_iAttackerTimeout;    // Timeout for attacker.
	int64 m_iNotoTimeout;       // Timeout for NOTOriety checks.
	uint m_iMaxSkill;           // Records the higher [SKILL ] index.

	int	m_iDistanceYell;        // Max distance at which Yells can be readed.
	int	m_iDistanceWhisper;     // Max distance at which Whispers can be readed.
	int m_iDistanceTalk;        // Max distance at which Talking can be readed.

	CSString	m_sSpeechSelf;  // [SPEECH ] associated to players.
	CSString	m_sSpeechPet;   // [SPEECH ] associated to pets.
	CSString	m_sSpeechOther; // unused?
	CSString	m_sCommandTrigger;//Function to call if client is executing a command to override the default.

#ifdef _DUMPSUPPORT
	CSString	m_sDumpAccPackets;
#endif

	CSString            m_sEventsPet;			// Key to add Events to all pets.
	CResourceRefArray   m_pEventsPetLink;		// EventsPet.

	CSString            m_sEventsPlayer;		// Key to add Events to all players.
	CResourceRefArray   m_pEventsPlayerLink;	// EventsPlayer.

	CSString            m_sEventsRegion;		// Key to add Events to all regions.
	CResourceRefArray   m_pEventsRegionLink;	// EventsRegion.

	CSString            m_sEventsItem;			// Key to add Events to all items.
	CResourceRefArray   m_iEventsItemLink;		// EventsItem.

	// Third Party Tools
	CSString m_sStripPath;	// Strip Path for TNG and Axis.
	bool m_fCUOStatus;      // Enable or disable the response to ConnectUO pings
	bool m_fUOGStatus;      // Enable or disable the response to UOGateway pings

	int64 m_iWalkBuffer;	// Walk limiting code: buffer size (in tenths of second).
	int m_iWalkRegen;		// Walk limiting code: regen speed (%)

	int m_iCommandLog;		// Only commands issued by this plevel and higher will be logged
	bool m_fTelnetLog;		// Set to 1 to enable logging of commands issued via telnet

	bool m_fUsecrypt;		// Set this to 1 to allow login to encrypted clients
	bool m_fUsenocrypt;		// Set this to 1 to allow login to unencrypted clients

	bool m_fPayFromPackOnly;    // Pay only from main pack?
	int  m_iOverSkillMultiply;  // multiplyer to get over skillclass
	bool m_fSuppressCapitals;   // Enable/Disable capital letters suppression

#define ADVANCEDLOS_DISABLED		0x00
#define	ADVANCEDLOS_PLAYER			0x01
#define	ADVANCEDLOS_NPC				0x02
	int	m_iAdvancedLos;     // AdvancedLOS

	// New ones
	int	m_iFeatureT2A;		// T2A features.
	int	m_iFeatureLBR;		// LBR features.
	int	m_iFeatureAOS;		// AOS features.
	int	m_iFeatureSE;		// SE features.
	int	m_iFeatureML;		// ML features.
	int	m_iFeatureKR;		// KR features.
	int	m_iFeatureSA;		// SA features.
	int	m_iFeatureTOL;		// TOL features.
	int	m_iFeatureExtra;	// Extra features.

	int	m_iMaxLoopTimes;
#define	STAT_FLAG_NORMAL    0x00    //    MAX* status allowed (default)
#define STAT_FLAG_DENYMAX   0x01    //    MAX* denied
#define STAT_FLAG_DENYMAXP  0x02    //        .. for players
#define STAT_FLAG_DENYMAXN  0x04    //        .. for npcs
	uint m_iStatFlag;

#define NPC_AI_PATH				0x00001     // NPC pathfinding.
#define	NPC_AI_FOOD				0x00002     // NPC food search (objects + grass).
#define	NPC_AI_EXTRA			0x00004     // NPC magics, combat, etc.
#define NPC_AI_ALWAYSINT		0x00008     // NPC pathfinding does not check int, always smart.
#define NPC_AI_INTFOOD			0x00010     // NPC food search (more intelligent and trusworthy).
#define NPC_AI_COMBAT			0x00040     // Look for friends in combat.
#define NPC_AI_VEND_TIME		0x00080     // Vendors closing their shops at nighttime.
#define NPC_AI_LOOTING			0x00100     // Loot corpses an the way.
#define	NPC_AI_MOVEOBSTACLES	0x00200     // If moveable items block my way, try to move them.
#define NPC_AI_PERSISTENTPATH	0x00400     // NPC will try often to find a path with pathfinding.
#define NPC_AI_THREAT			0x00800     // Enable the use of the threat variable when finding for target while fighting.
	uint m_iNpcAi;      // NPCAI Flags.

	//	Experience system
	bool m_bExperienceSystem;   // Enables the experience system.
#define EXP_MODE_RAISE_COMBAT   0x0001  // Gain experience in combat.
#define	EXP_MODE_RAISE_CRAFT    0x0002  // Gain experience in crafts.
#define	EXP_MODE_ALLOW_DOWN     0x0004  // Allow experience to go down.
#define	EXP_MODE_DOWN_NOLEVEL   0x0008  // Limit experience decrease by a range witheen a current level.
#define	EXP_MODE_AUTOSET_EXP    0x0010  // Auto-init EXP/LEVEL for NPCs if not set in @Create.
	int  m_iExperienceMode;     // Experience system settings:
	int  m_iExperienceKoefPVM;  // If combat experience gain is allowed, use these percents for gaining exp in Player versus Monster. 0 Disables the gain.
	int  m_iExperienceKoefPVP;  // If combat experience gain is allowed, use these percents for gaining exp in Player versus Player. 0 Disables the gain.
	bool m_bLevelSystem;        // Enable levels system (as a part of experience system).
#define LEVEL_MODE_LINEAR   0   // (each NextLevelAt exp will give a level up).
#define	LEVEL_MODE_DOUBLE   1   // (you need (NextLevelAt * (level+1)) to get a level up).
	int  m_iLevelMode;          // Level system settings
	uint m_iLevelNextAt;        // Amount of experience to raise to the next level.

	bool m_bAutoResDisp;        // Set account RESDISP automatically based on player client version.
	int  m_iAutoPrivFlags;      // Default setting for new accounts specifying default priv level.

	char m_cCommandPrefix;      // Prefix for ingame commands (Default = '.').

	int m_iDefaultCommandLevel;// Set from 0 - 7 to set what the default plevel is to use commands.

	//	color noto flag
	HUE_TYPE m_iColorNotoGood;          // Blue
	HUE_TYPE m_iColorNotoGuildSame;     // Green
	HUE_TYPE m_iColorNotoNeutral;       // Grey
	HUE_TYPE m_iColorNotoCriminal;      // Grey
	HUE_TYPE m_iColorNotoGuildWar;      // Orange
	HUE_TYPE m_iColorNotoEvil;          // Red
	HUE_TYPE m_iColorNotoInvul;         // Yellow
	HUE_TYPE m_iColorNotoInvulGameMaster;// Purple
	HUE_TYPE m_iColorNotoDefault;       // Grey

	HUE_TYPE m_iColorInvis;     // 04001 = transparent color, 0 = default
	HUE_TYPE m_iColorInvisSpell;// 04001 = transparent color, 0 = default (This one is for s_invisibility spell, this includes the invis potion.)
	HUE_TYPE m_iColorHidden;    // 04001 = transparent color, 0 = default

	// notoriety inheritance
	int m_iPetsInheritNotoriety;// Which notoriety flags do pets inherit from their masters? (default 0).

	int m_iClientLoginMaxTries; // Max wrong password tries on client login screen before temporary ban client IP (0 is disabled).
	int64 m_iClientLoginTempBan;  // Duration (in minutes) of temporary ban to client IPs that reach max wrong password tries.
	int m_iMaxShipPlankTeleport;// How far from land i can be to take off a ship.

	//	MySQL features
	bool    m_bMySql;       // Enables MySQL.
	bool    m_bMySqlTicks;  // Enables ticks from MySQL.
	CSString m_sMySqlHost;  // MySQL Host.
	CSString m_sMySqlUser;  // MySQL User.
	CSString m_sMySqlPass;  // MySQL Password.
	CSString m_sMySqlDB;    // MySQL DB.

	// network settings
#ifdef _MTNETWORK
	uint m_iNetworkThreads;         // number of network threads to create
	uint m_iNetworkThreadPriority;  // priority of network threads
#endif
	int	 m_fUseAsyncNetwork;        // 0=normal send, 1=async send, 2=async send for 4.0.0+ only
	int	 m_iNetMaxPings;            // max pings before blocking an ip
	int	 m_iNetHistoryTTL;          // time to remember an ip
	int	 m_iNetMaxPacketsPerTick;   // max packets to send per tick (per queue)
	uint m_iNetMaxLengthPerTick;    // max packet length to send per tick (per queue) (also max length of individual packets)
	int	 m_iNetMaxQueueSize;        // max packets to hold per queue (comment out for unlimited)
	bool m_fUsePacketPriorities;    // true to prioritise sending packets
	bool m_fUseExtraBuffer;         // true to queue packet data in an extra buffer

	int64 m_iTooltipCache;            // time in seconds to cache tooltip for.
	int	m_iTooltipMode;             // tooltip mode (TOOLTIP_TYPE)
	int	m_iContextMenuLimit;        // max amount of options per context menu

	int64   m_iRegenRate[STAT_QTY]; // Regen's delay for each stat (in seconds in the ini, then converted to msecs).
    int64   _iItemHitpointsUpdate;  // Update period for CCItemDamageable (in seconds in the ini, then converted to msecs).
	int64   _iTimerCall;            // Amount of minutes (converted to milliseconds internally) to call f_onserver_timer (0 disables this, default).
	bool    m_bAllowLightOverride;  // Allow manual sector light override?
	CSString m_sZeroPoint;          // Zero point for sextant coordinates counting. Comment this line out if you are not using ML-sized maps.
	bool    m_bAllowBuySellAgent;   // Allow rapid Buy/Sell through Buy/Sell agent.

	bool    m_bAllowNewbTransfer;   // Set to 1 for items to keep their attr_newbie flag when item is transfered to an NPC.

	bool    m_NPCNoFameTitle;       // NPC will not be addressed as "Lord" or such if this is set.

	bool    m_bAgree;               // AGREE=n for nightly builds.
	int     m_iMaxPolyStats;        // Max amount of each Stat gained through Polymorph spell. This affects separatelly to each stat.

	// End INI file options.

	CResourceScript m_scpIni;       // Keep this around so we can link to it.
	CResourceScript m_scpCryptIni;  // Encryption keys are in here

public:
	CResourceScript m_scpTables;        // Script's loaded.

	CSStringSortArray m_ResourceList;   // Sections lists.

	CSStringSortArray       m_Obscene;  // Bad Names/Words etc.
	CSObjArray< CSString* > m_Fame;     // fame titles (fame.famous).
	CSObjArray< CSString* > m_Karma;    // karma titles (karma.wicked).
	CSObjArray< CSString* > m_Runes;    // Words of power (A-Z).

	CSTypedArray<int> m_NotoKarmaLevels;		// karma levels for noto titles.
	CSTypedArray<int> m_NotoFameLevels;		// fame levels for noto titles.
	CSObjArray< CSString* >  m_NotoTitles;			// Noto titles.

	CMultiDefArray m_MultiDefs;		// read from the MUL files. Cached here on demand.

	CObjNameSortArray            m_SkillNameDefs;		// const CSkillDef* Name sorted.
	CSPtrTypeArray< CSkillDef* > m_SkillIndexDefs;		// Defined Skills indexed by number.
	CSObjArray< CSpellDef* >     m_SpellDefs;			// Defined Spells.
	CSPtrTypeArray< CSpellDef* > m_SpellDefs_Sorted;	// Defined Spells, in skill order.

	CSStringSortArray m_PrivCommands[PLEVEL_QTY];		// what command are allowed for a priv level?

public:
	CObjNameSortArray m_Servers;	// Servers list. we act like the login server with this.
	CObjNameSortArray m_Functions;	// Subroutines that can be used in scripts.
	CRegionLinks m_RegionDefs;		// All [REGION ] stored inside.

	// static definition stuff from *TABLE.SCP mostly.
	CSObjArray< const CStartLoc* > m_StartDefs;			// Start points list
	CValueCurveDef m_StatAdv[STAT_BASE_QTY];			// "skill curve"
	CSTypedArray<CPointMap> m_MoonGates;	// The array of moongates.

	CResourceHashArray m_WebPages;		// These can be linked back to the script.

private:
	CResourceID ResourceGetNewID( RES_TYPE restype, lpctstr pszName, CVarDefContNum ** ppVarNum, bool fNewStyleDef );

public:
	CServerConfig();
	virtual ~CServerConfig();

private:
	CServerConfig(const CServerConfig& copy);
	CServerConfig& operator=(const CServerConfig& other);

public:
	virtual bool r_LoadVal( CScript &s ) override;
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc ) override;
	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef ) override;

    /**
     * @fn  bool CServerConfig::LoadIni( bool fTest );
     *
     * @brief   Loads sphere.ini.
     *
     * @param   fTest   true to test.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool LoadIni( bool fTest );

    /**
     * @fn  bool CServerConfig::LoadCryptIni( void );
     *
     * @brief   Loads sphereCrypt.ini.
     *
     * @author  Javier
     * @date    09/05/2016
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool LoadCryptIni( void );

    /**
     * @fn  bool CServerConfig::Load( bool fResync );
     *
     * @brief   Loads or resync scripts..
     *
     * @param   fResync Resync or normal load?.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool Load( bool fResync );

    /**
     * @fn  void CServerConfig::Unload( bool fResync );
     *
     * @brief   Unloads scripts and resources.
     *
     * @param   fResync true to resync.
     */
	void Unload( bool fResync );
	void OnTick( bool fNow );

    /**
     * @fn  bool CServerConfig::LoadResourceSection( CScript * pScript );
     *
     * @brief   Loads resource section ([SKILL ], [SPELL ], [CHARDEF ]...).
     *
     * @param [in,out]  pScript If non-null, the script.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool LoadResourceSection( CScript * pScript );

    /**
     * @fn  void CServerConfig::LoadSortSpells();
     *
     * @brief   Sort all spells in order.
     */
	void LoadSortSpells();

    /**
     * @fn  CResourceDef * CServerConfig::ResourceGetDef( RESOURCE_ID_BASE rid ) const;
     *
     * @brief   Get a CResourceDef from the RESOURCE_ID.
     *
     * @param   rid The rid.
     *
     * @return  null if it fails, else a pointer to a CResourceDef.
     */
	CResourceDef * ResourceGetDef( const CResourceID& rid ) const;

	// Print EF/OF Flags
	void PrintEFOFFlags( bool bEF = true, bool bOF = true, CTextConsole *pSrc = nullptr );

	// ResDisp Flag
	uint GetPacketFlag( bool bCharlist, RESDISPLAY_VERSION res = RDS_T2A, uchar chars = 5 );

    /**
     * @fn  bool CServerConfig::CanUsePrivVerb( const CScriptObj * pObjTarg, lpctstr pszCmd, CTextConsole * pSrc ) const;
     *
     * @brief   Can i use this verb on this object ?
     *
     * @param   pObjTarg        The object targ.
     * @param   pszCmd          The command.
     * @param [in,out]  pSrc    If non-null, source for the.
     *
     * @return  true if we can use priv verb, false if not.
     */
	bool CanUsePrivVerb( const CScriptObj * pObjTarg, lpctstr pszCmd, CTextConsole * pSrc ) const;

    /**
     * @fn  PLEVEL_TYPE CServerConfig::GetPrivCommandLevel( lpctstr pszCmd ) const;
     *
     * @brief   Gets this command's priv level.
     *
     * @param   pszCmd  The command.
     *
     * @return  The priv command level.
     */
	PLEVEL_TYPE GetPrivCommandLevel( lpctstr pszCmd ) const;

    /**
     * @fn  static STAT_TYPE CServerConfig::FindStatKey( lpctstr pszKey );
     *
     * @brief   Returns the Stat key from the given string( STR = STAT_STR... ).
     *
     * @param   pszKey  The key.
     *
     * @return  The found stat key.
     */
	static STAT_TYPE FindStatKey( lpctstr pszKey );

    /**
     * @fn  static bool CServerConfig::IsValidEmailAddressFormat( lpctstr pszText );
     *
     * @brief   Query if 'pszText' is valid email address format.
     *
     * @param   pszText The text.
     *
     * @return  true if valid email address format, false if not.
     */
	static bool IsValidEmailAddressFormat( lpctstr pszText );

    /**
     * @fn  bool CServerConfig::IsObscene( lpctstr pszText ) const;
     *
     * @brief   Query if 'pszText' is obscene.
     *
     * @param   pszText The text.
     *
     * @return  true if obscene, false if not.
     */
	bool IsObscene( lpctstr pszText ) const;

    /**
     * @fn  CWebPageDef * CServerConfig::FindWebPage( lpctstr pszPath ) const;
     *
     * @brief   Searches for the requested web page.
     *
     * @param   pszPath Full pathname of the file.
     *
     * @return  null if it fails, else the found web page.
     */
	CWebPageDef * FindWebPage( lpctstr pszPath ) const;

	/**
	* @fn  CServerRef CServerConfig::Server_GetDef( size_t index );
	*
	* @brief   Get the def for the server in position 'index' in servers list.
	*
	* @param   pszText The server index.
	*
	* @return  CServerRef of the server.
	*/
	CServerRef Server_GetDef( size_t index );

    /**
     * @fn  const CSpellDef * CServerConfig::GetSpellDef( SPELL_TYPE index ) const
     *
     * @brief   Gets spell definition.
     *
     * @param   index   Zero-based index of the.
     *
     * @return  null if it fails, else the spell definition.
     */
	const CSpellDef * GetSpellDef( SPELL_TYPE index ) const;

    /**
    * @fn  const CSpellDef * CServerConfig::GetSpellDef( SPELL_TYPE index ) const
    *
    * @brief   Gets spell definition.
    *
    * @param   index   Zero-based index of the.
    *
    * @return  null if it fails, else the spell definition.
    */
	CSpellDef * GetSpellDef( SPELL_TYPE index );

    /**
     * @fn  lpctstr CServerConfig::GetSkillKey( SKILL_TYPE index ) const
     *
     * @brief   Gets skill key.
     *
     * @param   index   Zero-based index of the.
     *
     * @return  The skill key.
     */
	lpctstr GetSkillKey( SKILL_TYPE index ) const;

    /**
     * @fn  bool CServerConfig::IsSkillFlag( SKILL_TYPE index, SKF_TYPE skf ) const
     *
     * @brief   Query if 'index' has 'skf' skill flag.
     *
     * @param   index   Zero-based index of the.
     * @param   skf     The skill flag.
     *
     * @return  true if skill flag, false if not.
     */
	bool IsSkillFlag( SKILL_TYPE index, SKF_TYPE skf ) const
	{
		const CSkillDef * pSkillDef = GetSkillDef( index );
		return ( pSkillDef && (pSkillDef->m_dwFlags & skf) );
	}

    /**
     * @fn  const CSkillDef* CServerConfig::GetSkillDef( SKILL_TYPE index ) const
     *
     * @brief   Gets skill definition.
     *
     * @param   index   Zero-based index of the.
     *
     * @return  null if it fails, else the skill definition.
     */
	const CSkillDef* GetSkillDef( SKILL_TYPE index ) const;

    /**
    * @fn  const CSkillDef* CServerConfig::GetSkillDef( SKILL_TYPE index ) const
    *
    * @brief   Gets skill definition.
    *
    * @param   index   Zero-based index of the.
    *
    * @return  null if it fails, else the skill definition.
    */
	CSkillDef* GetSkillDef( SKILL_TYPE index );

    /**
     * @fn  const CSkillDef* CServerConfig::FindSkillDef( lpctstr pszKey ) const
     *
     * @brief   Find the skill name in the alpha sorted list.
     *
     * @param   pszKey  The key.
     *
     * @return  null if it fails, else the found skill definition.
     */
	const CSkillDef* FindSkillDef( lpctstr pszKey ) const;

    /**
     * @fn  const CSkillDef* CServerConfig::SkillLookup( lpctstr pszKey );
     *
     * @brief   Search for the skill which NAME=pszKey.
     *
     * @param   pszKey  The key.
     *
     * @return  null if it fails, else a pointer to a const CSkillDef.
     */
	const CSkillDef* SkillLookup( lpctstr pszKey );

    /**
     * @fn  SKILL_TYPE CServerConfig::FindSkillKey( lpctstr pszKey ) const;
     *
     * @brief   Search for the skill which KEY=pszKey.
     *
     * @param   pszKey  The key.
     *
     * @return  The found skill key.
     */
	SKILL_TYPE FindSkillKey( lpctstr pszKey ) const;

    /**
     * @fn  int CServerConfig::GetSpellEffect( SPELL_TYPE spell, int iSkillval ) const;
     *
     * @brief   Gets the Damage/Healing/etc done by the given params.
     *
     * @param   spell       The spell.
     * @param   iSkillval   Skill value (decimal).
     *
     * @return  The spell effect.
     */
	int GetSpellEffect( SPELL_TYPE spell, int iSkillval ) const;

    /**
     * @fn  lpctstr CServerConfig::GetRune( tchar ch ) const
     *
     * @brief   Retrieves a Spell's rune (WOP).
     *
     * @param   ch  The ch.
     *
     * @return  The rune.
     */
	lpctstr GetRune( tchar ch ) const
	{
		size_t index = (size_t)(toupper(ch) - 'A');
		if ( ! m_Runes.IsValidIndex(index))
			return "?";
		return( m_Runes[index]->GetPtr() );
	}

    /**
     * @fn  lpctstr CServerConfig::GetNotoTitle( int iLevel, bool bFemale ) const;
     *
     * @brief   Gets noto title.
     *
     * @param   iLevel  Zero-based index of the level.
     * @param   bFemale true to female.
     *
     * @return  The noto title.
     */
	lpctstr GetNotoTitle( int iLevel, bool bFemale ) const;

	const CSphereMulti * GetMultiItemDefs( CItem * pItem );
	const CSphereMulti * GetMultiItemDefs( ITEMID_TYPE itemid );

    /**
     * @fn  bool CServerConfig::IsConsoleCmd( tchar ch ) const;
     *
     * @brief   Query if 'ch' is a console command or ingame one.
     *
     * @param   ch  The ch.
     *
     * @return  true if console command, false if not.
     */
	bool IsConsoleCmd( tchar ch ) const;

	CPointMap GetRegionPoint( lpctstr pCmd ) const; // Decode a teleport location number into X/Y/Z
	CRegion * GetRegion( lpctstr pKey ) const; // Find a region with the given name/defname

    /**
     * @fn  int CServerConfig::Calc_MaxCarryWeight( const CChar * pChar ) const;
     *
     * @brief   Calculates the maximum carry weight.
     *
     * @param   pChar   The character.
     *
     * @return  The calculated maximum carry weight.
     */
	int Calc_MaxCarryWeight( const CChar * pChar ) const;

    /**
     * @fn  int CServerConfig::Calc_CombatAttackSpeed( CChar * pChar, CItem * pWeapon );
     *
     * @brief   Calculates the combat attack speed.
     *
     * @param [in,out]  pChar   If non-null, the character.
     * @param [in,out]  pWeapon If non-null, the weapon.
     *
     * @return  The calculated combat attack speed.
     */
	int Calc_CombatAttackSpeed( const CChar * pChar, const CItem * pWeapon ) const;

    /**
     * @fn  int CServerConfig::Calc_CombatChanceToHit( CChar * pChar, CChar * pCharTarg, SKILL_TYPE skill );
     *
     * @brief   Calculates the combat chance to hit.
     *
     * @param [in,out]  pChar       If non-null, the character.
     * @param [in,out]  pCharTarg   If non-null, the character targ.
     * @param   skill               The skill.
     *
     * @return  The calculated combat chance to hit.
     */
	int Calc_CombatChanceToHit( CChar * pChar, CChar * pCharTarg);

    /**
     * @fn  int CServerConfig::Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark );
     *
     * @brief   Chance to steal and retrieve the item successfully
     *
     * @param [in,out]  pCharThief  If non-null, the character thief.
     * @param [in,out]  pItem       If non-null, the item.
     * @param [in,out]  pCharMark   If non-null, the character mark.
     *
     * @return  The calculated stealing item.
     */
	int  Calc_StealingItem( CChar * pCharThief, CItem * pItem, CChar * pCharMark );

    /**
     * @fn  bool CServerConfig::Calc_CrimeSeen( CChar * pCharThief, CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus );
     *
     * @brief   Chance to steal without being seen by a specific person.
     *
     * @param [in,out]  pCharThief  If non-null, the character thief.
     * @param [in,out]  pCharViewer If non-null, the character viewer.
     * @param   SkillToSee          The skill to see.
     * @param   fBonus              true to bonus.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool Calc_CrimeSeen( const CChar * pCharThief, const CChar * pCharViewer, SKILL_TYPE SkillToSee, bool fBonus ) const;

    /**
     * @fn  ushort CServerConfig::Calc_FameKill( CChar * pKill );
     *
     * @brief   Calculates the fame given by the kill.
     *
     * @param [in,out]  pKill   If non-null, the kill.
     *
     * @return  The calculated fame kill.
     */
	int Calc_FameKill( CChar * pKill );

    /**
     * @fn  int CServerConfig::Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem );
     *
     * @brief   Calculates the karma given or lost by the kill.
     *
     * @param [in,out]  pKill   If non-null, the kill.
     * @param   NotoThem        The noto them.
     *
     * @return  The calculated karma kill.
     */
	int Calc_KarmaKill( CChar * pKill, NOTO_TYPE NotoThem );

    /**
     * @fn  int CServerConfig::Calc_KarmaScale( int iKarma, int iKarmaChange );
     *
     * @brief   Scale the karma based on the current level, Should be harder to gain karma than to loose it.
     *
     * @param   iKarma          Zero-based index of the karma.
     * @param   iKarmaChange    Zero-based index of the karma change.
     *
     * @return  The calculated karma scale.
     */
	int Calc_KarmaScale( int iKarma, int iKarmaChange );

    /**
     * @fn  lpctstr CServerConfig::Calc_MaptoSextant( CPointMap pntCoords );
     *
     * @brief   Translates map coords to sextant coords.
     *
     * @author  Javier
     * @date    09/05/2016
     *
     * @param   pntCoords   The point coordinates.
     *
     * @return  The calculated mapto sextant.
     */
	lpctstr Calc_MaptoSextant( CPointMap pntCoords );

#define SysMessageDefault( msg )	SysMessage( g_Cfg.GetDefaultMsg( msg ) )

    /**
     * @fn  lpctstr CServerConfig::GetDefaultMsg(lpctstr pszKey);
     *
     * @brief   Gets default message (sphere_msgs.scp).
     *
     * @param   pszKey  The key.
     *
     * @return  The default message.
     */
	lpctstr GetDefaultMsg(lpctstr pszKey);

    /**
    * @fn  lpctstr CServerConfig::GetDefaultMsg(lpctstr pszKey);
    *
    * @brief   Gets default message (sphere_msgs.scp).
    *
    * @param   pszKey  The key.
    *
    * @return  The default message.
    */
	lpctstr	GetDefaultMsg(int lKeyNum);

typedef std::map<dword,dword> KRGumpsMap;
	KRGumpsMap m_mapKRGumps;

	bool SetKRDialogMap(dword rid, dword idKRDialog);
	dword GetKRDialogMap(dword idKRDialog);
	dword GetKRDialog(dword rid);

	bool GenerateDefname(tchar *pObjectName, size_t iInputLength, lpctstr pPrefix, tchar *pOutput, bool bCheckConflict = true, CVarDefMap* vDefnames = nullptr);
	bool DumpUnscriptedItems(CTextConsole * pSrc, lpctstr pszFilename);
} g_Cfg;



#define IsSetEF(ef)				((g_Cfg.m_iExperimentalFlags & ef) != 0)
#define IsSetOF(of)				((g_Cfg.m_iOptionFlags & of) != 0)
#define IsSetCombatFlags(of)	((g_Cfg.m_iCombatFlags & of) != 0)
#define IsSetMagicFlags(of)		((g_Cfg.m_iMagicFlags & of) != 0)


#endif	// _INC_CSERVERCONFIG_H
