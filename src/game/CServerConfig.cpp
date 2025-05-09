#include "../common/resource/sections/CDialogDef.h"
#include "../common/resource/sections/CItemTypeDef.h"
#include "../common/resource/sections/CSkillClassDef.h"
#include "../common/resource/sections/CRandGroupDef.h"
#include "../common/resource/sections/CRegionResourceDef.h"
#include "../common/resource/sections/CResourceNamedDef.h"
#include "../common/sphere_library/CSRand.h"
#include "../common/CException.h"
#include "../common/CExpression.h"
#include "../common/CUOInstall.h"
#include "../common/sphereversion.h"
#include "../network/CClientIterator.h"
#include "../network/CNetworkManager.h"
#include "../network/CSocket.h"
#include "../sphere/ProfileTask.h"
#include "clients/CAccount.h"
#include "clients/CClient.h"
#include "chars/CChar.h"
#include "chars/CCharBase.h"
#include "items/CItemBase.h"
#include "items/CItemStone.h"
#include "items/CItemMulti.h"
#include "items/CItemMultiCustom.h"
#include "components/CCChampion.h"
#include "uo_files/CUOItemInfo.h"
#include "uo_files/CUOTerrainInfo.h"
#include "CServer.h"
#include "CServerTime.h"
#include "CWorldTimedFunctions.h"
#include "CWorld.h"
#include "CWorldGameTime.h"
#include "CWorldMap.h"
#include "spheresvr.h"
#include "triggers.h"

#ifdef _WIN32
#   include "../sphere/ntwindow.h"     // g_NTWindow
#endif

#include <cstddef>
#ifndef OFFSETOF
//#   define OFFSETOF(TYPE, ELEMENT) (offsetof(TYPE, ELEMENT))
# define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
#endif


// .ini settings.
CServerConfig::CServerConfig()
{
    m_iniDirectory[0] = '\0';

	m_timePeriodic = 0;

	m_fUseNTService		= false;
	m_fUseHTTP			= 2;
	m_fUseAuthID		= true;
	_iMapCacheTime		= 2ll  * 60 * MSECS_PER_SEC;
	_iSectorSleepDelay  = 10ll * 60 * MSECS_PER_SEC;
	m_fUseMapDiffs		= false;
    m_fUseMobTypes      = false;

	m_iDebugFlags			= 0;	//DEBUGF_NPC_EMOTE
	m_fSecure				= true;
	m_iFreezeRestartTime	= 60;
	m_fAgree				= false;
	m_fMd5Passwords			= false;
    m_fDecimalVariables     = false; // In default, variables should return hexadecimal.

	//Magic
	m_fManaLossAbort		= false;
    m_fManaLossFail		    = false;
	m_fManaLossPercent		= 50;
	m_fNPCCanFizzleOnHit	= false;
	m_iNPCHealthreshold		= 30;
	m_fReagentLossAbort		= false;
	m_fReagentLossFail		= false;
    m_fReagentsRequired		= false;
	m_iWordsOfPowerColor	= HUE_TEXT_DEF;
	m_iWordsOfPowerFont		= FONT_NORMAL;
	m_fWordsOfPowerPlayer	= true;
	m_fWordsOfPowerStaff	= false;
    m_iWordsOfPowerTalkMode = TALKMODE_SPELL;
	m_fEquippedCast			= true;
	m_iMagicUnlockDoor		= 900;
	m_iSpellTimeout			= 0;

	m_iSpell_Teleport_Effect_Staff		= ITEMID_FX_FLAMESTRIKE;	// drama
	m_iSpell_Teleport_Sound_Staff		= 0x1f3;
	m_iSpell_Teleport_Effect_Players	= ITEMID_FX_TELE_VANISH;
	m_iSpell_Teleport_Sound_Players		= 0x01fe;
	m_iSpell_Teleport_Effect_NPC		= ITEMID_FX_HEAL_EFFECT;
	m_iSpell_Teleport_Sound_NPC			= 0x01fe;

	// Decay
	m_iDecay_Item			= 30ll * 60 * MSECS_PER_SEC;
	m_iDecay_CorpsePlayer	= 7ll * 60 * MSECS_PER_SEC;
	m_iDecay_CorpseNPC		= 7ll * 60 * MSECS_PER_SEC;
    m_uiItemTimers = 0;

	// Accounts
	m_iClientsMax		= FD_SETSIZE-1;
	m_iClientsMaxIP		= 16;
	m_iConnectingMax	= 32;
	m_iConnectingMaxIP	= 8;

	m_iGuestsMax			= 0;
	m_iArriveDepartMsg		= 1;
	m_iClientLingerTime		= 10ll * 60 * MSECS_PER_SEC;
	m_iDeadSocketTime		= 5ll * 60 * MSECS_PER_SEC;
	m_iMinCharDeleteTime	= 7ll * 24*60*60 * MSECS_PER_SEC;
	m_iMaxCharsPerAccount	= 5;
	m_fLocalIPAdmin			= true;
    _iMaxHousesAccount = 1;
    _iMaxHousesGuild = 1;
    _iMaxHousesPlayer = 1;
    _iMaxShipsAccount = 1;
    _iMaxShipsGuild = 1;
    _iMaxShipsPlayer = 1;

	// Save
	m_iSaveNPCSkills			= 10;
	m_iSaveBackupLevels			= 10;
	m_iSaveBackgroundTime		= 0;		// Use the new background save.
	m_fSaveGarbageCollect		= true;		// Always force a full garbage collection.
	m_iSavePeriod				= 20ll * 60 * MSECS_PER_SEC;
	m_iSaveSectorsPerTick		= 1;
	m_iSaveStepMaxComplexity	= 500;

	// In game effects.
	m_fCanUndressPets		= true;
    m_fCanPetsDrinkPotion   = true;
	m_fMonsterFight			= false;
	m_fMonsterFear			= false;
	m_iLightDungeon			= 27;
	m_iLightNight			= 25;	// dark before t2a.
	m_iLightDay				= LIGHT_BRIGHT;
    m_iContainerMaxItems    = MAX_ITEMS_CONT;
	m_iBackpackOverload		= 40 * WEIGHT_UNITS;
	m_iBankIMax				= 1000;
	m_iBankWMax				= 1000 * WEIGHT_UNITS;
	m_fAttackingIsACrime	= true;
	m_fGuardsInstantKill	= true;
	m_fGuardsOnMurderers	= true;
	m_iSnoopCriminal		= 100;
	m_iTradeWindowSnooping	= true;
	m_NPCShoveNPC			= false;
	m_iTrainSkillCost		= 1;
	m_iTrainSkillMax		= 420;
	m_iTrainSkillPercent	= 30;
	m_fDeadCannotSeeLiving	= 0;
	m_iMediumCanHearGhosts	= 1000;
	m_iSkillPracticeMax		= 300;
	m_iPacketDeathAnimation = true;
	m_fCharTags				= false;
	m_fVendorTradeTitle		= true;
	m_iVendorMaxSell		= 255;
	m_iVendorMarkup			= 15;
	m_iGameMinuteLength		= 20ll * MSECS_PER_SEC; // 20 seconds
	m_fNoWeather			= true;
	m_fFlipDroppedItems		= true;
	m_iItemsMaxAmount		= 60000;
	m_iMurderMinCount		= 5;
	m_iMurderDecayTime		= 8ll * 60 * 60 * MSECS_PER_SEC;
	m_iMaxCharComplexity	= 32;
	m_iMaxItemComplexity	= 25;
	m_iMaxSectorComplexity	= 1024;
	m_iPlayerKarmaNeutral	= -2000; // How much bad karma makes a player neutral?
	m_iPlayerKarmaEvil		= -8000;
	m_iMinKarma				= -10000;
	m_iMaxKarma				= 10000;
	m_iMaxFame				= 10000;
	m_iGuardLingerTime		= 3ll *60 * MSECS_PER_SEC;
	m_iCriminalTimer		= 3ll *60 * MSECS_PER_SEC;
	m_iHitpointPercentOnRez	= 10;
	m_iHitsHungerLoss		= 0;
	m_fLootingIsACrime		= true;
	m_fHelpingCriminalsIsACrime	= true;
	m_fGenericSounds		= true;
	m_fAutoNewbieKeys 		= true;
    _fAutoHouseKeys         = true;
    _fAutoShipKeys          = true;
	m_iMaxBaseSkill			= 200;
	m_iStamRunningPenalty 	= 50;
	m_iStamRunningPenaltyOverweight = 100;
	m_iStaminaLossAtWeight 	= 150;
	m_iStaminaLossOverweight = 5;
	m_iMountHeight			= false;
	m_iMoveRate				= 100;
	m_iArcheryMinDist		= 2;
	m_iArcheryMaxDist		= 15;
	m_iHitsUpdateRate		= MSECS_PER_SEC;
	m_iSpeedScaleFactor		= 80000;
	m_iCombatFlags			= 0;
	m_iCombatArcheryMovementDelay = 10;
	m_iCombatDamageEra		= 0;
	m_iCombatHitChanceEra	= 0;
	m_iCombatSpeedEra		= 3;
    m_iCombatParryingEra    = 0x1|0x10;
	m_iMagicFlags			= 0;
	m_iMaxPolyStats			= 150;
	m_iRacialFlags			= 0;
	m_iRevealFlags			= (REVEALF_DETECTINGHIDDEN|REVEALF_LOOTINGSELF|REVEALF_LOOTINGOTHERS|REVEALF_SPEAK|REVEALF_SPELLCAST);
	m_iEmoteFlags			= 0;
	m_fDisplayPercentAr = false;
	m_fDisplayElementalResistance = false;
	m_fNoResRobe		= 0;
    m_iBounceMessage        = false;
	m_iLostNPCTeleport	= 50;
	m_iAutoProcessPriority = 0;
	m_iDistanceYell		= UO_MAP_VIEW_RADAR;
	m_iDistanceWhisper	= 3;
	m_iDistanceTalk		= UO_MAP_VIEW_SIZE_DEFAULT;
	m_iDragWeightMax = 300;
    m_iNPCDistanceHear  = UO_MAP_VIEW_SIGHT; // Why it was 4? Default range must match with the default value under CClient::Event_Talk_Common.
	_uiExperimentalFlags= 0;
	_uiOptionFlags		= (OF_Command_Sysmsgs|OF_NoHouseMuteSpeech);
	_uiAreaFlags		= AREAF_RoomInheritsFlags;
	_fMeditationMovementAbort = false;

  m_iMapViewSize      = UO_MAP_VIEW_SIZE_DEFAULT;
  m_iMapViewSizeMax   = UO_MAP_VIEW_SIZE_MAX;
  m_iMapViewRadar     = UO_MAP_VIEW_RADAR;


	m_iMaxSkill			= SKILL_QTY;
	m_iWalkBuffer		= 15;
	m_iWalkRegen		= 25;
	m_iWoolGrowthTime	= 30ll * 60 * MSECS_PER_SEC;
	m_iAttackerTimeout	= 30;

	m_iCommandLog		= 0;
	m_fTelnetLog		= true;

	m_fUsecrypt 		= true;		// Server wants crypted client?
	m_fUsenocrypt		= false;	// Server wants unencrypted client? (version guessed by cliver)
	m_fPayFromPackOnly	= false;	// pay vendors from packs only

	m_iOverSkillMultiply	= 2;
	m_iCanSeeSamePLevel		= 0;
	m_fSuppressCapitals		= false;

	m_iAdvancedLos		= ADVANCEDLOS_DISABLED;
    m_iDistanceFormula = DISTANCE_FORMULA_NODIAGONAL_NOZ;

	// New ones
	m_iFeatureT2A		= FEATURE_T2A_UPDATE;
	m_iFeatureLBR		= 0;
	m_iFeatureAOS		= 0;
	m_iFeatureSE		= 0;
	m_iFeatureML		= 0;
	m_iFeatureKR		= 0;
	m_iFeatureSA		= 0;
	m_iFeatureTOL		= 0;
	m_iFeatureExtra		= 0;

	_uiStatFlag			= 0;

	m_iNpcAi			= 0;
    m_iNPCWanderLookAroundChance = 30;

	m_iMaxLoopTimes		= 100000;

	// Third Party Tools
	m_fCUOStatus		= true;
	m_fUOGStatus		= true;

	//	Experience
	m_fExperienceSystem		= false;
	m_iExperienceMode		= 0;
	m_iExperienceKoefPVP	= 100;
	m_iExperienceKoefPVM	= 100;
	m_fLevelSystem			= false;
	m_iLevelMode			= LEVEL_MODE_DOUBLE;
	m_iLevelNextAt			= 0;

	//	MySQL support
	m_fMySql				= false;
	m_fMySqlTicks			= false;

    m_fAutoResDisp          = true;
	m_iAutoPrivFlags = 0;

	_iEraLimitGear = RESDISPLAY_VERSION(RDS_QTY - 1); // Always latest by default
	_iEraLimitLoot = RESDISPLAY_VERSION(RDS_QTY - 1); // Always latest by default
	_iEraLimitProps = RESDISPLAY_VERSION(RDS_QTY - 1); // Always latest by default

	m_cCommandPrefix		= '.';

	m_iDefaultCommandLevel	= 7;	// PLevel 7 default for command levels.

	m_iRegenRate[STAT_STR]	= 40ll * MSECS_PER_SEC;       // Seconds to heal ONE hp (before stam/food adjust)
	m_iRegenRate[STAT_INT]	= 20ll * MSECS_PER_SEC;       // Seconds to heal ONE mn
	m_iRegenRate[STAT_DEX]	= 10ll * MSECS_PER_SEC;       // Seconds to heal ONE stm
	m_iRegenRate[STAT_FOOD] = 60ll * 60 * MSECS_PER_SEC;  // Food usage (1 time per 60 minutes)
    _iItemHitpointsUpdate   = 10ll * MSECS_PER_SEC;       // Delay to send hitpoints update packet for items.

	_iTimerCall			= 0;
	_iTimerCallUnit		= 0;
	m_fAllowLightOverride	= true;
	m_fAllowNewbTransfer	= false;
	m_sZeroPoint			= "1323,1624,0";
	m_fAllowBuySellAgent	= false;

	m_iColorNotoGood			= 0x59;		// blue
	m_iColorNotoGoodNPC			= 0x59;		// blue (npcs)
	m_iColorNotoGuildSame		= 0x3f;		// green
	m_iColorNotoNeutral			= 0x3b2;	// grey (can be attacked)
	m_iColorNotoCriminal		= 0x3b2;	// grey (criminal)
	m_iColorNotoGuildWar		= 0x90;		// orange (enemy guild)
	m_iColorNotoEvil			= 0x22;		// red
	m_iColorNotoInvul			= 0x35;		// yellow
	m_iColorNotoInvulGameMaster = 0x0b;		// purple
	m_iColorNotoDefault			= 0x3b2;	// grey (if not any other)

	m_iColorInvisItem   = 1000;
	m_iColorInvis		= 0;
	m_iColorInvisSpell	= 0;
	m_iColorHidden		= 0;

	m_iNotoTimeout		= 30;				// seconds to remove this character from notoriety list.

	m_iPetsInheritNotoriety = 0;

	// Networking
	_uiNetworkThreads		= 0;				// if there aren't the ini settings, by default we'll not use additional network threads
    _iNetworkThreadPriority = enum_alias_cast<int>(ThreadPriority::Disabled);
	m_fUseAsyncNetwork		= 0;
	m_iNetMaxPings			= 15;
	m_iNetHistoryTTLSeconds 		= 300;
	_uiNetMaxPacketsPerTick = 50;
	_uiNetMaxLengthPerTick	= 18'000;
	m_iNetMaxQueueSize		= 75;
    _iMaxConnectRequestsPerIP = 5;
    _iTimeoutIncompleteConnectionMs = 5ll * MSECS_PER_SEC;
	_iMaxSizeClientOut		= 80'000;
	_iMaxSizeClientIn		= 10'000;
	m_fUsePacketPriorities	= false;
	m_fUseExtraBuffer		= true;

	m_iTooltipCache			= 30ll * MSECS_PER_SEC;
	m_iTooltipMode			= TOOLTIPMODE_SENDVERSION;
	m_iContextMenuLimit		= 15;

	m_iClientLoginMaxTries	= 0;		// maximum bad password tries before a temp ip ban
	m_iClientLoginTempBan	= 3ll * 60 * MSECS_PER_SEC;
	m_iMaxShipPlankTeleport = UO_MAP_VIEW_SIZE_DEFAULT;
	m_sChatStaticChannels = "General, Help, Trade, Looking For Group";
	m_iChatFlags = (CHATF_AUTOJOIN | CHATF_CHANNELCREATION | CHATF_CHANNELMODERATION | CHATF_CUSTOMNAMES);

	m_NPCNoFameTitle = false;
}

CServerConfig::~CServerConfig()
{
	for ( size_t i = 0; i < ARRAY_COUNT(m_ResHash.m_Array); ++i )
	{
		for ( size_t j = 0; j < m_ResHash.m_Array[i].size(); ++j )
		{
			CResourceDef* pResDef = m_ResHash.m_Array[i][j].get();
			if ( pResDef != nullptr )
				pResDef->UnLink();
		}
	}

	Unload(false);
}


// SKILL ITEMDEF, etc
bool CServerConfig::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CServerConfig::r_GetRef");
	tchar * pszSep = const_cast<tchar*>(strchr( ptcKey, '(' ));
	if ( pszSep == nullptr )
	{
		pszSep = const_cast<tchar*>(strchr( ptcKey, '.' ));
		if ( pszSep == nullptr )
			return false;
	}

	tchar oldChar = *pszSep;
	*pszSep = '\0';

	int iResType = FindTableSorted( ptcKey, sm_szResourceBlocks, RES_QTY );
	bool fNewStyleDef = false;

	if ( iResType < 0 )
	{
		if ( !strnicmp(ptcKey, "MULTIDEF", 8) )
		{
			iResType = RES_ITEMDEF;
			fNewStyleDef = true;
		}
		else
		{
			*pszSep = oldChar;
			return false;
		}
	}

	*pszSep = '.';

	// Now get the index.
	ptcKey = pszSep + 1;
	if (ptcKey[0] == '\0')
	{
		*pszSep = oldChar;
		return false;
	}

	pszSep = const_cast<tchar*>(strchr( ptcKey, '.' ));
	if ( pszSep != nullptr )
		*pszSep = '\0';

	if ( iResType == RES_SERVERS )
	{
		pRef = nullptr;
	}
	else if ( iResType == RES_CHARDEF )
	{
		//pRef = CCharBase::FindCharBase(static_cast<CREID_TYPE>(Exp_GetVal(ptcKey)));
		pRef = CCharBase::FindCharBase((CREID_TYPE)(ResourceGetIndexType(RES_CHARDEF, ptcKey)));
	}
	else if ( iResType == RES_ITEMDEF )
	{
		if (fNewStyleDef && IsDigit(ptcKey[0]))
			pRef = CItemBase::FindItemBase((ITEMID_TYPE)(Exp_GetVal(ptcKey) + ITEMID_MULTI));
		else
			pRef = CItemBase::FindItemBase((ITEMID_TYPE)(ResourceGetIndexType(RES_ITEMDEF, ptcKey)));
	}
	else if ( iResType == RES_SPELL && *ptcKey == '-' )
	{
		++ptcKey;
		size_t uiOrder = Exp_GetSTVal( ptcKey );
		if ( !m_SpellDefs_Sorted.valid_index( uiOrder ) )
			pRef = nullptr;
		else
			pRef = m_SpellDefs_Sorted[uiOrder].get();
	}
	else
	{
		CResourceID	rid	= ResourceGetID((RES_TYPE)iResType, ptcKey, RES_PAGE_ANY);

		// check the found resource type matches what we searched for
		if ( rid.GetResType() == iResType )
			pRef = RegisteredResourceGetDef( rid );
	}

	if ( pszSep != nullptr )
	{
		*pszSep = oldChar; //*pszSep = '.';
		ptcKey = pszSep + 1;
	}
	else
	{
		ptcKey += strlen(ptcKey);
	}

	return (pRef != nullptr);
}


enum RC_TYPE
{
	RC_ACCTFILES,				// m_sAcctBaseDir
	RC_ADVANCEDLOS,				// m_iAdvancedLos
	RC_AGREE,
	RC_ALLOWBUYSELLAGENT,		// m_fAllowBuySellAgent
	RC_ALLOWLIGHTOVERRIDE,		// m_fAllowLightOverride
	RC_ALLOWNEWBTRANSFER,		// m_fAllowNewbTransfer
	RC_ARCHERYMAXDIST,			// m_iArcheryMaxDist
	RC_ARCHERYMINDIST,			// m_iArcheryMinDist
	RC_AREAFLAGS,				// _uiAreaFlags
	RC_ARRIVEDEPARTMSG,
	RC_ATTACKERTIMEOUT,			// m_iAttackerTimeout
	RC_ATTACKINGISACRIME,		// m_fAttackingIsACrime
    RC_AUTOHOUSEKEYS,           // _fAutoHouseKeys
	RC_AUTONEWBIEKEYS,			// m_fAutoNewbieKeys
	RC_AUTOPRIVFLAGS,			// m_iAutoPrivFlags
    RC_AUTOPROCESSPRIORITY,     // m_iAutoProcessPriority
	RC_AUTORESDISP,				// m_fAutoResDisp
    RC_AUTOSHIPKEYS,            // _fAutoShipKeys
	RC_BACKPACKOVERLOAD,       // m_iBackpackOverload
	RC_BACKUPLEVELS,			// m_iSaveBackupLevels
	RC_BANKMAXITEMS,
	RC_BANKMAXWEIGHT,
	RC_BUILD,
    RC_BUILDBRANCH,
    RC_BUILDNUM,
    RC_BUILDSTR,
    RC_CANPETSDRINKPOTION,      // m_fCanPetsDrinkPotion
	RC_CANSEESAMEPLEVEL,		// m_iCanSeeSamePLevel
	RC_CANUNDRESSPETS,			// m_fCanUndressPets
	RC_CHARTAGS,				// m_fCharTags
	RC_CHATFLAGS,				// m_iChatFlags
	RC_CHATSTATICCHANNELS,		// m_sChatStaticChannels
	RC_CLIENTLINGER,
	RC_CLIENTLOGINMAXTRIES,		// m_iClientLoginMaxTries
	RC_CLIENTLOGINTEMPBAN,		// m_iClientLoginTempBan
	RC_CLIENTMAX,				// m_iClientsMax
	RC_CLIENTMAXIP,				// m_iClientsMaxIP
	RC_CLIENTS,
	RC_COLORHIDDEN,
	RC_COLORINVIS,
	RC_COLORINVISITEM,
	RC_COLORINVISSPELL,
	RC_COLORNOTOCRIMINAL,		// m_iColorNotoCriminal
	RC_COLORNOTODEFAULT,		// m_iColorNotoDefault
	RC_COLORNOTOEVIL,			// m_iColorNotoEvil
	RC_COLORNOTOGOOD,			// m_iColorNotoGood
	RC_COLORNOTOGOODNPC,		// m_iColorNotoGoodNPC
	RC_COLORNOTOGUILDSAME,		// m_iColorNotoGuildSame
	RC_COLORNOTOGUILDWAR,		// m_iColorNotoGuildWar
	RC_COLORNOTOINVUL,			// m_iColorNotoInvul
	RC_COLORNOTOINVULGAMEMASTER,// m_iColorNotoInvulGameMaster
	RC_COLORNOTONEUTRAL,		// m_iColorNotoNeutral
	RC_COMBATARCHERYMOVEMENTDELAY, // m_iCombatArcheryMovementDelay
	RC_COMBATDAMAGEERA,			// m_iCombatDamageEra
	RC_COMBATFLAGS,				// m_iCombatFlags
	RC_COMBATHITCHANCEERA,		// m_iCombatHitChanceEra
	RC_COMBATPARRYINGERA,       // m_iCombatParryingEra
	RC_COMBATSPEEDERA,			// m_iCombatSpeedEra
	RC_COMMANDLOG,
	RC_COMMANDPREFIX,
	RC_COMMANDTRIGGER,			// m_sCommandTrigger
	RC_CONNECTINGMAX,			// m_iConnectingMax
	RC_CONNECTINGMAXIP,			// m_iConnectingMaxIP
    RC_CONTAINERMAXITEMS,       // m_iContainerMaxItems
	RC_CONTEXTMENULIMIT,		// m_iContextMenuLimit
	RC_CORPSENPCDECAY,
	RC_CORPSEPLAYERDECAY,
	RC_CRIMINALTIMER,			// m_iCriminalTimer
	RC_CUOSTATUS,				// m_fCUOStatus
	RC_DEADCANNOTSEELIVING,
	RC_DEADSOCKETTIME,
	RC_DEBUGFLAGS,
	RC_DECAYTIMER,
	RC_DEFAULTCOMMANDLEVEL,		//m_iDefaultCommandLevel
	RC_DISPLAYPERCENTAR,	    //m_fDisplayPercentAr
	RC_DISPLAYELEMENTALRESISTANCE, //m_fDisplayElementalResistance
    RC_DISTANCEFORMULA,
    RC_DISTANCETALK,
	RC_DISTANCEWHISPER,
	RC_DISTANCEYELL,
    RC_DECIMALVARIABLES,
	RC_DRAGWEIGHTMAX,
#ifdef _DUMPSUPPORT
	RC_DUMPPACKETSFORACC,
#endif
	RC_DUNGEONLIGHT,
	RC_EMOTEFLAGS,				// m_iEmoteFlags
	RC_EQUIPPEDCAST,			// m_fEquippedCast
    RC_ERALIMITGEAR,			// _iEraLimitGear
    RC_ERALIMITLOOT,			// _iEraLimitLoot
    RC_ERALIMITPROPS,			// _iEraLimitProps
	RC_EVENTSITEM,				// m_sEventsItem
	RC_EVENTSPET,				// m_sEventsPet
	RC_EVENTSPLAYER,			// m_sEventsPlayer
	RC_EVENTSREGION,			// m_sEventsRegion
	RC_EXPERIENCEKOEFPVM,		// m_iExperienceKoefPVM
	RC_EXPERIENCEKOEFPVP,		// m_iExperienceKoefPVP
	RC_EXPERIENCEMODE,			// m_iExperienceMode
	RC_EXPERIENCESYSTEM,		// m_fExperienceSystem
	RC_EXPERIMENTAL,			// _uiExperimentalFlags
	RC_FEATURESAOS,
	RC_FEATURESEXTRA,
	RC_FEATURESKR,
	RC_FEATURESLBR,
	RC_FEATURESML,
	RC_FEATURESSA,
	RC_FEATURESSE,
	RC_FEATUREST2A,
	RC_FEATURESTOL,
	RC_FLIPDROPPEDITEMS,		// m_fFlipDroppedItems
	RC_FORCEGARBAGECOLLECT,		// m_fSaveGarbageCollect
	RC_FREEZERESTARTTIME,		// m_iFreezeRestartTime
	RC_GAMEMINUTELENGTH,		// m_iGameMinuteLength
	RC_GENERICSOUNDS,			// m_fGenericSounds
	RC_GUARDLINGER,				// m_iGuardLingerTime
	RC_GUARDSINSTANTKILL,
	RC_GUARDSONMURDERERS,
	RC_GUESTSMAX,
	RC_GUILDS,
	RC_HEARALL,
	RC_HELPINGCRIMINALSISACRIME,// m_fHelpingCriminalsIsACrime
	RC_HITPOINTPERCENTONREZ,	// m_iHitpointPercentOnRez
	RC_HITSHUNGERLOSS,			// m_iHitsHungerLoss
	RC_HITSUPDATERATE,
    RC_ITEMHITPOINTSUPDATE,     // _iItemHitpointsUpdate
	RC_ITEMSMAXAMOUNT,			// m_iItemsMaxAmount
    RC_ITEMTIMERS,              // m_uiItemTimers
	RC_LEVELMODE,				// m_iLevelMode
	RC_LEVELNEXTAT,				// m_iLevelNextAt
	RC_LEVELSYSTEM,				// m_fLevelSystem
	RC_LIGHTDAY,				// m_iLightDay
	RC_LIGHTNIGHT,				// m_iLightNight
	RC_LOCALIPADMIN,			// m_fLocalIPAdmin
	RC_LOG,
	RC_LOGMASK,					// GetLogMask
	RC_LOOTINGISACRIME,			// m_fLootingIsACrime
	RC_LOSTNPCTELEPORT,			// m_fLostNPCTeleport
	RC_MAGICFLAGS,
	RC_MAGICUNLOCKDOOR,			// m_iMagicUnlockDoor
	RC_MANALOSSABORT,			// m_fManaLossAbort
    RC_MANALOSSFAIL,			// m_fManaLossFail
	RC_MANALOSSPERCENT,			// m_fManaLossPercent
	RC_MAPCACHETIME,
    RC_MAPVIEWRADAR,
    RC_MAPVIEWSIZE,
    RC_MAPVIEWSIZEMAX,
	RC_MAXBASESKILL,			// m_iMaxBaseSkill
	RC_MAXCHARSPERACCOUNT,		//
	RC_MAXCOMPLEXITY,			// m_iMaxCharComplexity
    RC_MAXCONNECTREQUESTSPERIP,  // m_iMaxConnectRequestsPerIP
	RC_MAXFAME,					// m_iMaxFame
    RC_MAXHOUSESACCOUNT,        // _iMaxHousesAccount
    RC_MAXHOUSESGUILD,          // _iMaxHousesGuild
    RC_MAXHOUSESPLAYER,         // _iMaxHousesPlayer
	RC_MAXITEMCOMPLEXITY,		// m_iMaxItemComplexity
	RC_MAXKARMA,				// m_iMaxKarma
	RC_MAXLOOPTIMES,			// m_iMaxLoopTimes
	RC_MAXPACKETSPERTICK,		// _uiNetMaxPacketsPerTick
	RC_MAXPINGS,				// m_iNetMaxPings
	RC_MAXPOLYSTATS,			// m_iMaxPolyStats
	RC_MAXQUEUESIZE,			// m_iNetMaxQueueSize
	RC_MAXSECTORCOMPLEXITY,		// m_iMaxSectorComplexity
	RC_MAXSHIPPLANKTELEPORT,	// m_iMaxShipPlankTeleport
    RC_MAXSHIPSACCOUNT,         // _iMaxShipsAccount
    RC_MAXSHIPSGUILD,           // _iMaxShipsGuild
    RC_MAXSHIPSPLAYER,          // _iMaxShipsPlayer
	RC_MAXSIZECLIENTIN,			// _uiMaxSizeClientIn
	RC_MAXSIZECLIENTOUT,		// _uiMaxSizeClientOut
	RC_MAXSIZEPERTICK,			// _uiNetMaxLengthPerTick
	RC_MD5PASSWORDS,			// m_fMd5Passwords
	RC_MEDITATIONMOVEMENTABORT,  // _fMeditationMovementAbort
	RC_MEDIUMCANHEARGHOSTS,		// m_iMediumCanHearGhosts
	RC_MINCHARDELETETIME,
	RC_MINKARMA,				// m_iMinKarma
	RC_MONSTERFEAR,				// m_fMonsterFear
	RC_MONSTERFIGHT,
	RC_MOUNTHEIGHT,				// m_iMountHeight
	RC_MOVERATE,				// m_iMoveRate
	RC_MULFILES,
	RC_MURDERDECAYTIME,			// m_iMurderDecayTime;
	RC_MURDERMINCOUNT,			// m_iMurderMinCount
	RC_MYSQL,					// m_fMySql
	RC_MYSQLDB,					// m_sMySqlDatabase
	RC_MYSQLHOST,				// m_sMySqlHost
	RC_MYSQLPASS,				// m_sMySqlPassword
	RC_MYSQLTICKS,				// m_fMySqlTicks
	RC_MYSQLUSER,				// m_sMySqlUser
	RC_NETTTL,					// m_iNetHistoryTTL
    RC_NETWORKTHREADPRIORITY,	// _iNetworkThreadPriority
	RC_NETWORKTHREADS,			// _uiNetworkThreads
	RC_NORESROBE,
	RC_NOTOTIMEOUT,
	RC_NOWEATHER,				// m_fNoWeather
	RC_NPCAI,					// m_iNpcAi
	RC_NPCCANFIZZLEONHIT,		// m_fNPCCanFizzle
    RC_NPCDISTANCEHEAR,         // m_iNPCDistanceHear
	RC_NPCHEALTHRESHOLD,		// m_iNPCHealtreshold
	RC_NPCNOFAMETITLE,			// m_NPCNoFameTitle
	RC_NPCSHOVENPC,				// m_NPCShoveNPC
	RC_NPCSKILLSAVE,			// m_iSaveNPCSkills
	RC_NPCTRAINCOST,			// m_iTrainSkillCost
	RC_NPCTRAINMAX,				// m_iTrainSkillMax
	RC_NPCTRAINPERCENT,			// m_iTrainSkillPercent
    RC_NPCWANDERINGLOOKAROUNDCHANCE, // m_iNPCWanderingLookAroundChance
	RC_NTSERVICE,				// m_fUseNTService
	RC_OPTIONFLAGS,				// _uiOptionFlags
	RC_OVERSKILLMULTIPLY,		// m_iOverSkillMultiply
	RC_PACKETDEATHANIMATION,	// m_iPacketDeathAnimation
	RC_PAYFROMPACKONLY,			// m_fPayFromPackOnly
	RC_PETSINHERITNOTORIETY,	// m_iPetsInheritNotoriety
	RC_PLAYEREVIL,				// m_iPlayerKarmaEvil
	RC_PLAYERNEUTRAL,			// m_iPlayerKarmaNeutral
	RC_PROFILE,
	RC_RACIALFLAGS,				// m_iRacialFlags
	RC_REAGENTLOSSABORT,		// m_fReagentLossAbort
	RC_REAGENTLOSSFAIL,			// m_fReagentLossFail
	RC_REAGENTSREQUIRED,
	RC_REVEALFLAGS,				// m_iRevealFlags
	RC_RTICKS,
	RC_RTIME,
	RC_RUNNINGPENALTY,			// m_iStamRunningPenalty
	RC_RUNNINGPENALTYOVERWEIGHT,// m_iStamRunningPenaltyOverweight
	RC_SAVEBACKGROUND,			// m_iSaveBackgroundTime
	RC_SAVEPERIOD,
	RC_SAVESECTORSPERTICK,		// m_iSaveSectorsPerTick
    RC_SAVESTEPMAXCOMPLEXITY,	// m_iSaveStepMaxComplexity
	RC_SCPFILES,
	RC_SECTORSLEEP,				// _iSectorSleepDelay
	RC_SECURE,
	RC_SKILLPRACTICEMAX,		// m_iSkillPracticeMax
	RC_SNOOPCRIMINAL,
	RC_SPEECHOTHER,
	RC_SPEECHPET,
	RC_SPEECHSELF,
	RC_SPEEDSCALEFACTOR,
	RC_SPELLTIMEOUT,
	RC_STAMINALOSSATWEIGHT,		// m_iStaminaLossAtWeight
	RC_STAMINALOSSOVERWEIGHT,	// m_iStaminaLossOverweight
	RC_STATSFLAGS,				// _uiStatFlag
	RC_STRIPPATH,				// for TNG
	RC_SUPPRESSCAPITALS,
	RC_TELEPORTEFFECTNPC,		// m_iSpell_Teleport_Effect_NPC
	RC_TELEPORTEFFECTPLAYERS,	// m_iSpell_Teleport_Effect_Players
	RC_TELEPORTEFFECTSTAFF,		// m_iSpell_Teleport_Effect_Staff
	RC_TELEPORTSOUNDNPC,		// m_iSpell_Teleport_Sound_NPC
	RC_TELEPORTSOUNDPLAYERS,	// m_iSpell_Teleport_Sound_Players
	RC_TELEPORTSOUNDSTAFF,		// m_iSpell_Teleport_Sound_Staff
	RC_TELNETLOG,				// m_fTelnetLog
    RC_TICKPERIOD,
    RC_TIMEOUTINCOMPLETECONN,   // _iTimeoutIncompleteConnectionMs
	RC_TIMERCALL,				// _iTimerCall
	RC_TIMERCALLUNIT,			// _iTimerCallUnit
	RC_TIMEUP,
	RC_TOOLTIPCACHE,			// m_iTooltipCache
	RC_TOOLTIPMODE,				// m_iTooltipMode
	RC_TRADEWINDOWSNOOPING,		// m_iTradeWindowSnooping
	RC_UOGSTATUS,				// m_fUOGStatus
	RC_USEASYNCNETWORK,			// m_fUseAsyncNetwork
	RC_USEAUTHID,				// m_fUseAuthID
	RC_USECRYPT,				// m_Usecrypt
	RC_USEEXTRABUFFER,			// m_fUseExtraBuffer
	RC_USEHTTP,					// m_fUseHTTP
	RC_USEMAPDIFFS,				// m_fUseMapDiffs
    RC_USEMOBTYPES,             // m_fUseMobTypes
	RC_USENOCRYPT,				// m_Usenocrypt
	RC_USEPACKETPRIORITY,		// m_fUsePacketPriorities
	RC_VENDORMARKUP,			// m_iVendorMarkup
	RC_VENDORMAXSELL,			// m_iVendorMaxSell
	RC_VENDORTRADETITLE,		// m_fVendorTradeTitle
	RC_VERBOSEITEMBOUNCE,		// m_iBounceMessage
	RC_VERSION,
	RC_WALKBUFFER,
	RC_WALKREGEN,
	RC_WOOLGROWTHTIME,			// m_iWoolGrowthTime
	RC_WOPCOLOR,
	RC_WOPFONT,
	RC_WOPPLAYER,
	RC_WOPSTAFF,
	RC_WOPTALKMODE,
	RC_WORLDSAVE,
	RC_ZEROPOINT,				// m_sZeroPoint
	RC_QTY
};

// NOTE: Need to be alphabetized order

// TODO: use offsetof by cstddef. Though, it requires the class/struct to be a POD type, so we need to encapsulate the values in a separate struct.
// This hack does happen because this class hasn't virtual methods? Or simply because the compiler is so smart and protects us from ourselves?
#if NON_MSVC_COMPILER
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

const CAssocReg CServerConfig::sm_szLoadKeys[RC_QTY + 1]
{
    { "ACCTFILES",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sAcctBaseDir)			}},
    { "ADVANCEDLOS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iAdvancedLos)			}},
    { "AGREE",					{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAgree)				}},
    { "ALLOWBUYSELLAGENT",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAllowBuySellAgent)	}},
    { "ALLOWLIGHTOVERRIDE",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAllowLightOverride)	}},
    { "ALLOWNEWBTRANSFER",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAllowNewbTransfer)	}},
    { "ARCHERYMAXDIST",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iArcheryMaxDist)		}},
    { "ARCHERYMINDIST",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iArcheryMinDist)		}},
    { "AREAFLAGS",				{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_uiAreaFlags)			}},
    { "ARRIVEDEPARTMSG",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iArriveDepartMsg)		}},
    { "ATTACKERTIMEOUT",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iAttackerTimeout)		}},
    { "ATTACKINGISACRIME",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAttackingIsACrime)	}},
    { "AUTOHOUSEKEYS",          { ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,_fAutoHouseKeys)			}},
    { "AUTONEWBIEKEYS",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAutoNewbieKeys)		}},
    { "AUTOPRIVFLAGS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iAutoPrivFlags)		}},
    { "AUTOPROCESSPRIORITY",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iAutoProcessPriority)	}},
    { "AUTORESDISP",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fAutoResDisp)			}},
    { "AUTOSHIPKEYS",           { ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,_fAutoShipKeys)		    }},
    { "BACKPACKOVERLOAD",		{ ELEM_INT,     static_cast<uint>OFFSETOF(CServerConfig,m_iBackpackOverload)	}},
    { "BACKUPLEVELS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSaveBackupLevels)	}},
    { "BANKMAXITEMS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iBankIMax)			}},
    { "BANKMAXWEIGHT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iBankWMax)			}},
    { "BUILD",					{ ELEM_VOID,	0																}},
    { "BUILDBRANCH",			{ ELEM_VOID,	0																}},
    { "BUILDNUM",				{ ELEM_VOID,	0																}},
    { "BUILDSTR",				{ ELEM_VOID,	0																}},
    { "CANPETSDRINKPOTION",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fCanPetsDrinkPotion)	}},
    { "CANSEESAMEPLEVEL",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCanSeeSamePLevel)	}},
    { "CANUNDRESSPETS",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fCanUndressPets)		}},
    { "CHARTAGS",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fCharTags)			}},
    { "CHATFLAGS",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iChatFlags)			}},
    { "CHATSTATICCHANNELS",		{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sChatStaticChannels)	}},
    { "CLIENTLINGER",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iClientLingerTime)	}},
    { "CLIENTLOGINMAXTRIES",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iClientLoginMaxTries)	}},
    { "CLIENTLOGINTEMPBAN",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iClientLoginTempBan)	}},
    { "CLIENTMAX",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iClientsMax)			}},
    { "CLIENTMAXIP",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iClientsMaxIP)			}},
    { "CLIENTS",				{ ELEM_VOID,	0											    }},	// duplicate
    { "COLORHIDDEN",			{ ELEM_VOID,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorHidden)			}},
    { "COLORINVIS",				{ ELEM_VOID,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorInvis)			}},
    { "COLORINVISITEM",			{ ELEM_VOID,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorInvisItem)		}},
    { "COLORINVISSPELL",		{ ELEM_VOID,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorInvisSpell)		}},
    { "COLORNOTOCRIMINAL",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoCriminal)	}},
    { "COLORNOTODEFAULT",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoDefault)		}},
    { "COLORNOTOEVIL",			{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoEvil)		}},
    { "COLORNOTOGOOD",			{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoGood)		}},
    { "COLORNOTOGOODNPC",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoGoodNPC)		}},
    { "COLORNOTOGUILDSAME",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoGuildSame)	}},
    { "COLORNOTOGUILDWAR",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoGuildWar)	}},
    { "COLORNOTOINVUL",			{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoInvul)		}},
    { "COLORNOTOINVULGAMEMASTER",{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoInvulGameMaster)	}},
    { "COLORNOTONEUTRAL",		{ ELEM_WORD,	static_cast<uint>OFFSETOF(CServerConfig,m_iColorNotoNeutral)	}},
    { "COMBATARCHERYMOVEMENTDELAY",{ ELEM_INT,	static_cast<uint>OFFSETOF(CServerConfig,m_iCombatArcheryMovementDelay)	}},
    { "COMBATDAMAGEERA",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCombatDamageEra)		}},
    { "COMBATFLAGS",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iCombatFlags)			}},
    { "COMBATHITCHANCEERA",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCombatHitChanceEra)	}},
    { "COMBATPARRYINGERA",		{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iCombatParryingEra)	}},
    { "COMBATSPEEDERA",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCombatSpeedEra)		}},
    { "COMMANDLOG",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCommandLog)			}},
    { "COMMANDPREFIX",			{ ELEM_BYTE,	static_cast<uint>OFFSETOF(CServerConfig,m_cCommandPrefix)		}},
    { "COMMANDTRIGGER",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sCommandTrigger)		}},
    { "CONNECTINGMAX",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iConnectingMax)		}},
    { "CONNECTINGMAXIP",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iConnectingMaxIP)		}},
    { "CONTAINERMAXITEMS",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iContainerMaxItems)	}},
    { "CONTEXTMENULIMIT",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iContextMenuLimit)		}},
    { "CORPSENPCDECAY",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDecay_CorpseNPC)		}},
    { "CORPSEPLAYERDECAY",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDecay_CorpsePlayer)	}},
    { "CRIMINALTIMER",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iCriminalTimer)		}},
    { "CUOSTATUS",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fCUOStatus)			}},
    { "DEADCANNOTSEELIVING",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_fDeadCannotSeeLiving)	}},
    { "DEADSOCKETTIME",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDeadSocketTime)		}},
    { "DEBUGFLAGS",				{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iDebugFlags)			}},
    { "DECAYTIMER",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDecay_Item)			}},
    { "DECIMALVARIABLES",       { ELEM_BOOL,    static_cast<uint>OFFSETOF(CServerConfig,m_fDecimalVariables)    }},
    { "DEFAULTCOMMANDLEVEL",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDefaultCommandLevel)	}},
    { "DISPLAYARMORASPERCENT",  { ELEM_BOOL,    static_cast<uint>OFFSETOF(CServerConfig,m_fDisplayPercentAr)		}},
    { "DISPLAYELEMENTALRESISTANCE",{ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fDisplayElementalResistance)}},
    { "DISTANCEFORMULA",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDistanceFormula)		}},
    { "DISTANCETALK",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDistanceTalk)		}},
    { "DISTANCEWHISPER",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDistanceWhisper)		}},
    { "DISTANCEYELL",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDistanceYell)		}},
    { "DRAGWEIGHTMAX",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iDragWeightMax)		}},
#ifdef _DUMPSUPPORT
    { "DUMPPACKETSFORACC",		{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sDumpAccPackets)		}},
#endif
    { "DUNGEONLIGHT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLightDungeon)			}},
    { "EMOTEFLAGS",				{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iEmoteFlags)			}},
    { "EQUIPPEDCAST",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fEquippedCast)			}},
    { "ERALIMITGEAR",			{ ELEM_BYTE,	static_cast<uint>OFFSETOF(CServerConfig,_iEraLimitGear)			}},
    { "ERALIMITLOOT",			{ ELEM_BYTE,	static_cast<uint>OFFSETOF(CServerConfig,_iEraLimitLoot)			}},
    { "ERALIMITPROPS",			{ ELEM_BYTE,	static_cast<uint>OFFSETOF(CServerConfig,_iEraLimitProps)			}},
    { "EVENTSITEM",				{ ELEM_CSTRING, static_cast<uint>OFFSETOF(CServerConfig,m_sEventsItem)			}},
    { "EVENTSPET",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sEventsPet)			}},
    { "EVENTSPLAYER",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sEventsPlayer)			}},
    { "EVENTSREGION",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sEventsRegion)			}},
    { "EXPERIENCEKOEFPVM",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iExperienceKoefPVM)	}},
    { "EXPERIENCEKOEFPVP",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iExperienceKoefPVP)	}},
    { "EXPERIENCEMODE",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iExperienceMode)		}},
    { "EXPERIENCESYSTEM",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fExperienceSystem)		}},
    { "EXPERIMENTAL",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_uiExperimentalFlags)	}},
    { "FEATUREAOS",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureAOS)			}},
    { "FEATUREEXTRA",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureExtra)			}},
    { "FEATUREKR",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureKR)			}},
    { "FEATURELBR",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureLBR)			}},
    { "FEATUREML",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureML)			}},
    { "FEATURESA",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureSA)			}},
    { "FEATURESE",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureSE)			}},
    { "FEATURET2A",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureT2A)			}},
    { "FEATURETOL",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFeatureTOL)			}},
    { "FLIPDROPPEDITEMS",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fFlipDroppedItems)		}},
    { "FORCEGARBAGECOLLECT",	{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fSaveGarbageCollect)	}},
    { "FREEZERESTARTTIME",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iFreezeRestartTime)	}},
    { "GAMEMINUTELENGTH",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iGameMinuteLength)		}},
    { "GENERICSOUNDS",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fGenericSounds)		}},
    { "GUARDLINGER",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iGuardLingerTime)		}},
    { "GUARDSINSTANTKILL",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fGuardsInstantKill)	}},
    { "GUARDSONMURDERERS",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fGuardsOnMurderers)	}},
    { "GUESTSMAX",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iGuestsMax)			}},
    { "GUILDS",					{ ELEM_VOID,	0												}},
    { "HEARALL",				{ ELEM_VOID,	0												}},
    { "HELPINGCRIMINALSISACRIME",{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fHelpingCriminalsIsACrime)	}},
    { "HITPOINTPERCENTONREZ",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iHitpointPercentOnRez) }},
    { "HITSHUNGERLOSS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iHitsHungerLoss)		}},
    { "HITSUPDATERATE",			{ ELEM_VOID,	0												}},
    { "ITEMHITPOINTSUPDATE",    { ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_iItemHitpointsUpdate),  }},
    { "ITEMSMAXAMOUNT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iItemsMaxAmount),		}},
    { "ITEMTIMERS",             { ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_uiItemTimers),  } },
    { "LEVELMODE",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLevelMode),			}},
    { "LEVELNEXTAT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLevelNextAt),			}},
    { "LEVELSYSTEM",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fLevelSystem),			}},
    { "LIGHTDAY",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLightDay),			}},
    { "LIGHTNIGHT",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLightNight),			}},
    { "LOCALIPADMIN",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fLocalIPAdmin),		}}, // The local ip is assumed to be the admin.
    { "LOG",					{ ELEM_VOID,	0												}},
    { "LOGMASK",				{ ELEM_VOID,	0												}}, // GetLogMask
    { "LOOTINGISACRIME",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fLootingIsACrime)		}},
    { "LOSTNPCTELEPORT",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iLostNPCTeleport)		}},
    { "MAGICFLAGS",				{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iMagicFlags)			}},
    { "MAGICUNLOCKDOOR",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMagicUnlockDoor)		}},
    { "MANALOSSABORT",		    { ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fManaLossAbort)		}},
    { "MANALOSSFAIL",		    { ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fManaLossFail)			}},
    { "MANALOSSPERCENT",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_fManaLossPercent)		}},
    { "MAPCACHETIME",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_iMapCacheTime)			}},
    { "MAPVIEWRADAR",           { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,m_iMapViewRadar)        }},
    { "MAPVIEWSIZE",            { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,m_iMapViewSize)         }},
    { "MAPVIEWSIZEMAX",         { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,m_iMapViewSizeMax)      }},
	{ "MAXBASESKILL",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxBaseSkill)			}},
	{ "MAXCHARSPERACCOUNT",		{ ELEM_BYTE,	static_cast<uint>OFFSETOF(CServerConfig,m_iMaxCharsPerAccount)	}},
	{ "MAXCOMPLEXITY",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxCharComplexity)	}},
    { "MAXCONNECTREQUESTSPERIP",{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_iMaxConnectRequestsPerIP)	} },
    { "MAXFAME",                { ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxFame)				}},
    { "MAXHOUSESACCOUNT",       { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxHousesAccount)		}},
    { "MAXHOUSESGUILD",         { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxHousesGuild)		}},
    { "MAXHOUSESPLAYER",        { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxHousesPlayer)		}},
	{ "MAXITEMCOMPLEXITY",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxItemComplexity)	}},
	{ "MAXKARMA",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxKarma)				}},
	{ "MAXLOOPTIMES",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxLoopTimes)			}},
	{ "MAXPACKETSPERTICK",		{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_uiNetMaxPacketsPerTick)	}},
	{ "MAXPINGS",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iNetMaxPings)			}},
	{ "MAXPOLYSTATS",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxPolyStats)			}},
	{ "MAXQUEUESIZE",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iNetMaxQueueSize)		}},
	{ "MAXSECTORCOMPLEXITY",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxSectorComplexity)	}},
	{ "MAXSHIPPLANKTELEPORT",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMaxShipPlankTeleport)	}},
    { "MAXSHIPSACCOUNT",        { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxShipsAccount)		}},
    { "MAXSHIPSGUILD",          { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxShipsGuild)			}},
    { "MAXSHIPSPLAYER",         { ELEM_BYTE,    static_cast<uint>OFFSETOF(CServerConfig,_iMaxShipsPlayer)		}},
	{ "MAXSIZECLIENTIN",		{ ELEM_INT64,	static_cast<uint>OFFSETOF(CServerConfig,_iMaxSizeClientIn)		}},
	{ "MAXSIZECLIENTOUT",		{ ELEM_INT64,	static_cast<uint>OFFSETOF(CServerConfig,_iMaxSizeClientOut)		}},
	{ "MAXSIZEPERTICK",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_uiNetMaxLengthPerTick)	}},
	{ "MD5PASSWORDS",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fMd5Passwords) 		}},
	{ "MEDITATIONMOVEMENTABORT",{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,_fMeditationMovementAbort)	}},
	{ "MEDIUMCANHEARGHOSTS",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMediumCanHearGhosts)	}},
	{ "MINCHARDELETETIME",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMinCharDeleteTime)	}},
	{ "MINKARMA",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMinKarma)				}},
	{ "MONSTERFEAR",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fMonsterFear)			}},
	{ "MONSTERFIGHT",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fMonsterFight)			}},
	{ "MOUNTHEIGHT",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_iMountHeight)			}},
	{ "MOVERATE",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMoveRate)				}},
	{ "MULFILES",				{ ELEM_VOID,	0												}},
	{ "MURDERDECAYTIME",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMurderDecayTime)		}},
	{ "MURDERMINCOUNT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iMurderMinCount)		}}, // amount of murders before we get title.
	{ "MYSQL",					{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fMySql)				}},
	{ "MYSQLDATABASE",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sMySqlDB)				}},
	{ "MYSQLHOST",				{ ELEM_CSTRING, static_cast<uint>OFFSETOF(CServerConfig,m_sMySqlHost)			}},
	{ "MYSQLPASSWORD",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sMySqlPass)			}},
	{ "MYSQLTICKS",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fMySqlTicks)			}},
	{ "MYSQLUSER",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sMySqlUser)			}},
	{ "NETTTL",					{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig, m_iNetHistoryTTLSeconds)		}},
    { "NETWORKTHREADPRIORITY",	{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_iNetworkThreadPriority)}},
	{ "NETWORKTHREADS",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_uiNetworkThreads)		}},
	{ "NORESROBE",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fNoResRobe)			}},
	{ "NOTOTIMEOUT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iNotoTimeout)			}},
	{ "NOWEATHER",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fNoWeather)			}},
	{ "NPCAI",					{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iNpcAi)				}},
	{ "NPCCANFIZZLEONHIT",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fNPCCanFizzleOnHit)	}},
    { "NPCDISTANCEHEAR",        { ELEM_INT,     static_cast<uint>OFFSETOF(CServerConfig,m_iNPCDistanceHear)		}},
	{ "NPCHEALTHRESHOLD",       { ELEM_INT,     static_cast<uint>OFFSETOF(CServerConfig,m_iNPCHealthreshold)     }},
	{ "NPCNOFAMETITLE",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_NPCNoFameTitle)		}},
	{ "NPCSHOVENPC",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_NPCShoveNPC)			}},
	{ "NPCSKILLSAVE",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSaveNPCSkills)		}},
	{ "NPCTRAINCOST",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iTrainSkillCost)		}},
	{ "NPCTRAINMAX",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iTrainSkillMax)		}},
	{ "NPCTRAINPERCENT",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iTrainSkillPercent)	}},
    { "NPCWANDERLOOKAROUNDCHANCE", { ELEM_MASK_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iNPCWanderLookAroundChance)}},
    { "NTSERVICE",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUseNTService)			}},
	{ "OPTIONFLAGS",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,_uiOptionFlags)			}},
	{ "OVERSKILLMULTIPLY",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iOverSkillMultiply)	}},
	{ "PACKETDEATHANIMATION",	{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_iPacketDeathAnimation)	}},
	{ "PAYFROMPACKONLY",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fPayFromPackOnly)		}},
	{ "PETSINHERITNOTORIETY",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iPetsInheritNotoriety)	}},
	{ "PLAYEREVIL",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iPlayerKarmaEvil)		}},
	{ "PLAYERNEUTRAL",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iPlayerKarmaNeutral)	}},
	{ "PROFILE",				{ ELEM_VOID,	0												}},
	{ "RACIALFLAGS",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iRacialFlags)			}},
	{ "REAGENTLOSSABORT",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fReagentLossAbort)		}},
	{ "REAGENTLOSSFAIL",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fReagentLossFail)		}},
	{ "REAGENTSREQUIRED",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fReagentsRequired)		}},
	{ "REVEALFLAGS",			{ ELEM_MASK_INT,static_cast<uint>OFFSETOF(CServerConfig,m_iRevealFlags)			}},
	{ "RTICKS",					{ ELEM_VOID,	0												}},
	{ "RTIME",					{ ELEM_VOID,	0												}},
	{ "RUNNINGPENALTY",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iStamRunningPenalty)	}},
	{ "RUNNINGPENALTYOVERWEIGHT",{ ELEM_INT,	static_cast<uint>OFFSETOF(CServerConfig,m_iStamRunningPenalty)	} },
	{ "SAVEBACKGROUND",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSaveBackgroundTime)	}},
	{ "SAVEPERIOD",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSavePeriod)			}},
	{ "SAVESECTORSPERTICK",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSaveSectorsPerTick)	}},
	{ "SAVESTEPMAXCOMPLEXITY",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSaveStepMaxComplexity)}},
	{ "SCPFILES",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sSCPBaseDir)			}},
	{ "SECTORSLEEP",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_iSectorSleepDelay)		}},
	{ "SECURE",					{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fSecure)				}},
	{ "SKILLPRACTICEMAX",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSkillPracticeMax)		}},
	{ "SNOOPCRIMINAL",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSnoopCriminal)		}},
	{ "SPEECHOTHER",			{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sSpeechOther)			}},
	{ "SPEECHPET",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sSpeechPet)			}},
	{ "SPEECHSELF",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sSpeechSelf)			}},
	{ "SPEEDSCALEFACTOR",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpeedScaleFactor)		}},
	{ "SPELLTIMEOUT",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpellTimeout)			}},
	{ "STAMINALOSSATWEIGHT",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iStaminaLossAtWeight)	}},
	{ "STAMINALOSSOVERWEIGHT",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iStaminaLossOverweight)	}},
	{ "STATSFLAGS",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_uiStatFlag)				}},
	{ "STRIPPATH",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_sStripPath)			}},
	{ "SUPPRESSCAPITALS",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fSuppressCapitals)		}},
	{ "TELEPORTEFFECTNPC",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Effect_NPC)	}},
	{ "TELEPORTEFFECTPLAYERS",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Effect_Players)}},
	{ "TELEPORTEFFECTSTAFF",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Effect_Staff)	}},
	{ "TELEPORTSOUNDNPC",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Sound_NPC)		}},
	{ "TELEPORTSOUNDPLAYERS",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Sound_Players)	}},
	{ "TELEPORTSOUNDSTAFF",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iSpell_Teleport_Sound_Staff)	}},
	{ "TELNETLOG",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fTelnetLog)			}},
    { "TICKPERIOD",				{ ELEM_INT,	    0			                                    }},
    { "TIMEOUTINCOMPLETECONN",	{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_iTimeoutIncompleteConnectionMs) } },
	{ "TIMERCALL",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,_iTimerCall)				}},
	{ "TIMERCALLUNIT",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,_iTimerCallUnit)			}},
	{ "TIMEUP",					{ ELEM_VOID,	0												}},
	{ "TOOLTIPCACHE",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iTooltipCache)			}},
	{ "TOOLTIPMODE",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iTooltipMode)			}},
	{ "TRADEWINDOWSNOOPING",	{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_iTradeWindowSnooping)	}},
	{ "UOGSTATUS",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUOGStatus)			}},
	{ "USEASYNCNETWORK",		{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_fUseAsyncNetwork)		}},
	{ "USEAUTHID",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUseAuthID)			}},	// we use authid like osi
	{ "USECRYPT",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUsecrypt)				}},	// we don't want crypt clients
	{ "USEEXTRABUFFER",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUseExtraBuffer)		}},
	{ "USEHTTP",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_fUseHTTP)				}},
	{ "USEMAPDIFFS",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUseMapDiffs)			}},
    { "USEMOBTYPES",			{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUseMobTypes)			} },
	{ "USENOCRYPT",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUsenocrypt)			}},	// we don't want no-crypt clients
	{ "USEPACKETPRIORITY",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fUsePacketPriorities)	}},
	{ "VENDORMARKUP",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iVendorMarkup)			}},
	{ "VENDORMAXSELL",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iVendorMaxSell)		}},
	{ "VENDORTRADETITLE",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fVendorTradeTitle)		}},
	{ "VERBOSEITEMBOUNCE",		{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_iBounceMessage)		}},
	{ "VERSION",				{ ELEM_VOID,	0												}},
	{ "WALKBUFFER",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iWalkBuffer)			}},
	{ "WALKREGEN",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iWalkRegen)			}},
	{ "WOOLGROWTHTIME",			{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iWoolGrowthTime)		}},
	{ "WOPCOLOR",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iWordsOfPowerColor)	}},
	{ "WOPFONT",				{ ELEM_INT,		static_cast<uint>OFFSETOF(CServerConfig,m_iWordsOfPowerFont)		}},
	{ "WOPPLAYER",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fWordsOfPowerPlayer)	}},
	{ "WOPSTAFF",				{ ELEM_BOOL,	static_cast<uint>OFFSETOF(CServerConfig,m_fWordsOfPowerStaff)	}},
    { "WOPTALKMODE",            { ELEM_INT,     static_cast<uint>OFFSETOF(CServerConfig,m_iWordsOfPowerTalkMode) }},
	{ "WORLDSAVE",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sWorldBaseDir)			}},
	{ "ZEROPOINT",				{ ELEM_CSTRING,	static_cast<uint>OFFSETOF(CServerConfig,m_sZeroPoint)			}},
	{ nullptr,					{ ELEM_VOID,	0,												}}
};

#if defined(__GNUC__) || defined(__clang__)
    #pragma GCC diagnostic pop
#endif

void CServerConfig::SetIniDirectory(const char* path)
{
    if (path && path[0] != '\0')
    {
        strncpy(m_iniDirectory, path, SPHERE_MAX_PATH - 1);
        m_iniDirectory[SPHERE_MAX_PATH - 1] = '\0';
    }
}

bool CServerConfig::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CServerConfig::r_LoadVal");
	EXC_TRY("LoadVal");

#define DEBUG_MSG_NOINIT(x) if (g_Serv.GetServerMode() != SERVMODE_PreLoadingINI) DEBUG_MSG(x)
#define LOG_WARN_NOINIT(x)  if (g_Serv.GetServerMode() != SERVMODE_PreLoadingINI) g_Log.EventWarn(x)
#define LOG_ERR_NOINIT(x)   if (g_Serv.GetServerMode() != SERVMODE_PreLoadingINI) g_Log.EventError(x)

	int i = FindCAssocRegTableHeadSorted( s.GetKey(), reinterpret_cast<lpctstr const *>(sm_szLoadKeys), ARRAY_COUNT( sm_szLoadKeys )-1, sizeof(sm_szLoadKeys[0]));
	if ( i < 0 )
	{
		if ( s.IsKeyHead( "REGEN", 5 ))			//	REGENx=<stat regeneration rate>
		{
			int index = atoi(s.GetKey()+5);
			if (index < 0 || index > STAT_FOOD)
				return false;
			m_iRegenRate[index] = (s.GetArgLLVal() * MSECS_PER_SEC);
			return true;
		}
		else if ( s.IsKeyHead("MAP", 3) )		//	MAPx=settings
		{
			bool ok = true;
			TemporaryString ts(s.GetKey() + 3);
			lptstr ptcStr = ts.buffer();

			for ( size_t j = 0; j < ts.size(); ++j )
			{
				if ( IsDigit(ptcStr[j]) )
					continue;

				ok = false;
				break;
			}
			if ( ok && ts.size() > 0 )
				return g_MapList.Load(atoi(ptcStr), s.GetArgRaw());

			size_t length = ts.size();

			if ( length >= 2 /*at least .X*/ && ptcStr[0] == '.' && isdigit(ptcStr[1]) )
			{
				lpctstr pszStr = &(ptcStr[1]);
				int nMapNumber = Exp_GetVal(pszStr);

				if ( g_MapList.IsMapSupported(nMapNumber) )
				{
					if ( !strnicmp(pszStr, "ALLSECTORS", 10) )
					{
						const int nSectors = CSectorList::Get()->GetSectorQty(nMapNumber);
						pszStr = s.GetArgRaw();

						if ( pszStr && *pszStr )
						{
							CScript script(pszStr);
							script.CopyParseState(s);
							for (int nIndex = 0; nIndex < nSectors; ++nIndex)
							{
								CSector* pSector = CWorldMap::GetSector(nMapNumber, nIndex);
								ASSERT(pSector);
								pSector->r_Verb(script, &g_Serv);
							}

							return true;
						}

						return false;
					}
					else if ( !strnicmp( pszStr, "SECTOR.",7 ) )
					{
                        pszStr = pszStr + 7;
                        const int iSecNumber = Exp_GetVal(pszStr);
                        pszStr = s.GetArgRaw();
                        if (pszStr && *pszStr)
                        {
                            CSector* pSector = CWorldMap::GetSector(nMapNumber, iSecNumber);
                            if (pSector)
                            {
                                CScript script(pszStr);
								script.CopyParseState(s);
                                return pSector->r_Verb(script, &g_Serv);
                            }

                        }
                        return false;
					}
				}
			}
			LOG_ERR_NOINIT(("Bad usage of MAPx. Check your sphere.ini or scripts (SERV.MAP is a read only property)\n"));
			return false;
		}
		else if ( s.IsKeyHead("PACKET", 6) )	//	PACKETx=<function name to execute upon packet>
		{
			int index = atoi(s.GetKey() + 6);
			if (( index >= 0 ) && ( index < 255 )) // why XCMD_QTY? let them hook every possible custom packet
			{
				char *args = s.GetArgRaw();
				if ( !args || ( strlen(args) >= 31 ))
					g_Log.EventError("Invalid function name for packet filtering (limit is 30 chars).\n");
				else
				{
					Str_CopyLimitNull(g_Serv.m_PacketFilter[index], args, sizeof(g_Serv.m_PacketFilter[0]));
					DEBUG_MSG_NOINIT(("PACKET FILTER: Hooked packet 0x%x with function %s.\n", index, args));
					return true;
				}
			}
			else
				g_Log.EventError("Packet filtering index %d out of range [0..254]\n", index);
		}
		else if ( s.IsKeyHead("OUTPACKET", 9) )	//	OUTPACKETx=<function name to execute upon packet>
		{
			int index = atoi(s.GetKey() + 9);
			if (( index >= 0 ) && ( index < 255 ))
			{
				char *args = s.GetArgRaw();
				if ( !args || ( strlen(args) >= 31 ))
					g_Log.EventError("Invalid function name for outgoing packet filtering (limit is 30 chars).\n");
				else
				{
					Str_CopyLimitNull(g_Serv.m_OutPacketFilter[index], args, sizeof(g_Serv.m_OutPacketFilter[0]));
					DEBUG_MSG_NOINIT(("OUTGOING PACKET FILTER: Hooked packet 0x%x with function %s.\n", index, args));
					return true;
				}
			}
			else
				g_Log.EventError("Outgoing packet filtering index %d out of range [0..254]\n", index);
		}

		return false;
	}

	switch (i)
	{
		case RC_AGREE:
			m_fAgree = (s.GetArgVal() != 0);
			break;
		case RC_ACCTFILES:	// Put acct files here.
			m_sAcctBaseDir = CSFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_ATTACKERTIMEOUT:
			m_iAttackerTimeout = s.GetArgVal();
			break;
        case RC_BACKPACKOVERLOAD:
            m_iBackpackOverload = s.GetArgVal() * WEIGHT_UNITS;
            break;
		case RC_BANKMAXWEIGHT:
			m_iBankWMax = s.GetArgVal() * WEIGHT_UNITS;
			break;
		case RC_CHATFLAGS:
			m_iChatFlags = s.GetArgVal();
			break;
		case RC_CHATSTATICCHANNELS:
			m_sChatStaticChannels = s.GetArgStr();
			break;
		case RC_CLIENTLINGER:
			m_iClientLingerTime = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_CLIENTLOGINMAXTRIES:
			{
				m_iClientLoginMaxTries = s.GetArgVal();
				if ( m_iClientLoginMaxTries < 0 )
					m_iClientLoginMaxTries = 0;
			} break;
		case RC_CLIENTLOGINTEMPBAN:
			m_iClientLoginTempBan = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;
		case RC_CLIENTMAX:
		case RC_CLIENTS:
			m_iClientsMax = s.GetArgVal();
			if (m_iClientsMax > FD_SETSIZE-1)	// Max number we can deal with. compile time thing.
				m_iClientsMax = FD_SETSIZE-1;
            else if (m_iClientsMax < 0)
                m_iClientsMax = 0;
			break;
		case RC_COLORHIDDEN:
			m_iColorHidden = (HUE_TYPE)(s.GetArgVal());
			break;
		case RC_COLORINVIS:
			m_iColorInvis = (HUE_TYPE)(s.GetArgVal());
			break;
		case RC_COLORINVISITEM:
			m_iColorInvisItem = (HUE_TYPE)(s.GetArgVal());
			break;
		case RC_COLORINVISSPELL:
			m_iColorInvisSpell = (HUE_TYPE)(s.GetArgVal());
			break;
		case RC_COMBATARCHERYMOVEMENTDELAY:
		{
			const int iVal = s.GetArgVal();
			m_iCombatArcheryMovementDelay = maximum(iVal, 0);
            break;
		}
        case RC_COMBATFLAGS:
        {
            uint uiVal = s.GetArgUVal();
            if ((uiVal & (COMBAT_PREHIT|COMBAT_SWING_NORANGE)) == (COMBAT_PREHIT|COMBAT_SWING_NORANGE))
            {
                uiVal ^= COMBAT_SWING_NORANGE;
                LOG_WARN_NOINIT("CombatFlags COMBAT_PREHIT and COMBAT_SWING_NORANGE cannot coexist. Turning off COMBAT_SWING_NORANGE.\n");
            }
            m_iCombatFlags = uiVal;
        }
		break;
        case RC_CONTAINERMAXITEMS:
        {
            const uint uiVal = s.GetArgUVal();
            if ((uiVal > 0) && (uiVal < MAX_ITEMS_CONT))
                m_iContainerMaxItems = uiVal;
        }
		break;
		case RC_CORPSENPCDECAY:
			m_iDecay_CorpseNPC = s.GetArgLLVal()*60*MSECS_PER_SEC;
			break;
		case RC_CORPSEPLAYERDECAY:
			m_iDecay_CorpsePlayer = s.GetArgLLVal()*60*MSECS_PER_SEC;
			break;
		case RC_CRIMINALTIMER:
			m_iCriminalTimer = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;
		case RC_STRIPPATH:	// Put TNG or Axis stripped files here.
			m_sStripPath = CSFile::GetMergedFileName( s.GetArgStr(), "" );
			break;
		case RC_DEADSOCKETTIME:
			m_iDeadSocketTime = s.GetArgLLVal()*60*MSECS_PER_SEC;
			break;
		case RC_DECAYTIMER:
			m_iDecay_Item = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;
		case RC_FREEZERESTARTTIME:
			m_iFreezeRestartTime = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_GAMEMINUTELENGTH:
			m_iGameMinuteLength = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_GUARDLINGER:
			m_iGuardLingerTime = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;
		case RC_HEARALL:
			g_Log.SetLogMask( s.GetArgFlag( g_Log.GetLogMask(), LOGM_PLAYER_SPEAK ));
			break;
		case RC_HITSUPDATERATE:
			m_iHitsUpdateRate = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_ITEMSMAXAMOUNT:
		{
			int iVal = s.GetArgVal();
			m_iItemsMaxAmount = minimum(iVal, UINT16_MAX);
		}
		break;
		case RC_LOG:
			g_Log.OpenLog( s.GetArgStr());
			break;
		case RC_LOGMASK:
			g_Log.SetLogMask( s.GetArgVal());
			break;
		case RC_MULFILES:
			g_Install.SetPreferPath( CSFile::GetMergedFileName( s.GetArgStr(), "" ));
			break;
		case RC_MAPCACHETIME:
			_iMapCacheTime = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
        case RC_MAPVIEWRADAR:
            m_iMapViewRadar = s.GetArgBVal();
            break;
        case RC_MAPVIEWSIZE:
            m_iMapViewSize = s.GetArgBVal();
            break;
        case RC_MAPVIEWSIZEMAX:
            m_iMapViewSizeMax = s.GetArgBVal();
            break;
		case RC_MAXCHARSPERACCOUNT:
			m_iMaxCharsPerAccount = (uchar)(s.GetArgVal());
			if ( m_iMaxCharsPerAccount > MAX_CHARS_PER_ACCT )
				m_iMaxCharsPerAccount = MAX_CHARS_PER_ACCT;
			break;
        case RC_MAXHOUSESACCOUNT:
            _iMaxHousesAccount = s.GetArgUCVal();
            break;
        case RC_MAXHOUSESPLAYER:
            _iMaxHousesPlayer = s.GetArgUCVal();
            break;
		case RC_MAXFAME:
			m_iMaxFame = s.GetArgVal();
			if (m_iMaxFame < 0)
				m_iMaxFame = 0;
			break;
		case RC_MAXKARMA:
			m_iMaxKarma = s.GetArgVal();
			if (m_iMaxKarma < m_iMinKarma)
				m_iMaxKarma = m_iMinKarma;
			break;
        case RC_MAXPOLYSTATS:
        {
            int iMax = s.GetArgVal();
            m_iMaxPolyStats = (ushort)minimum(iMax, UINT16_MAX);
            break;
        }
		case RC_MINCHARDELETETIME:
			m_iMinCharDeleteTime = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_MINKARMA:
			m_iMinKarma = s.GetArgVal();
			if (m_iMinKarma > m_iMaxKarma)
				m_iMinKarma = m_iMaxKarma;
			break;
		case RC_MURDERDECAYTIME:
			m_iMurderDecayTime = s.GetArgLLVal() * MSECS_PER_SEC;
			break;
		case RC_NOTOTIMEOUT:
			m_iNotoTimeout = s.GetArgVal();
			break;
        case RC_NPCDISTANCEHEAR:
            m_iNPCDistanceHear = s.GetArgVal();
            break;
		case RC_WOOLGROWTHTIME:
			m_iWoolGrowthTime = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;
        case RC_ITEMHITPOINTSUPDATE:
            _iItemHitpointsUpdate = s.GetArgLLVal() * MSECS_PER_SEC;
            break;
		case RC_PROFILE:
			{
				int seconds = std::max(0, s.GetArgVal());   // 0 does not enable it
				size_t threadCount = ThreadHolder::get().getActiveThreads();
				for (size_t j = 0; j < threadCount; ++j)
				{
					AbstractSphereThread* thread = static_cast<AbstractSphereThread*>(ThreadHolder::get().getThreadAt(j));
					if (thread != nullptr)
						thread->m_profile.SetActive(seconds);
				}
			}
			break;

		case RC_PLAYEREVIL:	// How much bad karma makes a player evil?
			m_iPlayerKarmaEvil = s.GetArgVal();
			if ( m_iPlayerKarmaNeutral < m_iPlayerKarmaEvil )
				m_iPlayerKarmaNeutral = m_iPlayerKarmaEvil;
			break;

		case RC_PLAYERNEUTRAL:	// How much bad karma makes a player neutral?
			m_iPlayerKarmaNeutral = s.GetArgVal();
			if ( m_iPlayerKarmaEvil > m_iPlayerKarmaNeutral )
				m_iPlayerKarmaEvil = m_iPlayerKarmaNeutral;
			break;

		case RC_GUARDSINSTANTKILL:
			m_fGuardsInstantKill = s.GetArgVal() ? true : false;
			break;

		case RC_GUARDSONMURDERERS:
			m_fGuardsOnMurderers = s.GetArgVal() ? true : false;
			break;

		case RC_CONTEXTMENULIMIT:
			m_iContextMenuLimit = s.GetArgVal();
			break;

		case RC_SCPFILES: // Get SCP files from here.
			m_sSCPBaseDir = CSFile::GetMergedFileName( s.GetArgStr(), "" );
			break;

		case RC_SECURE:
			m_fSecure = (s.GetArgVal() != 0);
			if ( !g_Serv.IsLoading() )
				g_Serv.SetSignals();
			break;

		case RC_PACKETDEATHANIMATION:
			m_iPacketDeathAnimation = s.GetArgVal() > 0 ? true : false;
			break;

		case RC_SKILLPRACTICEMAX:
			m_iSkillPracticeMax = s.GetArgVal();
			break;

		case RC_SAVEPERIOD:
			m_iSavePeriod = s.GetArgLLVal()*60*MSECS_PER_SEC;
			break;

		case RC_SPELLTIMEOUT:
			m_iSpellTimeout = s.GetArgLLVal() * MSECS_PER_SEC;
			break;

		case RC_SECTORSLEEP:
			_iSectorSleepDelay = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;

		case RC_SAVEBACKGROUND:
			m_iSaveBackgroundTime = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;

        case RC_SAVESECTORSPERTICK:
            m_iSaveSectorsPerTick = s.GetArgUVal();
            if( m_iSaveSectorsPerTick <= 0 )
                m_iSaveSectorsPerTick = 1;
            break;

        case RC_SAVESTEPMAXCOMPLEXITY:
            m_iSaveStepMaxComplexity = s.GetArgUVal();
            break;

		case RC_WORLDSAVE: // Put save files here.
			m_sWorldBaseDir = CSFile::GetMergedFileName( s.GetArgStr(), "" );
			break;

		case RC_COMMANDPREFIX:
			m_cCommandPrefix = *s.GetArgStr();
			break;

		case RC_EXPERIMENTAL:
			_uiExperimentalFlags = s.GetArgUVal();
			//PrintEFOFFlags(true, false);
			break;

		case RC_OPTIONFLAGS:
			_uiOptionFlags = s.GetArgUVal();
			//PrintEFOFFlags(false, true);
			break;

		case RC_TIMERCALLUNIT:
			_iTimerCallUnit = s.GetArgVal();
			break;

		case RC_TIMERCALL:
			if (_iTimerCallUnit)
				_iTimerCall = s.GetArgLLVal() *  MSECS_PER_SEC;
			else
				_iTimerCall = s.GetArgLLVal() * 60 * MSECS_PER_SEC;
			break;

		case RC_TOOLTIPCACHE:
			m_iTooltipCache = s.GetArgLLVal() * MSECS_PER_SEC;
			break;

		case RC_NETWORKTHREADS:
			if (g_Serv.IsLoading())
			{
				int iNetThreads = s.GetArgVal();
				//if (iNetThreads < 0)
				//	iNetThreads = 0;
				//else if (iNetThreads > 10)
				//	iNetThreads = 10;
				_uiNetworkThreads = (uint)iNetThreads;
			}
			else
				g_Log.EventError("The value of NetworkThreads cannot be modified after the server has started\n");
			break;

		case RC_NETWORKTHREADPRIORITY:
			{
                int priority = s.GetArgVal();
                if (priority < 1)
                    priority = enum_alias_cast<int>(ThreadPriority::Normal);
                else if (priority > 4)
                    priority = enum_alias_cast<int>(ThreadPriority::RealTime);
				else
                    priority = enum_alias_cast<int>(ThreadPriority::Low) + priority;

                _iNetworkThreadPriority = priority;
			}
			break;
		case RC_WALKBUFFER:
			m_iWalkBuffer = s.GetArgVal() * MSECS_PER_TENTH;
			break;
        case RC_MEDITATIONMOVEMENTABORT:
            _fMeditationMovementAbort = s.GetArgVal() > 0 ? true : false;
            break;
		default:
			return( sm_szLoadKeys[i].m_elem.SetValStr( this, s.GetArgRaw()));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;

#undef DEBUG_MSG_NOINIT
#undef LOG_ERR_NOINIT
#undef LOG_WARN_NOINIT
}

const CSpellDef * CServerConfig::GetSpellDef( SPELL_TYPE index ) const
{
    // future: underlying type for SPELL_TYPE to avoid casts
    if (index <= SPELL_NONE)
        return nullptr;
	const size_t uiIndex = (size_t)index;
    if (!m_SpellDefs.valid_index(uiIndex))
        return nullptr;
    return m_SpellDefs[uiIndex].get();
}

CSpellDef * CServerConfig::GetSpellDef( SPELL_TYPE index )
{
    // future: underlying type for SPELL_TYPE to avoid casts
    if (index <= SPELL_NONE)
        return nullptr;
    const size_t uiIndex = (size_t)index;
    if (!m_SpellDefs.valid_index(uiIndex))
        return nullptr;
    return m_SpellDefs[uiIndex].get();
}

lpctstr CServerConfig::GetSkillKey( SKILL_TYPE index ) const
{
    // future: underlying type for SPELL_TYPE to avoid casts
    if (index < 0)
        return nullptr;
	const size_t uiIndex = (size_t)index;
    if (!m_SkillIndexDefs.valid_index(uiIndex))
        return nullptr;
    return m_SkillIndexDefs[uiIndex]->GetKey();
}

const CSkillDef* CServerConfig::GetSkillDef( SKILL_TYPE index ) const
{
    if (index < 0)
        return nullptr;
	const size_t uiIndex = (size_t)index;
    if (!m_SkillIndexDefs.valid_index(uiIndex) )
        return nullptr;
    return m_SkillIndexDefs[uiIndex].get();
}

CSkillDef* CServerConfig::GetSkillDef( SKILL_TYPE index )
{
    if (index < 0)
        return nullptr;
	const size_t uiIndex = (size_t)index;
    if (!m_SkillIndexDefs.valid_index(uiIndex) )
        return nullptr;
    return m_SkillIndexDefs[uiIndex].get();
}

const CSkillDef* CServerConfig::FindSkillDef( lpctstr ptcKey ) const
{
    // Find the skill name in the alpha sorted list.
    // RETURN: SKILL_NONE = error.
	const size_t i = m_SkillNameDefs.find_sorted(ptcKey);
    if ( i == sl::scont_bad_index() )
        return nullptr;
    return m_SkillNameDefs[i].get();
}

const CSkillDef * CServerConfig::SkillLookup( lpctstr ptcKey )
{
	ADDTOCALLSTACK("CServerConfig::SkillLookup");

	const size_t iLen = strlen( ptcKey );
    const CSkillDef * pDef;
	for ( size_t i = 0; i < m_SkillIndexDefs.size(); ++i )
	{
		pDef = static_cast<const CSkillDef *>(m_SkillIndexDefs[i].get());
		ASSERT(pDef);
		if ( !strnicmp(ptcKey, (pDef->m_sName.IsEmpty() ? pDef->GetKey() : pDef->m_sName.GetBuffer()), iLen) )
			return pDef;
	}
	return nullptr;
}


bool CServerConfig::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallParent);
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CServerConfig::r_WriteVal");
	EXC_TRY("WriteVal");
	// Just do stats values for now.
	int index = FindCAssocRegTableHeadSorted( ptcKey, reinterpret_cast<lpctstr const *>(sm_szLoadKeys), ARRAY_COUNT(sm_szLoadKeys) - 1, sizeof(sm_szLoadKeys[0]) );
	if ( index < 0 )
	{
		if ( !strnicmp( ptcKey, "REGEN", 5 ))
		{
			index = atoi(ptcKey+5);
			if (( index < 0 ) || ( index >= STAT_QTY ))
				return false;
			sVal.FormatLLVal(m_iRegenRate[index] / MSECS_PER_SEC);
			return true;
		}

		if ( !strnicmp( ptcKey, "LOOKUPSKILL", 11 ) )
		{
			ptcKey	+= 12;
			SKIP_SEPARATORS( ptcKey );
			GETNONWHITESPACE( ptcKey );

			const CSkillDef * pSkillDef = SkillLookup( ptcKey );
			if ( !pSkillDef )
				sVal.FormatVal( -1 );
			else
				sVal.FormatVal( pSkillDef->GetResourceID().GetResIndex() );
			return true;
		}

		if ( !strnicmp(ptcKey, "MAP(", 4) )
		{
			ptcKey += 4;
			sVal.SetValFalse();

			// Parse the arguments after the round brackets
			tchar * pszArgsNext;
			Str_Parse( const_cast<tchar*>(ptcKey), &pszArgsNext, ")" );

			CPointMap pt;
			if ( IsDigit( ptcKey[0] ) || ptcKey[0] == '-' )
			{
				// Set the default values
				pt.m_map = 0;
				pt.m_z = 0;

				// Parse the arguments inside the round brackets
				tchar * ppVal[4];
				int iArgs = Str_ParseCmds( const_cast<tchar*>(ptcKey), ppVal, ARRAY_COUNT( ppVal ), "," );

				switch ( iArgs )
				{
					default:
					case 4:
						if (IsDigit(ppVal[3][0]))
						{
							pt.m_map = (byte)(atoi(ppVal[3]));
						}
						FALLTHROUGH;

					case 3:
						if ( IsDigit(ppVal[2][0]) || (( iArgs == 4 ) && ( ppVal[2][0] == '-' )) )
						{
							pt.m_z = (char)(( iArgs == 4 ) ? atoi(ppVal[2]) : 0);
							if ( iArgs == 3 )
								pt.m_map = (byte)(atoi(ppVal[2]));
						}
						FALLTHROUGH;

					case 2:
						pt.m_y = (short)(atoi(ppVal[1]));
						FALLTHROUGH;

					case 1:
						pt.m_x = (short)(atoi(ppVal[0]));
						FALLTHROUGH;

					case 0:
						break;
				}
			}

			ptcKey = pszArgsNext;
			SKIP_SEPARATORS(ptcKey);
			return pt.r_WriteVal(ptcKey, sVal);
		}

		if ( !strnicmp( ptcKey, "MAPLIST.", 8) )
		{
			lpctstr pszCmd = ptcKey + 8;
			int iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.SetValFalse();

			if (!*pszCmd)
			{
				sVal.FormatVal(g_MapList.IsMapSupported(iNumber));
			}
			else
			{
				if (g_MapList.IsMapSupported(iNumber))
				{
					if (!strnicmp(pszCmd, "BOUND.X", 7))
						sVal.FormatVal(g_MapList.GetMapSizeX(iNumber));
					else if (!strnicmp(pszCmd, "BOUND.Y", 7))
						sVal.FormatVal(g_MapList.GetMapSizeY(iNumber));
					else if (!strnicmp(pszCmd, "CENTER.X", 8))
						sVal.FormatVal(g_MapList.GetMapCenterX(iNumber));
					else if (!strnicmp(pszCmd, "CENTER.Y", 8))
						sVal.FormatVal(g_MapList.GetMapCenterY(iNumber));
					else if (!strnicmp(pszCmd, "SECTOR.", 7))
					{
						pszCmd += 7;
						const CSectorList* pSectors = CSectorList::Get();

						if (!strnicmp(pszCmd, "SIZE", 4))
							sVal.FormatVal(pSectors->GetSectorSize(iNumber));
						else if (!strnicmp(pszCmd, "ROWS", 4))
							sVal.FormatVal(pSectors->GetSectorRows(iNumber));
						else if (!strnicmp(pszCmd, "COLS", 4))
							sVal.FormatVal(pSectors->GetSectorCols(iNumber));
						else if (!strnicmp(pszCmd, "QTY", 3))
							sVal.FormatVal(pSectors->GetSectorQty(iNumber));
						else
							return false;
					}
				}
				else
					return false;
			}

			return true;
		}

		if ( !strnicmp( ptcKey, "MAP", 3 ))
		{
			ptcKey = ptcKey + 3;
			int iMapNumber = Exp_GetVal(ptcKey);
			SKIP_SEPARATORS(ptcKey);
			sVal.SetValFalse();

			if ( g_MapList.IsMapSupported(iMapNumber) )
			{
				if ( !strnicmp( ptcKey, "SECTOR", 6 ))
				{
					ptcKey = ptcKey + 6;
					int iSecNumber = Exp_GetVal(ptcKey);
					SKIP_SEPARATORS(ptcKey);
					CSector* pSector = CWorldMap::GetSector(iMapNumber, iSecNumber);
					return !pSector ? false : pSector->r_WriteVal(ptcKey, sVal, pSrc);
				}
			}
			g_Log.EventError("Unsupported Map %d\n", iMapNumber);
			return false;
		}

		if (!strnicmp(ptcKey, "MULTIS.", 7))
		{
			bool bMulti = !strnicmp(ptcKey, "MULTIS.", 7);
			lpctstr pszCmd = ptcKey + 7;
			CItemMulti* pMulti = nullptr;
			size_t x = 0;

			if (!strnicmp(pszCmd, "COUNT", 5))
			{
				for (size_t i = 0; i < g_World.m_Multis.size(); ++i)
				{
					pMulti = g_World.m_Multis[i].get();
					if (pMulti == nullptr)
						continue;

					if ((pMulti->GetType() == IT_MULTI) && bMulti)
						++x;
					else if ((pMulti->GetType() == IT_MULTI_CUSTOM) && bMulti)
						++x;
					else if ((pMulti->GetType() == IT_SHIP) && bMulti)
						++x;
				}

				sVal.FormatSTVal(x);
				return true;
			}

			size_t iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.SetValFalse();

			for (size_t i = 0; i < g_World.m_Multis.size(); ++i)
			{
				pMulti = g_World.m_Multis[i].get();
				if (pMulti == nullptr)
					continue;

				if ((pMulti->GetType() == IT_MULTI) && bMulti)
				{
					if (iNumber == x)
						return pMulti->r_WriteVal(pszCmd, sVal, pSrc);
					++x;
				}
				else if ((pMulti->GetType() == IT_MULTI_CUSTOM) && bMulti)
				{
					if (iNumber == x)
						return pMulti->r_WriteVal(pszCmd, sVal, pSrc);
					++x;
				}
				else if ((pMulti->GetType() == IT_SHIP) && bMulti)
				{
					if (iNumber == x)
						return pMulti->r_WriteVal(pszCmd, sVal, pSrc);
					++x;
				}
			}

			return true;
		}

		if (!strnicmp( ptcKey, "FUNCTIONS.", 10))
		{
			lpctstr pszCmd = ptcKey + 10;

			if ( !strnicmp( pszCmd, "COUNT", 5 ))
			{
				sVal.FormatSTVal(m_Functions.size());
				return true;
			}
			else if ( m_Functions.ContainsKey(pszCmd) )
			{
				sVal.FormatVal((int)GetPrivCommandLevel(pszCmd));
				return true;
			}

			int iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.SetValFalse();

			if (iNumber < 0 || iNumber >= (int) m_Functions.size()) //invalid index can potentially crash the server, this check is strongly needed
			{
				g_Log.EventError("Invalid command index %d\n",iNumber);
				return false;
			}

			if ( *pszCmd == '\0')
			{
				sVal = m_Functions[iNumber]->GetName();
				return true;
			}
			else if ( !strnicmp( pszCmd, "PLEVEL", 5 ))
			{
				sVal.FormatVal((int)(GetPrivCommandLevel(m_Functions[iNumber]->GetName())));
				return true;
			}
		}

		if ( ( !strnicmp( ptcKey, "GUILDSTONES.", 12) ) || ( !strnicmp( ptcKey, "TOWNSTONES.", 11) ) )
		{
			bool bGuild = !strnicmp( ptcKey, "GUILDSTONES.",12);
			lpctstr pszCmd = ptcKey + 11 + ( (bGuild) ? 1 : 0 );
			CItemStone * pStone = nullptr;
			size_t x = 0;

			if (!strnicmp(pszCmd,"COUNT", 5))
			{
				for ( size_t i = 0; i < g_World.m_Stones.size(); ++i )
				{
					pStone = g_World.m_Stones[i].get();
					if ( pStone == nullptr )
						continue;

					if (( pStone->GetType() == IT_STONE_GUILD ) && bGuild )
						++x;
					else if (( pStone->GetType() == IT_STONE_TOWN ) && !bGuild )
						++x;
				}

				sVal.FormatSTVal(x);
				return true;
			}

			size_t iNumber = Exp_GetVal(pszCmd);
			SKIP_SEPARATORS(pszCmd);
			sVal.SetValFalse();

			for ( size_t i = 0; i < g_World.m_Stones.size(); ++i )
			{
				pStone = g_World.m_Stones[i].get();
				if ( pStone == nullptr )
					continue;

				if (( pStone->GetType() == IT_STONE_GUILD ) && bGuild )
				{
					if ( iNumber == x )
						return pStone->r_WriteVal(pszCmd, sVal, pSrc);
					++x;
				}
				else if (( pStone->GetType() == IT_STONE_TOWN ) && !bGuild )
				{
					if ( iNumber == x )
						return pStone->r_WriteVal(pszCmd, sVal, pSrc) ;
					++x;
				}
			}

			return true;
		}

		if ( !strnicmp( ptcKey, "CLIENT.",7))
		{
			ptcKey += 7;
			uint cli_num = (Exp_GetUVal(ptcKey));
			uint i = 0;
			SKIP_SEPARATORS(ptcKey);

			if (cli_num >= g_Serv.StatGet( SERV_STAT_CLIENTS ))
				return false;

			sVal.SetValFalse();
			ClientIterator it;
			for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
			{
				if ( cli_num == i )
				{
					CChar * pChar = pClient->GetChar();

					if ( *ptcKey == '\0' ) // checking reference
						sVal.FormatVal( pChar != nullptr ? 1 : 0 );
					else if ( pChar != nullptr )
						return pChar->r_WriteVal(ptcKey, sVal, pSrc, false); // it calls also CClient::r_WriteVal
					else
						return pClient->r_WriteVal(ptcKey, sVal, pSrc, false);
				}
				++i;
			}
			return true;
		}

		if ( !strnicmp(ptcKey, "TILEDATA.", 9) )
		{
			ptcKey += 9;

			bool fTerrain = false;
			if (!strnicmp(ptcKey, "TERRAIN(", 8))
			{
				ptcKey += 8;
				fTerrain = true;
			}
			else if (!strnicmp(ptcKey, "ITEM(", 5))
			{
				ptcKey += 5;
			}
            else
            {
                return false;
            }

			// Get the position of the arguments after the round brackets
			tchar * pszArgsNext;
			if (! Str_Parse(const_cast<tchar*>(ptcKey), &pszArgsNext, ")") )
				return false;

			uint id = Exp_GetUVal(ptcKey);
			if ( fTerrain && (id >= TERRAIN_QTY) )
			{
				// Invalid id
				return false;
			}

			ptcKey = pszArgsNext;
			if (ptcKey[0] != '.')
				return false;
			SKIP_SEPARATORS(ptcKey);
			if (fTerrain)
			{
				enum {ITATTR_FLAGS, ITATTR_UNK, ITATTR_IDX, ITATTR_NAME};
				int iAttr = 0;
				if (!strnicmp(ptcKey, "FLAGS", 5))			iAttr = ITATTR_FLAGS;
				else if (!strnicmp(ptcKey, "UNK", 7))		iAttr = ITATTR_UNK;
				else if (!strnicmp(ptcKey, "INDEX", 5))		iAttr = ITATTR_IDX;
				else if (!strnicmp(ptcKey, "NAME", 4))		iAttr = ITATTR_NAME;
				else
				{
					// Invalid attr
					return false;
				}
				CUOTerrainInfo landInfo( (TERRAIN_TYPE)id );
				switch (iAttr)
				{
					case ITATTR_FLAGS:	sVal.FormatDWVal(landInfo.m_flags);		break;
					case ITATTR_UNK:	sVal.FormatDWVal(landInfo.m_unknown);	break;
					case ITATTR_IDX:	sVal.FormatWVal(landInfo.m_index);		break;
					case ITATTR_NAME:	sVal = landInfo.m_name;					break;
					default: return false;
				}
			}
			else
			{
				enum {TTATTR_FLAGS, TTATTR_WEIGHT, TTATTR_LAYER, TTATTR_UNK11, TTATTR_ANIM, TTATTR_HUE, TTATTR_LIGHT, TTATTR_HEIGHT, TTATTR_NAME};
				int iAttr = 0;
				if (!strnicmp(ptcKey, "FLAGS", 5))			iAttr = TTATTR_FLAGS;
				else if (!strnicmp(ptcKey, "WEIGHT", 6))	iAttr = TTATTR_WEIGHT;
				else if (!strnicmp(ptcKey, "LAYER", 5))		iAttr = TTATTR_LAYER;
				else if (!strnicmp(ptcKey, "UNK11", 5))		iAttr = TTATTR_UNK11;
				else if (!strnicmp(ptcKey, "ANIM", 4))		iAttr = TTATTR_ANIM;
				else if (!strnicmp(ptcKey, "HUE", 5))		iAttr = TTATTR_HUE;
                else if (!strnicmp(ptcKey, "LIGHT", 5))		iAttr = TTATTR_LIGHT;
				else if (!strnicmp(ptcKey, "HEIGHT", 6))	iAttr = TTATTR_HEIGHT;
				else if (!strnicmp(ptcKey, "NAME", 4))		iAttr = TTATTR_NAME;
				else
				{
					// Invalid attr
					return false;
				}
				CUOItemInfo itemInfo((ITEMID_TYPE)id);
				switch (iAttr)
				{
					case TTATTR_FLAGS:	sVal.FormatU64Val(itemInfo.m_flags);    break;
					case TTATTR_WEIGHT:	sVal.FormatBVal(itemInfo.m_weight);	    break;
					case TTATTR_LAYER:	sVal.FormatBVal(itemInfo.m_layer);	    break;
					case TTATTR_UNK11:	sVal.FormatDWVal(itemInfo.m_dwUnk11);   break;
					case TTATTR_ANIM:	sVal.FormatWVal(itemInfo.m_wAnim);	    break;
					case TTATTR_HUE:	sVal.FormatWVal(itemInfo.m_wHue);	    break;
                    case TTATTR_LIGHT:	sVal.FormatWVal(itemInfo.m_wLightIndex);break;
					case TTATTR_HEIGHT:	sVal.FormatBVal(itemInfo.m_height);	    break;
					case TTATTR_NAME:	sVal = itemInfo.m_name;				    break;
					default: return false;
				}
			}
			return true;
		}

		return false;
	}

	switch (index)
	{
        case RC_ATTACKERTIMEOUT:
            sVal.FormatVal(m_iAttackerTimeout);
            break;
        case RC_BACKPACKOVERLOAD:
            sVal.FormatVal(m_iBackpackOverload / WEIGHT_UNITS);
            break;
        case RC_BANKMAXWEIGHT:
            sVal.FormatVal(m_iBankWMax / WEIGHT_UNITS);
            break;

        case RC_BUILD:
        case RC_BUILDNUM:
            sVal.FormatVal(SPHERE_BUILD_VER);
        break;

        case RC_BUILDBRANCH:
            sVal = SPHERE_BUILD_BRANCH_NAME;
        break;

        case RC_BUILDSTR:
#ifdef __GITREVISION__
            sVal.Format("%s (%d)", __GITBRANCH__, __GITREVISION__);
#else
            sVal.Format("%s (%d)", SPHERE_BUILD_BRANCH_NAME, __DATE__);
#endif
            break;

        case RC_CHATFLAGS:
			sVal.FormatHex(m_iChatFlags);
			break;
		case RC_CHATSTATICCHANNELS:
			sVal = m_sChatStaticChannels;
			break;
		case RC_CLIENTLINGER:
			sVal.FormatLLVal( m_iClientLingerTime / MSECS_PER_SEC );
			break;
		case RC_COLORHIDDEN:
			sVal.FormatHex( m_iColorHidden );
			break;
		case RC_COLORINVIS:
			sVal.FormatHex( m_iColorInvis );
			break;
		case RC_COLORINVISITEM:
			sVal.FormatHex( m_iColorInvisItem );
			break;
		case RC_COLORINVISSPELL:
			sVal.FormatHex( m_iColorInvisSpell );
			break;
		case RC_CORPSENPCDECAY:
			sVal.FormatLLVal( m_iDecay_CorpseNPC / (60*MSECS_PER_SEC));
			break;
		case RC_CORPSEPLAYERDECAY:
			sVal.FormatLLVal( m_iDecay_CorpsePlayer / (60*MSECS_PER_SEC));
			break;
		case RC_CRIMINALTIMER:
			sVal.FormatLLVal( m_iCriminalTimer / (60*MSECS_PER_SEC));
			break;
		case RC_DEADSOCKETTIME:
			sVal.FormatLLVal( m_iDeadSocketTime / (60*MSECS_PER_SEC));
			break;
		case RC_DECAYTIMER:
			sVal.FormatLLVal( m_iDecay_Item / (60*MSECS_PER_SEC));
			break;
		case RC_FREEZERESTARTTIME:
			sVal.FormatLLVal(m_iFreezeRestartTime / MSECS_PER_SEC);
			break;
		case RC_GAMEMINUTELENGTH:
			sVal.FormatLLVal(m_iGameMinuteLength / MSECS_PER_SEC);
			break;
		case RC_GUARDLINGER:
			sVal.FormatLLVal( m_iGuardLingerTime / (60*MSECS_PER_SEC));
			break;
		case RC_HEARALL:
			sVal.FormatVal( g_Log.GetLogMask() & LOGM_PLAYER_SPEAK );
			break;
		case RC_HITSUPDATERATE:
			sVal.FormatLLVal( m_iHitsUpdateRate / MSECS_PER_SEC );
			break;
		case RC_LOG:
			sVal = g_Log.GetLogDir();
			break;
		case RC_LOGMASK:
			sVal.FormatHex( g_Log.GetLogMask());
			break;
		case RC_MULFILES:
			sVal = g_Install.GetPreferPath(nullptr);
			break;
		case RC_MAPCACHETIME:
			sVal.FormatLLVal( _iMapCacheTime / MSECS_PER_SEC );
			break;
        case RC_MAPVIEWRADAR:
            sVal.FormatBVal(m_iMapViewRadar);
            break;
        case RC_MAPVIEWSIZE:
            sVal.FormatBVal(m_iMapViewSize);
            break;
        case RC_MAPVIEWSIZEMAX:
            sVal.FormatBVal(m_iMapViewSizeMax);
            break;
		case RC_NOTOTIMEOUT:
			sVal.FormatVal(m_iNotoTimeout);
			break;
        case RC_NPCDISTANCEHEAR:
            sVal.FormatVal(m_iNPCDistanceHear);
            break;
        case RC_MAXHOUSESACCOUNT:
            sVal.FormatUCVal(_iMaxHousesAccount);
            break;
        case RC_MAXHOUSESPLAYER:
            sVal.FormatUCVal(_iMaxHousesPlayer);
            break;
		case RC_MAXFAME:
			sVal.FormatVal( m_iMaxFame );
			break;
		case RC_MAXKARMA:
			sVal.FormatVal( m_iMaxKarma );
			break;
		case RC_MINCHARDELETETIME:
			sVal.FormatLLVal( m_iMinCharDeleteTime / MSECS_PER_SEC );
			break;
		case RC_MINKARMA:
			sVal.FormatVal( m_iMinKarma );
			break;
		case RC_MURDERDECAYTIME:
			sVal.FormatLLVal( m_iMurderDecayTime / (60*MSECS_PER_SEC ));
			break;
		case RC_WOOLGROWTHTIME:
			sVal.FormatLLVal( m_iWoolGrowthTime /( 60*MSECS_PER_SEC ));
			break;
        case RC_ITEMHITPOINTSUPDATE:
            sVal.FormatLLVal(_iItemHitpointsUpdate / MSECS_PER_SEC);
            break;
		case RC_PROFILE:
			sVal.FormatVal(GetCurrentProfileData().GetActiveWindow());
			break;
		case RC_RTICKS:
			{
				if ( ptcKey[6] != '.' )
					sVal.FormatLLVal((llong)(CSTime::GetCurrentTime().GetTime()));
				else
				{
					ptcKey += 6;
					SKIP_SEPARATORS(ptcKey);
					if ( !strnicmp("FROMTIME", ptcKey, 8) )
					{
						ptcKey += 8;
						GETNONWHITESPACE(ptcKey);
						int64 piVal[6];

						// year, month, day, hour, minute, second
						size_t iQty = Str_ParseCmds(const_cast<tchar*>(ptcKey), piVal, ARRAY_COUNT(piVal));
						if ( iQty != 6 )
							return false;

						CSTime datetime((int)(piVal[0]), (int)(piVal[1]), (int)(piVal[2]), (int)(piVal[3]), (int)(piVal[4]), (int)(piVal[5]));
						if ( datetime.GetTime() == -1 )
							sVal.FormatVal(-1);
						else
							sVal.FormatLLVal((llong)(datetime.GetTime()));
					}
					else if ( !strnicmp("FORMAT", ptcKey, 6) )
					{
						ptcKey += 6;
						GETNONWHITESPACE( ptcKey );
						tchar *ppVal[2];

						// timestamp, formatstr
						size_t iQty = Str_ParseCmds(const_cast<tchar*>(ptcKey), ppVal, ARRAY_COUNT(ppVal));
						if ( iQty < 1 )
							return false;

						time_t lTime = Exp_GetVal(ppVal[0]);

						CSTime datetime(lTime);
						sVal = datetime.Format(iQty > 1? ppVal[1]: nullptr);
					}
				}
			}
			break;
		case RC_RTIME:
			{
				if ( ptcKey[5] != '.' )
					sVal = CSTime::GetCurrentTime().Format(nullptr);
				else
				{
					ptcKey += 5;
					SKIP_SEPARATORS(ptcKey);
					if (!strnicmp("GMTFORMAT",ptcKey,9))
					{
						ptcKey += 9;
						GETNONWHITESPACE( ptcKey );
						sVal = CSTime::GetCurrentTime().FormatGmt(ptcKey);
					}
					if (!strnicmp("FORMAT",ptcKey,6))
					{
						ptcKey += 6;
						GETNONWHITESPACE( ptcKey );
						sVal = CSTime::GetCurrentTime().Format(ptcKey);
					}
				}
			} break;
		case RC_SAVEPERIOD:
			sVal.FormatLLVal( m_iSavePeriod / (60 * MSECS_PER_SEC));
			break;
		case RC_SECTORSLEEP:
			sVal.FormatLLVal(_iSectorSleepDelay / (60 * MSECS_PER_SEC));
			break;
		case RC_SAVEBACKGROUND:
			sVal.FormatLLVal( m_iSaveBackgroundTime / (60 * MSECS_PER_SEC));
			break;
        case RC_SAVESECTORSPERTICK:
            sVal.FormatVal( m_iSaveSectorsPerTick );
            break;
        case RC_SAVESTEPMAXCOMPLEXITY:
            sVal.FormatVal( m_iSaveStepMaxComplexity );
            break;
		case RC_SPELLTIMEOUT:
			sVal.FormatLLVal(m_iSpellTimeout / MSECS_PER_SEC);
			break;
		case RC_GUILDS:
			sVal.FormatSTVal(g_World.m_Stones.size());
			break;
        case RC_TICKPERIOD:
            sVal.FormatVal( TICKS_PER_SEC );
            break;
		case RC_TIMEUP:
			sVal.FormatLLVal( CWorldGameTime::GetCurrentTime().GetTimeDiff( g_World._iTimeStartup ) / MSECS_PER_SEC );
			break;
		case RC_TIMERCALL:
			if (_iTimerCallUnit)
				sVal.FormatLLVal(_iTimerCall / MSECS_PER_SEC);
			else
				sVal.FormatLLVal(_iTimerCall / 60 * MSECS_PER_SEC);
			break;
		case RC_VERSION:
			sVal = g_sServerDescription.c_str();
			break;
		case RC_EXPERIMENTAL:
			sVal.FormatHex( _uiExperimentalFlags );
			//PrintEFOFFlags(true, false, pSrc);
			break;
		case RC_OPTIONFLAGS:
			sVal.FormatHex( _uiOptionFlags );
			//PrintEFOFFlags(false, true, pSrc);
			break;
		case RC_CLIENTS:		// this is handled by CServerDef as SV_CLIENTS
			return false;
		case RC_PLAYEREVIL:
			sVal.FormatVal( m_iPlayerKarmaEvil );
			break;
		case RC_PLAYERNEUTRAL:
			sVal.FormatVal( m_iPlayerKarmaNeutral );
			break;
		case RC_TOOLTIPCACHE:
			sVal.FormatLLVal( m_iTooltipCache / MSECS_PER_SEC );
			break;
		case RC_GUARDSINSTANTKILL:
			sVal.FormatVal(m_fGuardsInstantKill);
			break;
		case RC_GUARDSONMURDERERS:
			sVal.FormatVal(m_fGuardsOnMurderers);
			break;
		case RC_CONTEXTMENULIMIT:
			sVal.FormatVal(m_iContextMenuLimit);
			break;
		case RC_WALKBUFFER:
			sVal.FormatLLVal(m_iWalkBuffer);
			break;
		default:
			return( sm_szLoadKeys[index].m_elem.GetValStr( this, sVal ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}


//*************************************************************

bool CServerConfig::IsConsoleCmd( tchar ch ) const
{
	ADDTOCALLSTACK("CServerConfig::IsConsoleCmd");
	return (ch == '.' || ch == '/' );
}

SKILL_TYPE CServerConfig::FindSkillKey( lpctstr ptcKey ) const
{
	ADDTOCALLSTACK("CServerConfig::FindSkillKey");
	// Find the skill name in the alpha sorted list.
	// RETURN: SKILL_NONE = error.

	if ( IsDigit( ptcKey[0] ) )
	{
		SKILL_TYPE skill = (SKILL_TYPE)(Exp_GetVal(ptcKey));
		if ( ( !CChar::IsSkillBase(skill) || !m_SkillIndexDefs.valid_index(skill) ) && !CChar::IsSkillNPC(skill) )
			return SKILL_NONE;
		return skill;
	}

	const CSkillDef * pSkillDef = FindSkillDef( ptcKey );
	if ( pSkillDef == nullptr )
		return SKILL_NONE;
	return (SKILL_TYPE)(pSkillDef->GetResourceID().GetResIndex());
}


static constexpr lpctstr _ptcStatName[STAT_QTY] = // not alphabetically sorted obviously.
{
    "STR",
    "INT",
    "DEX",
    "FOOD"
};

STAT_TYPE CServerConfig::GetStatKey( lpctstr ptcKey ) // static
{
	//ADDTOCALLSTACK_DEBUG("CServerConfig::GetStatKey");
	return (STAT_TYPE) FindTable( ptcKey, _ptcStatName, ARRAY_COUNT(_ptcStatName));
}

lpctstr CServerConfig::GetStatName(STAT_TYPE iKey) // static
{
    //ADDTOCALLSTACK_DEBUG("CServerConfig::GetStatName");
    ASSERT(iKey >= STAT_STR && iKey < STAT_QTY);
    return _ptcStatName[iKey];
}


int CServerConfig::GetSpellEffect( SPELL_TYPE spell, int iSkillVal ) const
{
	ADDTOCALLSTACK("CServerConfig::GetSpellEffect");
	// NOTE: Any randomizing of the effect must be done by varying the skill level .
	// iSkillVal = 0-1000
	if ( !spell )
		return 0;
	const CSpellDef * pSpellDef = GetSpellDef( spell );
	if ( pSpellDef == nullptr )
		return 0;
	return pSpellDef->m_vcEffect.GetLinear( iSkillVal );
}

lpctstr CServerConfig::GetRune( tchar ch ) const
{
    uint index = (uint)(toupper(ch) - 'A');
    if ( ! m_Runes.IsValidIndex(index))
        return "?";
    return m_Runes[index]->GetBuffer();
}

lpctstr CServerConfig::GetNotoTitle( int iLevel, bool bFemale ) const
{
	ADDTOCALLSTACK("CServerConfig::GetNotoTitle");
	// Retrieve the title used for the given noto level and gender

	if ( !m_NotoTitles.IsValidIndex(iLevel) )
	{
		return "";
	}
	else
	{
		// check if a female title is present
		lpctstr pFemaleTitle = strchr(m_NotoTitles[iLevel]->GetBuffer(), ',');
		if (pFemaleTitle == nullptr)
			return m_NotoTitles[iLevel]->GetBuffer();

		++pFemaleTitle;
		if (bFemale)
			return pFemaleTitle;

		// copy string so that it can be null-terminated without modifying m_NotoTitles
		tchar* pTitle = Str_GetTemp();
        Str_CopyLimitNull(pTitle, m_NotoTitles[iLevel]->GetBuffer(), (int)(m_NotoTitles[iLevel]->GetLength() - strlen(pFemaleTitle)));
		return pTitle;
	}
}

bool CServerConfig::IsValidEmailAddressFormat( lpctstr pszEmail ) // static
{
	ADDTOCALLSTACK("CServerConfig::IsValidEmailAddressFormat");
	// what are the invalid email name chars ?
	// Valid characters are, numbers, letters, underscore "_", dash "-" and the dot ".").

	size_t len1 = strlen( pszEmail );
	if ( len1 <= 0 || len1 > 128 )
		return false;

	tchar szEmailStrip[256];
	size_t len2 = Str_GetBare( szEmailStrip, pszEmail, sizeof(szEmailStrip), " !\"#%&()*,/:;<=>?[\\]^{|}'`+" );
	if ( len2 != len1 )
		return false;

	lpctstr pszAt =strchr( pszEmail, '@' );
	if ( ! pszAt )
		return false;
	if ( pszAt == pszEmail )
		return false;
	if ( ! strchr( pszAt, '.' ) )
		return false;

	return true;
}

CServerRef CServerConfig::Server_GetDef( size_t index )
{
	ADDTOCALLSTACK("CServerConfig::Server_GetDef");
	if ( ! m_Servers.IsValidIndex(index))
		return nullptr;
	return CServerRef( static_cast <CServerDef*>( m_Servers[index] ));
}

CWebPageDef * CServerConfig::FindWebPage( lpctstr pszPath ) const
{
	ADDTOCALLSTACK("CServerConfig::FindWebPage");
	if ( pszPath == nullptr )
	{
		if (m_WebPages.empty())
			return nullptr;
		// Take this as the default page.
		return static_cast <CWebPageDef*>( m_WebPages[0].get() );
	}

	lpctstr pszTitle = CSFile::GetFilesTitle(pszPath);

	if ( pszTitle == nullptr || pszTitle[0] == '\0' )
	{
		// This is just the root index.
		if (m_WebPages.size() <= 0 )
			return nullptr;
		// Take this as the default page.
		return static_cast <CWebPageDef*>( m_WebPages[0].get() );
	}

	for ( size_t i = 0; i < m_WebPages.size(); ++i )
	{
		if ( m_WebPages[i] == nullptr )	// not sure why this would happen
			continue;

		CWebPageDef * pWeb = static_cast <CWebPageDef*>(m_WebPages[i].get() );
		ASSERT(pWeb);
		if ( pWeb->IsMatch(pszTitle))
			return pWeb;
	}

	return nullptr;
}

bool CServerConfig::IsObscene( lpctstr pszText ) const
{
	ADDTOCALLSTACK("CServerConfig::IsObscene");
	// does this text contain obscene content?
	// NOTE: allow partial match syntax *fuck* or ass (alone)

	CSString match;
	for ( size_t i = 0; i < m_Obscene.size(); ++i )
	{
		match.Resize(int(2 * strlen(m_Obscene[i])));
        lptstr ptcMatch = const_cast<lptstr>(match.GetBuffer()); // Do that just because we know that the buffer has the right size and we won't write past the buffer's end.
		snprintf(ptcMatch, match.GetCapacity(), "%s%s%s", "*", m_Obscene[i], "*");
		MATCH_TYPE ematch = Str_Match( ptcMatch , pszText );

		if ( ematch == MATCH_VALID )
			return true;
	}
	return false;
}

bool CServerConfig::SetKRDialogMap(dword rid, dword idKRDialog)
{
	ADDTOCALLSTACK("CServerConfig::SetKRDialogMap");
	// Defines a link between the given ResourceID and KR DialogID, so that
	// the dialogs of KR clients can be handled in scripts.
	KRGumpsMap::const_iterator it;

	// prevent double mapping of same dialog
	it = m_mapKRGumps.find(rid);
	if ( it != m_mapKRGumps.end() )
	{
		if ( it->second == idKRDialog )	// already mapped to this kr dialog
			return true;

		g_Log.Event( LOGL_WARN, "Dialog '%s' is already mapped to KR dialog '%u'.\n", ResourceGetName(CResourceID(RES_DIALOG, rid)), it->second);
	}

	// prevent double mapping of KR dialog
	KRGumpsMap::const_iterator end = m_mapKRGumps.end();
	for (it = m_mapKRGumps.begin(); it != end; ++it)
	{
		if (it->second != idKRDialog)
			continue;

		DEBUG_ERR(("KR Dialog '%u' is already mapped to dialog '%s'.\n", idKRDialog, ResourceGetName(CResourceID(RES_DIALOG, it->first))));
		return false;
	}

	m_mapKRGumps[rid] = idKRDialog;
	return true;
}

dword CServerConfig::GetKRDialogMap(dword idKRDialog)
{
	ADDTOCALLSTACK("CServerConfig::GetKRDialogMap");
	// Translates the given KR DialogID into the ResourceID of its scripted dialog.
	// Returns 0 on failure
	for (KRGumpsMap::const_iterator it = m_mapKRGumps.begin(); it != m_mapKRGumps.end(); ++it)
	{
		if (it->second != idKRDialog)
			continue;

		return it->first;
	}

	return 0;
}

dword CServerConfig::GetKRDialog(dword rid)
{
	ADDTOCALLSTACK("CServerConfig::GetKRDialog");
	// Translates the given ResourceID into it's equivalent KR DialogID.
	// Returns 0 on failure
	KRGumpsMap::const_iterator it = m_mapKRGumps.find(rid);
	if (it != m_mapKRGumps.end())
		return it->second;

	return 0;
}

const CUOMulti * CServerConfig::GetMultiItemDefs( CItem * pItem )
{
	ADDTOCALLSTACK("CServerConfig::GetMultiItemDefs(CItem*)");
	if ( !pItem )
		return nullptr;

	CItemMultiCustom *pItemMultiCustom = dynamic_cast<CItemMultiCustom*>(pItem);
	if ( pItemMultiCustom )
		return pItemMultiCustom->GetMultiItemDefs();	// customized multi

	return GetMultiItemDefs(pItem->GetDispID());		// multi.mul multi
}

const CUOMulti * CServerConfig::GetMultiItemDefs( ITEMID_TYPE itemid )
{
	ADDTOCALLSTACK("CServerConfig::GetMultiItemDefs(ITEMID_TYPE)");
	if ( !CItemBase::IsID_Multi(itemid) )
		return nullptr;

	MULTI_TYPE id = (MULTI_TYPE)(itemid - ITEMID_MULTI);
	size_t index = m_MultiDefs.FindKey(id);
	if ( index == sl::scont_bad_index() )
		index = m_MultiDefs.AddSortKey(new CUOMulti(id), id);
	else
		m_MultiDefs[index]->HitCacheTime();

	const CUOMulti *pMulti = m_MultiDefs[index];
	return pMulti;
}

PLEVEL_TYPE CServerConfig::GetPrivCommandLevel( lpctstr pszCmd ) const
{
	ADDTOCALLSTACK("CServerConfig::GetPrivCommandLevel");
	// What is this commands plevel ?
	// NOTE: This does not attempt to parse anything.

	// Is this command avail for your priv level (or lower) ?
	uint ilevel = PLEVEL_QTY;
	while ( ilevel > 0 )
	{
		--ilevel;
		lpctstr const * pszTable = m_PrivCommands[ilevel].data();
		int iCount = (int)m_PrivCommands[ilevel].size();
		if ( FindTableHeadSorted( pszCmd, pszTable, iCount ) >= 0 )
			return (PLEVEL_TYPE)ilevel;
	}

	// A GM will default to use all commands.
	// xcept those that are specifically named that i can't use.
	return (PLEVEL_TYPE)m_iDefaultCommandLevel; // default level.
}

bool CServerConfig::CanUsePrivVerb( const CScriptObj * pObjTarg, lpctstr pszCmd, CTextConsole * pSrc ) const
{
	ADDTOCALLSTACK("CServerConfig::CanUsePrivVerb");
	// can i use this verb on this object ?
	// Check just at entry points where commands are entered or targetted.
	// NOTE:
	//  Call this each time we change pObjTarg such as r_GetRef()
	// RETURN:
	//  true = i am ok to use this command.

	if ( !pSrc || !pObjTarg )
		return false;

	// Are they more privleged than me ?

	const CChar * pChar = dynamic_cast <const CChar*> (pObjTarg);
	if ( pChar )
	{
		if ( pSrc->GetChar() == pChar )
			return true;
		if ( pSrc->GetPrivLevel() < pChar->GetPrivLevel())
		{
			pSrc->SysMessage( "Target is more privileged than you\n" );
			return false;
		}
	}

	// can i use this verb on this object ?

	if ( pSrc->GetChar() == nullptr )
	{
		// I'm not a cchar. what am i ?
		CClient * pClient = dynamic_cast <CClient *>(pSrc);
		if ( pClient )
		{
			// We are not logged in as a player char ? so we cannot do much !
			if ( pClient->GetAccount() == nullptr )
			{
				// must be a console or web page ?
				// I guess we can just login !
				if ( ! strnicmp( pszCmd, "LOGIN", 5 ))
					return true;
				return false;
			}
		}
		else
		{
			// we might be the g_Serv or web page ?
		}
	}

	// Is this command avail for your priv level (or lower) ?
	PLEVEL_TYPE ilevel;
	tchar *myCmd = Str_GetTemp();

	size_t pOs = strcspn(pszCmd," "); //position of space :)
	Str_CopyLimit( myCmd, pszCmd, pOs );
	myCmd[pOs] = '\0';

	tchar * pOd; //position of dot :)
	while ( (pOd = strchr(myCmd, '.')) != nullptr )
	{
		ilevel = GetPrivCommandLevel( myCmd );
		if ( ilevel > pSrc->GetPrivLevel())
			return false;
        myCmd=pOd+1; //skip dot
	}


	ilevel = GetPrivCommandLevel( myCmd );
	if ( ilevel > pSrc->GetPrivLevel())
		return false;

	return true;
}


//*************************************************************

CPointMap CServerConfig::GetRegionPoint( lpctstr pCmd ) const // Decode a teleport location number into X/Y/Z
{
	ADDTOCALLSTACK("CServerConfig::GetRegionPoint");
	// get a point from a name. (probably the name of a region)
	// Might just be a point coord number ?

	GETNONWHITESPACE( pCmd );
	if ( pCmd[0] == '-' && !strchr( pCmd, ',' ) )	// Get location from start list.
	{
        if (m_StartDefs.empty())
            return CPointMap();

        SKIP_NONNUM(pCmd);
        std::optional<uint> iconv = Str_ToU(pCmd);
        if (!iconv.has_value())
            return CPointMap();

		size_t i = iconv.value();
        if (i > 0)
        {
            i -= 1;
            if ( ! m_StartDefs.IsValidIndex(i) )
            {
                i = 0;
            }
        }
		return m_StartDefs[i]->m_pt;
	}

	CPointMap pt;	// invalid point
	if ( IsDigit( pCmd[0] ) || pCmd[0] == '-' )
	{
		tchar *pszTemp = Str_GetTemp();
		Str_CopyLimitNull( pszTemp, pCmd, Str_TempLength() );
		const size_t uiCount = pt.Read( pszTemp );
		if ( uiCount >= 2 )
			return pt;
	}
	else
	{
		// Match the region name with global regions.
		const CRegion * pRegion = GetRegion(pCmd);
		if ( pRegion != nullptr )
		{
			return pRegion->m_pt;
		}
	}
	// no match.
	return pt;
}

CRegion * CServerConfig::GetRegion( lpctstr pKey ) const
{
	ADDTOCALLSTACK("CServerConfig::GetRegion");
	// get a region from a name or areadef.
    // pKey format: "name" or "name, map"

	GETNONWHITESPACE( pKey );
    lpctstr ptcColonPos = strchr(pKey, ',');
    if (ptcColonPos == nullptr)
    {
        // No map specified: return the one with lower map index
        std::vector<CRegion*> regions;
        for (CRegion* pRegion : m_RegionDefs)
        {
            if ( ! pRegion->GetNameStr().CompareNoCase(pKey) || ! strcmpi(pRegion->GetResourceName(), pKey) )
            {
                regions.emplace_back(pRegion);
            }
        }

        CRegion* pRet = nullptr;
        uchar minMap = 255;
        for (CRegion* pRegion : regions)
        {
			const uchar m = pRegion->GetRegionCorner(DIR_N).m_map;
            if (m <= minMap)
            {
                minMap = m;
                pRet = pRegion;
            }
        }
        return pRet;
    }
    else
    {
        CSString sName(pKey);
        sName[int(ptcColonPos - pKey)] = '\0';
        std::vector<CRegion*> regions;
        for (CRegion* pRegion : m_RegionDefs)
        {
            if ( ! sName.CompareNoCase(pRegion->GetNameStr()) || ! sName.CompareNoCase(pRegion->GetResourceName()) )
            {
                regions.emplace_back(pRegion);
            }
        }

        ptcColonPos += 1; // skip the , (which now is \0)
		const uchar uiMapIdx = Exp_GetUCVal(ptcColonPos);
        for (CRegion* pRegion : regions)
        {
			const uchar m = pRegion->GetRegionCorner(DIR_N).m_map;
            if (uiMapIdx == m)
            {
                return pRegion;
            }
        }
    }

	// no match.
	return nullptr;
}

void CServerConfig::LoadSortSpells()
{
	size_t iQtySpells = m_SpellDefs.size();
	if ( iQtySpells <= 0 )
		return;

	m_SpellDefs_Sorted.clear();
	m_SpellDefs_Sorted.push_back(m_SpellDefs[0]);		// the null spell

	for ( size_t i = 1; i < iQtySpells; ++i )
	{
		if ( !m_SpellDefs.valid_index( i ) )
			continue;

		int	iVal = 0;
		if (!m_SpellDefs[i]->GetPrimarySkill(nullptr, &iVal))
			continue;

		const size_t iQty = m_SpellDefs_Sorted.size();
		size_t k = 1;
		while ( k < iQty )
		{
			int	iVal2 = 0;
			if (m_SpellDefs_Sorted[k]->GetPrimarySkill(nullptr, &iVal2))
			{
				if (iVal2 > iVal)
					break;
			}
			++k;
		}
		m_SpellDefs_Sorted.emplace_index_grow(k, m_SpellDefs[i]);
	}
}


//*************************************************************

uint CServerConfig::GetPacketFlag( bool bCharlist, RESDISPLAY_VERSION res, uchar chars )
{
	// This is needed by the packet 0xB9, which is sent to the client very early, before we can know if this is a 2D, KR, EC, 3D client.
	// Using the CNetState here to know which kind of client is it is pointless, because at this time the client type is always the default value (2D).

	uint retValue = 0;

	// retValue size:
	//	byte[2] feature# (<= 6.0.14.1)
	//	byte[4] feature# (>= 6.0.14.2)

	if ( bCharlist )
	{
		/*
		CHAR LIST FLAGS
		0x0001	= unknown
		0x0002	= overwrite configuration button [send config/req logout (IGR?)]
		0x0004	= limit 1 character per account
		0x0008	= enable npc popup menus (context menus)
		0x0010	= can limit characters number
		[under -> since AOS]
		0x0020	= paladin and necromancer classes, tooltips
		0x0040	= sixth character Slot
		[under -> since SE]
		0x0080	= samurai and ninja classes
		[under -> since ML]
		0x0100	= elven race
		[under -> since KR]
		0x0200	= flag KR Unknown 1
		0x0400	= client will send 0xE1 packet at character list (possibly other unknown effects)
		[under -> since SA]
		0x1000	= seventh character Slot
		[under -> since HS?]
		0x4000	= new walk packets
		0x8000  = new faction strongholds (uses map0x.mul, statics0x.mul, etc) - 7.0.6
		*/

		// T2A - LBR don't have char list flags

		if ( res >= RDS_AOS )
		{
			retValue |= ( m_iFeatureAOS & FEATURE_AOS_POPUP ) ? 0x008 : 0x00;
			retValue |= ( m_iFeatureAOS & FEATURE_AOS_UPDATE_B ) ? 0x020 : 0x00;
		}

		if ( res >= RDS_SE )
		{
			retValue |= ( m_iFeatureSE & FEATURE_SE_NINJASAM ) ? 0x080 : 0x00;
		}

		if ( res >= RDS_ML )
		{
			retValue |= ( m_iFeatureML & FEATURE_ML_UPDATE ) ? 0x0100 : 0x00;
		}

		if ( res >= RDS_KR )
		{
			retValue |= ( m_iFeatureKR & FEATURE_KR_UPDATE ) ? 0x200 : 0x00;
			retValue |= ( m_iFeatureKR & FEATURE_KR_CLIENTTYPE ) ? 0x400 : 0x00;
		}

		if ( res >= RDS_SA )
		{
			retValue |= ( m_iFeatureSA & FEATURE_SA_MOVEMENT ) ? 0x4000 : 0x00;
		}

		retValue |= ( chars == 1 ) ? 0x0014 : 0x00;
		retValue |= ( chars >= 6 ) ? 0x0040 : 0x00;
		retValue |= ( chars >= 7 ) ? 0x1000 : 0x00;
	}
	else
	{
		/*
		FEATURE FLAGS

		0x01:		enable T2A features:			chat, regions, ?new spellbook?mage??
		0x02:		enable renaissance features:	damage packet, ?
		0x04:		enable third dawn features
		0x08:		enable LBR features:			skills, map, monsters, bufficon, plays MP3 instead of midis
		0x10:		enable AOS features 1:			skills, map, monsters?, spells, fightbook?, housing tiles
		0x20:		enable AOS features 2,
		0x40:		enable SE features:				skills, map, monsters?, spells, housing tiles
		0x80:		enable ML features:				elven race, skills, monsters?, spells, housing tiles
		0x100:		enable 8th age splash screen
		0x200:		enable 9th age splash screen and crystal/shadow housing tiles
		0x400:		enable 10th age
		0x800:		enable increased housing and bank storage
		0x1000:		7th character slot
		0x2000:		enable KR (roleplay) faces
		0x4000:		trial account
		0x8000:		non-trial (live) account (required on clients 4.0.0+, otherwise bits 3..14 will be ignored)
		0x10000:	enable SA features:				gargoyle race, spells, skills, housing tiles
		0x20000:	enable HS features:
		0x40000:	enable Gothic housing tiles
		0x80000:	enable Rustic housing tiles
        0x100000:	enable Jungle custom house tiles
        0x200000:	enable Shadowguard custom house tiles
        0x400000:	enable TOL features
        0x800000:	free account (Endless Journey)
		*/
		if ( res >= RDS_T2A )
		{
			retValue |= ( m_iFeatureT2A & FEATURE_T2A_UPDATE ) ? CLI_FEAT_T2A_FULL : 0x00;
			retValue |= ( m_iFeatureT2A & FEATURE_T2A_CHAT ) ? CLI_FEAT_T2A_CHAT : 0x00;
		}

		if ( res >= RDS_LBR )
		{
			retValue |= ( m_iFeatureLBR & FEATURE_LBR_UPDATE ) ? CLI_FEAT_LBR_FULL : 0x00;
			retValue |= ( m_iFeatureLBR & FEATURE_LBR_SOUND  ) ? CLI_FEAT_LBR_SOUND : 0x00;
		}

		if ( res >= RDS_AOS )
			retValue |= ( m_iFeatureAOS & FEATURE_AOS_UPDATE_A ) ? 0x08000|0x10 : 0x00;

		if ( res >= RDS_SE )
			retValue |= ( m_iFeatureSE & FEATURE_SE_NINJASAM ) ? 0x040 : 0x00;

		if ( res >= RDS_ML )
			retValue |= ( m_iFeatureML & FEATURE_ML_UPDATE ) ? 0x080 : 0x00;

		if ( res >= RDS_SA )
			retValue |= ( m_iFeatureSA & FEATURE_SA_UPDATE ) ? 0x10000 : 0x00;

		if ( res >= RDS_TOL )
			retValue |= ( m_iFeatureTOL & FEATURE_TOL_UPDATE ) ? 0x400000 : 0x00;

		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_CRYSTAL ) ? 0x0200 : 0x00;
		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_GOTHIC ) ? 0x40000 : 0x00;
		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_RUSTIC ) ? 0x80000 : 0x00;
		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_JUNGLE ) ? 0x100000 : 0x00;
		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_SHADOWGUARD ) ? 0x200000 : 0x00;

		retValue |= ( m_iFeatureExtra & FEATURE_EXTRA_ROLEPLAYFACES ) ? 0x2000 : 0x00;

		retValue |= ( chars >= 6 ) ? 0x0020 : 0x00;
		retValue |= ( chars >= 7 ) ? 0x1000 : 0x00;
	}

	return retValue;
}


//*************************************************************

bool CServerConfig::LoadResourceSection( CScript * pScript )
{
	ADDTOCALLSTACK("CServerConfig::LoadResourceSection");
	// Index or read any resource sections we know how to handle.

	ASSERT(pScript);
	CScriptFileContext FileContext( pScript );	// set this as the context.
    const CSString sSection(pScript->GetSection());
    lpctstr pszSection = sSection.GetBuffer();

	CVarDefContNum * pVarNum = nullptr;
	CResourceID rid;

    bool fNewStyleDef = false;
	RES_TYPE restype;
	if ( !strnicmp( pszSection, "DEFMESSAGE", 10 ) )
	{
		restype			= RES_DEFNAME;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "AREADEF", 7 ) )
	{
		restype			= RES_AREA;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "ROOMDEF", 7 ) )
	{
		restype			= RES_ROOM;
		fNewStyleDef	= true;
	}
	else if ( !strnicmp( pszSection, "GLOBALS", 7 ) )
	{
		restype			= RES_WORLDVARS;
	}
	else if ( !strnicmp( pszSection, "LIST", 4) )
	{
		restype			= RES_WORLDLISTS;
	}
	else if ( !strnicmp( pszSection, "MULTIDEF", 8 ) )
	{
		restype			= RES_ITEMDEF;
		fNewStyleDef	= true;
	}
    else
    {
        restype = (RES_TYPE)FindTableSorted(pszSection, sm_szResourceBlocks, ARRAY_COUNT(sm_szResourceBlocks));
    }


	if (( restype == RES_WORLDSCRIPT ) || ( restype == RES_WS ))
	{
		const lpctstr pszDef = pScript->GetArgStr();
		CVarDefCont * pVarBase = g_Exp.m_VarResDefs.GetKey( pszDef );
		pVarNum = nullptr;
		if ( pVarBase )
			pVarNum = dynamic_cast <CVarDefContNum*>( pVarBase );
		if ( !pVarNum )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		rid = CResourceID( (dword)pVarNum->GetValNum(), 0 );

        // This value won't be read, since we return anyways once in this branch.
        //restype = rid.GetResType();

		CResourceDef *	pRes = nullptr;
		size_t index = m_ResHash.FindKey( rid );
		if ( index != sl::scont_bad_index() )
			pRes = dynamic_cast <CResourceDef*> (m_ResHash.GetBarePtrAt( rid, index ) );

		if ( pRes == nullptr )
		{
			g_Log.Event( LOGL_WARN|LOGM_INIT, "Resource '%s' not found\n", pszDef );
			return false;
		}

		pRes->r_Load( *pScript );
		return true;
	}

	if ( restype < 0 )
	{
		g_Log.Event( LOGL_WARN|LOGM_INIT, "Unknown section '%s' in '%s'\n", pScript->GetKey(), pScript->GetFileTitle());
		return false;
	}
	else
	{
		// Create a new index for the block.
		// NOTE: rid is not created for all types.
		// NOTE: GetArgStr() is not always the DEFNAME
		rid = ResourceGetNewID( restype, pScript->GetArgStr(), &pVarNum, fNewStyleDef );
	}

	if ( !rid.IsValidUID() )
	{
		DEBUG_ERR(( "Invalid %s section, index '%s'\n", pszSection, pScript->GetArgStr()));
		return false;
	}

	EXC_TRY("LoadResourceSection");

	// NOTE: It is possible to be replacing an existing entry !!! Check for this.

	CResourceLink * pNewLink = nullptr;
	CResourceDef * pNewDef = nullptr;
	CResourceDef * pPrvDef;

	if ( m_ResourceList.ContainsKey( const_cast<tchar *>(pszSection) ))
	{
        // Add to DEFLIST
		CListDefCont* pListBase = g_Exp.m_ListInternals.GetKey(pszSection);
		if ( !pListBase )
			pListBase = g_Exp.m_ListInternals.AddList(pszSection);

		if ( pListBase )
			pListBase->r_LoadVal(pScript->GetArgStr());
	}

	switch ( restype )
	{
	case RES_SPHERE:
		// Define main global config info.
		g_Serv.r_Load(*pScript);
		return true;

	case RES_SPHERECRYPT:
		CCryptoKeysHolder::get()->LoadKeyTable(*pScript);
		return true;

	case RES_ACCOUNT:	// NOTE: ArgStr is not the DEFNAME
		// Load the account now. Not normal method !
		return g_Accounts.Account_Load( pScript->GetArgStr(), *pScript, false );

	case RES_ADVANCE:
		// Stat advance rates.
		while ( pScript->ReadKeyParse())
		{
			lpctstr ptcKey = pScript->GetKey();
			STAT_TYPE i = GetStatKey(ptcKey);
			if ((i <= STAT_NONE) || (i >= STAT_BASE_QTY))
			{
				g_Log.EventError("Invalid keyword '%s'.\n", ptcKey);
				continue;
			}
			m_StatAdv[i].Load( pScript->GetArgStr());
		}
		return true;

	case RES_BLOCKIP:
		{
			tchar* ipBuffer = Str_GetTemp();
			while ( pScript->ReadKeyParse())
			{
				Str_CopyLimitNull(ipBuffer, pScript->GetKey(), Str_TempLength());
				HistoryIP& history = g_NetworkManager.getIPHistoryManager().getHistoryForIP(ipBuffer);
				history.setBlocked(true);
			}
		}
		return true;

	case RES_COMMENT:
		// Just toss this.
		return true;

	case RES_DEFNAME:
		// just get a block of defs.
		while ( pScript->ReadKeyParse() )
		{
			const lpctstr ptcKey = pScript->GetKey();
			if ( fNewStyleDef )
			{
				//	search for this.
				size_t l;
				for ( l = 0; l < DEFMSG_QTY; ++l )
				{
					if ( !strcmpi(ptcKey, g_Exp.sm_szMsgNames[l]) )
					{
						Str_CopyLimitNull(g_Exp.sm_szMessages[l], pScript->GetArgStr(), sizeof(g_Exp.sm_szMessages[l]));
						break;
					}
				}
				if ( l == DEFMSG_QTY )
					g_Log.Event(LOGM_INIT|LOGL_ERROR, "Setting not used message override named '%s'\n", ptcKey);
				continue;
			}
			else
			{
				g_Exp.m_VarDefs.SetStr(ptcKey, false, pScript->GetArgStr(), false);
			}
		}
		return true;

	case RES_RESDEFNAME:
		// just get a block of resource aliases (like a classic DEF).
		while (pScript->ReadKeyParse())
		{
			const lpctstr ptcKey = pScript->GetKey();
			g_Exp.m_VarResDefs.SetStr(ptcKey, false, pScript->GetArgStr(), false);
		}
		return true;

	case RES_RESOURCELIST:
		{
			while ( pScript->ReadKey() )
			{
				const lpctstr pName = pScript->GetKeyBuffer();
				m_ResourceList.AddSortString( pName );
			}
		}
		return true;
	case RES_FAME:
		{
			size_t i = 0;
			while ( pScript->ReadKey())
			{
				lpctstr pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";

				m_Fame.assign_at_grow(i, new CSString(pName));
				++i;
			}
		}
		return true;
	case RES_KARMA:
		{
			size_t i = 0;
			while ( pScript->ReadKey())
			{
				lpctstr pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";

				m_Karma.assign_at_grow(i, new CSString(pName));
				++i;
			}
		}
		return true;
	case RES_NOTOTITLES:
		{
			if (pScript->ReadKey() == false)
			{
				g_Log.Event(LOGM_INIT|LOGL_ERROR, "NOTOTITLES section is missing the list of karma levels.\n");
				return true;
			}

			int64 piNotoLevels[64];
			size_t i = 0, iQty = 0;

			// read karma levels
			iQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, ARRAY_COUNT(piNotoLevels));
			for (i = 0; i < iQty; ++i)
				m_NotoKarmaLevels.assign_at_grow(i, (int) (piNotoLevels[i]));

			m_NotoKarmaLevels.resize(i);

			if (pScript->ReadKey() == false)
			{
				g_Log.Event(LOGM_INIT|LOGL_ERROR, "NOTOTITLES section is missing the list of fame levels.\n");
				return true;
			}

			// read fame levels
			iQty = Str_ParseCmds(pScript->GetKeyBuffer(), piNotoLevels, ARRAY_COUNT(piNotoLevels));
			for (i = 0; i < iQty; ++i)
				m_NotoFameLevels.assign_at_grow(i, (int) (piNotoLevels[i]));

			m_NotoFameLevels.resize(i);

			// read noto titles
			i = 0;
			while ( pScript->ReadKey())
			{
				lpctstr pName = pScript->GetKeyBuffer();
				if ( *pName == '<' )
					pName = "";

				m_NotoTitles.assign_at_grow(i, new CSString(pName));
				++i;
			}

            if (m_NotoTitles.size() != ((m_NotoKarmaLevels.size() + 1) * (m_NotoFameLevels.size() + 1)))
            {
                g_Log.Event(LOGM_INIT | LOGL_WARN, "Expected %" PRIuSIZE_T " titles in NOTOTITLES section but found %" PRIuSIZE_T ".\n",
                    (m_NotoKarmaLevels.size() + 1) * (m_NotoFameLevels.size() + 1),
                    m_NotoTitles.size());
            }
		}
		return true;
	case RES_OBSCENE:
		while ( pScript->ReadKey())
		{
			m_Obscene.AddSortString( pScript->GetKey() );
		}
		return true;
	case RES_PLEVEL:
		{
			int index = rid.GetResIndex();
			if ( index < 0 || (uint)(index) >= ARRAY_COUNT(m_PrivCommands) )
				return false;
			while ( pScript->ReadKey() )
			{
                // It's mandatory to convert to UPPERCASE the function name, because we perform a case-sensitive search
				tchar* key = const_cast<tchar*>(pScript->GetKey());
                _strupr(key);
				m_PrivCommands[index].AddSortString(key);
			}
		}
		return true;
	case RES_RESOURCES:
		// Add these all to the list of files we need to include.
        g_Log.Event(LOGM_NOCONTEXT|LOGM_INIT, "Caching script files...\n");
		while ( pScript->ReadKey() )
		{
			AddResourceFile( pScript->GetKey() );
		}
		return true;
	case RES_RUNES:
		// The full names of the magic runes.
		m_Runes.ClearFree();
		while ( pScript->ReadKey() )
		{
			m_Runes.push_back(new CSString(pScript->GetKey()));
		}
		return true;
	case RES_SECTOR: // saved in world file.
		{
			const CPointMap pt = GetRegionPoint( pScript->GetArgStr() ); // Decode a teleport location number into X/Y/Z
            CSector* pSector = pt.GetSector();
            if (!pSector)
                return false;
			return pSector->r_Load(*pScript);
		}
	case RES_SPELL:
		{
			CSpellDef * pSpell;
			pPrvDef = RegisteredResourceGetDef( rid );
			if ( pPrvDef )
				pSpell = dynamic_cast<CSpellDef*>(pPrvDef);
			else
				pSpell = new CSpellDef((SPELL_TYPE)(rid.GetResIndex()));
			ASSERT(pSpell);
			pNewLink = pSpell;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
				m_SpellDefs.emplace_index_grow(rid.GetResIndex(), std::unique_ptr<CSpellDef>(pSpell));
		}
		break;

	case RES_SKILL:
		{
			CSkillDef * pSkill;
			pPrvDef = RegisteredResourceGetDef( rid );
			if ( pPrvDef )
			{
				pSkill = dynamic_cast <CSkillDef*>(pPrvDef);
			}
			else
			{
				if ( rid.GetResIndex() >= (uint)(m_iMaxSkill) )
					m_iMaxSkill = rid.GetResIndex() + 1;

				// Just replace any previous CSkillDef
				pSkill = new CSkillDef((SKILL_TYPE)(rid.GetResIndex()));
			}

			ASSERT(pSkill);
			pNewLink = pSkill;

			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext );

			if ( !pPrvDef )
			{
				// build a name sorted list.
				const auto array_pos_iter = m_SkillNameDefs.emplace(pSkill);
				const size_t emplaced_idx = array_pos_iter - m_SkillNameDefs.begin();

				// Hard coded value for skill index.
				uint idx = rid.GetResIndex();

				m_SkillIndexDefs.emplace_index_grow(idx, m_SkillNameDefs[emplaced_idx]);
			}
		}
		break;

	case RES_TYPEDEF:
	{
		// Just index this for access later.
		pPrvDef = ResourceGetDef( rid );
		if ( pPrvDef )
		{
			CItemTypeDef * pTypeDef	= dynamic_cast <CItemTypeDef*>(pPrvDef);
			ASSERT( pTypeDef );
			pNewLink = pTypeDef;
			ASSERT(pNewLink);

			// clear old tile links to this type
			const size_t iQty = g_World.m_TileTypes.size();
			for ( size_t i = 0; i < iQty; ++i )
			{
				auto pCurTypeDef = static_cast<CItemTypeDef*>(g_World.m_TileTypes[i].get());
				if (pCurTypeDef == pTypeDef)
					g_World.m_TileTypes.emplace_index_grow(i, nullptr);
			}

		}
		else
		{
			pNewLink = new CItemTypeDef( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}

		ASSERT(pScript);
		CScriptLineContext LineContext = pScript->GetContext();
		pNewLink->r_Load(*pScript);
		pScript->SeekContext( LineContext );
		break;
	}

	//*******************************************************************
	// Might need to check if the link already exists ?

	case RES_BOOK:
	case RES_EVENTS:
	case RES_MENU:
	case RES_NAMES:
	case RES_NEWBIE:
	case RES_TIP:
	case RES_SPEECH:
	case RES_SCROLL:
	case RES_SKILLMENU:
		// Just index this for access later.
		pPrvDef = RegisteredResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CResourceLink*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CResourceLink( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;
	case RES_DIALOG:
		// Just index this for access later.
		pPrvDef = RegisteredResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CDialogDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CDialogDef( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	case RES_REGIONRESOURCE:
		// No need to Link to this really .
		pPrvDef = RegisteredResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRegionResourceDef*>( pPrvDef );
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRegionResourceDef( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}

		break;
	case RES_AREA:
		pPrvDef = RegisteredResourceGetDef(rid);
		if ( pPrvDef && fNewStyleDef )
		{
			CRegionWorld *	pRegion = dynamic_cast <CRegionWorld*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegionWorld * pRegion = new CRegionWorld( rid, pScript->GetArgStr());
			pRegion->r_Load( *pScript );
			if (!pRegion->RealizeRegion())
			{
				delete pRegion; // might be a dupe ?
			}
			else
			{
				pNewDef = pRegion;
				ASSERT(pNewDef);
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionDefname();
				m_RegionDefs.push_back(pRegion);
			}
		}
		break;
	case RES_ROOM:
		pPrvDef = RegisteredResourceGetDef(rid);
		if ( pPrvDef && fNewStyleDef )
		{
			CRegion * pRegion = dynamic_cast <CRegion*>( pPrvDef );
			pNewDef	= pRegion;
			ASSERT(pNewDef);
			pRegion->UnRealizeRegion();
			pRegion->r_Load(*pScript);
			pRegion->RealizeRegion();
		}
		else
		{
			CRegion * pRegion = new CRegion( rid, pScript->GetArgStr());
			pNewDef = pRegion;
			ASSERT(pNewDef);
			pRegion->r_Load(*pScript);
			if (!pRegion->RealizeRegion())
			{
				delete pRegion; // might be a dupe ?
			}
			else
			{
				m_ResHash.AddSortKey( rid, pRegion );
				// if it's old style but has a defname, it's already set via r_Load,
				// so this will do nothing, which is good
				// if ( !fNewStyleDef )
				//	pRegion->MakeRegionDefname();
				m_RegionDefs.push_back(pRegion);
			}
		}
		break;
	case RES_REGIONTYPE:
	case RES_SPAWN:
	case RES_TEMPLATE:
		pPrvDef = RegisteredResourceGetDef(rid);
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CRandGroupDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CRandGroupDef( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

    case RES_CHAMPION:
    {
		pPrvDef = RegisteredResourceGetDef(rid);
        if (pPrvDef)
        {
            pNewLink = dynamic_cast <CCChampionDef*>(pPrvDef);
            ASSERT(pNewLink);
        }
        else
        {
            pNewLink = new CCChampionDef(rid);
            if (pNewLink)
            {
                CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
                if (pLinkResScript != nullptr)
                    pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
                m_ResHash.AddSortKey(rid, pNewLink);
            }
        }
        {
            CScriptLineContext LineContext = pScript->GetContext();
            pNewLink->r_Load(*pScript);
            pScript->SeekContext(LineContext);
        }
        break;
    }
	case RES_SKILLCLASS:
		pPrvDef = RegisteredResourceGetDef(rid);
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CSkillClassDef*>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CSkillClassDef( rid );
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load( *pScript );
			pScript->SeekContext( LineContext );
		}
		break;

	case RES_CHARDEF:
	case RES_ITEMDEF:
		// ??? existing hard pointers to RES_CHARDEF ?
		// ??? existing hard pointers to RES_ITEMDEF ?
		pPrvDef = RegisteredResourceGetDef( rid );
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast<CResourceLink*>(pPrvDef);
			if ( !pNewLink )
				return true;

			CBaseBaseDef * pBaseDef = dynamic_cast <CBaseBaseDef*> (pNewLink);
			if ( pBaseDef )
			{
				pBaseDef->UnLink();
				CScriptLineContext LineContext = pScript->GetContext();
				pBaseDef->r_Load(*pScript);
				pScript->SeekContext( LineContext );
			}
		}
		else
		{
			pNewLink = new CResourceLink(rid);
			ASSERT(pNewLink);
			CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
			if (pLinkResScript != nullptr)
				pNewLink->SetLink(pLinkResScript);	// So later i can retrieve m_iResourceFileIndex and m_iLineNum from the CResourceScript
			m_ResHash.AddSortKey( rid, pNewLink );
		}
		break;

	// Map stuff that could be duplicated !!!
	// NOTE: ArgStr is NOT the DEFNAME in this case
	// ??? duplicate areas ?
	// ??? existing hard pointers to areas ?
	case RES_WEBPAGE:
		// Read a web page entry.
		pPrvDef = RegisteredResourceGetDef(rid);
		if ( pPrvDef )
		{
			pNewLink = dynamic_cast <CWebPageDef *>(pPrvDef);
			ASSERT(pNewLink);
		}
		else
		{
			pNewLink = new CWebPageDef( rid );
			ASSERT(pNewLink);
			m_WebPages.emplace(pNewLink);
		}
		{
			CScriptLineContext LineContext = pScript->GetContext();
			pNewLink->r_Load(*pScript);
			pScript->SeekContext( LineContext ); // set the pos back so ScanSection will work.
		}
		break;

	case RES_FUNCTION:
	{
        lpctstr ptcFunctionName = pScript->GetArgStr();
        const size_t uiFunctionIndex = m_Functions.find_sorted(ptcFunctionName);
        if (uiFunctionIndex == sl::scont_bad_index())
        {
            // Define a char macro. (Name is NOT DEFNAME)
            pNewLink = new CResourceNamedDef(rid, ptcFunctionName);

            // Link the CResourceLink to the CResourceScript it was read and created,
            //	so later we can retrieve the file and the line for debugging purposes.
            CResourceScript* pLinkResScript = dynamic_cast<CResourceScript*>(pScript);
            if (pLinkResScript != nullptr)
                pNewLink->SetLink(pLinkResScript);

            m_Functions.emplace(static_cast<CResourceNamedDef*>(pNewLink));
        }
        else
        {
            pNewLink = m_Functions[uiFunctionIndex].get();
        }
	}
		break;

	case RES_SERVERS:	// Old way to define a block of servers.
		{
			bool fReadSelf = false;

			while ( pScript->ReadKey())
			{
				// Does the name already exist ?
				bool fAddNew = false;
				CServerRef pServ;
				const size_t i = m_Servers.FindKey( pScript->GetKey());
				if ( i == sl::scont_bad_index() )
				{
					pServ = new CServerDef( pScript->GetKey(), CSocketAddressIP( SOCKET_LOCAL_ADDRESS ));
					fAddNew = true;
				}
				else
					pServ = Server_GetDef(i);
				ASSERT(pServ != nullptr);

				if ( pScript->ReadKey())
				{
					pServ->m_ip.SetHostPortStr( pScript->GetKey());
					if ( pScript->ReadKey())
						pServ->m_ip.SetPort( (word)(pScript->GetArgVal()));
				}

				if ( ! strcmpi( pServ->GetName(), g_Serv.GetName()))
					fReadSelf = true;
				if ( g_Serv.m_ip == pServ->m_ip )
					fReadSelf = true;
				if ( fReadSelf )
				{
					// I can be listed first here. (old way)
					g_Serv.SetName( pServ->GetName());
					g_Serv.m_ip = pServ->m_ip;
					delete pServ;
					fReadSelf = false;
					continue;
				}

				if ( fAddNew )
					m_Servers.AddSortKey( pServ, pServ->GetName());
			}
		}
		return true;

	case RES_TYPEDEFS:
        {
            // just get a block of defs.

            //pPrvDef = RegisteredResourceGetDef(rid); // useless
            while ( pScript->ReadKeyParse())
            {
                const lpctstr ptcName = pScript->GetKey();
                const int iIndex = pScript->GetArgVal();
                CResourceID ridnew( RES_TYPEDEF, iIndex );

                CResourceDef *pResDef = RegisteredResourceGetDef(ridnew);
                // Do we already have a TYPEDEF with the same index?
                if (pResDef)
                {
                    pResDef->SetResourceName( ptcName );   // update name
                }
                else
                {
                    if (!ptcName || (*ptcName == '\0'))
                    {
                        g_Log.EventError("Empty typedef name for id %d?\n", iIndex);
                        continue;
                    }
                    pResDef = new CItemTypeDef( ridnew );
                    ASSERT(pResDef);
                    pResDef->SetResourceName( ptcName );
                    m_ResHash.AddSortKey( ridnew, pResDef );
                }
            }

            return true;
        }

	case RES_STARTS:
		{
			const int iStartVersion = pScript->GetArgVal();
			m_StartDefs.ClearFree();
			while ( pScript->ReadKey())
			{
				CStartLoc * pStart = new CStartLoc( pScript->GetKey());
				if ( pScript->ReadKey())
				{
					pStart->m_sName = pScript->GetKey();
					if ( pScript->ReadKey())
						pStart->m_pt.Read( pScript->GetKeyBuffer());

					if (iStartVersion == 2)
					{
                        if ( pScript->ReadKey())
                        {
                            std::optional<int> iconv = Str_ToI(pScript->GetKey());
                            if (!iconv.has_value())
                                continue;

                            pStart->iClilocDescription = *iconv;
                        }
					}
				}
				m_StartDefs.emplace_back(pStart);
			}

			return true;
		}
	case RES_MOONGATES:
		m_MoonGates.clear();
		while ( pScript->ReadKey())
		{
			CPointMap pt = GetRegionPoint( pScript->GetKey());
			m_MoonGates.push_back(pt);
		}
		return true;
	case RES_WORLDVARS:
		while ( pScript->ReadKeyParse() )
		{
			bool fQuoted = false;
			lpctstr ptcKey = pScript->GetKey();
			if ( strstr(ptcKey, "VAR.") )  // This is for backward compatibility from Rcs
				ptcKey = ptcKey + 4;

            lpctstr ptcArg = pScript->GetArgStr( &fQuoted );
			g_Exp.m_VarGlobals.SetStr( ptcKey, fQuoted, ptcArg );
		}
		return true;
	case RES_WORLDLISTS:
		{
			CListDefCont* pListBase = g_Exp.m_ListGlobals.AddList(pScript->GetArgStr());

			if ( !pListBase )
			{
				DEBUG_ERR(("Unable to create list '%s'...\n", pScript->GetArgStr()));

				return false;
			}

			while ( pScript->ReadKeyParse() )
			{
				if ( !pListBase->r_LoadVal(*pScript) )
					DEBUG_ERR(("Unable to add element '%s' to list '%s'...\n", pScript->GetArgStr(), pListBase->GetKey()));
			}
		}
		return true;
	case RES_TIMERF:
		while ( pScript->ReadKeyParse() )
		{
			bool fQuoted = false;
            lpctstr ptcArg = pScript->GetArgStr( &fQuoted );
			CWorldTimedFunctions::Load( pScript->GetKey(), fQuoted, ptcArg );
		}
		return true;
	case RES_TELEPORTERS:
		while ( pScript->ReadKey())
		{
			// Add the teleporter to the CSector.
			CTeleport * pTeleport = new CTeleport( pScript->GetKeyBuffer());
			ASSERT(pTeleport);
			// make sure this is not a dupe.
			if (!pTeleport->RealizeTeleport()) {
				delete pTeleport;
			}
		}
		return true;
	case RES_KRDIALOGLIST:
		while ( pScript->ReadKeyParse() )
		{
			CDialogDef *pDef = dynamic_cast<CDialogDef *>( ResourceGetDefByName(RES_DIALOG, pScript->GetKey()) );
			if ( pDef != nullptr )
				SetKRDialogMap( pDef->GetResourceID().GetPrivateUID(), pScript->GetArgVal());
			else
				DEBUG_ERR(("Dialog '%s' not found...\n", pScript->GetKey()));
		}
		return true;

	// Saved in the world file.

	case RES_GMPAGE:	// saved in world file. (Name is NOT DEFNAME)
		{
			CGMPage * pGMPage = g_World.m_GMPages.emplace_back(std::make_unique<CGMPage>(pScript->GetArgStr())).get();
			return pGMPage->r_Load( *pScript );
		}
	case RES_WC:
	case RES_WORLDCHAR:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Undefined char type '%s'\n", pScript->GetArgStr());
			return false;
		}
		return CChar::CreateBasic( CREID_TYPE(rid.GetResIndex()) )->r_Load(*pScript);
	case RES_WI:
	case RES_WORLDITEM:	// saved in world file.
		if ( ! rid.IsValidUID())
		{
			g_Log.Event(LOGL_ERROR|LOGM_INIT, "Undefined item type '%s'\n", pScript->GetArgStr());
			return false;
		}
		return CItem::CreateBase( ITEMID_TYPE(rid.GetResIndex()) )->r_Load(*pScript);

	default:
		break;
	}

	if ( pNewLink )
	{
		pNewLink->SetResourceVar( pVarNum );

		// NOTE: we should not be linking to stuff in the *WORLD.SCP file.
		CResourceScript* pResScript = dynamic_cast <CResourceScript*>(pScript);
		if ( pResScript == nullptr )	// can only link to it if it's a CResourceScript !
		{
			DEBUG_ERR(( "Can't link resources in the world save file\n" ));
			return false;
		}
		// Now scan it for DEFNAME= or DEFNAME2= stuff ?
		pNewLink->SetLink(pResScript);
		pNewLink->ScanSection( restype );
	}
	else if ( pNewDef && pVarNum )
	{
		// Not linked but still may have a var name
		pNewDef->SetResourceVar( pVarNum );
	}
	return true;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("ExcInfo: section '%s' key '%s' args '%s'\n", pszSection,  pScript ? pScript->GetKey() : "",  pScript ? pScript->GetArgStr() : "");
	EXC_DEBUG_END;
	return false;
}

//*************************************************************

CResourceID CServerConfig::ResourceGetNewID( RES_TYPE restype, lpctstr pszName, CVarDefContNum ** ppVarNum, bool fNewStyleDef )
{
	ADDTOCALLSTACK("CServerConfig::ResourceGetNewID");
	// We are reading in a script block.
	// We may be creating a new id or replacing an old one.
	// ARGS:
	//	restype = the data type we are reading in.
	//  pszName = MAy or may not be the DEFNAME depending on type.

	ASSERT(pszName);
	ASSERT(ppVarNum);

	const CResourceID ridInvalid;	// LINUX wants this for some reason.
	CResourceID rid;
	word wPage = 0;	// sub page

	// Some types don't use named indexes at all. (Single instance)
	switch ( restype )
	{
	case RES_UNKNOWN:
		return ridInvalid;
	case RES_ADVANCE:
	case RES_BLOCKIP:
	case RES_COMMENT:
	case RES_DEFNAME:
	case RES_KRDIALOGLIST:
	case RES_MOONGATES:
	case RES_NOTOTITLES:
	case RES_OBSCENE:
	case RES_RESDEFNAME:
	case RES_RESOURCES:
	case RES_RUNES:
	case RES_SERVERS:
	case RES_SPHERE:
	case RES_STARTS:
	case RES_TELEPORTERS:
	case RES_TYPEDEFS:
	case RES_WORLDVARS:
	case RES_WORLDLISTS:
		// Single instance stuff. (fully read in)
		// Ignore any resource name.
		return CResourceID( restype );
	case RES_FUNCTION:		// Define a new command verb script that applies to a char.
	case RES_ACCOUNT:
	case RES_GMPAGE:
	case RES_SECTOR:
		// These must have a resource name but do not use true RESOURCE_ID format.
		// These are multiple instance but name is not a RESOURCE_ID
		if ( pszName[0] == '\0' )
			return ridInvalid;	// invalid
		return CResourceID( restype );
	// Extra args are allowed.
	case RES_BOOK:		// BOOK BookName page
	case RES_DIALOG:	// DIALOG GumpName ./TEXT/BUTTON
	case RES_REGIONTYPE:
		{
			if ( pszName[0] == '\0' )
				return ridInvalid;

			tchar * pArg1 = Str_GetTemp();
			Str_CopyLimitNull( pArg1, pszName, Str_TempLength() );
			pszName = pArg1;
			tchar * pArg2;
			Str_Parse( pArg1, &pArg2 );

			// For dialog resources, we use the page bits to mark if it's the TEXT or BUTTON block
			if ( !strnicmp( pArg2, "TEXT", 4 ) )
				wPage = RES_DIALOG_TEXT;
			else if ( !strnicmp( pArg2, "BUTTON", 6 ) )
				wPage = RES_DIALOG_BUTTON;
			else if ( !strnicmp( pArg2, "PREBUTTON", 9) )
				wPage = RES_DIALOG_PREBUTTON;
			else
			{
				// For a book the page is... the page number
				// For a REGIONTYPE block, the page (pArg2) is the landtile type associated with the REGIONTYPE
                int iArgPage = ResGetIndex(Exp_GetDWVal(pArg2));
                if ( iArgPage < RES_PAGE_MAX )
                    wPage = (word)iArgPage;
                else
                    DEBUG_ERR(( "Bad resource index page %d for Resource named %s\n", iArgPage, pszName ));
			}
		}
		break;
	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		{
			if ( pszName[0] == '\0' )
				return ridInvalid;
			tchar * pArg1 = Str_GetTemp();
			Str_CopyLimitNull( pArg1, pszName, Str_TempLength() );
			pszName = pArg1;
			tchar * pArg2;
			Str_Parse( pArg1, &pArg2 );
			if ( ! strcmpi( pArg2, "ELF" ))
				wPage = (word)RACETYPE_ELF;
			else if ( ! strcmpi( pArg2, "GARG" ))
                wPage = (word)RACETYPE_GARGOYLE;
			else if (*pArg2)
			{
				g_Log.EventWarn("Unrecognized race for a NEWBIE section. Defaulting to human.\n");
                wPage = (word)RACETYPE_HUMAN;
			}
			else
                wPage = (word)RACETYPE_HUMAN;

			if ( ! strcmpi( pszName, "MALE_DEFAULT" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_MALE_DEFAULT, wPage );
			else if ( ! strcmpi( pszName, "FEMALE_DEFAULT" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_FEMALE_DEFAULT, wPage );

			if ( ! strcmpi( pszName, "PROFESSION_ADVANCED" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_ADVANCED, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_WARRIOR" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_WARRIOR, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_MAGE" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_MAGE, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_BLACKSMITH" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_BLACKSMITH, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_NECROMANCER" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_NECROMANCER, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_PALADIN" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_PALADIN, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_SAMURAI" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_SAMURAI, wPage );
			else if ( ! strcmpi( pszName, "PROFESSION_NINJA" ))
				return CResourceID( RES_NEWBIE, RES_NEWBIE_PROF_NINJA, wPage );
		}
		break;
	case RES_AREA:
	case RES_ROOM:
		if ( !fNewStyleDef )
		{
			// Name is not the defname or id, just find a free id.
			pszName = nullptr;	// fake it out for now.
			break;
		}
		// otherwise, passthrough to default
		FALLTHROUGH;
	default:
		// The name is a DEFNAME or id number
		ASSERT( restype > RES_UNKNOWN && restype < RES_QTY );
		break;
	}


	int iIndex;
	if ( pszName )
	{
		if ( pszName[0] == '\0' )	// absence of resourceid = index 0
		{
			// This might be ok.
			return CResourceID( restype, 0, wPage );
		}
		if ( IsDigit(pszName[0]) )	// Its just an index.
		{
			iIndex = Exp_GetVal(pszName);
            if (iIndex > RES_INDEX_MASK)
            {
                g_Log.EventError("Requested resource with invalid ID. Value is too high (> 0%x).\n", RES_INDEX_MASK);
                return ridInvalid;
            }
			rid = CResourceID( restype, iIndex );
			switch ( restype )
			{
			case RES_BOOK:			// A book or a page from a book.
			case RES_DIALOG:		// A scriptable gump dialog: text or handler block.
			case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
				rid = CResourceID( restype, iIndex, wPage );
				break;
			case RES_SKILLMENU:
			case RES_MENU:			// General scriptable menus.
			case RES_EVENTS:		// An Event handler block with the trigger type in it. ON=@Death etc.
			case RES_SPEECH:		// A speech block with ON=*blah* in it.
			case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
			case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
			case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
			case RES_TYPEDEF:		// Define a trigger block for a RES_WORLDITEM m_type.
			case RES_CHARDEF:		// Define a char type.
			case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
            case RES_CHAMPION:
				break;
			case RES_ITEMDEF:		// Define an item type
				if (fNewStyleDef)	// indicates this is a multi and should have an appropriate offset applied
					rid = CResourceID(restype, iIndex + ITEMID_MULTI);
				break;
			default:
				return rid;
			}
#ifdef _DEBUG
			if ( g_Serv.GetServerMode() != SERVMODE_ResyncLoad )	// this really is ok.
			{
				// Warn of duplicates.
				size_t duplicateIndex = m_ResHash.FindKey( rid );
				if ( duplicateIndex != sl::scont_bad_index() )	// i found it. So i have to find something else.
					ASSERT(m_ResHash.GetBarePtrAt(rid, duplicateIndex));
			}
#endif
			return rid;
		}


		CVarDefCont * pVarBase = g_Exp.m_VarResDefs.GetKey( pszName );
		if ( pVarBase )
		{
			// An existing VarDef with the same name ?
			// We are creating a new Block but using an old name ? weird.
			// just check to see if this is a strange type conflict ?

			CVarDefContNum * pVarNum = dynamic_cast <CVarDefContNum*>( pVarBase );
			if ( pVarNum == nullptr )
			{
				switch (restype)
				{
					case RES_WC:
					case RES_WI:
					case RES_WS:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_WORLDSCRIPT:
					{
						const CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr*>( pVarBase );
						if ( pVarStr != nullptr )
							return ResourceGetNewID(restype, pVarStr->GetValStr(), ppVarNum, fNewStyleDef);
					}
					break;
					default:
						DEBUG_ERR(( "Re-Using DEFNAME='%s' to define a new block\n", pszName ));
						return ridInvalid;
				}
			}
			rid = CResourceID( (dword)pVarNum->GetValNum(), 0 );
			if ( restype != rid.GetResType())
			{
				switch ( restype )
				{
					case RES_WC:
					case RES_WI:
					case RES_WORLDCHAR:
					case RES_WORLDITEM:
					case RES_NEWBIE:
					case RES_PLEVEL:
						// These are not truly defining a new DEFNAME
						break;
					default:
						DEBUG_ERR(( "Redefined resource with DEFNAME='%s' from ResType %s to %s.\n",
							pszName, GetResourceBlockName(rid.GetResType()), GetResourceBlockName(restype)) );
						return ridInvalid;
				}
			}
			else if ( fNewStyleDef && (dword)pVarNum->GetValNum() != rid.GetPrivateUID() )
			{
				DEBUG_ERR(( "WARNING: region redefines DEFNAME='%s' for another region!\n", pszName ));
			}
			else if (RegisteredResourceGetDefRef(CResourceID(rid.GetResType(), rid.GetResIndex(), wPage)) )
			{
				// Books and dialogs have pages; if it's not a book or dialog, the if is 0 == 0, so execute it always

				// We are redefining an item we have already read in ?
				// Why do this unless it's a Resync ?
				if ( !g_Serv.IsResyncing() )
				{
					// Ensure it's not a "type", because hardcoded types indexes are defined in sphere_defs.scp,
					//  which is usually parsed before sphere_types.scp or its TYPEDEF block. So some time after declaring the
					//	index for a type we'll read its TYPEDEF, it would be normal to find another "declaration" for the type.
					if ( restype != RES_TYPEDEF )
						g_Log.EventWarn("Redefinition of resource with DEFNAME='%s' (res type=%d, index=0%x, page=%d).\n", pszName, rid.GetResType(), rid.GetResIndex(), wPage);
				}
			}
			rid = CResourceID( restype, rid.GetResIndex(), wPage );
			*ppVarNum = pVarNum;
			return rid;
		}
	}


	// At this point, we must define this as a new, unique entry.
	// Find a new free entry.

	int iHashRange = 0;
	switch ( restype )
	{

	// Some cannot create new id's
	// Do not allow NEW named indexs for some types for now.

	case RES_SKILL:			// Define attributes for a skill (how fast it raises etc)
		// rid = m_SkillDefs.GetCount();
	case RES_SPELL:			// Define a magic spell. (0-64 are reserved)
		// rid = m_SpellDefs.GetCount();
		return ridInvalid;

	// These MUST exist !

	case RES_NEWBIE:	// MALE_DEFAULT, FEMALE_DEFAULT, Skill
		return ridInvalid;
	case RES_PLEVEL:	// 0-7
		return ridInvalid;
	case RES_WC:
	case RES_WI:
	case RES_WORLDCHAR:
	case RES_WORLDITEM:
		return ridInvalid;

	// Just find a free entry in proper range.

	case RES_CHARDEF:		// Define a char type.
		iHashRange = 2000;
		iIndex = NPCID_SCRIPT + 0x2000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_ITEMDEF:		// Define an item type
		iHashRange = 2000;
		iIndex = ITEMID_SCRIPT2 + 0x4000;	// add another offset to avoid Sphere ranges.
		break;
	case RES_TEMPLATE:		// Define lists of items. (for filling loot etc)
		iHashRange = 2000;
		iIndex = ITEMID_TEMPLATE + 100000;
		break;

	case RES_BOOK:			// A book or a page from a book.
	case RES_DIALOG:		// A scriptable gump dialog: text or handler block.
		if ( wPage )	// We MUST define the main section FIRST !
			return ridInvalid;
		FALLTHROUGH;

	case RES_REGIONTYPE:	// Triggers etc. that can be assinged to a RES_AREA
		iHashRange = 100;
		iIndex = 1000;
		break;

	case RES_AREA:
		iHashRange = 1000;
		iIndex = 10000;
		break;
	case RES_ROOM:
	case RES_SKILLMENU:
	case RES_MENU:			// General scriptable menus.
	case RES_EVENTS:			// An Event handler block with the trigger type in it. ON=@Death etc.
	case RES_SPEECH:			// A speech block with ON=*blah* in it.
	case RES_NAMES:			// A block of possible names for a NPC type. (read as needed)
	case RES_SCROLL:		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
	case RES_TIP:			// Tips (similar to RES_SCROLL) that can come up at startup.
	case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
	case RES_SKILLCLASS:		// Define specifics for a char with this skill class. (ex. skill caps)
	case RES_REGIONRESOURCE:
		iHashRange = 1000;
		iIndex = 10000;
		break;
	case RES_SPAWN:			// Define a list of NPC's and how often they may spawn.
		iHashRange = 1000;
		iIndex = SPAWNTYPE_START + 100000;
		break;
    case RES_CHAMPION:
        iHashRange = 100;
        iIndex = 10000;	// RES_SPAWN +10k, no reason to have 100k [SPAWN ] templates ... but leaving this huge margin.
        break;
	case RES_WEBPAGE:		// Define a web page template.
		iIndex = (int)m_WebPages.size() + 1;
		break;

	default:
		ASSERT(0);
		return ridInvalid;
	}


	if ( iHashRange )
	{
		// find a new FREE entry starting here
        int iRandIndex = iIndex + g_Rand.GetVal(iHashRange);
        rid = CResourceID(restype, iRandIndex, wPage);

        const bool fCheckPage = (pszName && (g_Exp.m_VarResDefs.GetKeyNum(pszName) != 0));
		while (true)
		{
            if (fCheckPage)
            {
                // Same defname but different page?
                if (m_ResHash.FindKey(rid) == sl::scont_bad_index())
                    break;
            }
            else if (m_ResHash.FindKey(CResourceID(restype, iRandIndex, RES_PAGE_ANY)) == sl::scont_bad_index())
                break;

            ++iRandIndex;
            rid = CResourceID(restype, iRandIndex, wPage);
		}
	}
	else
	{
		// find a new FREE entry starting here
        rid = CResourceID(restype, iIndex ? iIndex : 1, wPage);
        ASSERT(m_ResHash.FindKey(rid) == sl::scont_bad_index());
	}

	if ( pszName )
	{
		CVarDefContNum* pVarTemp = g_Exp.m_VarResDefs.SetNum( pszName, rid.GetPrivateUID() );
        ASSERT(pVarTemp);
		*ppVarNum = pVarTemp;
	}

	return rid;
}

sl::smart_ptr_view<CResourceDef> CServerConfig::RegisteredResourceGetDefRefByName(RES_TYPE restype, lpctstr ptcName, word wPage)
{
	ADDTOCALLSTACK("CServerConfig::RegisteredResourceGetDefRefByName");
	return ResourceGetDefRefByName(restype, ptcName, wPage);
}

sl::smart_ptr_view<CResourceDef> CServerConfig::RegisteredResourceGetDefRef(const CResourceID& rid) const
{
	ADDTOCALLSTACK("CServerConfig::RegisteredResourceGetDefRef");
	// Get a CResourceDef from the RESOURCE_ID.
	// ARGS:
	//	restype = id must be this type.

	if (!rid.IsValidResource())
		return {};

    const int iIndex = rid.GetResIndex();
    const RES_TYPE iType = rid.GetResType();
    switch (iType)
	{
	case RES_WEBPAGE:
	{
		const size_t i = m_WebPages.find_sorted(rid);
		if (i == sl::scont_bad_index())
			return {};
		return m_WebPages[i];
	}

	case RES_SKILL:
        if (!m_SkillIndexDefs.valid_index(iIndex))
			return {};
        return m_SkillIndexDefs[iIndex];

	case RES_SPELL:
        if (!m_SpellDefs.valid_index(iIndex))
			return {};
        return m_SpellDefs[iIndex];

	case RES_UNKNOWN:	// legal to use this as a ref but it is unknown
		return {};

	case RES_BOOK:				// A book or a page from a book.
	case RES_EVENTS:
	case RES_DIALOG:			// A scriptable gump dialog: text or handler block.
	case RES_MENU:
	case RES_NAMES:				// A block of possible names for a NPC type. (read as needed)
	case RES_NEWBIE:			// MALE_DEFAULT, FEMALE_DEFAULT, Skill
	case RES_REGIONRESOURCE:
	case RES_REGIONTYPE:		// Triggers etc. that can be assinged to a RES_AREA
	case RES_SCROLL:			// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD: SCROLL_NEWBIE
	case RES_SPEECH:
	case RES_TIP:				// Tips (similar to RES_SCROLL) that can come up at startup.
	case RES_TYPEDEF:			// Define a trigger block for a RES_WORLDITEM m_type.
	case RES_TEMPLATE:
	case RES_SKILLMENU:
	case RES_ITEMDEF:
	case RES_CHARDEF:
	case RES_SPAWN:				// the char spawn tables
	case RES_SKILLCLASS:
	case RES_AREA:
	case RES_ROOM:
	case RES_CHAMPION:
		break;

	default:
		return {};
	}

	return CResourceHolder::ResourceGetDefRef(rid);
}

CResourceDef * CServerConfig::RegisteredResourceGetDef( const CResourceID& rid ) const
{
	ADDTOCALLSTACK("CServerConfig::RegisteredResourceGetDef");
	return RegisteredResourceGetDefRef(rid).get();
}

CResourceDef* CServerConfig::RegisteredResourceGetDefByName(RES_TYPE restype, lpctstr ptcName, word wPage)
{
	ADDTOCALLSTACK("CServerConfig::RegisteredResourceGetDefByName");
	return RegisteredResourceGetDefRefByName(restype, ptcName, wPage).get();
}

lpctstr CServerConfig::ResourceTypedGetName(const CResourceIDBase& rid, RES_TYPE iExpectedType, lptstr* ptcOutError)
{
    ADDTOCALLSTACK("CServerConfig::ResourceTypedGetName");
    CResourceID ridValid = CResourceID(iExpectedType, 0);
    if (!rid.IsValidResource())
    {
        if (rid.GetResIndex() != 0)
        {
            if (ptcOutError)
            {
                *ptcOutError = Str_GetTemp();
                snprintf(*ptcOutError, Str_TempLength(), "Expected a valid resource. Ignoring it/Converting it to an empty one.\n");
            }
        }
    }
    else if (rid.GetResType() != iExpectedType)
    {
        if (ptcOutError)
        {
            *ptcOutError = Str_GetTemp();
            snprintf(*ptcOutError, Str_TempLength(), "Expected resource with type %d, got %d. Ignoring it/Converting it to an empty one.\n",
                iExpectedType, rid.GetResType());
        }
    }
    else
    {
        ridValid = rid;
    }
    return ResourceGetName(ridValid); // Even it's 0, we should return it's name, as it can be mr_nothing.
}

lpctstr CServerConfig::ResourceGetName( const CResourceID& rid ) const
{
    ADDTOCALLSTACK("CServerConfig::ResourceGetName");
    // Get a portable name for the resource id type.

    if (rid.IsValidResource())
    {
        const CResourceDef* pResourceDef = RegisteredResourceGetDef(rid);
        if (pResourceDef)
            return pResourceDef->GetResourceName();
    }

    tchar * pszTmp = Str_GetTemp();
    ASSERT(pszTmp);
    if ( !rid.IsValidUID() )
        snprintf( pszTmp, Str_TempLength(), "%d", (int)rid.GetPrivateUID() );
    else
        snprintf( pszTmp, Str_TempLength(), "0%" PRIx32, rid.GetResIndex() );
    return pszTmp;
}

////////////////////////////////////////////////////////////////////////////////

void CServerConfig::_OnTick( bool fNow )
{
	ADDTOCALLSTACK("CServerConfig::_OnTick");
	// Give a tick to the less critical stuff.
	if ( !fNow && ( g_Serv.IsLoading() || ( m_timePeriodic > CWorldGameTime::GetCurrentTime().GetTimeRaw()) ) )
		return;

	if ( this->m_fUseHTTP )
	{
		// Update WEBPAGES resources
		for ( size_t i = 0; i < m_WebPages.size(); i++ )
		{
			EXC_TRY("WebTick");
			if ( !m_WebPages[i] )
				continue;
			CWebPageDef * pWeb = static_cast <CWebPageDef *>(m_WebPages[i].get());
			if ( pWeb )
			{
				pWeb->WebPageUpdate(fNow, nullptr, &g_Serv);
				pWeb->WebPageLog();
			}
			EXC_CATCH;

			EXC_DEBUG_START;
			CWebPageDef * pWeb = static_cast <CWebPageDef *>(m_WebPages[i].get());
			g_Log.EventDebug("web '%s' dest '%s' now '%d' index '%" PRIuSIZE_T "'/'%" PRIuSIZE_T "'\n",
				pWeb ? pWeb->GetName() : "", pWeb ? pWeb->GetDstName() : "",
				fNow? 1 : 0, i, m_WebPages.size());
			EXC_DEBUG_END;
		}
	}

	m_timePeriodic = CWorldGameTime::GetCurrentTime().GetTimeRaw() + ( 60 * MSECS_PER_SEC );
}

void CServerConfig::PrintEFOFFlags(bool bEF, bool bOF, CTextConsole *pSrc)
{
	ADDTOCALLSTACK("CServerConfig::PrintEFOFFlags");
	if ( g_Serv.IsLoading() )
        return;

#define catresname(a,b)	\
{	\
	if ( *(a) ) strcat(a, " + "); \
	strcat(a, b); \
}

	if ( bOF )
	{
		tchar zOptionFlags[512];
		zOptionFlags[0] = '\0';

		if ( IsSetOF(OF_NoDClickTarget) )			catresname(zOptionFlags, "NoDClickTarget");
		if ( IsSetOF(OF_NoSmoothSailing) )			catresname(zOptionFlags, "NoSmoothSailing");
		if ( IsSetOF(OF_ScaleDamageByDurability) )	catresname(zOptionFlags, "ScaleDamageByDurability");
		if ( IsSetOF(OF_Command_Sysmsgs) )			catresname(zOptionFlags, "CommandSysmessages");
		if ( IsSetOF(OF_PetSlots) )					catresname(zOptionFlags, "PetSlots");
		if ( IsSetOF(OF_OSIMultiSight) )			catresname(zOptionFlags, "OSIMultiSight");
		if ( IsSetOF(OF_Items_AutoName) )			catresname(zOptionFlags, "ItemsAutoName");
		if ( IsSetOF(OF_FileCommands) )				catresname(zOptionFlags, "FileCommands");
		if ( IsSetOF(OF_NoItemNaming) )				catresname(zOptionFlags, "NoItemNaming");
		if ( IsSetOF(OF_NoHouseMuteSpeech) )		catresname(zOptionFlags, "NoHouseMuteSpeech");
		if ( IsSetOF(OF_NoContextMenuLOS) )			catresname(zOptionFlags, "NoContextMenuLOS");
        if ( IsSetOF(OF_MapBoundarySailing) )		catresname(zOptionFlags, "MapBoundarySailing");
		if ( IsSetOF(OF_Flood_Protection) )			catresname(zOptionFlags, "FloodProtection");
		if ( IsSetOF(OF_Buffs) )					catresname(zOptionFlags, "Buffs");
		if ( IsSetOF(OF_NoPrefix) )					catresname(zOptionFlags, "NoPrefix");
		if ( IsSetOF(OF_DyeType) )					catresname(zOptionFlags, "DyeType");
		if ( IsSetOF(OF_DrinkIsFood) )				catresname(zOptionFlags, "DrinkIsFood");
		if ( IsSetOF(OF_NoDClickTurn) )				catresname(zOptionFlags, "NoDClickTurn");
        if ( IsSetOF(OF_NoPaperdollTradeTitle) )	catresname(zOptionFlags, "NoPaperdollTradeTitle");
        if ( IsSetOF(OF_NoTargTurn) )				catresname(zOptionFlags, "NoTargTurn");
        if ( IsSetOF(OF_StatAllowValOverMax) )		catresname(zOptionFlags, "StatAllowValOverMax");
        if ( IsSetOF(OF_GuardOutsideGuardedArea) )	catresname(zOptionFlags, "GuardOutsideGuardedArea");
        if ( IsSetOF(OF_OWNoDropCarriedItem) )		catresname(zOptionFlags, "OWNoDropCarriedItem");
		if ( IsSetOF(OF_AllowContainerInsideContainer)) catresname(zOptionFlags, "AllowContainerInsideContainer");
        if ( IsSetOF(OF_VendorStockLimit) )		    catresname(zOptionFlags, "VendorStockLimit");
		if ( IsSetOF(OF_NoDclickEquip) )				catresname(zOptionFlags, "NoDclickEquip");

		if ( zOptionFlags[0] != '\0' )
		{
			if ( pSrc )
				pSrc->SysMessagef("Option flags: %s\n", zOptionFlags);
			else
				g_Log.Event(LOGM_INIT, "Option flags: %s\n", zOptionFlags);
		}
	}
	if ( bEF )
	{
		tchar zExperimentalFlags[512];
		zExperimentalFlags[0] = '\0';

		if ( IsSetEF(EF_NoDiagonalCheckLOS) )		catresname(zExperimentalFlags, "NoDiagonalCheckLOS");
        if ( IsSetEF(EF_Dynamic_Backsave) )			catresname(zExperimentalFlags, "DynamicBacksave");
		if ( IsSetEF(EF_ItemStacking) )				catresname(zExperimentalFlags, "ItemStacking");
		if ( IsSetEF(EF_ItemStackDrop) )			catresname(zExperimentalFlags, "ItemStackDrop");
        if ( IsSetEF(EF_FastWalkPrevention) )		catresname(zExperimentalFlags, "FastWalkPrevention");
		if ( IsSetEF(EF_Intrinsic_Locals) )			catresname(zExperimentalFlags, "IntrinsicLocals");
		if ( IsSetEF(EF_Item_Strict_Comparison) )	catresname(zExperimentalFlags, "ItemStrictComparison");
		if ( IsSetEF(EF_AllowTelnetPacketFilter) )	catresname(zExperimentalFlags, "TelnetPacketFilter");
		if ( IsSetEF(EF_Script_Profiler) )			catresname(zExperimentalFlags, "ScriptProfiler");
		if ( IsSetEF(EF_DamageTools) )				catresname(zExperimentalFlags, "DamageTools");
		if ( IsSetEF(EF_UsePingServer) )			catresname(zExperimentalFlags, "UsePingServer");
		if ( IsSetEF(EF_FixCanSeeInClosedConts) )	catresname(zExperimentalFlags, "FixCanSeeInClosedConts");
        if ( IsSetEF(EF_WalkCheckHeightMounted) )	catresname(zExperimentalFlags, "WalkCheckHeightMounted");

		if ( zExperimentalFlags[0] != '\0' )
		{
			if ( pSrc )
				pSrc->SysMessagef("Experimental flags: %s\n", zExperimentalFlags);
			else
				g_Log.Event(LOGM_INIT, "Experimental flags: %s\n", zExperimentalFlags);
		}
	}

#undef catresname
}

bool CServerConfig::LoadIni(bool fTest)
{
	ADDTOCALLSTACK("CServerConfig::LoadIni");

    char filename[SPHERE_MAX_PATH] = SPHERE_FILE ".ini";

    // Check, if CLI argument -I=/path/to/ini/directory/ was used.
    if (m_iniDirectory[0] != '\0')
    {
        int const ret = snprintf(filename, SPHERE_MAX_PATH, "%s" SPHERE_FILE ".ini", m_iniDirectory);
        if (ret < 0)
        {
			g_Log.Event(LOGL_FATAL|LOGM_INIT|LOGF_CONSOLE_ONLY, "Path to %s" SPHERE_FILE ".ini is too long.\n", m_iniDirectory);
            return false;
        }
    }
    else
    {
        Str_CopyLimitNull(filename, SPHERE_FILE ".ini", SPHERE_MAX_PATH);
    }

	// Load my INI file first.
	if (!OpenResourceFind(m_scpIni, filename, !fTest))
	{
		if (!fTest)
		{
			g_Log.Event(LOGL_FATAL|LOGM_INIT|LOGF_CONSOLE_ONLY, SPHERE_FILE ".ini has not been found.\n");
			g_Log.Event(LOGL_FATAL|LOGM_INIT|LOGF_CONSOLE_ONLY, "Download a sample sphere.ini from https://github.com/Sphereserver/Source-X/tree/master/src\n");
		}
		return false;
	}

	LoadResourcesOpen(&m_scpIni);
	m_scpIni.Close();
	m_scpIni.CloseForce();

	return true;
}

bool CServerConfig::LoadCryptIni( void )
{
	ADDTOCALLSTACK("CServerConfig::LoadCryptIni");

    char filename[SPHERE_MAX_PATH] = SPHERE_FILE "Crypt.ini";

    // Check, if CLI argument -I=/path/to/ini/directory/ was used.
    if (m_iniDirectory[0] != '\0')
    {
        int const ret = snprintf(filename, SPHERE_MAX_PATH, "%s" SPHERE_FILE "Crypt.ini", m_iniDirectory);
        if (ret < 0)
        {
            g_Log.Event(LOGL_FATAL|LOGM_INIT|LOGF_CONSOLE_ONLY, "Path to %s" SPHERE_FILE "Crypt.ini is too long.\n", m_iniDirectory);
            return false;
        }
    }
    else
    {
        Str_CopyLimitNull(filename, SPHERE_FILE "Crypt.ini", SPHERE_MAX_PATH);
    }

    if (!OpenResourceFind(m_scpCryptIni, filename, false))
	{
		g_Log.Event(LOGL_WARN|LOGM_INIT, "Could not open " SPHERE_FILE "Crypt.ini, encryption might not be available\n");
		return false;
	}

	LoadResourcesOpen(&m_scpCryptIni);
	m_scpCryptIni.Close();
	m_scpCryptIni.CloseForce();

	g_Log.Event(LOGM_INIT, "Loaded %" PRIuSIZE_T " client encryption keys.\n",
		CCryptoKeysHolder::get()->client_keys.size());

	return true;
}

void CServerConfig::Unload( bool fResync )
{
	ADDTOCALLSTACK("CServerConfig::Unload");
	if ( fResync )
	{
		// Unlock all the MUL/UOP files.
		//g_Install.CloseFiles();   // Don't do this, since now we don't load again those files on resync.

        // Unlock all the SCP files.
		for ( size_t j = 0; ; ++j )
		{
			CResourceScript * pResFile = GetResourceFile(j);
			if ( pResFile == nullptr )
				break;
			pResFile->CloseForce();
		}
		m_scpIni.CloseForce();
		m_scpTables.CloseForce();
		return;
	}

	m_ResourceFiles.ClearFree();

	// m_ResHash.Clear();

	m_Obscene.ClearFree();
	m_Fame.ClearFree();
	m_Karma.ClearFree();
	m_NotoTitles.ClearFree();
	m_NotoKarmaLevels.clear();
	m_NotoFameLevels.clear();
	m_Runes.ClearFree();	// Words of power. (A-Z)
	// m_MultiDefs
	m_SkillNameDefs.clear();	// Defined Skills
	m_SkillIndexDefs.clear();
	// m_Servers
	m_Functions.clear();
	m_StartDefs.ClearFree(); // Start points list
	// m_StatAdv
	for ( int j=0; j<PLEVEL_QTY; ++j )
	{
		m_PrivCommands[j].ClearFree();
	}
	m_MoonGates.clear();
	// m_WebPages
	m_SpellDefs.clear();	// Defined Spells
	m_SpellDefs_Sorted.clear();
}

bool CServerConfig::Load( bool fResync )
{
	ADDTOCALLSTACK("CServerConfig::Load");
	// ARGS:
	//  fResync = just look for changes.

	if ( ! fResync )
	{
		g_Install.FindInstall();

        // Open the MUL files I need.
        g_Log.Event(LOGM_INIT, "\nIndexing the client files...\n");
        VERFILE_TYPE i = g_Install.OpenFiles(
            (1ull<<VERFILE_MAP)|
            (1<<VERFILE_STAIDX)|
            (1<<VERFILE_STATICS)|
            (1<<VERFILE_TILEDATA)|
            (1<<VERFILE_MULTIIDX)|
            (1<<VERFILE_MULTI)|
            (1<<VERFILE_VERDATA)
        );
        if ( i != VERFILE_QTY )
        {
            g_Log.Event( LOGL_FATAL|LOGM_INIT, "File '%s' not found...\n", g_Install.GetBaseFileName(i));
            return false;
        }

        // Load the optional verdata cache. (modifies MUL stuff)
        try
        {
            g_VerData.Load( g_Install.m_File[VERFILE_VERDATA] );
        }
        catch ( const CSError& e )
        {
            g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " SPHERE_FILE ".ini file is corrupt, missing, or there was an error while loading the settings.\n" );
            g_Log.CatchEvent( &e, "g_VerData.Load" );
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
            return false;
        }
        catch (...)
        {
            g_Log.Event( LOGL_FATAL|LOGM_INIT, "The " SPHERE_FILE ".ini file is corrupt, missing, or there was an error while loading the settings.\n" );
            g_Log.CatchEvent( nullptr, "g_VerData.Load" );
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
            return false;
        }
	}
	else
	{
		m_scpIni.ReSync();
		m_scpIni.CloseForce();
	}

	// Now load the *TABLES.SCP file.
	if ( ! fResync )
	{
		if ( ! OpenResourceFind( m_scpTables, SPHERE_FILE "tables" SPHERE_SCRIPT_EXT ))
		{
			g_Log.Event( LOGL_FATAL|LOGM_INIT, "Error opening table definitions file (" SPHERE_FILE "tables" SPHERE_SCRIPT_EXT ")...\n" );
			return false;
		}

        g_Log.Event(LOGL_EVENT|LOGM_INIT, "Loading table definitions file (" SPHERE_FILE "tables" SPHERE_SCRIPT_EXT ")...\n");
		LoadResourcesOpen(&m_scpTables);
		m_scpTables.Close();
	}
	else
	{
		m_scpTables.ReSync();
	}
	m_scpTables.CloseForce();

	//	Initialize the world sectors
	if (!fResync)
	{
		g_Log.Printf("\n");
		g_Log.Event(LOGM_INIT, "Initializing the world...\n");
        g_World.Init();
	}

	// open and index all my script files i'm going to use.
	AddResourceDir( m_sSCPBaseDir );		// if we want to get *.SCP files from elsewhere.

	size_t count = m_ResourceFiles.size();
	g_Log.Printf("\n");
	g_Log.Event(LOGM_INIT, "Indexing %" PRIuSIZE_T " script files...\n", count);

	for ( size_t j = 0; ; ++j )
	{
        if (g_Serv.GetExitFlag())
        {
            g_Log.Event(LOGM_INIT, "Script loading aborted!\n");
            break;
        }
		CResourceScript * pResFile = GetResourceFile(j);
		if ( !pResFile )
			break;

		if ( !fResync )
			LoadResources( pResFile );
		else
			pResFile->ReSync();

		g_Serv.PrintPercent( (size_t)(j + 1), count);
	}

	// Now that we have parsed every script, we can end the configuration of some resources...
		// ROOMs have to inherit stuff from the parent AREADEF
	for (CRegion* pCurRegion : m_RegionDefs)
	{
		const RES_TYPE resType = pCurRegion->GetResourceID().GetResType();
		if (resType != RES_ROOM)
			continue;

		const CPointBase ptRoomCenter = pCurRegion->m_rectUnion.GetCenter();
		CRegionLinks vTopRegions;
		ptRoomCenter.GetRegions(REGION_TYPE_AREA, &vTopRegions);
		if (vTopRegions.empty())
			continue;

		CSString sEventsList;
		for (CRegion* pTopRegion : vTopRegions)
		{
			const auto curRegionFlags = pCurRegion->GetRegionFlags();
			if ((g_Cfg._uiAreaFlags & AREAF_RoomInheritsEvents) || (curRegionFlags & REGION_FLAG_INHERIT_PARENT_EVENTS))
			{
				pTopRegion->m_Events.WriteResourceRefList(sEventsList);

				pCurRegion->SetModified(REGMOD_EVENTS);
				CScript scriptEvent("EVENTS", sEventsList.GetBuffer());
				pCurRegion->m_Events.r_LoadVal(scriptEvent, RES_REGIONTYPE);
			}

			if ((g_Cfg._uiAreaFlags & AREAF_RoomInheritsTAGs) || (curRegionFlags & REGION_FLAG_INHERIT_PARENT_TAGS))
			{
				pCurRegion->m_TagDefs.Copy(&pTopRegion->m_TagDefs, false);
			}

			if ((g_Cfg._uiAreaFlags & AREAF_RoomInheritsFlags) || (curRegionFlags & REGION_FLAG_INHERIT_PARENT_FLAGS))
			{
				pCurRegion->SetRegionFlags(curRegionFlags | pTopRegion->GetRegionFlags());
			}
		}
	}

	// Make sure we have the basics.
	if ( g_Serv.GetName()[0] == '\0' )	// make sure we have a set name
	{
		tchar szName[ MAX_SERVER_NAME_SIZE ];
		int iRet = gethostname( szName, sizeof( szName ));
		g_Serv.SetName(( ! iRet && szName[0] ) ? szName : SPHERE_TITLE );
	}

	if ( !RegisteredResourceGetDefRef( CResourceID( RES_SKILLCLASS, 0 )) )
	{
		// must have at least 1 skill class.
		CSkillClassDef * pSkillClass = new CSkillClassDef( CResourceID( RES_SKILLCLASS ));
		ASSERT(pSkillClass);
		m_ResHash.AddSortKey( CResourceID( RES_SKILLCLASS, 0 ), pSkillClass );
	}

	if ( !fResync )
	{
		int total, used;
		Triglist(total, used);
        g_Log.Event(LOGL_EVENT, "Done loading scripts (%d of %d triggers used).\n", used, total);
	}
	else
		g_Log.Event(LOGM_INIT, "Done loading scripts.\n");

	if (m_StartDefs.empty() )
	{
		g_Log.Event(LOGM_INIT|LOGL_ERROR, "No START locations specified. Add them and try again.\n");
		return false;
	}

	// Make region DEFNAMEs
    size_t iRegionMax = m_RegionDefs.size();
    for (size_t k = 0; k < iRegionMax; ++k)
    {
        CRegion * pRegion = dynamic_cast <CRegion*> (m_RegionDefs[k]);
        if (!pRegion)
            continue;
        pRegion->MakeRegionDefname();
    }

	// parse eventsitem
	m_iEventsItemLink.clear();
	if ( ! m_sEventsItem.IsEmpty() )
	{
		CScript script("EVENTSITEM", m_sEventsItem);
		m_iEventsItemLink.r_LoadVal(script, RES_EVENTS);
	}

	// parse eventspet
	m_pEventsPetLink.clear();
	if ( ! m_sEventsPet.IsEmpty() )
	{
		CScript script("EVENTSPET", m_sEventsPet);
		m_pEventsPetLink.r_LoadVal(script, RES_EVENTS);
	}

	// parse eventsplayer
	m_pEventsPlayerLink.clear();
	if ( ! m_sEventsPlayer.IsEmpty() )
	{
		CScript script("EVENTSPLAYER", m_sEventsPlayer);
		m_pEventsPlayerLink.r_LoadVal(script, RES_EVENTS);
	}

	// parse eventsregion
	m_pEventsRegionLink.clear();
	if ( ! m_sEventsRegion.IsEmpty() )
	{
		CScript script("EVENTSREGION", m_sEventsRegion);
		m_pEventsRegionLink.r_LoadVal(script, RES_REGIONTYPE);
	}

	if (!m_sChatStaticChannels.IsEmpty())
	{
		tchar* ppArgs[32];
		size_t iChannels = Str_ParseCmds(const_cast<tchar *>(g_Cfg.m_sChatStaticChannels.GetBuffer()), ppArgs, ARRAY_COUNT(ppArgs), ",");
		for (size_t i = 0; i < iChannels; i++)
			g_Serv.m_Chats.CreateChannel(ppArgs[i]);
	}

	LoadSortSpells();
    g_Log.Event(LOGL_EVENT, "\n");


	// Load crypt keys from SphereCrypt.ini
	if ( ! fResync )
	{
		LoadCryptIni();
	}
	else
	{
		m_scpCryptIni.ReSync();
		m_scpCryptIni.CloseForce();
	}

	// Yay for crypt version
	g_Serv.SetCryptVersion();

	return true;
}

lpctstr CServerConfig::GetDefaultMsg(int lKeyNum)
{
	ADDTOCALLSTACK("CServerConfig::GetDefaultMsg");
	if (( lKeyNum < 0 ) || ( lKeyNum >= DEFMSG_QTY ))
	{
		g_Log.EventError("Defmessage %d out of range [0..%d]\n", lKeyNum, DEFMSG_QTY-1);
		return "";
	}
	return g_Exp.sm_szMessages[lKeyNum];
}

lpctstr CServerConfig::GetDefaultMsg(lpctstr ptcKey)
{
	ADDTOCALLSTACK("CServerConfig::GetDefaultMsg");
	for (int i = 0; i < DEFMSG_QTY; ++i )
	{
		if ( !strcmpi(ptcKey, g_Exp.sm_szMsgNames[i]) )
			return g_Exp.sm_szMessages[i];

	}
	g_Log.EventError("Defmessage \"%s\" non existent\n", ptcKey);
	return "";
}

bool CServerConfig::GenerateDefname(tchar *pObjectName, size_t iInputLength, lpctstr pPrefix, TemporaryString *pOutput, bool bCheckConflict, CVarDefMap* vDefnames)
{
	ADDTOCALLSTACK("CServerConfig::GenerateDefname");
	if ( !pOutput )
		return false;

	tchar* buf = const_cast<tchar*>(pOutput->buffer());
	if ( !pObjectName || !pObjectName[0] )
	{
		buf[0] = '\0';
		return false;
	}

	size_t iOut = 0;

	if (pPrefix)
	{
		// write prefix
		for (size_t i = 0; pPrefix[i] != '\0'; ++i)
			buf[iOut++] = pPrefix[i];
	}

	// write object name
	for (size_t i = 0; i < iInputLength; ++i)
	{
		if (pObjectName[i] == '\0')
			break;

		if ( IsWhitespace(pObjectName[i]) )
		{
			if (iOut > 0 && buf[iOut - 1] != '_') // avoid double '_'
				buf[iOut++] = '_';
		}
		else if ( _ISCSYMF(pObjectName[i]) )
		{
			if (pObjectName[i] != '_' || (iOut > 0 && buf[iOut - 1] != '_')) // avoid double '_'
				buf[iOut++] = static_cast<tchar>(tolower(pObjectName[i]));
		}
	}

	// remove trailing _
	while (iOut > 0 && buf[iOut - 1] == '_')
		--iOut;

	buf[iOut] = '\0';
	if (iOut == 0)
		return false;

	if (bCheckConflict == true)
	{
		// check for conflicts
		size_t iEnd = iOut;
		int iAttempts = 1;

		for (;;)
		{
			bool isValid = true;
			if (g_Exp.m_VarResDefs.GetKey(buf) != nullptr)
			{
				// check loaded defnames
				isValid = false;
			}
			else if (vDefnames && vDefnames->GetKey(buf) != nullptr)
			{
				isValid = false;
			}

			if (isValid == true)
				break;

			// attach suffix
			iOut = iEnd;
			buf[iOut++] = '_';
            Str_FromI(++iAttempts, &buf[iOut], pOutput->capacity(), 10);
		}
	}

	// record defname
	if (vDefnames != nullptr)
		vDefnames->SetNum(buf, 1);

	return true;
}

bool CServerConfig::DumpUnscriptedItems( CTextConsole * pSrc, lpctstr pszFilename )
{
	ADDTOCALLSTACK("CServerConfig::DumpUnscriptedItems");
	if ( pSrc == nullptr )
		return false;

	if ( pszFilename == nullptr || pszFilename[0] == '\0' )
		pszFilename	= "unscripted_items" SPHERE_SCRIPT_EXT;
	else if ( strlen( pszFilename ) <= 4 )
		return false;

	// open file
	CScript s;
	if ( ! s.Open( pszFilename, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
		return false;

	tchar ptcItemName[21];
	TemporaryString tsDefnameBuffer;
	CVarDefMap defnames;

	s.Printf("// Unscripted items, generated by " SPHERE_TITLE " at %s\n", CSTime::GetCurrentTime().Format(nullptr));

	ITEMID_TYPE idMaxItem = ITEMID_TYPE(g_Install.m_tiledata.GetItemMaxIndex());
	if (idMaxItem > ITEMID_MULTI)
		idMaxItem = ITEMID_MULTI;

	for (uint i = 0; i < idMaxItem; ++i)
	{
		if ( !( i % 0xff ))
			g_Serv.PrintPercent(i, idMaxItem);

		CResourceID rid = CResourceID(RES_ITEMDEF, i);
		if (m_ResHash.FindKey(rid) != sl::scont_bad_index())
			continue;

		// check item in tiledata
		CUOItemTypeRec_HS tiledata;
		if (CItemBase::GetItemData((ITEMID_TYPE)i, &tiledata) == false)
			continue;

		// ensure there is actually some data here, treat "MissingName" as blank since some tiledata.muls
		// have this name set in blank slots
		if ( !tiledata.m_flags && !tiledata.m_weight && !tiledata.m_layer &&
			 !tiledata.m_dwUnk11 && !tiledata.m_wAnim && !tiledata.m_wHue && !tiledata.m_wLightIndex && !tiledata.m_height &&
			 (!tiledata.m_name[0] || strcmp(tiledata.m_name, "MissingName") == 0))
			 continue;

		s.WriteSection("ITEMDEF 0%04x", i);
        Str_CopyLimitNull(ptcItemName, tiledata.m_name, ARRAY_COUNT(ptcItemName));

		// generate a suitable defname
		if (GenerateDefname(ptcItemName, ARRAY_COUNT(ptcItemName), "i_", &tsDefnameBuffer, true, &defnames))
		{
			s.Printf("// %s\n", ptcItemName);
			s.WriteKeyStr("DEFNAME", tsDefnameBuffer.buffer());
		}
		else
		{
			s.Printf("// (unnamed object)\n");
		}

		s.WriteKeyStr("TYPE", ResourceGetName(CResourceID(RES_TYPEDEF, CItemBase::GetTypeBase((ITEMID_TYPE)i, tiledata))));
	}

	s.WriteSection("EOF");
	s.Close();

#ifdef _WIN32
    g_NTWindow.SetWindowTitle();
#endif
	return true;
}
