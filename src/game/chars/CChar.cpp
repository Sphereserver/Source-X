//  CChar is either an NPC or a Player.

#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../common/CUID.h"
#include "../../common/CRect.h"
#include "../../common/CLog.h"
#include "../../sphere/ProfileTask.h"
#include "../clients/CAccount.h"
#include "../clients/CClient.h"
#include "../items/CItem.h"
#include "../items/CItemContainer.h"
#include "../items/CItemMemory.h"
#include "../items/CItemShip.h"
#include "../items/CItemMulti.h"
#include "../components/CCPropsChar.h"
#include "../components/CCPropsItemChar.h"
#include "../components/CCSpawn.h"
#include "../CContainer.h"
#include "../CSector.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldTickingList.h"
#include "../spheresvr.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharBase.h"
#include "CCharNPC.h"
#include <algorithm>


lpctstr const CChar::sm_szTrigName[CTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@AfterClick",
	"@Attack",				// I am attacking someone (SRC)
	"@CallGuards",

	"@charAttack",			// Here starts @charXXX section
	"@charClick",
	"@charClientTooltip",
	"@charClientTooltip_AfterDefault",
	"@charContextMenuRequest",
	"@charContextMenuSelect",
	"@charDClick",
	"@charTradeAccepted",

	"@Click",				// I got clicked on by someone.
	"@ClientTooltip", 		// Sending tooltips to someone
	"@ClientTooltip_AfterDefault",
	"@CombatAdd",
	"@CombatDelete",
	"@CombatEnd",
	"@CombatStart",
	"@ContextMenuRequest",
	"@ContextMenuSelect",
	"@Create",				// Newly created (not in the world yet)
    "@CreateLoot",
	"@Criminal",			// Called before somebody becomes "gray" for someone
	"@DClick",				// Someone has dclicked on me.
	"@Death",				//+I just got killed.
	"@DeathCorpse",
	"@Destroy",				//+I am nearly destroyed
	"@Dismount",			// I am trying to get rid of my ride right now
	"@Dye",					// My color has been changed
	"@Eat",
	"@EnvironChange",		// my environment changed somehow (light,weather,season,region)
	"@ExpChange",			// EXP is going to change
	"@ExpLevelChange",		// Experience LEVEL is going to change
	"@FameChange",				// Fame changed
	"@FollowersUpdate",

	"@GetHit",				// I just got hit.
	"@Hit",					// I just hit someone. (TARG)
	"@HitCheck",			// Checking if I can hit my target, overriding also default checks if set to.
	"@HitIgnore",			// I'm going to avoid a target (attacker.n.ignore=1) , should I un-ignore him?.
	"@HitMiss",				// I just missed.
	"@HitParry",			// I succesfully parried an hit.
	"@HitTry",				// I am trying to hit someone. starting swing.
    "@HouseDesignBegin",    // Starting to customize.
    "@HouseDesignCommit",	// I committed a new house design.
    "@HouseDesignCommitItem",// I committed an item to the house design.
	"@HouseDesignExit",		// I exited house design mode.

	// ITRIG_QTY
	"@itemAfterClick",
	"@itemBuy",
	"@itemCarveCorpse",			// I am carving a corpse.
	"@itemClick",			// I clicked on an item
	"@itemClientTooltip", 	// Receiving tooltip for something
	"@itemClientTooltip_AfterDefault",
	"@itemContextMenuRequest",
	"@itemContextMenuSelect",
	"@itemCreate",			//?
	"@itemDamage",			//?
	"@itemDCLICK",			// I have dclicked item
	"@itemDestroy",			//+I am nearly destroyed
	"@itemDropOn_Char",		// I have been dropped on this char
	"@itemDropOn_Ground",	// I dropped an item on the ground
	"@itemDropOn_Item",		// I have been dropped on this item
	"@itemDropOn_Self",		// An item has been dropped on
	"@ItemDropOn_Trade",
	"@itemEQUIP",			// I have equipped an item
	"@itemEQUIPTEST",
	"@itemMemoryEquip",
	"@itemPICKUP_GROUND",
	"@itemPICKUP_PACK",		// picked up from inside some container.
	"@itemPICKUP_SELF",		// picked up from this container
	"@itemPICKUP_STACK",	// picked up from a stack
    "@itemRedeed",
    "@ItemRegionEnter",
    "@ItemRegionLeave",
	"@itemSELL",
	"@itemSmelt",			// I am smelting an item.
	"@itemSPELL",			// cast some spell on the item.
	"@itemSTEP",			// stepped on an item
	"@itemTARGON_CANCEL",
	"@itemTARGON_CHAR",
	"@itemTARGON_GROUND",
	"@itemTARGON_ITEM",		// I am being combined with an item
	"@itemTimer",			//?
	"@itemToolTip",			// Did tool tips on an item
	"@itemUNEQUIP",			// i have unequipped (or try to unequip) an item

	"@Jailed",
	"@KarmaChange",				// Karma chaged
	"@Kill",				//+I have just killed someone
	"@LogIn",				// Client logs in
	"@LogOut",				// Client logs out (21)
	"@Mount",				// I'm trying to mount my horse (or whatever)
	"@MurderDecay",			// One of my kills is gonna to be cleared
	"@MurderMark",			// I am gonna to be marked as a murderer
	"@NotoSend",			// Sending notoriety

	"@NPCAcceptItem",		// (NPC only) i've been given an item i like (according to DESIRES)
	"@NPCActFight",
	"@NPCActFollow",		// (NPC only) following someone right now
	"@NPCAction",
	"@NPCActWander",		// (NPC only) i'm wandering aimlessly
	"@NPCHearGreeting",		// (NPC only) i have been spoken to for the first time. (no memory of previous hearing)
	"@NPCHearUnknown",		//+(NPC only) I heard something i don't understand.
	"@NPCLookAtChar",		// (NPC only) look at a character
	"@NPCLookAtItem",		// (NPC only) look at a character
	"@NPCLostTeleport",		//+(NPC only) ready to teleport back to spawn
	"@NPCRefuseItem",		// (NPC only) i've been given an item i don't want.
	"@NPCRestock",			// (NPC only)
	"@NPCSeeNewPlayer",		//+(NPC only) i see u for the first time. (in 20 minutes) (check memory time)
	"@NPCSeeWantItem",		// (NPC only) i see something good.
	"@NPCSpecialAction",	// Idle

	"@PartyDisband",		//I just disbanded my party
	"@PartyInvite",			//SRC invited me to join a party, so I may chose
	"@PartyLeave",
	"@PartyRemove",			//I have ben removed from the party by SRC

    "@PayGold",             // I'm going to give out money for a service (Skill Training, hiring...).
	"@PersonalSpace",		// +i just got stepped on.
	"@PetDesert",			// I just went wild again
	"@Profile",				// someone hit the profile button for me.
	"@ReceiveItem",			// I was just handed an item (Not yet checked if i want it)
	"@RegenStat",			// Regenerating any stat

	"@RegionEnter",
	"@RegionLeave",
	"@RegionResourceFound",	// I just discovered a resource
	"@RegionResourceGather",

	"@Rename",
	"@Resurrect",
	"@SeeCrime",			// I saw a crime
	"@SeeHidden",			// Can I see hidden chars?
	"@SeeSnoop",

	// SKTRIG_QTY
	"@SkillAbort",
	"@SkillChange",
	"@SkillFail",
	"@SkillGain",
	"@SkillMakeItem",
	"@SkillMenu",
	"@SkillPreStart",
	"@SkillSelect",
	"@SkillStart",
	"@SkillStroke",
	"@SkillSuccess",
	"@SkillTargetCancel",
	"@SkillUseQuick",
	"@SkillWait",

	"@SpellBook",
	"@SpellCast",			// Char is casting a spell.
	"@SpellEffect",			// A spell just hit me.
	"@SpellEffectAdd",		// A spell memory item is going to be placed upon me.
	"@SpellEffectRemove",   // A spell memory item is going to be removed from me.
    "@SpellEffectTick",		// A spell is going to tick and have an effect on me.
	"@SpellFail",			// The spell failed
	"@SpellSelect",			// Selected a spell
	"@SpellSuccess",		// The spell succeeded
	"@SpellTargetCancel",	// cancelled spell target
	"@StatChange",
	"@StepStealth",			// Made a step in stealth mode
	"@Targon_Cancel",		//closing target from TARGETF*
	"@ToggleFlying",		//Flying On/Off
	"@ToolTip",				// someone did tool tips on me.
	"@TradeAccepted",		// Everything went well, and we are about to exchange trade items
	"@TradeClose",
	"@TradeCreate",

	"@UserBugReport",
	"@UserChatButton",
	"@UserExtCmd",
	"@UserExWalkLimit",
	"@UserGuildButton",
	"@UserKRToolbar",
	"@UserMailBag",
	"@UserQuestArrowClick",
	"@UserQuestButton",
	"@UserSkills",
	"@UserSpecialMove",
	"@UserStats",
    "@UserUltimaStoreButton",
	"@UserVirtue",
	"@UserVirtueInvoke",
	"@UserWarmode",	        // War mode ?
    nullptr
};


/////////////////////////////////////////////////////////////////
// -CChar

// Create the "basic" NPC. Not NPC or player yet.
// NOTE: NEVER return nullptr
CChar * CChar::CreateBasic(CREID_TYPE baseID) // static
{
	ADDTOCALLSTACK("CChar::CreateBasic");
    CChar *pChar = new CChar(baseID);
	return pChar;
}

CChar::CChar( CREID_TYPE baseID ) :
	CTimedObject(PROFILE_CHARS),
	CObjBase( false ),
	m_sTitle(false),
    m_Skill{}, m_Stat{}
{
	g_Serv.StatInc( SERV_STAT_CHARS );	// Count created CChars.

	m_pArea = nullptr;
	m_pParty = nullptr;
	m_pClient = nullptr;	// is the char a logged in player?
	m_pPlayer = nullptr;	// May even be an off-line player!
	m_pNPC	  = nullptr;
	m_pRoom = nullptr;
	_uiStatFlag = 0;

	if ( g_World.m_fSaveParity )
		StatFlag_Set(STATF_SAVEPARITY);	// It will get saved next time.

	m_dirFace = DIR_SE;
	m_fonttype = FONT_NORMAL;
	m_SpeechHueOverride = 0;
	m_EmoteHueOverride = 0;

    m_exp = 0;
    m_level = 0;
    m_defense = 0;
	m_height = 0;
	m_ModMaxWeight = 0;
    _uiRange = RANGE_MAKE(1, 0);   // RangeH = 1; RangeL = 0

	m_StepStealth = 0;
	m_iVisualRange = UO_MAP_VIEW_SIZE_DEFAULT;
	m_virtualGold = 0;

	_iTimePeriodicTick = 0;
    _iTimeCreate = _iTimeLastHitsUpdate = CWorldGameTime::GetCurrentTime().GetTimeRaw();
    _iTimeNextRegen = _iTimeLastHitsUpdate + MSECS_PER_SEC;  // make it regen in one second from now, no need to instant regen.
    _iRegenTickCount = 0;
	_iTimeLastCallGuards = 0;

    m_zClimbHeight = 0;
	m_fClimbUpdated = false;
	_wPrev_Hue = HUE_DEFAULT;
	_iPrev_id = CREID_INVALID;
	SetID( baseID );

	CCharBase* pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	m_attackBase = pCharDef->m_attackBase;
	m_attackRange = pCharDef->m_attackRange;
	m_defenseBase = pCharDef->m_defenseBase;
	m_defenseRange = pCharDef->m_defenseRange;
	_wBloodHue = pCharDef->_wBloodHue;	// when damaged , what color is the blood (-1) = no blood

	SetName( pCharDef->GetTypeName());	// set the name in case there is a name template.

    Stat_SetVal( STAT_FOOD, Stat_GetMaxAdjusted(STAT_FOOD) );
    m_uiFame = 0;
    m_iKarma = 0;

	m_Act_Difficulty = 0;
	m_Act_SkillCurrent = SKILL_NONE;
    m_atUnk.m_dwArg1 = 0;
    m_atUnk.m_dwArg2 = 0;
    m_atUnk.m_dwArg3 = 0;

	g_World.m_uidLastNewChar = GetUID();	// for script access.

    // SubscribeComponent Prop Components
	TrySubscribeComponentProps<CCPropsChar>();
	TrySubscribeComponentProps<CCPropsItemChar>();

    // SubscribeComponent regular Components
    SubscribeComponent(new CCFaction());

	ASSERT(IsDisconnected());
}

// Delete character
CChar::~CChar()
{
	EXC_TRY("Cleanup in destructor");
	ADDTOCALLSTACK("CChar::~CChar");

	CChar::DeletePrepare();
	CChar::DeleteCleanup(true);

    if (IsClientActive())    // this should never happen.
    {
        ASSERT( m_pClient );
        m_pClient->GetNetState()->markReadClosed();
    }

    if (m_pParty)
    {
        m_pParty->RemoveMember( GetUID(), GetUID() );
        m_pParty = nullptr;
    }
    Guild_Resign(MEMORY_GUILD);
    Guild_Resign(MEMORY_TOWN);
    Attacker_RemoveChar();		// Removing me from enemy's attacker list (I asume that if he is on my list, I'm on his one and no one have me on their list if I dont have them)
    if (m_pNPC)
        NPC_PetClearOwners();	// Clear follower slots on pet owner

    ClearNPC();
    ClearPlayer();

    g_Serv.StatDec( SERV_STAT_CHARS );

	EXC_CATCH;
}

void CChar::DeleteCleanup(bool fForce)
{
	ADDTOCALLSTACK("CChar::DeleteCleanup");
	_fDeleting = true;

	// We don't want to have invalid pointers over there
	// Already called by CObjBase::DeletePrepare -> CObjBase::_GoSleep
	//CWorldTickingList::DelObjSingle(this);
	//CWorldTickingList::DelObjStatusUpdate(this, false);

	CWorldTickingList::DelCharPeriodic(this, false);


	if (IsStatFlag(STATF_RIDDEN))
	{
		CItem* pItem = Horse_GetMountItem();
		if (pItem)
		{
			pItem->m_itFigurine.m_UID.InitUID();    // unlink it first.
			pItem->Delete(fForce);
		}
		StatFlag_Clear(STATF_RIDDEN);
	}
}

// Called before Delete()
// @Destroy or f_onchar_delete can prevent the deletion
bool CChar::NotifyDelete()
{
	ADDTOCALLSTACK("CChar::NotifyDelete");
	if (IsDeleted())
		return false;

	//	Checking for delete allowed in @Destroy
	if (IsTrigUsed(TRIGGER_DESTROY))
	{
		//We can forbid the deletion in here with no pain
		if (CChar::OnTrigger(CTRIG_Destroy, &g_Serv) == TRIGRET_RET_TRUE)
			return false;
	}

	// If this is a player, check for f_onchar_delete
	if (m_pClient)
	{
		TRIGRET_TYPE trigReturn;
		CScriptTriggerArgs Args;
		Args.m_pO1 = m_pClient;
		r_Call("f_onchar_delete", this, &Args, nullptr, &trigReturn);
		if (trigReturn == TRIGRET_RET_TRUE)
			return false;
	}
	
	// Clear follower slots on pet owner
	if (m_pNPC)
		NPC_PetClearOwners();

	ContentNotifyDelete();
	return true;
}

void CChar::DeletePrepare()
{
	ADDTOCALLSTACK("CChar::DeletePrepare");
	CContainer::ContentDelete(true);	// This object and its contents need to be deleted on the same tick
	CObjBase::DeletePrepare();
}

bool CChar::Delete(bool fForce)
{
	ADDTOCALLSTACK("CChar::Delete");

	if ((NotifyDelete() == false) && !fForce)
		return false;

	// Character has been deleted
	if (IsClientActive())
	{
		CClient* pClient = GetClientActive();
		pClient->CharDisconnect();
		pClient->GetNetState()->markReadClosed();
	}
	
	DeleteCleanup(fForce);	// not virtual

	return CObjBase::Delete();
}

// Client is detaching from this CChar.
void CChar::ClientDetach()
{
	ADDTOCALLSTACK("CChar::ClientDetach");

	// remove all trade windows.
	for (CSObjContRec* pObjRec : GetIterationSafeCont())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if (pItem->IsType(IT_EQ_TRADE_WINDOW) )
			pItem->Delete();
	}
	if ( !IsClientActive() )
		return;

	if ( m_pParty && m_pParty->IsPartyMaster( this ))
	{
		// Party must disband if the master is logged out.
		m_pParty->Disband(GetUID());
		m_pParty = nullptr;
	}

	// If this char is on a IT_SHIP then we need to stop the ship !
	if ( m_pArea && m_pArea->IsFlag( REGION_FLAG_SHIP ))
	{
		CItemShip * pShipItem = dynamic_cast <CItemShip *>( m_pArea->GetResourceID().ItemFindFromResource());
		if ( pShipItem )
			pShipItem->Stop();
	}

    m_pClient = nullptr;
}

// Client is Attaching to this CChar.
void CChar::ClientAttach( CClient * pClient )
{
	ADDTOCALLSTACK("CChar::ClientAttach");
	if ( GetClientActive() == pClient )
		return;
	if ( ! SetPlayerAccount( pClient->GetAccount()))	// i now own this char.
		return;

	ASSERT(m_pPlayer);
	m_pPlayer->_iTimeLastUsed = CWorldGameTime::GetCurrentTime().GetTimeRaw();

	m_pClient = pClient;
	FixClimbHeight();
}

bool CChar::IsClientType() const noexcept
{
	return (m_pClient || m_pPlayer);
}

// Client logged out or NPC is dead.
void CChar::SetDisconnected(CSector* pNewSector)
{
	ADDTOCALLSTACK("CChar::SetDisconnected");
    if (IsClientActive())
    {
        GetClientActive()->GetNetState()->markReadClosed();
    }

	if (m_pPlayer)
	{
		m_pPlayer->_iTimeLastDisconnected = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	}

    if (m_pParty)
    {
        m_pParty->RemoveMember( GetUID(), GetUID() );
        m_pParty = nullptr;
    }

    if ( IsDisconnected() )
        return;

	// If the char goes offline, we don't want its items to tick anymore when the timer expires.
	_GoSleep();

    RemoveFromView();	// Remove from views.
    MoveToRegion(nullptr, false);

	CSector* pCurSector = GetTopPoint().GetSector();
	if (pNewSector && (pNewSector != pCurSector))
	{
		pNewSector->m_Chars_Disconnect.AddCharDisconnected(this);
	}
	else
	{
		ASSERT(pCurSector);
		pCurSector->m_Chars_Disconnect.AddCharDisconnected(this);
	}
}

void CChar::ClearPlayer()
{
    ADDTOCALLSTACK("CChar::ClearPlayer");
    if ( m_pPlayer == nullptr )
        return;

	CAccount* pAccount = m_pPlayer->GetAccount();
	if (!pAccount)
	{
		g_Log.EventError("Player '%s' (UID 0%x) not attached to account?\n", GetName(), (dword)GetUID());
	}
	else
	{
		if (g_Serv.GetServerMode() != SERVMODE_Exiting)
		{
			g_Log.EventWarn("Player delete '%s' name from account '%s'.\n", GetName(), pAccount->GetName());
		}

		pAccount->DetachChar(this);	// unlink me from my account.
	}
    
    delete m_pPlayer;
    m_pPlayer = nullptr;
}

// Set up the char as a Player.
bool CChar::SetPlayerAccount(CAccount *pAccount)
{
    ADDTOCALLSTACK("CChar::SetPlayerAccount");
    if ( !pAccount )
        return false;

    if ( m_pPlayer )
    {
        if ( m_pPlayer->GetAccount() == pAccount )
            return true;

        DEBUG_ERR(("SetPlayerAccount '%s' already set '%s' != '%s'!\n", GetName(), m_pPlayer->GetAccount()->GetName(), pAccount->GetName()));
        return false;
    }

    if ( m_pNPC )
    {
        // This could happen if the account is set manually through
        // scripts
        ClearNPC();
    }

    m_pPlayer = new CCharPlayer(this, pAccount);
    pAccount->AttachChar(this);
    return true;
}

bool CChar::SetPlayerAccount( lpctstr pszAccName )
{
    ADDTOCALLSTACK("CChar::SetPlayerAccount");
    CAccount * pAccount = g_Accounts.Account_Find( pszAccName );
    if ( pAccount == nullptr )
    {
        DEBUG_ERR(( "Trying to attach Char '%s' to unexistent Account '%s'!\n", GetName(), pszAccName ));
        return false;
    }
    return( SetPlayerAccount( pAccount ));
}

// Set up the char as an NPC
bool CChar::SetNPCBrain( NPCBRAIN_TYPE NPCBrain )
{
    ADDTOCALLSTACK("CChar::SetNPCBrain");
    if ( NPCBrain == NPCBRAIN_NONE )
        return false;

    if ( m_pPlayer != nullptr )
    {
        if ( m_pPlayer->GetAccount() != nullptr )
            DEBUG_ERR(( "SetNPCBrain to Player Account '%s'\n", m_pPlayer->GetAccount()->GetName() ));
        else
            DEBUG_ERR(( "SetNPCBrain to Player Name '%s'\n", GetName()));
        return false;
    }

    if ( m_pNPC == nullptr )
        m_pNPC = new CCharNPC( this, NPCBrain );
    else
        m_pNPC->m_Brain = NPCBrain;		// just replace existing brain
    return true;
}


// Is there something wrong with this char?
// RETURN: invalid code.
int CChar::IsWeird() const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::IsWeird");
	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
		return iResultCode;

	if ( IsDisconnected())
	{
		if ( ! GetTopSector()->IsCharDisconnectedIn( this ))
		{
			iResultCode = 0x1102;
			return iResultCode;
		}
		if ( m_pNPC )
		{
			if ( IsStatFlag( STATF_RIDDEN ))
			{
				if ( Skill_GetActive() != NPCACT_RIDDEN )
				{
					iResultCode = 0x1103;
					return iResultCode;
				}

				// Make sure we are still linked back to the world.
				if ( Horse_GetMountItem() == nullptr )
				{
					iResultCode = 0x1104;
					return iResultCode;
				}
			}
			else
			{
				if ( ! IsStatFlag( STATF_DEAD ))
				{
					iResultCode = 0x1106;
					return iResultCode;
				}
			}
		}
	}

	if ( ! m_pPlayer && ! m_pNPC )
	{
		iResultCode = 0x1107;
		return iResultCode;
	}

	if ( ! GetTopPoint().IsValidPoint())
	{
		iResultCode = 0x1108;
		return iResultCode;
	}

	return 0;
}

// Get the Z we should be at
char CChar::GetFixZ( const CPointMap& pt, dword dwBlockFlags)
{
	const dword dwCanFlags = GetCanFlags();
	const dword dwCanMoveFlags = GetCanMoveFlags(dwCanFlags);

	if ( !dwBlockFlags )
		dwBlockFlags = dwCanMoveFlags;
	
    if (dwCanMoveFlags == 0xFFFFFFFF)
        return pt.m_z;
	if (dwCanMoveFlags & CAN_C_WALK )
		dwBlockFlags |= CAN_I_CLIMB; // If we can walk than we can climb. Ignore CAN_C_FLY at all here

	const short iZClimbed = pt.m_z + m_zClimbHeight;
    const height_t uiHeightMount = GetHeightMount( false );

	const int iBlockMaxHeight = std::max(int(iZClimbed + uiHeightMount), int(INT8_MAX));
	const height_t uiClimbHeight = height_t(std::max(short(iZClimbed + 2), short(UINT8_MAX)));

	CServerMapBlockState block( dwBlockFlags, pt.m_z, iBlockMaxHeight, uiClimbHeight, uiHeightMount );
	CWorldMap::GetFixPoint( pt, block );

	dwBlockFlags = block.m_Bottom.m_dwBlockFlags;
	if ( block.m_Top.m_dwBlockFlags )
	{
		dwBlockFlags |= CAN_I_ROOF;	// we are covered by something.
		if ( block.m_Top.m_z < (iZClimbed + ((block.m_Top.m_dwTile > TERRAIN_QTY) ? uiHeightMount : uiHeightMount/2 )) )
			dwBlockFlags |= CAN_I_BLOCK; // we can't fit under this!
	}
	if ( dwBlockFlags != 0x0 )
	{
		if ( (dwBlockFlags & CAN_I_DOOR) && Can(CAN_C_GHOST, dwCanFlags) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( (dwBlockFlags & CAN_I_WATER) && Can(CAN_C_SWIM, dwCanFlags) )
			dwBlockFlags &= ~CAN_I_BLOCK;

		if ( !Can(CAN_C_FLY, dwCanFlags) )
		{
			if ( ! ( dwBlockFlags & CAN_I_CLIMB ) ) // we can climb anywhere
			{
				if ( block.m_Bottom.m_dwTile > TERRAIN_QTY )
				{
					if ( block.m_Bottom.m_z > uiClimbHeight) // Too high to climb.
						return pt.m_z;
				}
				else
				{
					if (block.m_Bottom.m_z > (iZClimbed + uiHeightMount + 3))
						return pt.m_z;
				}
			}
		}
		if ( (dwBlockFlags & CAN_I_BLOCK) && !Can(CAN_C_PASSWALLS, dwCanFlags) )
			return pt.m_z;

		if ( block.m_Bottom.m_z >= UO_SIZE_Z )
			return pt.m_z;
	}

	if ( (uiHeightMount + pt.m_z >= block.m_Top.m_z) && g_Cfg.m_iMountHeight && !IsPriv(PRIV_ALLMOVE) )
		return pt.m_z;

	return block.m_Bottom.m_z;
}


/*
bool CChar::_IsStatFlag(uint64 uiStatFlag) const noexcept
{
	return (_uiStatFlag & uiStatFlag);
}
*/
bool CChar::IsStatFlag( uint64 uiStatFlag) const noexcept
{
//	THREAD_SHARED_LOCK_SET;
	return (_uiStatFlag & uiStatFlag);
}

/*
void CChar::_StatFlag_Set( uint64 uiStatFlag) noexcept
{
    _uiStatFlag |= uiStatFlag;
}
*/
void CChar::StatFlag_Set(uint64 uiStatFlag) noexcept
{
//	THREAD_UNIQUE_LOCK_SET;
	_uiStatFlag |= uiStatFlag;
}

/*
void CChar::_StatFlag_Clear(uint64 uiStatFlag) noexcept
{
    _uiStatFlag &= ~uiStatFlag;
}
*/
void CChar::StatFlag_Clear(uint64 uiStatFlag) noexcept
{
//	THREAD_UNIQUE_LOCK_SET;
	_uiStatFlag &= ~uiStatFlag;
}

/*
void CChar::_StatFlag_Mod(uint64 uiStatFlag, bool fMod) noexcept
{
	if ( fMod )
        _uiStatFlag |= uiStatFlag;
	else
        _uiStatFlag &= ~uiStatFlag;
}
*/
void CChar::StatFlag_Mod(uint64 uiStatFlag, bool fMod) noexcept
{
//	THREAD_UNIQUE_LOCK_SET;
//	_StatFlag_Mod(uiStatFlag, fMod);
	if (fMod)
		_uiStatFlag |= uiStatFlag;
	else
		_uiStatFlag &= ~uiStatFlag;
}

bool CChar::IsPriv( word flag ) const
{	
	// PRIV_GM flags
	if ( m_pPlayer == nullptr )
		return false;	// NPC's have no privs.
	return m_pPlayer->GetAccount()->IsPriv( flag );
}

PLEVEL_TYPE CChar::GetPrivLevel() const
{
	// The higher the better. // PLEVEL_Counsel
	if ( ! m_pPlayer )
		return PLEVEL_Player;
	return m_pPlayer->GetAccount()->GetPrivLevel();
}

CCharBase * CChar::Char_GetDef() const
{
	return static_cast <CCharBase*>( Base_GetDef() );
}

CRegionWorld * CChar::GetRegion() const
{
	return m_pArea; // What region are we in now. (for guarded message)
}

CRegion * CChar::GetRoom() const
{
	return m_pRoom; // What room are we in now.
}

int CChar::GetVisualRange() const
{
	return (int)m_iVisualRange;
}

void CChar::SetVisualRange(byte newSight)
{
	CClient* pClient;
	{
		THREAD_UNIQUE_LOCK_SET;
		// max value is 18 on classic clients prior 7.0.55.27 version and 24 on enhanced clients and latest classic clients
		m_iVisualRange = minimum(newSight, UO_MAP_VIEW_SIZE_MAX);
		pClient = GetClientActive();
	}
	if (pClient)
    {
        pClient->addVisualRange(m_iVisualRange);
        pClient->addReSync();
    }
}

// Clean up weird flags.
// fix Weirdness.
// NOTE:
//  Deleting a player char is VERY BAD ! Be careful !
//
// RETURN: false = i can't fix this.
int CChar::FixWeirdness()
{
    ADDTOCALLSTACK("CChar::FixWeirdness");
	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
		// Not recoverable - must try to delete the object.
		return iResultCode;

	// NOTE: Stats and skills may go negative temporarily.

    const CCharBase * pCharDef = Char_GetDef();

	// Make sure my flags are good.

	if ( IsStatFlag( STATF_HASSHIELD ))
	{
        const CItem * pShield = LayerFind( LAYER_HAND2 );
		if ( pShield == nullptr )
			StatFlag_Clear( STATF_HASSHIELD );
	}
	if ( IsStatFlag( STATF_ONHORSE ))
	{
        const CItem * pHorse = LayerFind( LAYER_HORSE );
		if ( pHorse == nullptr )
			StatFlag_Clear( STATF_ONHORSE );
	}
	if ( IsStatFlag( STATF_SPAWNED ))
	{
		if (!GetSpawn())
			StatFlag_Clear( STATF_SPAWNED );
	}
	if ( IsStatFlag( STATF_PET ))
	{
        const CItemMemory *pMemory = Memory_FindTypes( MEMORY_IPET );
		if ( pMemory == nullptr )
			StatFlag_Clear( STATF_PET );
	}
	if ( IsStatFlag( STATF_RIDDEN ))
	{
		// Move the ridden creature to the same location as it's rider.
		if ( m_pPlayer || ! IsDisconnected())
			StatFlag_Clear( STATF_RIDDEN );
		else
		{
			if ( Skill_GetActive() != NPCACT_RIDDEN )
			{
				iResultCode = 0x1203;
				return iResultCode;
			}
			const CItem * pFigurine = Horse_GetValidMountItem();
			if ( pFigurine == nullptr )
			{
				iResultCode = 0x1204;
				return iResultCode;
			}
			const CPointMap& pt = pFigurine->GetTopLevelObj()->GetTopPoint();
			if ( pt != GetTopPoint())
			{
				MoveToChar( pt, true, true );
				SetDisconnected();
			}
		}
	}
	if ( IsStatFlag( STATF_CRIMINAL ))
	{
        const CItem * pMemory = LayerFind( LAYER_FLAG_Criminal );
		if ( pMemory == nullptr )
			StatFlag_Clear( STATF_CRIMINAL );
	}

	if ( ! IsIndividualName() && pCharDef->GetTypeName()[0] == '#' )
		SetName( pCharDef->GetTypeName());

	if ( m_pPlayer )	// Player char.
	{
		Memory_ClearTypes( MEMORY_IPET );
		StatFlag_Clear( STATF_RIDDEN );

		if ( m_pPlayer->GetSkillClass() == nullptr )	// this should never happen.
		{
			m_pPlayer->SetSkillClass( this, CResourceID( RES_SKILLCLASS ));
			ASSERT(m_pPlayer->GetSkillClass());
		}

		// Make sure players don't get ridiculous stats.
		//		m_iOverSkillMultiply disables this check if set to < 1
		if (( GetPrivLevel() <= PLEVEL_Player ) && ( g_Cfg.m_iOverSkillMultiply > 0 ))
		{
			for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
			{
				const ushort uiSkillMax = Skill_GetMax((SKILL_TYPE)i);
				const ushort uiSkillVal = Skill_GetBase((SKILL_TYPE)i);
				if ( uiSkillVal > uiSkillMax * g_Cfg.m_iOverSkillMultiply )
					Skill_SetBase((SKILL_TYPE)i, uiSkillMax);
			}

			// ??? What if magically enhanced !!!
			if ( IsPlayableCharacter() && ( GetPrivLevel() < PLEVEL_Counsel ) && !IsStatFlag( STATF_POLYMORPH ))
			{
				for ( int j = STAT_STR; j < STAT_BASE_QTY; ++j )
				{
					const ushort uiStatMax = Stat_GetLimit((STAT_TYPE)j);
					if ( Stat_GetAdjusted((STAT_TYPE)j) > (uiStatMax * g_Cfg.m_iOverSkillMultiply) )
						Stat_SetBase((STAT_TYPE)j, uiStatMax);
				}
			}
		}
	}
	else
	{
		if ( ! m_pNPC )
		{
			iResultCode = 0x1205;
			return iResultCode;
		}

		// An NPC. Don't keep track of unused skills.
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
		{
			if ( m_Skill[i] > 0 && m_Skill[i] < g_Cfg.m_iSaveNPCSkills )
				Skill_SetBase((SKILL_TYPE)i, 0);
		}
	}

	if ( _GetTimerSAdjusted() > 60*60 )
		_SetTimeout(1);	// unreasonably long for a char?

	return IsWeird();
}

// Creating a new char. (Not loading from save file) Make sure things are set to reasonable values.
void CChar::CreateNewCharCheck()
{
	ADDTOCALLSTACK("CChar::CreateNewCharCheck");
	_iPrev_id = GetID();
	_wPrev_Hue = GetHue();

	Stat_SetVal(STAT_STR, Stat_GetMaxAdjusted(STAT_STR));
	Stat_SetVal(STAT_DEX, Stat_GetMaxAdjusted(STAT_DEX));
	Stat_SetVal(STAT_INT, Stat_GetMaxAdjusted(STAT_INT));

	if ( !m_pPlayer )	// need a starting brain tick.
	{
		//	auto-set EXP/LEVEL level
		if ( g_Cfg.m_bExperienceSystem && g_Cfg.m_iExperienceMode&EXP_MODE_AUTOSET_EXP )
		{
			if ( !m_exp )
			{
				CCharBase *pCharDef = Char_GetDef();

				int mult = (Stat_GetMaxAdjusted(STAT_STR) + (Stat_GetMaxAdjusted(STAT_DEX) / 2) + Stat_GetMaxAdjusted(STAT_INT)) / 3;
				ushort iSkillArchery = Skill_GetBase(SKILL_ARCHERY), iSkillThrowing = Skill_GetBase(SKILL_THROWING), iSkillSwordsmanship = Skill_GetBase(SKILL_SWORDSMANSHIP);
				ushort iSkillMacefighting = Skill_GetBase(SKILL_MACEFIGHTING), iSkillFencing = Skill_GetBase(SKILL_FENCING), iSkillWrestling = Skill_GetBase(SKILL_WRESTLING);
				m_exp = maximum(
						iSkillArchery,
						maximum(iSkillThrowing,
						maximum(iSkillSwordsmanship,
						maximum(iSkillMacefighting,
						maximum(iSkillFencing,
						iSkillWrestling))))
					) +
					(Skill_GetBase(SKILL_TACTICS)     / 4) +
					(Skill_GetBase(SKILL_PARRYING)    / 4) +
					(Skill_GetBase(SKILL_MAGERY)      / 3) +
					(Skill_GetBase(SKILL_PROVOCATION) / 4) +
					(Skill_GetBase(SKILL_PEACEMAKING) / 4) +
					(Skill_GetBase(SKILL_TAMING)      / 4) +
					(pCharDef->m_defense    * 3) +
					(pCharDef->m_attackBase * 6)
					;
				m_exp = (m_exp * mult) / 100;
			}

			if ( !m_level && g_Cfg.m_bLevelSystem && ( m_exp > g_Cfg.m_iLevelNextAt ))
				ChangeExperience();
		}

		_SetTimeout(1);
	}
}

bool CChar::DupeFrom(const CChar * pChar, bool fNewbieItems )
{
	// CChar part
	if ( !pChar )
		return false;

	m_pArea = pChar->m_pArea;
	m_pRoom = pChar->m_pRoom;
    _uiStatFlag = pChar->_uiStatFlag;

	if ( g_World.m_fSaveParity )
		StatFlag_Set(STATF_SAVEPARITY);	// It will get saved next time.

	m_dirFace = pChar->m_dirFace;
	m_fonttype = pChar->m_fonttype;
	m_SpeechHueOverride = pChar->m_SpeechHueOverride;
	m_EmoteHueOverride = pChar->m_EmoteHueOverride;

	m_StepStealth = pChar->m_StepStealth;
	m_iVisualRange = pChar->m_iVisualRange;
	m_virtualGold = pChar->m_virtualGold;

	m_exp = pChar->m_exp;
	m_level = pChar->m_level;
	m_defense = pChar->m_defense;
    m_height = pChar->m_height;
    m_ModMaxWeight = pChar->m_ModMaxWeight;
    _uiRange = pChar->_uiRange;

	m_atUnk.m_dwArg1 = pChar->m_atUnk.m_dwArg1;
	m_atUnk.m_dwArg2 = pChar->m_atUnk.m_dwArg2;
	m_atUnk.m_dwArg3 = pChar->m_atUnk.m_dwArg3;

	_iTimeNextRegen = pChar->_iTimeNextRegen;
	_iTimeCreate = pChar->_iTimeCreate;

	_iTimeLastHitsUpdate = pChar->_iTimeLastHitsUpdate;

	_wPrev_Hue = pChar->_wPrev_Hue;
	SetHue(pChar->GetHue());
	_iPrev_id = pChar->_iPrev_id;
	SetID( pChar->GetID() );
	m_CanMask = pChar->m_CanMask;
	_wBloodHue = pChar->_wBloodHue;
	//m_totalweight = 0;

	Skill_Cleanup();

	for ( uint i = 0; i < STAT_QTY; ++i )
	{
        m_Stat[i] = pChar->m_Stat[i];
	}

	for ( uint i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		m_Skill[i] = pChar->m_Skill[i];
	}

	m_fClimbUpdated = pChar->m_fClimbUpdated;
	/*if ( m_pNPC )
	{
		if (pChar->m_pNPC)
			m_pNPC->m_Speech.Copy(&(pChar->m_pNPC->m_Speech));
		else
			m_pNPC->m_Speech.Copy(&(pChar->m_pPlayer->m_Speech));
	}else{
		if (pChar->m_pNPC)
			m_pPlayer->m_Speech.Copy(&(pChar->m_pNPC->m_Speech));
		else
			m_pPlayer->m_Speech.Copy(&(pChar->m_pPlayer->m_Speech));
	}*/
	if (m_pNPC && pChar->m_pNPC)
	{
		m_pNPC->m_Brain = pChar->m_pNPC->m_Brain;
		//m_pNPC->m_bonded = pChar->m_pNPC->m_bonded;
	}

	FixWeirdness();

	SetName( pChar->GetName());	// SetName after FixWeirdness, otherwise it can be replaced again.
	// We copy tags,etc first and place it because of NPC_LoadScript and @Create trigger, so it have information before calling it
	m_TagDefs.Copy( &( pChar->m_TagDefs ) );
	m_BaseDefs.Copy( &( pChar->m_BaseDefs ) );
	//m_OEvents.Copy(&(pChar->m_OEvents));
	m_OEvents = pChar->m_OEvents;
	//NPC_LoadScript( false );	//Calling it now so everything above can be accessed and overrided in the @Create
	//Not calling NPC_LoadScript() because, in some part, it's breaking the name and looking for template names.
	// end of CChar

    CEntity::Copy(pChar);
	CEntityProps::Copy(pChar);

	const CUID& myUID(GetUID());
	g_World.m_uidLastNewChar = myUID;	// for script access.

	// Begin copying items.
	for ( int i = 0 ; i < LAYER_QTY; ++i)
	{
        LAYER_TYPE layer = (LAYER_TYPE)i;
		CItem * myLayer = LayerFind(layer);
		if ( myLayer )
			myLayer->Delete();

        CItem * fromLayer = pChar->LayerFind( layer );
		if ( !fromLayer )
			continue;

		CItem * pItem = CItem::CreateDupeItem(fromLayer, this, true);
		pItem->LoadSetContainer(myUID, layer);
		if ( fNewbieItems )
		{
			pItem->SetAttr(ATTR_NEWBIE);
			if (pItem->IsType(IT_CONTAINER) )
			{
				CItemContainer* pContainer = static_cast<CItemContainer*>(pItem);
				for (CSObjContRec* pObjRec : *pContainer)
				{
					CItem* pItemCont = static_cast<CItem*>(pObjRec);
					pItemCont->SetAttr(ATTR_NEWBIE);

					const CChar *pTest = CUID::CharFindFromUID(pItemCont->m_itNormal.m_more1);
					if (pTest && pTest == pChar)
						pItemCont->m_itNormal.m_more1 = myUID;

					const CChar *pTest2 = CUID::CharFindFromUID(pItemCont->m_itNormal.m_more2);
					if ( pTest2 && pTest2 == pChar )
						pItemCont->m_itNormal.m_more2 = myUID;

					const CChar *pTest3 = CUID::CharFindFromUID(pItemCont->m_uidLink);
					if ( pTest3 && pTest3 == pChar )
						pItemCont->m_uidLink = myUID;
				}
			}
		}
		const CChar * pTest = CUID::CharFindFromUID(pItem->m_itNormal.m_more1);
		if ( pTest && pTest == pChar)
			pItem->m_itNormal.m_more1 = myUID;

		const CChar * pTest2 = CUID::CharFindFromUID(pItem->m_itNormal.m_more2);
		if (pTest2)
		{
			if (pTest2 == pChar)
				pItem->m_itNormal.m_more2 = myUID;
			else if ( pTest2->NPC_IsOwnedBy(pChar, true) )	// Mount's fix
			{
				if ( fNewbieItems )	// Removing any mount references for the memory, so when character dies or dismounts ... no mount will appear.
					pItem->m_itNormal.m_more2 = 0;
				else	// otherwise we create a full new character.
				{
					pItem->m_itNormal.m_more2 = 0;	// more2 should be cleared before removing the memory or the real npc will be removed too.
					pItem->Delete(true);
					CChar * pChar2 = CreateNPC( pTest2->GetID());
					pChar2->SetTopPoint( pChar->GetTopPoint() );	// Moving the mount again because the dupe will place it in the same place as the 'invisible & disconnected' original (usually far away from where the guy will be, so the duped char can't touch the mount).
					pChar2->DupeFrom(pTest2, fNewbieItems);
					pChar2->NPC_PetSetOwner(this);
					Horse_Mount(pChar2);
				}
			}
		}

		CChar * pTest3 = CUID::CharFindFromUID(pItem->m_uidLink);
		if (pTest3)
		{
			if (pTest3 == pChar)
				pItem->m_uidLink = myUID; //If the character being duped has an item which linked to himself, set the newly duped character link instead.
			else if (IsSetOF(OF_PetSlots) &&  pItem->IsMemoryTypes(MEMORY_IPET) && pTest3 == NPC_PetGetOwner())
			{
				const short iFollowerSlots = (short)GetDefNum("FOLLOWERSLOTS", true, 1);
				//If we have reached the maximum follower slots we remove the ownership of the pet by clearing the memory flag instead of using NPC_PetClearOwners().
				if (!pTest3->FollowersUpdate(this, maximum(0, iFollowerSlots)))
					Memory_ClearTypes(MEMORY_IPET); 
			}
		}
	}
	// End copying items.

	FixWeight();

	if (!pChar->IsSleeping())
	{
		_GoAwake();
	}
	//g_World.m_uidNew stored the last duped item, so we need to set back again the newly duped character.
	g_World.m_uidNew.SetObjUID(GetUID());
	Update();
	return true;
}

// Reading triggers from CHARDEF
bool CChar::ReadScriptReducedTrig(CCharBase * pCharDef, CTRIG_TYPE trig, bool fVendor)
{
	ADDTOCALLSTACK("CChar::ReadScriptReducedTrig");
	if ( !pCharDef || !pCharDef->HasTrigger(trig) )
		return false;

	CResourceLock s;
	if ( !pCharDef->ResourceLock(s) || !OnTriggerFind(s, sm_szTrigName[trig]) )
		return false;

	return ReadScriptReduced(s, fVendor);
}

// If this is a regen they will have a pack already.
// RETURN:
//  true = default return. (mostly ignored).
bool CChar::ReadScriptReduced(CResourceLock &s, bool fVendor)
{
	ADDTOCALLSTACK("CChar::ReadScriptReduced");
	bool fFullInterp = false;
	bool fBlockItemAttr = false; //Set a temporary boolean to block item attributes to set on Character.

	CItem * pItem = nullptr;
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKeyHead("ON", 2) )
			break;

		int iCmd = FindTableSorted(s.GetKey(), CItem::sm_szTemplateTable, CountOf(CItem::sm_szTemplateTable)-1);
		bool fItemCreation = false;
		if ( fVendor )
		{
			if (iCmd != -1)
			{
				switch ( iCmd )
				{
					case ITC_BUY:
					case ITC_SELL:
					{
						fBlockItemAttr = false; //Make sure we reset the value, if the last input is not a ITEM(NEWBIE) or CONTAINER.
						CItemContainer * pCont = GetBank((iCmd == ITC_SELL) ? LAYER_VENDOR_STOCK : LAYER_VENDOR_BUYS );
						if ( pCont )
						{
							pItem = CItem::CreateHeader(s.GetArgRaw(), pCont, false);
							if ( pItem )
								pItem->m_TagDefs.SetNum("NOSAVE", 1);
						}
						pItem = nullptr;
						continue;
					}
					case ITC_ITEM:
					case ITC_CONTAINER:
					case ITC_ITEMNEWBIE:					
						fBlockItemAttr = true; //Set the value to block next Color or Attribute inputs for items.
						pItem = nullptr;
						continue;
					default:
						fBlockItemAttr = false; //Make sure we reset the value, if the last input is not a ITEM(NEWBIE) or CONTAINER.
						pItem = nullptr;
						continue;
				}
			}
		}
		else
		{
			switch ( iCmd )
			{
				case ITC_FULLINTERP:
					{
						lpctstr	pszArgs	= s.GetArgStr();
						GETNONWHITESPACE(pszArgs);
						fFullInterp = ( *pszArgs == '\0' ) ? true : ( s.GetArgVal() != 0);
						continue;
					}
				case ITC_NEWBIESWAP:
					{
						if ( !pItem )
							continue;

						if ( pItem->IsAttr( ATTR_NEWBIE ) )
						{
							if ( Calc_GetRandVal( s.GetArgVal() ) == 0 )
								pItem->ClrAttr(ATTR_NEWBIE);
						}
						else
						{
							if ( Calc_GetRandVal( s.GetArgVal() ) == 0 )
								pItem->SetAttr(ATTR_NEWBIE);
						}
						continue;
					}
				case ITC_ITEM:
				case ITC_CONTAINER:
				case ITC_ITEMNEWBIE:
					{
						fItemCreation = true;

						if ( IsStatFlag( STATF_CONJURED ) && iCmd != ITC_ITEMNEWBIE ) // This check is not needed.
							break; // conjured creates have no loot.

						pItem = CItem::CreateHeader( s.GetArgRaw(), this, iCmd == ITC_ITEMNEWBIE );
						if ( pItem == nullptr )
						{
							m_UIDLastNewItem = GetUID();	// Setting m_UIDLastNewItem to CChar's UID to prevent calling any following functions meant to be called on that item
							continue;
						}
						m_UIDLastNewItem.InitUID();	//Clearing the attr for the next cycle

						pItem->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
						pItem->_iCreatedResScriptLine = s.m_iLineNum;

						if ( iCmd == ITC_ITEMNEWBIE )
							pItem->SetAttr(ATTR_NEWBIE);

						if ( !pItem->IsItemInContainer() && !pItem->IsItemEquipped())
							pItem = nullptr;
						continue;
					}

				case ITC_BREAK:
				case ITC_BUY:
				case ITC_SELL:
					pItem = nullptr;
					continue;
			}

		}

		if ( m_UIDLastNewItem == GetUID() )
			continue;
		if ( fBlockItemAttr ) //Did we force to cancel item attributes?
			continue;
			
		if ( pItem != nullptr )
		{
			if ( fFullInterp )	// Modify the item.
				pItem->r_Verb( s, &g_Serv );
			else
				pItem->r_LoadVal( s );
		}
		else if (!fItemCreation)
		{
			TRIGRET_TYPE tRet = OnTriggerRun( s, TRIGRUN_SINGLE_EXEC, &g_Serv, nullptr, nullptr );
			if ( (tRet == TRIGRET_RET_FALSE) && fFullInterp )
				;
			else if ( tRet != TRIGRET_RET_DEFAULT )
			{
				m_UIDLastNewItem.InitUID();
				return (tRet == TRIGRET_RET_FALSE);
			}
		}
	}

	return true;
}

void CChar::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CChar::OnWeightChange");
	CContainer::OnWeightChange( iChange );
	UpdateStatsFlag();
}

int CChar::GetWeight(word amount) const
{
	UNREFERENCED_PARAMETER(amount);
	return CContainer::GetTotalWeight();
}

bool CChar::SetName( lpctstr pszName )
{
	ADDTOCALLSTACK("CChar::SetName");
	return SetNamePool( pszName );
}

height_t CChar::GetHeightMount( bool fEyeSubstract ) const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::GetHeightMount");
	height_t height = GetHeight();
	if ( IsStatFlag(STATF_ONHORSE|STATF_HOVERING) )
		height += 4;
	if ( fEyeSubstract )
		--height;
	return height; //if mounted +4, if not -1 (let's say it's eyes' height)
}

height_t CChar::GetHeight() const
{
	ADDTOCALLSTACK_INTENSIVE("CChar::GetHeight");
	if ( m_height ) //set by a dynamic variable (On=@Create  Height=10)
		return m_height;

	height_t tmpHeight;

	const CCharBase * pCharDef = Char_GetDef();
	tmpHeight = pCharDef->GetHeight();
	if ( tmpHeight ) //set by a chardef variable ([CHARDEF 10]  Height=10)
		return tmpHeight;

    // This is SLOW (since this method is called very frequently)! Move those defs value to CharDef!
	char * heightDef = Str_GetTemp();
    const uint uiDispID = (uint)pCharDef->GetDispID();

	sprintf(heightDef, "height_0%x", uiDispID);
	tmpHeight = (height_t)(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_0a)
		return tmpHeight;

	sprintf(heightDef, "height_%u", uiDispID);
	tmpHeight = (height_t)(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_10)
		return tmpHeight;

	return PLAYER_HEIGHT; //if everything fails
}

CREID_TYPE CChar::GetID() const
{
    const CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	return pCharDef->GetID();
}

word CChar::GetBaseID() const
{
	// future: strongly typed enums will remove the need for this cast
	return (word)(GetID());
}

CREID_TYPE CChar::GetDispID() const
{
	const CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef);
	return pCharDef->GetDispID();
}

// Just set the base id and not the actual display id.
// NOTE: Never return nullptr
void CChar::SetID( CREID_TYPE id )
{
	ADDTOCALLSTACK("CChar::SetID");

	CCharBase * pCharDef = CCharBase::FindCharBase(id);
	if ( pCharDef == nullptr )
	{
		if ( (id != -1) && (id != CREID_INVALID) )
			DEBUG_ERR(("Create Invalid Char 0%x\n", id));

		id = (CREID_TYPE)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, "DEFAULTCHAR"));
		if ( id < CREID_INVALID )
			id = CREID_MAN;

		pCharDef = CCharBase::FindCharBase(id);
	}

	ASSERT(pCharDef != nullptr);

	CCharBase* pCharOldDef = Char_GetDef();
	if ( pCharDef == pCharOldDef )
		return;

	if (pCharOldDef != nullptr)
	{
		pCharOldDef->DelInstance();
	}
	pCharDef->AddInstance();	// Increase object instance counter (different from the resource reference counter!)
	m_BaseRef.SetRef(pCharDef);	// Among the other things, it increases the new resource reference counter and decreases the old, if any

	if ( _iPrev_id == CREID_INVALID )
		_iPrev_id = GetID();

	if ( !IsMountCapable() )	// new body may not be capable of riding mounts
		Horse_UnMount();

    if (!IsGargoyle())		// new body may not be capable of use gargoyle fly ability
    {
        StatFlag_Clear(STATF_HOVERING);
    }

	if ( !Can(CAN_C_EQUIP) )	// new body may not be capable of equipping items (except maybe on hands)
	{
		UnEquipAllItems(nullptr, Can(CAN_C_USEHANDS));
	}
	else if ( !Can(CAN_C_USEHANDS) )
	{
		CItem *pHand = LayerFind(LAYER_HAND1);
		if ( pHand )
			GetPackSafe()->ContentAdd(pHand);

		pHand = LayerFind(LAYER_HAND2);
		if ( pHand )
			GetPackSafe()->ContentAdd(pHand);
	}
	UpdateMode(nullptr, true);
}


lpctstr CChar::GetName() const
{
	return GetName( true );
}

lpctstr CChar::GetNameWithoutIncognito() const
{
	if ( IsStatFlag( STATF_INCOGNITO ) )
	{
		CItem * pSpell = nullptr;
		pSpell = LayerFind(LAYER_SPELL_Incognito);
		if ( pSpell == nullptr )
			pSpell = LayerFind(LAYER_FLAG_Potion);

		if ( pSpell && pSpell->IsType(IT_SPELL) && (pSpell->m_itSpell.m_spell == SPELL_Incognito))
			return pSpell->GetName();
	}

	return GetName();
}

lpctstr CChar::GetName( bool fAllowAlt ) const
{
	if ( fAllowAlt )
	{
		lpctstr pAltName = GetKeyStr( "NAME.ALT" );
		if ( pAltName && *pAltName )
			return pAltName;
	}
	if ( ! IsIndividualName() )			// allow some creatures to go unnamed.
	{
		CCharBase * pCharDef = Char_GetDef();
		ASSERT(pCharDef);
		return pCharDef->GetTypeName();	// Just use it's type name instead.
	}
	return CObjBase::GetName();
}

// Create a brand new Player char. Called directly from the packet.
void CChar::InitPlayer( CClient *pClient, const char *pszCharname, bool fFemale, RACE_TYPE rtRace, ushort wStr, ushort wDex, ushort wInt,
	PROFESSION_TYPE prProf, SKILL_TYPE skSkill1, ushort uiSkillVal1, SKILL_TYPE skSkill2, ushort uiSkillVal2, SKILL_TYPE skSkill3, ushort uiSkillVal3, SKILL_TYPE skSkill4, ushort uiSkillVal4,
	HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, ITEMID_TYPE idFace, int iStartLoc )
{
	ADDTOCALLSTACK("CChar::InitPlayer");
	ASSERT(pClient);

	CAccount *pAccount = pClient->GetAccount();
	if ( pAccount )
		SetPlayerAccount(pAccount);

	switch ( rtRace )
	{
		default:
            g_Log.EventWarn("Character creation: invalid race. Defaulting to human.\n");
            rtRace = RACETYPE_HUMAN;
			FALLTHROUGH;
		case RACETYPE_HUMAN:
			SetID(fFemale ? CREID_WOMAN : CREID_MAN);
			break;
		case RACETYPE_ELF:
			SetID(fFemale ? CREID_ELFWOMAN : CREID_ELFMAN);
			break;
		case RACETYPE_GARGOYLE:
			SetID(fFemale ? CREID_GARGWOMAN : CREID_GARGMAN);
			break;
	}

	// Set name
	bool fNameIsAccepted = true;
	tchar *zCharName = Str_GetTemp();
    Str_CopyLimitNull(zCharName, pszCharname, MAX_NAME_SIZE);

	if ( !strlen(zCharName) || g_Cfg.IsObscene(zCharName) || Str_CheckName(zCharName) ||!strnicmp(zCharName, "lord ", 5) || !strnicmp(zCharName, "lady ", 5) ||
		!strnicmp(zCharName, "seer ", 5) || !strnicmp(zCharName, "gm ", 3) || !strnicmp(zCharName, "admin ", 6) || !strnicmp(zCharName, "counselor ", 10) )
	{
		fNameIsAccepted = false;
	}

	if ( fNameIsAccepted && IsTrigUsed(TRIGGER_RENAME) )
	{
		CScriptTriggerArgs args;
		args.m_s1 = zCharName;
		args.m_pO1 = this;
		if ( OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
			fNameIsAccepted = false;
	}

	if ( fNameIsAccepted )
		SetName(zCharName);
	else
		SetNamePool(fFemale ? "#NAMES_HUMANFEMALE" : "#NAMES_HUMANMALE");

	if ( g_Cfg.m_StartDefs.IsValidIndex(iStartLoc) )
		m_ptHome = g_Cfg.m_StartDefs[iStartLoc]->m_pt;
	else
		m_ptHome.InitPoint();

	if ( !m_ptHome.IsValidPoint() )
		DEBUG_ERR(("Invalid start location '%d' for character!\n", iStartLoc));

	SetUnkPoint(m_ptHome);	// don't actually put me in the world yet.

	// randomize the skills first.
	for ( uint i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		if ( g_Cfg.m_SkillIndexDefs.IsValidIndex(i) )
			Skill_SetBase((SKILL_TYPE)i, (ushort)Calc_GetRandVal(g_Cfg.m_iMaxBaseSkill));
	}

	if ( wStr > 60 )		wStr = 60;
	if ( wDex > 60 )		wDex = 60;
	if ( wInt > 60 )		wInt = 60;
	if ( uiSkillVal1 > 50 )	uiSkillVal1 = 50;
	if ( uiSkillVal2 > 50 )	uiSkillVal2 = 50;
	if ( uiSkillVal3 > 50 )	uiSkillVal3 = 50;
	if ( uiSkillVal4 > 50 )	uiSkillVal4 = 50;

	if ( skSkill4 != SKILL_NONE )
	{
		if ( (wStr + wDex + wInt) > 90 )
			wStr = wDex = wInt = 30;

		if ( (uiSkillVal1 + uiSkillVal2 + uiSkillVal3 + uiSkillVal4) > 120 )
			uiSkillVal4 = 1;
	}
	else
	{
		if ( (wStr + wDex + wInt) > 80 )
			wStr = wDex = wInt = 26;

		if ( (uiSkillVal1 + uiSkillVal2 + uiSkillVal3) > 100 )
			uiSkillVal3 = 1;
	}

	Stat_SetBase(STAT_STR, wStr);
	Stat_SetBase(STAT_DEX, wDex);
	Stat_SetBase(STAT_INT, wInt);

	if ( IsSkillBase(skSkill1) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill1) )
		Skill_SetBase(skSkill1, uiSkillVal1 * 10);
	if ( IsSkillBase(skSkill2) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill2) )
		Skill_SetBase(skSkill2, uiSkillVal2 * 10);
	if ( IsSkillBase(skSkill3) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill3) )
		Skill_SetBase(skSkill3, uiSkillVal3 * 10);
	if ( skSkill4 != SKILL_NONE )
	{
		if ( IsSkillBase(skSkill4) && g_Cfg.m_SkillIndexDefs.IsValidIndex(skSkill4) )
			Skill_SetBase(skSkill4, uiSkillVal4 * 10);
	}

    m_pPlayer->m_SpeechHue	= HUE_TEXT_DEF;		// Set default client-sent speech color
    m_pPlayer->m_EmoteHue	= HUE_EMOTE_DEF;	// Set default emote color
	m_fonttype				= FONT_NORMAL;		// Set speech font type
	m_SpeechHueOverride		= 0;				// Set no server-side speech color override
	m_EmoteHueOverride		= 0;				// Set no server-side emote color override
	m_sTitle.Clear();							// Set title

	GetBank(LAYER_BANKBOX);			// Create bankbox
	GetPackSafe();					// Create backpack

	// Check skin hue
	switch ( rtRace )
	{
		default:
		case RACETYPE_HUMAN:
			if ( wSkinHue < HUE_SKIN_LOW )
				wSkinHue = (HUE_TYPE)(HUE_SKIN_LOW);
			if ( wSkinHue > HUE_SKIN_HIGH )
				wSkinHue = (HUE_TYPE)(HUE_SKIN_HIGH);
			break;

		case RACETYPE_ELF:
		{
			static constexpr int sm_ElfSkinHues[] =
			{
				0x0BF, 0x24D, 0x24E, 0x24F, 0x353, 0x361, 0x367, 0x374, 0x375, 0x376, 0x381, 0x382, 0x383, 0x384, 0x385, 0x389,
				0x3DE, 0x3E5, 0x3E6, 0x3E8, 0x3E9, 0x430, 0x4A7, 0x4DE, 0x51D, 0x53F, 0x579, 0x76B, 0x76C, 0x76D, 0x835, 0x903
			};
			constexpr uint iMax = CountOf(sm_ElfSkinHues);
			bool isValid = 0;
			for ( uint i = 0; i < iMax; ++i )
			{
				if ( sm_ElfSkinHues[i] == wSkinHue )
				{
					isValid = 1;
					break;
				}
			}
			if ( !isValid )
				wSkinHue = (HUE_TYPE)(sm_ElfSkinHues[0]);
		}
		break;

		case RACETYPE_GARGOYLE:
			if ( wSkinHue < HUE_GARGSKIN_LOW )
				wSkinHue = (HUE_TYPE)(HUE_GARGSKIN_LOW);
			if ( wSkinHue > HUE_GARGSKIN_HIGH )
				wSkinHue = (HUE_TYPE)(HUE_GARGSKIN_HIGH);
			break;
	}
	SetHue((wSkinHue|HUE_UNDERWEAR));

	// Create hair
	switch ( rtRace )
	{
		default:
		case RACETYPE_HUMAN:
			if ( !(((idHair >= ITEMID_HAIR_SHORT) && (idHair <= ITEMID_HAIR_PONYTAIL)) || ((idHair >= ITEMID_HAIR_MOHAWK) && (idHair <= ITEMID_HAIR_TOPKNOT))) )
				idHair = ITEMID_NOTHING;	// human can use only a restricted subset of hairs
			if ( (fFemale && idHair == ITEMID_HAIR_RECEDING) || (!fFemale && idHair == ITEMID_HAIR_BUNS) )
				idHair = ITEMID_NOTHING;
			break;

		case RACETYPE_ELF:
			if ( !(((idHair >= ITEMID_HAIR_ML_ELF) && (idHair <= ITEMID_HAIR_ML_MULLET)) || ((idHair >= ITEMID_HAIR_ML_FLOWER) && (idHair <= ITEMID_HAIR_ML_SPYKE))) )
				idHair = ITEMID_NOTHING;	// elf can use only a restricted subset of hairs
			if ( (fFemale && (idHair == ITEMID_HAIR_ML_LONG2 || idHair == ITEMID_HAIR_ML_ELF)) || (!fFemale && (idHair == ITEMID_HAIR_ML_FLOWER || idHair == ITEMID_HAIR_ML_LONG4)) )
				idHair = ITEMID_NOTHING;
			break;

		case RACETYPE_GARGOYLE:
			if ( fFemale )
			{
				if ( !((idHair == ITEMID_GARG_HORN_FEMALE_1) || (idHair == ITEMID_GARG_HORN_FEMALE_2) || ((idHair >= ITEMID_GARG_HORN_FEMALE_3) && (idHair <= ITEMID_GARG_HORN_FEMALE_5)) || (idHair == ITEMID_GARG_HORN_FEMALE_6) || (idHair == ITEMID_GARG_HORN_FEMALE_7) || (idHair == ITEMID_GARG_HORN_FEMALE_8)) )
					idHair = ITEMID_NOTHING;
			}
			else
			{
				if ( !((idHair >= ITEMID_GARG_HORN_1) && (idHair <= ITEMID_GARG_HORN_8)) )
					idHair = ITEMID_NOTHING;
			}
			break;
	}

	if ( idHair )
	{
		CItem *pHair = CItem::CreateScript(idHair, this);
		ASSERT(pHair);
		if ( !pHair->IsType(IT_HAIR) )
			pHair->Delete();
		else
		{
			switch ( rtRace )
			{
				default:
				case RACETYPE_HUMAN:
					if ( wHairHue < HUE_HAIR_LOW )
						wHairHue = (HUE_TYPE)(HUE_HAIR_LOW);
					if ( wHairHue > HUE_HAIR_HIGH )
						wHairHue = (HUE_TYPE)(HUE_HAIR_HIGH);
					break;

				case RACETYPE_ELF:
				{
					static constexpr int sm_ElfHairHues[] =
					{
						0x034, 0x035, 0x036, 0x037, 0x038, 0x039, 0x058, 0x08E, 0x08F, 0x090, 0x091, 0x092,
						0x101, 0x159, 0x15A, 0x15B, 0x15C, 0x15D, 0x15E, 0x128, 0x12F, 0x1BD, 0x1E4, 0x1F3,
						0x207, 0x211, 0x239, 0x251, 0x26C, 0x2C3, 0x2C9, 0x31D, 0x31E, 0x31F, 0x320, 0x321,
						0x322, 0x323, 0x324, 0x325, 0x326, 0x369, 0x386, 0x387, 0x388, 0x389, 0x38A, 0x59D,
						0x6B8, 0x725, 0x853
					};
					constexpr uint iMax = CountOf(sm_ElfHairHues);
					bool isValid = 0;
					for ( uint i = 0; i < iMax; ++i )
					{
						if ( sm_ElfHairHues[i] == wHairHue )
						{
							isValid = 1;
							break;
						}
					}
					if ( !isValid )
						wHairHue = (HUE_TYPE)(sm_ElfHairHues[0]);
				}
				break;

				case RACETYPE_GARGOYLE:
				{
					static constexpr int sm_GargoyleHornHues[] =
					{
						0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B,
						0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D
					};
					constexpr uint iMax = CountOf(sm_GargoyleHornHues);
					bool isValid = 0;
					for ( uint i = 0; i < iMax; ++i )
					{
						if ( sm_GargoyleHornHues[i] == wHairHue )
						{
							isValid = 1;
							break;
						}
					}
					if ( !isValid )
						wHairHue = (HUE_TYPE)(sm_GargoyleHornHues[0]);
				}
				break;
			}
			pHair->SetHue(wHairHue);
			pHair->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			LayerAdd(pHair);	// add content
		}
	}

	// Create beard
	if (fFemale)
		idBeard = ITEMID_NOTHING;
	else
	{
		switch (rtRace)
		{
			default:
			case RACETYPE_HUMAN:
				if (!(((idBeard >= ITEMID_BEARD_LONG) && (idBeard <= ITEMID_BEARD_MOUSTACHE)) || ((idBeard >= ITEMID_BEARD_SH_M) && (idBeard <= ITEMID_BEARD_GO_M))))
					idBeard = ITEMID_NOTHING;
				break;
			case RACETYPE_ELF:
				idBeard = ITEMID_NOTHING;
				break;
			case RACETYPE_GARGOYLE:
				if (!((idBeard >= ITEMID_GARG_HORN_FACIAL_1) && (idBeard <= ITEMID_GARG_HORN_FACIAL_4)))
					idBeard = ITEMID_NOTHING;
				break;
		}
	}


	if ( idBeard )
	{
		CItem *pBeard = CItem::CreateScript(idBeard, this);
		ASSERT(pBeard);
		if ( !pBeard->IsType(IT_BEARD) )
			pBeard->Delete();
		else
		{
			switch ( rtRace )
			{
				case RACETYPE_HUMAN:
					if ( wBeardHue < HUE_HAIR_LOW )
						wBeardHue = (HUE_TYPE)(HUE_HAIR_LOW);
					if ( wBeardHue > HUE_HAIR_HIGH )
						wBeardHue = (HUE_TYPE)(HUE_HAIR_HIGH);
					break;

				case RACETYPE_GARGOYLE:
				{
					static constexpr int sm_GargoyleBeardHues[] =
					{
						0x709, 0x70B, 0x70D, 0x70F, 0x711, 0x763, 0x765, 0x768, 0x76B,
						0x6F3, 0x6F1, 0x6EF, 0x6E4, 0x6E2, 0x6E0, 0x709, 0x70B, 0x70D
					};
					int iMax = CountOf(sm_GargoyleBeardHues);
					bool isValid = 0;
					for ( int i = 0; i < iMax; ++i )
					{
						if ( sm_GargoyleBeardHues[i] == wHairHue )
						{
							isValid = 1;
							break;
						}
					}
					if ( !isValid )
						wHairHue = (HUE_TYPE)(sm_GargoyleBeardHues[0]);
				}
				break;

				default:
					break;
			}
			pBeard->SetHue(wBeardHue);
			pBeard->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
			LayerAdd(pBeard);	// add content
		}
	}

	// Create face (enhanced clients only)
	if ( idFace != ITEMID_NOTHING )
	{
		switch ( rtRace )
		{
			case RACETYPE_GARGOYLE:
				if ( !((idFace >= ITEMID_FACE_1_GARG) && (idFace <= ITEMID_FACE_6_GARG)) )
					idFace = ITEMID_NOTHING;
				break;

			default:
				if ( !(((idFace >= ITEMID_FACE_1) && (idFace <= ITEMID_FACE_10)) || ((idFace >= ITEMID_FACE_ANIME) && (idFace <= ITEMID_FACE_VAMPIRE))) )
					idFace = ITEMID_NOTHING;
				break;
		}

		CItem *pFace = CItem::CreateScript(idFace, this);
		ASSERT(pFace);
		pFace->SetHue(wSkinHue);
		pFace->SetAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER);
		LayerAdd(pFace);
	}

	// Get starting items for the profession / skills.
	int iProfession = INT32_MAX;
	bool fCreateSkillItems = true;
	switch ( prProf )
	{
        //default: // not good for custom stuff
        //    g_Log.EventWarn("Character creation: invalid profession. Defaulting to advanced.\n");
		case PROFESSION_ADVANCED:
			iProfession = RES_NEWBIE_PROF_ADVANCED;
			break;
		case PROFESSION_WARRIOR:
			iProfession = RES_NEWBIE_PROF_WARRIOR;
			break;
		case PROFESSION_MAGE:
			iProfession = RES_NEWBIE_PROF_MAGE;
			break;
		case PROFESSION_BLACKSMITH:
			iProfession = RES_NEWBIE_PROF_BLACKSMITH;
			break;
		case PROFESSION_NECROMANCER:
			iProfession = RES_NEWBIE_PROF_NECROMANCER;
			fCreateSkillItems = false;
			break;
		case PROFESSION_PALADIN:
			iProfession = RES_NEWBIE_PROF_PALADIN;
			fCreateSkillItems = false;
			break;
		case PROFESSION_SAMURAI:
			iProfession = RES_NEWBIE_PROF_SAMURAI;
			fCreateSkillItems = false;
			break;
		case PROFESSION_NINJA:
			iProfession = RES_NEWBIE_PROF_NINJA;
			fCreateSkillItems = false;
			break;
	}

	CResourceLock s;
	if ( g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, fFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT, (word)rtRace)) )
		ReadScriptReduced(s);
	else if ( g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, fFemale ? RES_NEWBIE_FEMALE_DEFAULT : RES_NEWBIE_MALE_DEFAULT)) )
		ReadScriptReduced(s);

    if (iProfession != INT32_MAX)
    {
        if ( g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, iProfession, (word)rtRace)) )
            ReadScriptReduced(s);
        else if ( g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, iProfession)) )
            ReadScriptReduced(s);
    }

	if ( fCreateSkillItems )
	{
		for ( int i = 1; i < 5; ++i )
		{
			int iSkill = INT32_MAX;
			switch ( i )
			{
				case 1:
					iSkill = skSkill1;
					break;
				case 2:
					iSkill = skSkill2;
					break;
				case 3:
					iSkill = skSkill3;
					break;
				case 4:
					iSkill = skSkill4;
					break;
			}

			if ( !g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, iSkill, (word)rtRace)) )
			{
				if ( !g_Cfg.ResourceLock(s, CResourceID(RES_NEWBIE, iSkill)) )
					continue;
			}
			ReadScriptReduced(s);
		}
	}

	CItem *pLayer = LayerFind(LAYER_SHIRT);
	if ( pLayer )
	{
		if ( wShirtHue < HUE_BLUE_LOW )
			wShirtHue = HUE_BLUE_LOW;
		if ( wShirtHue > HUE_DYE_HIGH )
			wShirtHue = HUE_DYE_HIGH;
		pLayer->SetHue(wShirtHue);
	}
	pLayer = LayerFind(LAYER_PANTS);
	if ( pLayer )
	{
		if ( wPantsHue < HUE_BLUE_LOW )
			wPantsHue = HUE_BLUE_LOW;
		if ( wPantsHue > HUE_DYE_HIGH )
			wPantsHue = HUE_DYE_HIGH;
		pLayer->SetHue(wPantsHue);
	}
	CreateNewCharCheck();
}

enum CHR_TYPE
{
	CHR_ACCOUNT,
	CHR_ACT,
	CHR_FINDLAYER,
    CHR_HOUSE,
	CHR_MEMORYFIND,
	CHR_MEMORYFINDTYPE,
	CHR_OWNER,
	CHR_REGION,
    CHR_SHIP,
	CHR_WEAPON,
	CHR_QTY
};

lpctstr const CChar::sm_szRefKeys[CHR_QTY+1] =
{
	"ACCOUNT",
	"ACT",
	"FINDLAYER",
    "HOUSE",
	"MEMORYFIND",
	"MEMORYFINDTYPE",
	"OWNER",
	"REGION",
    "SHIP",
	"WEAPON",
	nullptr
};

bool CChar::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CChar::r_GetRef");

    if (CEntity::r_GetRef(ptcKey, pRef))
    {
        return true;
    }

	int i = FindTableHeadSorted( ptcKey, sm_szRefKeys, CountOf(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		ptcKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(ptcKey);
		switch (i)
		{
			case CHR_ACCOUNT:
				if ( ptcKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_pPlayer ? m_pPlayer->GetAccount() : nullptr;
				return true;
			case CHR_ACT:
				if ( ptcKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = m_Act_UID.ObjFind();
				return true;
			case CHR_FINDLAYER:	// Find equipped layers.
				pRef = LayerFind( (LAYER_TYPE) Exp_GetSingle( ptcKey ));
				SKIP_SEPARATORS(ptcKey);
				return true;
            case CHR_HOUSE:
				if (m_pPlayer)
				{
					const int16 iPos = (int16)Exp_GetSingle(ptcKey);
					CMultiStorage* pMultiStorage = m_pPlayer->GetMultiStorage();
					if (pMultiStorage->GetHouseCountReal() <= iPos)
					{
						return false;
					}
					pRef = static_cast<CItemMulti*>(pMultiStorage->GetHouseAt(iPos).ItemFind());
					SKIP_SEPARATORS(ptcKey);
					return true;
				}
				return false;
            case CHR_SHIP:
				if (m_pPlayer)
				{
					const int16 iPos = (int16)Exp_GetSingle(ptcKey);
					CMultiStorage* pMultiStorage = m_pPlayer->GetMultiStorage();
					if (pMultiStorage->GetShipCountReal() <= iPos)
					{
						return false;
					}
					pRef = static_cast<CItemShip*>(pMultiStorage->GetShipAt(iPos).ItemFind());
					SKIP_SEPARATORS(ptcKey);
					return true;
				}
				return false;
			case CHR_MEMORYFINDTYPE:	// FInd a type of memory.
				pRef = Memory_FindTypes((word)(Exp_GetSingle(ptcKey)));
				SKIP_SEPARATORS(ptcKey);
				return true;
			case CHR_MEMORYFIND:	// Find a memory of a UID
				pRef = Memory_FindObj( (CUID) Exp_GetSingle( ptcKey ));
				SKIP_SEPARATORS(ptcKey);
				return true;
			case CHR_OWNER:
				pRef = GetOwner();
				return true;
			case CHR_WEAPON:
				pRef = m_uidWeapon.ObjFind();
				return true;
			case CHR_REGION:
				pRef = m_pArea;
				return true;
		}
	}

	if ( r_GetRefContainer( ptcKey, pRef ))
		return true;

	return ( CObjBase::r_GetRef( ptcKey, pRef ));
}

enum CHC_TYPE
{
	#define ADD(a,b) CHC_##a,
	#include "../../tables/CChar_props.tbl"
	#undef ADD
	CHC_QTY
};

lpctstr const CChar::sm_szLoadKeys[CHC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CChar_props.tbl"
	#undef ADD
	nullptr
};

bool CChar::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CChar::r_WriteVal");
    EXC_TRY("WriteVal");

	if ( IsClientActive() && GetClientActive()->r_WriteVal(ptcKey, sVal, pSrc, true, true) ) // Call CClient::r_WriteVal
		return true;

    if (!fNoCallChildren)
    {
        // Checking Props CComponents first (first check CChar props, if not found then check CCharBase)
        EXC_SET_BLOCK("EntityProp");
        if (CEntityProps::r_WritePropVal(ptcKey, sVal, this, Base_GetDef()))
        {
            return true;
        }

        // Now check regular CComponents
        EXC_SET_BLOCK("Entity");
        if (CEntity::r_WriteVal(ptcKey, sVal, pSrc))
        {
            return true;
        }
    }

    EXC_SET_BLOCK("Keyword");
	const CHC_TYPE iKeyNum = (CHC_TYPE) FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	if ( iKeyNum < 0 )
	{
do_default:
		if ( m_pPlayer )
		{
			if ( m_pPlayer->r_WriteVal( this, ptcKey, sVal ))
				return true;
		}
		if ( m_pNPC )
		{
			if ( m_pNPC->r_WriteVal( this, ptcKey, sVal ))
				return true;
		}

		if ( r_WriteValContainer(ptcKey, sVal, pSrc))
			return true;

		// special write values
		const SKILL_TYPE iSkill = (SKILL_TYPE)g_Cfg.FindSkillKey( ptcKey );
		if ( IsSkillBase(iSkill) )
		{
			// Check some skill name.
			const ushort uiVal = Skill_GetBase( iSkill );
			sVal.Format( "%u.%u", uiVal/10, uiVal%10 );
			return true;
		}

		return (fNoCallParent ? false : CObjBase::r_WriteVal( ptcKey, sVal, pSrc, false ));
	}

    const CCharBase * pCharDef = Char_GetDef();
    ASSERT(pCharDef);
    CChar * pCharSrc = pSrc->GetChar();
	switch ( iKeyNum )
	{
		//return as decimal number or 0 if not set
		case CHC_CURFOLLOWER:
			sVal.FormatLLVal(GetDefNum(ptcKey,false));
			break;
		//On these ones, check BaseDef if not found on dynamic
		case CHC_MAXFOLLOWER:
		case CHC_SPELLTIMEOUT:
		case CHC_TITHING:
			sVal.FormatLLVal(GetDefNum(ptcKey, true));
			break;
		case CHC_ATTACKER:
			{
				if ( strlen( ptcKey ) == 8 )
				{
					sVal.FormatSTVal(m_lastAttackers.size());
					return true;
				}

				sVal.FormatVal(0);
				ptcKey += 8;

				if ( *ptcKey == '.' )
				{
					++ptcKey;
					if ( !strnicmp(ptcKey, "ID", 2 ) )
					{
						ptcKey += 3;	// ID + whitspace
						const CChar * pChar = CUID::CharFindFromUID(Exp_GetSingle(ptcKey));
						sVal.FormatVal(Attacker_GetID(pChar));
						return true;
					}
					else if ( !strnicmp(ptcKey, "TARGET", 6 ) )
					{
						ptcKey += 6;
						if (m_Act_UID.IsValidUID())
							sVal.FormatHex((dword)(m_Fight_Targ_UID));
						else
							sVal.FormatVal(-1);
						return true;
					}
					if ( !m_lastAttackers.empty() )
					{
						int attackerIndex = (int)m_lastAttackers.size();
						if( !strnicmp(ptcKey, "MAX", 3) )
						{
							ptcKey += 3;
							int64 iMaxDmg = -1, iCurDmg = 0;

							for ( size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker )
							{
								iCurDmg = m_lastAttackers[iAttacker].amountDone;
								if ( iCurDmg > iMaxDmg )
								{
									iMaxDmg = iCurDmg;
									attackerIndex = (int)iAttacker;
								}
							}
						}
						else if( !strnicmp(ptcKey, "LAST", 4) )
						{
							ptcKey += 4;
							int64 iLastTime = INT64_MAX, iCurTime = 0;

							for ( size_t iAttacker = 0; iAttacker < m_lastAttackers.size(); ++iAttacker )
							{
								iCurTime = m_lastAttackers[iAttacker].elapsed;
								if ( iCurTime <= iLastTime )
								{
									iLastTime = iCurTime;
									attackerIndex = (int)iAttacker;
								}
							}
						}
						else
						{
							attackerIndex = Exp_GetVal(ptcKey);
						}

						SKIP_SEPARATORS(ptcKey);
						if ( attackerIndex < (int)m_lastAttackers.size() )
						{
							const LastAttackers & refAttacker = m_lastAttackers[(size_t)attackerIndex];

							if( !strnicmp(ptcKey, "DAM", 3) )
							{
								sVal.FormatLLVal(refAttacker.amountDone);
								return true;
							}
							else if( !strnicmp(ptcKey, "ELAPSED", 7) )
							{
								sVal.FormatLLVal(refAttacker.elapsed);
								return true;
							}
							else if (( !strnicmp(ptcKey, "UID", 3) ) || ( *ptcKey == '\0' ))
							{
                                const CUID uid(refAttacker.charUID);
								sVal.FormatHex( uid.CharFind() ? refAttacker.charUID : 0 );
								return true;
							}
							else if (!strnicmp(ptcKey, "THREAT", 6))
							{
								sVal.FormatVal(refAttacker.threat);
								return true;
							}
							else if (!strnicmp(ptcKey, "IGNORE", 6))
							{
								sVal.FormatVal(refAttacker.ignore ? 1:0);
								return true;
							}
						}
					}
				}

				return true;
			}
		case CHC_BREATH:
			{
				if( !strnicmp(ptcKey, "BREATH.DAM", 10) )
				{
					CVarDefCont * pVar = GetDefKey(ptcKey, true);
					sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
					return true;
				}
				else if ( !strnicmp(ptcKey, "BREATH.HUE", 10) || !strnicmp(ptcKey, "BREATH.ANIM", 11) || !strnicmp(ptcKey, "BREATH.TYPE", 11) || !strnicmp(ptcKey, "BREATH.DAMTYPE", 14))
				{
					CVarDefCont * pVar = GetDefKey(ptcKey, true);
					sVal.FormatHex(pVar ? (dword)(pVar->GetValNum()) : 0);
					return true;
				}
				return false;
			}
		case CHC_NOTOSAVE:
			{
				if ( strlen( ptcKey ) == 8 )
				{
					sVal.FormatSTVal(m_notoSaves.size());
					return true;
				}

				sVal.FormatVal(0);
				ptcKey += 8;

				if ( *ptcKey == '.' )
				{
					++ptcKey;
					if ( !strnicmp(ptcKey, "ID", 2 ) )
					{
						ptcKey += 2;	// ID + whitspace
						CChar * pChar = static_cast<CChar*>( CUID(Exp_GetSingle(ptcKey)).CharFind() );
						if ( !NotoSave_GetID(pChar) )
							sVal.FormatVal( -1 );
						else
							sVal.FormatVal(NotoSave_GetID(pChar));
						return true;
					}
					if ( m_notoSaves.size() )
					{
						size_t notoIndex = Exp_GetVal(ptcKey);
						SKIP_SEPARATORS(ptcKey);
						if ( notoIndex < m_notoSaves.size() )
						{
							NotoSaves & refnoto = m_notoSaves[notoIndex];

							if ( !strnicmp(ptcKey, "VALUE", 5) )
							{
								sVal.FormatVal(refnoto.value);
								return true;
							}
							else if ( !strnicmp(ptcKey, "ELAPSED", 7) )
							{
								sVal.FormatLLVal(refnoto.time);
								return true;
							}
							else if (( !strnicmp(ptcKey, "UID", 3) ) || ( *ptcKey == '\0' ))
							{
								const CUID uid(refnoto.charUID);
								sVal.FormatHex( uid.CharFind() ? refnoto.charUID : 0 );
								return true;
							}
							else if (!strnicmp(ptcKey, "COLOR", 5))
							{
								sVal.FormatVal(refnoto.color);
								return true;
							}
							return false;
						}
					}
				}

				return true;
			}

		case CHC_FIGHTRANGE: //RANGE is now writable so this is changed to FIGHTRANGE as readable only
			sVal.FormatVal( Fight_CalcRange( m_uidWeapon.ItemFind() ) );
			return true;
		case CHC_BLOODCOLOR:
			sVal.FormatHex( _wBloodHue );
			break;
        case CHC_OFAME:
		case CHC_FAME:
			// How much respect do i give this person ?
			// Fame is never negative !
			{
                if (ptcKey[4] != '.')
                {
                    sVal.FormatUSVal(GetFame());
                    break;
                }

				if (g_Cfg.m_Fame.size() <= 0 )
				{
					DEBUG_ERR(("FAME ranges have not been defined.\n"));
					sVal.FormatVal( 0 );
					return true;
				}

				tchar * ppLevel_sep[100];
				const CSString* pFameAt0 = g_Cfg.m_Fame[0];
                const size_t uiLen = (size_t)pFameAt0->GetLength() + 1;

				tchar * pszFameAt0 = new tchar[uiLen];
				Str_CopyLimitNull(pszFameAt0, pFameAt0->GetBuffer(), uiLen);

				int iFame = GetFame();
				int i = Str_ParseCmds( pszFameAt0, ppLevel_sep, CountOf(ppLevel_sep), "," ) - 1; //range
				for (;;)
				{
					if ( !IsStrNumeric( ppLevel_sep[i] ) )
					{
						DEBUG_ERR(("'%s' is not a valid fame value.\n", ppLevel_sep[i]));
					}
					else if ( iFame >= Str_ToI(ppLevel_sep[ i ]) )
					{
						sVal = ( !g_Cfg.m_Fame[(size_t)i + 1]->CompareNoCase( ptcKey + 5 )) ? "1" : "0";
						delete[] pszFameAt0;
						return true;
					}

					if ( i == 0 )
						break;

					--i;
				}

				sVal = 0;
				delete[] pszFameAt0;
				return true;
			}
		case CHC_SKILLCHECK:	// odd way to get skills checking into the triggers.
			ptcKey += 10;
			SKIP_SEPARATORS(ptcKey);
			{
				tchar * ppArgs[2];
				if ( !Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, CountOf(ppArgs)) )
					return false;
				SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
				if ( iSkill == SKILL_NONE )
					return false;
				sVal.FormatVal( Skill_CheckSuccess( iSkill, Exp_GetVal( ppArgs[1] )));
			}
			return true;
		case CHC_SKILLADJUSTED:
			ptcKey += 13;
			SKIP_SEPARATORS(ptcKey);
			{
				SKILL_TYPE iSkill = g_Cfg.FindSkillKey(ptcKey);
				if (iSkill == SKILL_NONE)
					return false;
				ushort iValAdjusted = Skill_GetAdjusted(iSkill);
				sVal.Format("%hu.%hu", iValAdjusted / 10, iValAdjusted % 10);
			}
			return true;
		case CHC_SKILLBEST:
			// Get the top skill.
			ptcKey += 9;
			{
				uint iRank = 0;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS(ptcKey);
					iRank = Exp_GetSingle(ptcKey);
				}
				sVal.FormatVal(Skill_GetBest(iRank));
			}
			return true;
		case CHC_SEX:	// <SEX milord/milady>	sep chars are :,/
			ptcKey += 3;
			SKIP_SEPARATORS(ptcKey);
			{
				tchar * ppArgs[2];
				if ( !Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, CountOf(ppArgs), ":,/" ) )
					return false;
				sVal = ( pCharDef->IsFemale()) ? ppArgs[1] : ppArgs[0];
			}
			return true;
        case CHC_OKARMA:
		case CHC_KARMA:
			// What do i think of this person.
			{
                if (ptcKey[5] != '.')
                {
                    sVal.FormatSVal(GetKarma());
                    break;
                }

				if (g_Cfg.m_Karma.empty())
				{
					DEBUG_ERR(("KARMA ranges have not been defined.\n"));
					sVal.FormatVal( 0 );
					return true;
				}

				tchar * ppLevel_sep[100];
				const CSString* pKarmaAt0 = g_Cfg.m_Karma[0];
				const size_t uiLen = (size_t)pKarmaAt0->GetLength() + 1;

				tchar * pszKarmaAt0 = new tchar[uiLen];
				Str_CopyLimitNull(pszKarmaAt0, pKarmaAt0->GetBuffer(), uiLen);

				short iKarma = GetKarma();

				int i = Str_ParseCmds( pszKarmaAt0, ppLevel_sep, CountOf(ppLevel_sep), "," ) - 1; //range
				for (;;)
				{
					if ( ppLevel_sep[i][0] != '-' && !IsStrNumeric( ppLevel_sep[i] ) )
					{
						DEBUG_ERR(("'%s' is not a valid karma value.\n", ppLevel_sep[i]));
					}
					else if ( iKarma >= Str_ToI(ppLevel_sep[ i ]) )
					{
						sVal = ( !g_Cfg.m_Karma[(size_t)i + 1]->CompareNoCase( ptcKey + 6 )) ? "1" : "0";
						delete[] pszKarmaAt0;
						return true;
					}

					if ( i == 0 )
						break;
					--i;
				}

				sVal = 0;
				delete[] pszKarmaAt0;
				return true;
			}
		case CHC_AR:
		case CHC_AC:
			sVal.FormatVal( m_defense + pCharDef->m_defense );
			return true;
		case CHC_AGE:
			sVal.FormatLLVal( CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeCreate) / ( MSECS_PER_SEC * 60 * 60 *24 ) ); //displayed in days
			return true;
		case CHC_BANKBALANCE:
		{
			const CItemContainer* pBank = GetBank();
			if (!pBank)
				return false;
			sVal.FormatVal(pBank->ContentCount(CResourceID(RES_TYPEDEF, IT_GOLD)));
			return true;
		}
		case CHC_CANCAST:
			{
				ptcKey += 7;
				GETNONWHITESPACE(ptcKey);

				tchar * ppArgs[2];
				int iQty = Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, CountOf( ppArgs ));

				// Check that we have at least the first argument
				if ( iQty <= 0 )
					return false;

				// Lookup the spell ID to ensure it's valid
				SPELL_TYPE spell = (SPELL_TYPE)(g_Cfg.ResourceGetIndexType( RES_SPELL, ppArgs[0] ));
				bool fCheckAntiMagic = true; // AntiMagic check is enabled by default

				// Set AntiMagic check if second argument has been provided
				if ( iQty == 2 )
					fCheckAntiMagic = ( Exp_GetVal( ppArgs[1] ) >= 1 );

				sVal.FormatVal( Spell_CanCast( spell, true, this, false, fCheckAntiMagic ) );
			}
			return true;
		case CHC_CANMAKE:
			{
				// use m_Act_UID ?
				ptcKey += 7;
				ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ptcKey ));
				sVal.FormatVal( Skill_MakeItem( id,	CUID(UID_CLEAR), SKTRIG_SELECT ) );
			}
			return true;
		case CHC_CANMAKESKILL:
			{
				ptcKey += 12;
				ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ptcKey ));
				sVal.FormatVal( Skill_MakeItem( id,	CUID(UID_CLEAR), SKTRIG_SELECT, true ) );
			}
			return true;
		case CHC_SKILLUSEQUICK:
			{
				ptcKey += 13;
				GETNONWHITESPACE( ptcKey );

				if ( *ptcKey )
				{
					tchar * ppArgs[4];
					int iQty = Str_ParseCmds(const_cast<tchar *>(ptcKey), ppArgs, CountOf( ppArgs ));
					if ( iQty >= 2 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return false;

						sVal.FormatVal( Skill_UseQuick( iSkill, Exp_GetVal( ppArgs[1] ), true ,(Exp_GetVal(ppArgs[2]) != 0 ? false : true), (Exp_GetVal(ppArgs[3]) != 0 ? true : false)));
						return true;
					}
				}
			} return false;
		case CHC_SKILLTEST:
			{
				ptcKey += 9;

				if ( *ptcKey )
				{
					CResourceQtyArray Resources;
					if ( Resources.Load(ptcKey) > 0 && SkillResourceTest( &Resources ) )
					{
						sVal.FormatVal(1);
						return true;
					}
				}
			}
			sVal.FormatVal(0);
			return true;
		case CHC_CANMOVE:
			{
				ptcKey += 7;
				GETNONWHITESPACE(ptcKey);

				CPointMap	ptDst = GetTopPoint();
				DIR_TYPE	dir   = GetDirStr(ptcKey);
				ptDst.Move( dir );
				dword		dwBlockFlags = 0;
				CRegion	*	pArea;
				pArea = CheckValidMove( ptDst, &dwBlockFlags, dir, nullptr );
				sVal.FormatHex( pArea ? pArea->GetResourceID().IsValidUID() : 0 );
			}
			return true;

		case CHC_MOUNT:
			{
				CItem *pItem = LayerFind(LAYER_HORSE);
				if ( pItem )
					sVal.FormatHex(pItem->m_itFigurine.m_UID);
				else
					sVal.FormatVal(0);
				return true;
			}
		case CHC_MOVE:
			{
				ptcKey += 4;
				GETNONWHITESPACE(ptcKey);

                CPointMap ptDst = GetTopPoint();
				ptDst.Move( GetDirStr( ptcKey ) );
				CRegion * pArea = ptDst.GetRegion( REGION_TYPE_MULTI | REGION_TYPE_AREA );
				if ( !pArea )
					sVal.FormatHex( UINT32_MAX );
				else
				{
					dword dwBlockFlags = 0;
					CWorldMap::GetHeightPoint2( ptDst, dwBlockFlags, true );
					sVal.FormatHex( dwBlockFlags );
				}
			}
			return true;
		case CHC_DISPIDDEC:	// for properties dialog.
			sVal.FormatVal( pCharDef->m_trackID );
			return true;
		case CHC_GUILDABBREV:
			{
				lpctstr pszAbbrev = Guild_Abbrev(MEMORY_GUILD);
				sVal = ( pszAbbrev ) ? pszAbbrev : "";
			}
			return true;
		case CHC_ID:
			sVal = g_Cfg.ResourceGetName( pCharDef->GetResourceID());
			return true;
		case CHC_ISGM:
			sVal.FormatVal( IsPriv(PRIV_GM));
			return true;
		case CHC_ISINPARTY:
			if ( m_pPlayer != nullptr )
				sVal = ( m_pParty != nullptr ) ? "1" : "0";
			else
				sVal = "0";
			return true;
		case CHC_ISMYPET:
			if (!m_pNPC)
				return false;
			sVal = NPC_IsOwnedBy( pCharSrc, true ) ? "1" : "0";
			return true;
		case CHC_ISONLINE:
			if ( m_pPlayer != nullptr )
			{
				sVal = IsClientActive() ? "1" : "0";
				return true;
			}
			if ( m_pNPC != nullptr )
			{
				sVal = IsDisconnected() ? "0" : "1";
				return true;
			}
			sVal = 0;
			return true;
		case CHC_ISSTUCK:
			{
                sVal.FormatVal(IsStuck(true));
				return true;
			}
		case CHC_ISVENDOR:
			sVal.FormatVal(NPC_IsVendor() ? 1 : 0);
			return true;
		case CHC_ISVERTICALSPACE:
			{
			ptcKey += 15;
			CPointMap pt;
			if ( strlen( ptcKey ) )
			{
				pt = g_Cfg.GetRegionPoint(ptcKey);
				if ( ! pt.IsValidPoint() )
				{
					DEBUG_ERR(("An invalid point passed as an argument to the function IsVerticalSpace.\n"));
					return false;
				}
			}
			else
				pt = GetTopPoint();
			sVal.FormatVal( IsVerticalSpace( pt, false ) );
			return true;
			}
		case CHC_MEMORY:
			// What is our memory flags about this pSrc person.
			{
				uint uiFlags = 0;
				CItemMemory *pMemory;
				ptcKey += 6;
				if ( *ptcKey == '.' )
				{
					++ptcKey;
					pMemory	= Memory_FindObj(CUID(Exp_GetVal(ptcKey)));
				}
				else
					pMemory	= Memory_FindObj( pCharSrc );
				if ( pMemory != nullptr )
				{
					uiFlags = pMemory->GetMemoryTypes();
				}
				sVal.FormatHex( uiFlags );
			}
			return true;
		case CHC_NAME:
			sVal = GetName(false);
			break;
		case CHC_SKILLTOTAL:
			{
				ptcKey += 10;
				SKIP_SEPARATORS(ptcKey);
				GETNONWHITESPACE(ptcKey);

				int		iVal	= 0;
				bool	fComp	= true;
				if ( *ptcKey == '\0' )
					;
				else if ( *ptcKey == '+' )
					iVal	= Exp_GetVal( ++ptcKey );
				else if ( *ptcKey == '-' )
					iVal	= - Exp_GetVal( ++ptcKey );
				else
				{
					iVal	= Exp_GetVal( ptcKey );
					fComp	= false;
				}

				sVal.FormatVal( GetSkillTotal(iVal,fComp) );
			}
			return true;
		case CHC_SWING:
			sVal.FormatVal( m_atFight.m_iWarSwingState );
			break;
		case CHC_TOWNABBREV:
			{
				lpctstr pszAbbrev = Guild_Abbrev(MEMORY_TOWN);
				sVal = ( pszAbbrev ) ? pszAbbrev : "";
			}
			return true;
		case CHC_MAXWEIGHT:
			sVal.FormatVal( g_Cfg.Calc_MaxCarryWeight(this));
			return true;
		case CHC_ACCOUNT:
			if ( ptcKey[7] == '.' )	// used as a ref ?
			{
				if ( m_pPlayer != nullptr )
				{
					ptcKey += 7;
					SKIP_SEPARATORS(ptcKey);

					CScriptObj * pRef = m_pPlayer->GetAccount();
					if ( pRef )
					{
						if ( pRef->r_WriteVal( ptcKey, sVal, pSrc ) )
							break;
						return ( false );
					}
				}
			}
			if ( m_pPlayer == nullptr )
				sVal.Clear();
			else
				sVal = m_pPlayer->GetAccount()->GetName();
			break;
		case CHC_ACT:
			if ( ptcKey[3] == '.' )	// used as a ref ?
				goto do_default;
			sVal.FormatHex( m_Act_UID.GetObjUID() );	// uid
			break;
		case CHC_ACTP:
			if (ptcKey[4] == '.')
			{
				ptcKey += 4;
				SKIP_SEPARATORS(ptcKey);
				return m_Act_p.r_WriteVal(ptcKey, sVal);
			}
			sVal = m_Act_p.WriteUsed();
			break;
		case CHC_ACTPRV:
			sVal.FormatHex( m_Act_Prv_UID.GetObjUID() );
			break;
		case CHC_ACTDIFF:
			if (m_Act_Difficulty >= 0)
			{
				sVal.FormatVal(m_Act_Difficulty * 10);
			}
			else
			{
				sVal.FormatVal(m_Act_Difficulty);
			}
			break;
		case CHC_ACTARG1:
			sVal.FormatHex( m_atUnk.m_dwArg1 );
			break;
		case CHC_ACTARG2:
			sVal.FormatHex( m_atUnk.m_dwArg2 );
			break;
		case CHC_ACTARG3:
			sVal.FormatHex( m_atUnk.m_dwArg3 );
			break;
		case CHC_ACTION:
		{
			const CSkillDef* pSkillDef = g_Cfg.GetSkillDef(Skill_GetActive());
			if (pSkillDef != nullptr)
				sVal = pSkillDef->GetKey();
			else
			{
				tchar *z = Str_GetTemp();
				sprintf(z, "%d", Skill_GetActive());
				sVal = z;
			}
		}
			break;
		case CHC_BODY:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, GetDispID()) );
			break;
		case CHC_CREATE:
			sVal.FormatLLVal( CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeCreate) / MSECS_PER_TENTH );  // Displayed in Tenths of Second.
			break;
		case CHC_DIR:
			{
				ptcKey +=3;
				CChar * pChar = CUID::CharFindFromUID(Exp_GetSingle(ptcKey));
				if ( pChar )
					sVal.FormatVal( GetDir(pChar) );
				else
					sVal.FormatVal( m_dirFace );
			}break;
		case CHC_EMOTEACT:
			sVal.FormatVal( IsStatFlag(STATF_EMOTEACTION) );
			break;
		case CHC_FLAGS:
			sVal.FormatULLHex(_uiStatFlag);
			break;
		case CHC_FONT:
			sVal.FormatVal( m_fonttype );
			break;
		case CHC_SPEECHCOLOROVERRIDE:
			sVal.FormatWVal( m_SpeechHueOverride );
			break;
		case CHC_EMOTECOLOROVERRIDE:
			sVal.FormatWVal( m_EmoteHueOverride );
			break;
        case CHC_STEPSTEALTH:
            sVal.FormatVal( m_StepStealth );
            break;
		case CHC_HEIGHT:
			sVal.FormatUCVal( GetHeight() );
			break;
        case CHC_OSTR:
            sVal.FormatUSVal( Stat_GetBase(STAT_STR) );
            break;
        case CHC_MODSTR:
            sVal.FormatVal( Stat_GetMod(STAT_STR) );
            break;
        case CHC_STR:
            sVal.FormatUSVal( Stat_GetAdjusted(STAT_STR) );
            break;
        case CHC_ODEX:
            sVal.FormatUSVal( Stat_GetBase(STAT_DEX) );
            break;
        case CHC_MODDEX:
            sVal.FormatVal( Stat_GetMod(STAT_DEX) );
            break;
        case CHC_DEX:
            sVal.FormatUSVal( Stat_GetAdjusted(STAT_DEX) );
            break;
        case CHC_OINT:
            sVal.FormatUSVal( Stat_GetBase(STAT_INT) );
            break;
        case CHC_MODINT:
            sVal.FormatVal( Stat_GetMod(STAT_INT) );
            break;
        case CHC_INT:
            sVal.FormatUSVal( Stat_GetAdjusted(STAT_INT) );
            break;
		case CHC_HITPOINTS:
		case CHC_HITS:
			sVal.FormatUSVal( Stat_GetVal(STAT_STR) );
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			sVal.FormatUSVal( Stat_GetVal(STAT_DEX) );
			break;
        case CHC_MANA:
            sVal.FormatUSVal( Stat_GetVal(STAT_INT) );
            break;
        case CHC_OFOOD:
            sVal.FormatUSVal( Stat_GetBase(STAT_FOOD) );
            break;
        case CHC_FOOD:
            sVal.FormatUSVal( Stat_GetVal(STAT_FOOD) );
            break;
		case CHC_MAXFOOD:
			sVal.FormatUSVal( Stat_GetMaxAdjusted(STAT_FOOD) );
			break;
		case CHC_MAXHITS:
			sVal.FormatUSVal( Stat_GetMaxAdjusted(STAT_STR) );
			break;
        case CHC_OMAXHITS:
            sVal.FormatUSVal( Stat_GetMax(STAT_STR) );
            break;
		case CHC_MAXMANA:
			sVal.FormatUSVal( Stat_GetMaxAdjusted(STAT_INT) );
			break;
        case CHC_OMAXMANA:
            sVal.FormatUSVal( Stat_GetMax(STAT_INT) );
            break;
		case CHC_MAXSTAM:
			sVal.FormatUSVal( Stat_GetMaxAdjusted(STAT_DEX) );
			break;
        case CHC_OMAXSTAM:
            sVal.FormatUSVal( Stat_GetMax(STAT_DEX) );
            break;
        case CHC_MODMAXHITS:
            sVal.FormatVal( Stat_GetMaxMod(STAT_STR) );
            break;
        case CHC_MODMAXMANA:
            sVal.FormatVal( Stat_GetMaxMod(STAT_INT) );
            break;
        case CHC_MODMAXSTAM:
            sVal.FormatVal( Stat_GetMaxMod(STAT_DEX) );
            break;
        case CHC_REGENFOOD:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_FOOD) / MSECS_PER_SEC);
            break;
        case CHC_REGENFOODD:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_FOOD) / MSECS_PER_TENTH);
            break;
        case CHC_REGENHITS:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_STR) / MSECS_PER_SEC);
            break;
        case CHC_REGENHITSD:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_STR) / MSECS_PER_TENTH);
            break;
        case CHC_REGENSTAM:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_DEX) / MSECS_PER_SEC);
            break;
        case CHC_REGENSTAMD:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_DEX) / MSECS_PER_TENTH);
            break;
        case CHC_REGENMANA:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_INT) / MSECS_PER_SEC);
            break;
        case CHC_REGENMANAD:
            sVal.FormatLLVal(Stats_GetRegenRate(STAT_INT) / MSECS_PER_TENTH);
            break;
        case CHC_REGENVALFOOD:
            sVal.FormatUSVal( Stats_GetRegenVal(STAT_FOOD) );
            break;
        case CHC_REGENVALHITS:
            sVal.FormatUSVal( Stats_GetRegenVal(STAT_STR) );
            break;
        case CHC_REGENVALSTAM:
            sVal.FormatUSVal( Stats_GetRegenVal(STAT_DEX) );
            break;
        case CHC_REGENVALMANA:
            sVal.FormatUSVal( Stats_GetRegenVal(STAT_INT) );
            break;
		case CHC_HOME:
			sVal = m_ptHome.WriteUsed();
			break;
		case CHC_NOTOGETFLAG:
			{
				ptcKey += 11;
				GETNONWHITESPACE(ptcKey);

				const CUID uid(Exp_GetDWVal(ptcKey));
				SKIP_ARGSEP( ptcKey );
				const bool fAllowIncog = ( Exp_GetVal( ptcKey ) >= 1 );
				SKIP_ARGSEP( ptcKey );
				const bool fAllowInvul = ( Exp_GetVal( ptcKey ) >= 1 );

				const CChar * pChar;
				if ( ! uid.IsValidUID() )
					pChar = pCharSrc;
				else
				{
					pChar = uid.CharFind();
					if ( ! pChar )
						pChar = pCharSrc;
				}
				sVal.FormatVal( Noto_GetFlag( pChar, fAllowIncog, fAllowInvul ) );
			}
			break;
		case CHC_NPC:
			goto do_default;

		case CHC_OBODY:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, _iPrev_id ));
			break;

		case CHC_OSKIN:
			sVal.FormatHex( _wPrev_Hue );
			break;
		case CHC_P:
			goto do_default;
        case CHC_RANGE:
        {
            const uchar iRangeH = GetRangeH(), iRangeL = GetRangeL();
            if ( iRangeL == 0 )
                sVal.Format( "%hhd", iRangeH );
            else
                sVal.Format( "%hhd,%hhd", iRangeH, iRangeL );
            break;
        }
        case CHC_RANGEH:
            sVal.FormatBVal(GetRangeH());
            break;
        case CHC_RANGEL:
            sVal.FormatBVal(GetRangeL());
            break;
		case CHC_STONE:
			sVal.FormatVal( IsStatFlag( STATF_STONE ));
			break;
		case CHC_TITLE:
			{
				if (strlen(ptcKey) == 5)
					sVal = m_sTitle; //GetTradeTitle
				else
					sVal = GetTradeTitle();
			}
			break;
		case CHC_EXP:
			sVal.FormatVal(m_exp);
			break;
		case CHC_LEVEL:
			sVal.FormatVal(m_level);
			break;

        case CHC_GOLD:
            if (!(g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD))
            {
                sVal.FormatVal(ContentCount(CResourceID(RES_TYPEDEF, IT_GOLD)));
                break;
            }
            // not break, just run down to VIRTUALGOLD
			FALLTHROUGH;
		case CHC_VIRTUALGOLD:
			sVal.FormatLLVal(m_virtualGold);
			break;
		case CHC_VISUALRANGE:
			sVal.FormatVal(GetVisualRange());
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}



bool CChar::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CChar::r_LoadVal");
	EXC_TRY("LoadVal");

    // Checking Props CComponents first (first check CChar props, if not found then check CCharBase)
    EXC_SET_BLOCK("EntityProps");
    if (CEntityProps::r_LoadPropVal(s, this, Base_GetDef()))
    {
        return true;
    }

    // Now check regular CComponents
    EXC_SET_BLOCK("CEntity");
    if (CEntity::r_LoadVal(s))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
	lpctstr	ptcKey = s.GetKey();
	CHC_TYPE iKeyNum = (CHC_TYPE) FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	if ( iKeyNum < 0 )
	{
		if ( m_pPlayer )
		{
			if ( m_pPlayer->r_LoadVal( this, s ))
				return true;
		}
		if ( m_pNPC )
		{
			if ( m_pNPC->r_LoadVal( this, s ))
				return true;
		}

        // special load values
		int i = g_Cfg.FindSkillKey( ptcKey );
		if ( IsSkillBase((SKILL_TYPE)i) )
		{
			// Check some skill name.
			Skill_SetBase((SKILL_TYPE)i, s.GetArgUSVal() );
			return true;
		}

		return CObjBase::r_LoadVal( s );
	}
	switch (iKeyNum)
	{
		//Status Update Variables
        case CHC_REGENHITS:
            Stats_SetRegenRate(STAT_STR, s.GetArg64Val()* MSECS_PER_SEC);
            break;
        case CHC_REGENHITSD:
            Stats_SetRegenRate(STAT_STR, s.GetArg64Val()* MSECS_PER_TENTH);
            break;
        case CHC_REGENSTAM:
            Stats_SetRegenRate(STAT_DEX, s.GetArg64Val()* MSECS_PER_SEC);
            break;
        case CHC_REGENSTAMD:
            Stats_SetRegenRate(STAT_DEX, s.GetArg64Val()* MSECS_PER_TENTH);
            break;
        case CHC_REGENMANA:
            Stats_SetRegenRate(STAT_INT, s.GetArg64Val()* MSECS_PER_SEC);
            break;
        case CHC_REGENMANAD:
            Stats_SetRegenRate(STAT_INT, s.GetArg64Val()* MSECS_PER_TENTH);
            break;
        case CHC_REGENFOOD:
            Stats_SetRegenRate(STAT_FOOD, s.GetArg64Val()* MSECS_PER_SEC);
            break;
        case CHC_REGENFOODD:
            Stats_SetRegenRate(STAT_FOOD, s.GetArg64Val()* MSECS_PER_TENTH);
            break;
        case CHC_REGENVALHITS:
            Stats_SetRegenVal(STAT_STR, s.GetArgUSVal());
            break;
        case CHC_REGENVALSTAM:
            Stats_SetRegenVal(STAT_DEX, s.GetArgUSVal());
            break;
        case CHC_REGENVALMANA:
            Stats_SetRegenVal(STAT_INT, s.GetArgUSVal());
            break;
        case CHC_REGENVALFOOD:
            Stats_SetRegenVal(STAT_FOOD, s.GetArgUSVal());
            break;

		case CHC_CURFOLLOWER:
		case CHC_MAXFOLLOWER:
		case CHC_TITHING:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
				UpdateStatsFlag();
			}
			break;
		//Set as numbers only
		case CHC_SPELLTIMEOUT:
			{
				SetDefNum(s.GetKey(), s.GetArgVal(), false);
			}
			break;

		case CHC_BLOODCOLOR:
			_wBloodHue = (HUE_TYPE)(s.GetArgVal());
			break;
        case CHC_MODSTR:
            Stat_SetMod(STAT_STR, s.GetArgSVal());
            break;
        case CHC_OSTR:
        case CHC_STR:
            Stat_SetBase(STAT_STR, s.GetArgUSVal());
            break;
        case CHC_MODDEX:
            Stat_SetMod(STAT_DEX, s.GetArgSVal());
            break;
        case CHC_ODEX:
        case CHC_DEX:
            Stat_SetBase(STAT_DEX, s.GetArgUSVal());
            break;
        case CHC_MODINT:
            Stat_SetMod(STAT_INT, s.GetArgSVal());
            break;
        case CHC_OINT:
        case CHC_INT:
            Stat_SetBase(STAT_INT, s.GetArgUSVal());
			break;
		case CHC_MAXFOOD:
			Stat_SetMax(STAT_FOOD, s.GetArgUSVal());
			break;
        case CHC_MODMAXHITS:
            Stat_SetMaxMod(STAT_STR, s.GetArgSVal());
            break;
		case CHC_MAXHITS:   // In the save files OMaxHits is stored as MaxHits (for backwards compatibility)
        case CHC_OMAXHITS:
            Stat_SetMax(STAT_STR, s.GetArgUSVal());
            break;
        case CHC_MODMAXSTAM:
            Stat_SetMaxMod(STAT_DEX, s.GetArgSVal());
            break;
        case CHC_MAXSTAM:   // In the save files OMaxStam is stored as MaxStam (for backwards compatibility)
        case CHC_OMAXSTAM:
            Stat_SetMax(STAT_DEX, s.GetArgUSVal());
            break;
        case CHC_MODMAXMANA:
            Stat_SetMaxMod(STAT_INT, s.GetArgSVal());
            break;
        case CHC_MAXMANA:   // In the save files OMaxMana is stored as MaxMana (for backwards compatibility)
        case CHC_OMAXMANA:
            Stat_SetMax(STAT_INT, s.GetArgUSVal());
            break;
		case CHC_ACCOUNT:
			return SetPlayerAccount( s.GetArgStr() );
		case CHC_ACT:
			m_Act_UID.SetObjUID(s.GetArgDWVal());
			break;
		case CHC_ACTP:
			if ( ! s.HasArgs() )
				m_Act_p = GetTopPoint();
			else
				m_Act_p.Read( s.GetArgStr() );
			break;
		case CHC_ACTPRV:
			m_Act_Prv_UID.SetObjUID(s.GetArgVal());
			break;
		case CHC_ACTDIFF:
			{
				int iVal = s.GetArgVal();
				if (iVal < -1)
				{
					iVal = -1;
				}
				else if (iVal > 0)
				{
					iVal /= 10;
				}
				m_Act_Difficulty = iVal;
			}
			break;
		case CHC_ACTARG1:
			m_atUnk.m_dwArg1 = s.GetArgVal();
			break;
		case CHC_ACTARG2:
			m_atUnk.m_dwArg2 = s.GetArgVal();
			break;
		case CHC_ACTARG3:
			m_atUnk.m_dwArg3 = s.GetArgVal();
			break;
		case CHC_ACTION:
		{
			lpctstr argStr = s.GetArgStr();
			SKILL_TYPE skillKey = g_Cfg.FindSkillKey(argStr);
			if ( (skillKey == SKILL_NONE) && (s.GetArgVal() != -1) && !IsSkillNPC(skillKey) )
				g_Log.EventError("Invalid skill key: %s\n", argStr);
			return Skill_Start(skillKey);
		}
		case CHC_ATTACKER:
		{
			if ( strlen(ptcKey) > 8 )
			{
				ptcKey += 8;
				if ( *ptcKey == '.' )
				{
					++ptcKey;
					if ( !strnicmp(ptcKey, "CLEAR", 5) )
					{
						if ( !m_lastAttackers.empty() )
							Fight_ClearAll();
						return true;
					}
					else if ( !strnicmp(ptcKey, "DELETE", 6) )
					{
						if ( !m_lastAttackers.empty() )
						{
							int idx = s.GetArgVal();
							CChar *pChar = CUID::CharFindFromUID(idx);
							if (!pChar)
								return false;
							Attacker_Delete(idx, false, ATTACKER_CLEAR_SCRIPT);
						}
						return true;
					}
					else if ( !strnicmp(ptcKey, "ADD", 3) )
					{
						CChar *pChar = CUID::CharFindFromUID(s.GetArgVal());
						if ( !pChar )
							return false;
						Fight_Attack(pChar);
						return true;
					}
					else if ( !strnicmp(ptcKey, "TARGET", 6) )
					{
						CChar *pChar = CUID::CharFindFromUID(s.GetArgVal());
						if ( !pChar || (pChar == this) )	// can't set ourself as target
						{
							m_Fight_Targ_UID.InitUID();
							return false;
						}
						m_Fight_Targ_UID = pChar->GetUID();
						return true;
					}

					int attackerIndex = Exp_GetVal(ptcKey);

					SKIP_SEPARATORS(ptcKey);
					if ( attackerIndex < GetAttackersCount() )
					{
						if ( !strnicmp(ptcKey, "DAM", 3) )
						{
							Attacker_SetDam(attackerIndex, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(ptcKey, "ELAPSED", 7) )
						{
							Attacker_SetElapsed(attackerIndex, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(ptcKey, "THREAT", 6) )
						{
							Attacker_SetThreat(attackerIndex, s.GetArgVal());
							return true;
						}
						else if ( !strnicmp(ptcKey, "DELETE", 6) )
						{
							Attacker_Delete(attackerIndex, false, ATTACKER_CLEAR_SCRIPT);
							return true;
						}
						else if ( !strnicmp(ptcKey, "IGNORE", 6) )
						{
							bool fIgnore = s.GetArgVal() < 1 ? 0 : 1;
							Attacker_SetIgnore(attackerIndex, fIgnore);
							return true;
						}
					}
				}
			}
			return false;
		}
		case CHC_BODY:
			SetID( (CREID_TYPE)(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr())) );
			break;
		case CHC_BREATH:
			{
				if ( !strnicmp(ptcKey, "BREATH.DAM", 10) || !strnicmp(ptcKey, "BREATH.HUE", 10) || !strnicmp(ptcKey, "BREATH.ANIM", 11) || !strnicmp(ptcKey, "BREATH.TYPE", 11) || !strnicmp(ptcKey, "BREATH.DAMTYPE", 14))
				{
					SetDefNum(s.GetKey(), s.GetArgLLVal());
					return true;
				}
				return false;
			}break;
        case CHC_CREATE:
            {
                if (g_Serv.IsLoading())
                    {
                        _iTimeCreate = (CWorldGameTime::GetCurrentTime().GetTimeRaw() - (s.GetArgLLVal() * MSECS_PER_TENTH));
                        break;
                    }
                return false;
            }
		case CHC_DIR:
			{
				DIR_TYPE dir = static_cast<DIR_TYPE>(s.GetArgVal());
				if (dir <= DIR_INVALID || dir >= DIR_QTY)
					dir = DIR_SE;
				m_dirFace = dir;
				UpdateDir( dir );
			}
			break;
		case CHC_DISMOUNT:
			Horse_UnMount();
			break;
		case CHC_EMOTEACT:
			{
				bool fSet = IsStatFlag(STATF_EMOTEACTION);
				if ( s.HasArgs() )
					fSet = s.GetArgVal() ? true : false;
				else
					fSet = ! fSet;
				StatFlag_Mod(STATF_EMOTEACTION,fSet);
			}
			break;
		case CHC_FLAGS:
			if (g_Serv.IsLoading())
			{
				// Don't set STATF_SAVEPARITY at server startup, otherwise the first worldsave will not save these chars
				_uiStatFlag = s.GetArgLLVal() & ~STATF_SAVEPARITY;
				break;
			}
			// Don't modify STATF_SAVEPARITY, STATF_PET, STATF_SPAWNED here
			_uiStatFlag = (_uiStatFlag & (STATF_SAVEPARITY | STATF_PET | STATF_SPAWNED)) | (s.GetArgLLVal() & ~(STATF_SAVEPARITY | STATF_PET | STATF_SPAWNED));
			NotoSave_Update();
			break;
		case CHC_FONT:
			m_fonttype = (FONT_TYPE)s.GetArgVal();
			if (m_fonttype >= FONT_QTY)
				m_fonttype = FONT_NORMAL;
			break;
		case CHC_SPEECHCOLOROVERRIDE:
			m_SpeechHueOverride = (HUE_TYPE)s.GetArgWVal();
			break;
		case CHC_EMOTECOLOROVERRIDE:
			m_EmoteHueOverride = (HUE_TYPE)s.GetArgWVal();
			break;
        case CHC_OFOOD: // used in the save file
            Stat_SetBase(STAT_FOOD, s.GetArgUSVal());
            break;
		case CHC_FOOD:
			Stat_SetVal(STAT_FOOD, s.GetArgUSVal());
			break;
		case CHC_HITPOINTS:
		case CHC_HITS:
			Stat_SetVal(STAT_STR, (ushort)std::max(s.GetArgVal(), 0));
			UpdateHitsFlag();
			break;
		case CHC_MANA:
			Stat_SetVal(STAT_INT, (ushort)std::max(s.GetArgVal(), 0));
			UpdateManaFlag();
			break;
		case CHC_STAM:
		case CHC_STAMINA:
			Stat_SetVal(STAT_DEX, (ushort)std::max(s.GetArgVal(), 0));
			UpdateStamFlag();
			break;
		case CHC_STEPSTEALTH:
			m_StepStealth = s.GetArgVal();
			break;
		case CHC_HEIGHT:
			m_height = (height_t)(s.GetArgVal());
			break;
		case CHC_HOME:
			if ( ! s.HasArgs() )
				m_ptHome = GetTopPoint();
			else
				m_ptHome.Read( s.GetArgStr() );
			break;
		case CHC_NAME:
			{
				if ( IsTrigUsed(TRIGGER_RENAME) )
				{
					CScriptTriggerArgs args;
					args.m_s1 = s.GetArgStr();
					args.m_pO1 = this;
					if ( this->OnTrigger(CTRIG_Rename, this, &args) == TRIGRET_RET_TRUE )
						return false;

					SetName( args.m_s1 );
				}
				else
					SetName( s.GetArgStr() );
			}
			break;
        case CHC_OFAME:
		case CHC_FAME:
            SetFame(s.GetArgUSVal());
            break;
        case CHC_OKARMA:
		case CHC_KARMA:
			SetKarma(s.GetArgSVal());
            break;
		case CHC_SKILLUSEQUICK:
			{
				if ( s.GetArgStr() )
				{
					tchar * ppArgs[4];
					int iQty = Str_ParseCmds(const_cast<tchar *>(s.GetArgStr()), ppArgs, CountOf( ppArgs ));
					if ( iQty >= 2 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return false;

						Skill_UseQuick( iSkill, Exp_GetVal( ppArgs[1] ), true, (Exp_GetVal(ppArgs[2]) != 0 ? false : true), (Exp_GetVal(ppArgs[3]) != 0 ? true : false));
						return true;
					}
				}
			} return false;
		case CHC_MEMORY:
			{
				int64 piCmd[2];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, CountOf(piCmd) );
				if ( iArgQty < 2 )
					return false;

				const CUID uid((dword)piCmd[0]);
				const word wFlags = (word)piCmd[1];

				Memory_AddObjTypes( uid, wFlags );
			}
			break;
		case CHC_NPC:
			return SetNPCBrain((NPCBRAIN_TYPE)(s.GetArgVal()));
		case CHC_OBODY:
			{
				CREID_TYPE id = (CREID_TYPE)(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr()));
				if ( ! CCharBase::FindCharBase( id ) )
				{
					DEBUG_ERR(( "OBODY Invalid Char 0%x\n", id ));
					return false;
				}
				_iPrev_id = id;
			}
			break;
		case CHC_OSKIN:
			_wPrev_Hue = (HUE_TYPE)(s.GetArgWVal());
			break;
		case CHC_P:
			{
				CPointMap pt;
				pt.Read( s.GetArgStr() );
                if (pt.IsValidPoint())
				    MoveTo(pt);
                else
                    return false;
			}
			break;
        case CHC_RANGE:
        {
			_uiRange = CBaseBaseDef::ConvertRangeStr(s.GetArgStr());
            break;
        }
        case CHC_RANGEH:
        case CHC_RANGEL:
            return false;
		case CHC_STONE:
			{
				bool fSet;
				bool fChange = IsStatFlag(STATF_STONE);
				if ( s.HasArgs() )
				{
					fSet = s.GetArgVal() ? true : false;
					fChange = ( fSet != fChange );
				}
				else
				{
					fSet = ! fChange;
					fChange = true;
				}
				StatFlag_Mod(STATF_STONE,fSet);
				if ( fChange )
				{
					UpdateMode(nullptr, true);
					if ( IsClientActive() )
						m_pClient->addCharMove(this);
				}
			}
			break;
		case CHC_SWING:
			{
                int iVal = s.GetArgVal();
				if ( iVal && ((iVal < -1) || (iVal > WAR_SWING_SWINGING) ) )
					return false;
				m_atFight.m_iWarSwingState = (WAR_SWING_TYPE)iVal;
			}
			break;
		case CHC_TITLE:
			m_sTitle = s.GetArgStr();
			break;
		case CHC_EXP:
			m_exp = s.GetArgUVal();
			ChangeExperience();			//	auto-update level if applicable
			break;
		case CHC_LEVEL:
			m_level = s.GetArgUVal();
			ChangeExperience();
			break;
        case CHC_GOLD:
        {
            if (!(g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD))
            {
                int newGold = s.GetArgVal();
                if (newGold < 0)
                    return false;

                int currentGold = ContentCount(CResourceID(RES_TYPEDEF, IT_GOLD));
                if (newGold < currentGold)
                {
                    ContentConsume(CResourceID(RES_TYPEDEF, IT_GOLD), currentGold - newGold);
                }
                else if (newGold > currentGold)
                {
                    CItemContainer *pBank = GetBank();
                    if (!pBank)
                        return false;
                    AddGoldToPack(newGold - currentGold, pBank);
                }
                UpdateStatsFlag();
                break;
            }
            // not break, just run down to VIRTUALGOLD
			FALLTHROUGH;
        }
		case CHC_VIRTUALGOLD:
			m_virtualGold = s.GetArgLLVal();
			UpdateStatsFlag();
			break;
		case CHC_VISUALRANGE:
			SetVisualRange(s.GetArgBVal());
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CChar::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CChar::r_Write");
	EXC_TRY("r_Write");

	s.WriteSection("WORLDCHAR %s", GetResourceName());
	s.WriteKeyVal("CREATE", CWorldGameTime::GetCurrentTime().GetTimeDiff(_iTimeCreate) / MSECS_PER_TENTH );

    // Do not save TAG.LastHit (used by PreHit combat flag). It's based on the server uptime, so if this tag isn't zeroed,
    //  after the server restart the char may not be able to attack until the server reaches the serv.time when the previous TAG.LastHit was set.
	int64 iValLastHit = 0;
	CVarDefContNum* pVarLastHit = m_TagDefs.GetKeyDefNum("LastHit");
	if (pVarLastHit)
	{
		iValLastHit = pVarLastHit->GetValNum();
		pVarLastHit->SetValNum(0);
	}

	CObjBase::r_Write(s);

	if (iValLastHit != 0)
	{
		pVarLastHit->SetValNum(iValLastHit);
	}

	if ( m_pPlayer )
		m_pPlayer->r_WriteChar(this, s);
	if ( m_pNPC )
		m_pNPC->r_WriteChar(this, s);

	const CPointMap& pt = GetTopPoint();
	if (pt.IsValidPoint())
		s.WriteKeyStr("P", pt.WriteUsed());

	if ( !m_sTitle.IsEmpty() )
		s.WriteKeyStr("TITLE", m_sTitle);
	if ( m_fonttype != FONT_NORMAL )
		s.WriteKeyVal("FONT", m_fonttype);
	if (m_SpeechHueOverride)
		s.WriteKeyVal("SPEECHCOLOROVERRIDE", m_SpeechHueOverride);
	if (m_EmoteHueOverride)
		s.WriteKeyVal("EMOTECOLOROVERRIDE", m_EmoteHueOverride);
	if ( m_dirFace != DIR_SE )
		s.WriteKeyVal("DIR", m_dirFace);
	if ( _iPrev_id != GetID() )
		s.WriteKeyStr("OBODY", g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _iPrev_id)));
	if ( _wPrev_Hue != HUE_DEFAULT )
		s.WriteKeyHex("OSKIN", _wPrev_Hue);
	if ( _uiStatFlag )
		s.WriteKeyHex("FLAGS", _uiStatFlag);
	if ( m_attackBase )
		s.WriteKeyFormat("DAM", "%" PRIu16 ",%" PRIu16, m_attackBase, m_attackBase + m_attackRange);
	if ( m_defense )
		s.WriteKeyVal("ARMOR", m_defense);

	const uint uiActUID = m_Act_UID.GetObjUID();
	if ((uiActUID & UID_UNUSED) != UID_UNUSED)
		s.WriteKeyHex("ACT", uiActUID);

	if ( m_Act_p.IsValidPoint() )
		s.WriteKeyStr("ACTP", m_Act_p.WriteUsed());

	const SKILL_TYPE action = Skill_GetActive();
	if (action != SKILL_NONE)
	{
		const CSkillDef* pSkillDef = g_Cfg.GetSkillDef(action);
		tchar* pszActionTemp;
		if (pSkillDef != nullptr)
			pszActionTemp = const_cast<tchar*>(pSkillDef->GetKey());
		else
			pszActionTemp = Str_FromI_Fast(action, Str_GetTemp(), STR_TEMPLENGTH, 10);
		s.WriteKeyStr("ACTION", pszActionTemp);

			/* We save ACTARG1/ACTARG2/ACTARG3 only if the following conditions are satisfied:
			ACTARG1/ACTARG2/ACTARG3 is different from 0 AND
			The character action is one of the valid skill OR
			The character action is one of the NPC Action that uses ACTARG1/ACTARG2/ACTARG3
			*/
		if ((action > SKILL_NONE && action < SKILL_QTY) || action == NPCACT_FLEE || action == NPCACT_TALK || action == NPCACT_TALK_FOLLOW || action == NPCACT_RIDDEN)
		{
			if (m_atUnk.m_dwArg1 != 0)
				s.WriteKeyHex("ACTARG1", m_atUnk.m_dwArg1);
			if (m_atUnk.m_dwArg2 != 0)
				s.WriteKeyHex("ACTARG2", m_atUnk.m_dwArg2);
			if (m_atUnk.m_dwArg3 != 0)
				s.WriteKeyHex("ACTARG3", m_atUnk.m_dwArg3);
		}
	}

	if ( m_virtualGold )
		s.WriteKeyVal("VIRTUALGOLD", m_virtualGold);

	if ( m_exp )
		s.WriteKeyVal("EXP", m_exp);
	if ( m_level )
		s.WriteKeyVal("LEVEL", m_level);

	if ( m_height )
		s.WriteKeyVal("HEIGHT", m_height);
	if ( m_ptHome.IsValidPoint() )
		s.WriteKeyStr("HOME", m_ptHome.WriteUsed());
	if ( m_StepStealth )
		s.WriteKeyVal("STEPSTEALTH", m_StepStealth);

    // Storing them with the O prefix for backwards compatibility
    s.WriteKeyVal("OKARMA", GetKarma() );
    s.WriteKeyVal("OFAME", GetFame() );

	int iVal;
	if ((iVal = Stat_GetMod(STAT_FOOD)) != 0)
		s.WriteKeyVal("MODFOOD", iVal);
	if ((iVal = Stat_GetBase(STAT_FOOD)) != Char_GetDef()->m_MaxFood)
		s.WriteKeyVal("OFOOD", iVal);
	s.WriteKeyVal("FOOD", Stat_GetVal(STAT_FOOD));

	static constexpr lpctstr _ptcKeyModStat[STAT_BASE_QTY] =
	{
		"MODSTR",
		"MODINT",
		"MODDEX"
	};
	static constexpr lpctstr _ptcKeyOStat[STAT_BASE_QTY] =
	{
		"OSTR",
		"OINT",
		"ODEX"
	};

	for (int j = 0; j < STAT_BASE_QTY; ++j)
	{
		// this is VERY important, saving the MOD first
		if ((iVal = Stat_GetMod((STAT_TYPE)j)) != 0)
		{
			s.WriteKeyVal(_ptcKeyModStat[j], iVal);
		}
		if ((iVal = Stat_GetBase((STAT_TYPE)j)) != 0)
		{
			s.WriteKeyVal(_ptcKeyOStat[j], iVal);
		}
	}

	if ((iVal = Stat_GetMaxMod(STAT_STR)) != 0)
		s.WriteKeyVal("MODMAXHITS", iVal);
	if ((iVal = Stat_GetMax(STAT_STR)) != Stat_GetAdjusted(STAT_STR))
		s.WriteKeyVal("MAXHITS", iVal);     // should be OMAXHITS, but we keep it like this for backwards compatibility
	s.WriteKeyVal("HITS", Stat_GetVal(STAT_STR));

	if ((iVal = Stat_GetMaxMod(STAT_DEX)) != 0)
		s.WriteKeyVal("MODMAXSTAM", iVal);
	if ((iVal = Stat_GetMax(STAT_DEX)) != Stat_GetAdjusted(STAT_DEX))
		s.WriteKeyVal("MAXSTAM", iVal);     // should be OMAXSTAM, but we keep it like this for backwards compatibility
	s.WriteKeyVal("STAM", Stat_GetVal(STAT_DEX));

	if ((iVal = Stat_GetMaxMod(STAT_INT)) != 0)
		s.WriteKeyVal("MODMAXMANA", iVal);
	if ((iVal = Stat_GetMax(STAT_INT)) != Stat_GetAdjusted(STAT_INT))
		s.WriteKeyVal("MAXMANA", iVal);     // should be OMAXMANA, but we keep it like this for backwards compatibility
	s.WriteKeyVal("MANA", Stat_GetVal(STAT_INT));

	static constexpr lpctstr _ptcKeyRegen[STAT_QTY] =
	{
		"REGENHITS",
		"REGENMANA",
		"REGENSTAM",
		"REGENFOOD"
	};
	for (ushort j = 0; j < STAT_QTY; ++j)
	{
		const int64 iRegen = Stats_GetRegenRate((STAT_TYPE)j); //we cannot use ushort here because by default REGENFOOD has a value higher than 65k.
		if ((iRegen >= 1) && (iRegen != g_Cfg.m_iRegenRate[j]))
			s.WriteKeyVal(_ptcKeyRegen[j], iRegen / MSECS_PER_SEC);
	}
    static constexpr lpctstr _ptcKeyRegenVal[STAT_QTY] =
    {
        "REGENVALHITS",
        "REGENVALMANA",
        "REGENVALSTAM",
        "REGENVALFOOD"
    };
    for (ushort j = 0; j < STAT_QTY; ++j)
    {
        const ushort uiRegenVal = Stats_GetRegenVal((STAT_TYPE)j);
        if (uiRegenVal > 1)
            s.WriteKeyVal(_ptcKeyRegenVal[j], uiRegenVal);
    }

	for ( uint j = 0; j < g_Cfg.m_iMaxSkill; ++j )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex((SKILL_TYPE)j) )
			continue;
        const ushort uiSkillVal = Skill_GetBase((SKILL_TYPE)j);
        if (uiSkillVal == 0)
            continue;
		s.WriteKeyVal(g_Cfg.GetSkillDef((SKILL_TYPE)j)->GetKey(), uiSkillVal );
	}
    CEntity::r_Write(s);
    CEntityProps::r_Write(s);

	r_WriteContent(s);
	EXC_CATCH;
}

void CChar::r_WriteParity( CScript & s )
{
	ADDTOCALLSTACK("CChar::r_WriteParity");
	// overload virtual for world save.

	// if ( GetPrivLevel() <= PLEVEL_Guest ) return;

	if ( g_World.m_fSaveParity == IsStatFlag(STATF_SAVEPARITY))
	{
		return; // already saved.
	}

	StatFlag_Mod( STATF_SAVEPARITY, g_World.m_fSaveParity );
	if ( IsWeird() )
		return;
	r_WriteSafe(s);
}

bool CChar::r_Load( CScript & s ) // Load a character from script
{
	ADDTOCALLSTACK("CChar::r_Load");
	CScriptObj::r_Load(s);

	// Init the STATF_SAVEPARITY flag.
	// StatFlag_Mod( STATF_SAVEPARITY, g_World.m_fSaveParity );

	// Make sure everything is ok.
	if (( m_pPlayer && ! IsClientActive()) ||
		( m_pNPC && IsStatFlag( STATF_RIDDEN )))	// ridden npc
	{
		SetDisconnected();
	}
	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
	{
		DEBUG_ERR(( "Char 0%x Invalid, id='%s', code=0%x\n", (dword)GetUID(), GetResourceName(), iResultCode ));
		Delete();
        return true;
	}

    if (m_pNPC)
        NPC_GetAllSpellbookSpells();

	return true;
}

enum CHV_TYPE
{
	#define ADD(a,b) CHV_##a,
	#include "../../tables/CChar_functions.tbl"
	#undef ADD
	CHV_QTY
};

lpctstr const CChar::sm_szVerbKeys[CHV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CChar_functions.tbl"
	#undef ADD
	nullptr
};

bool CChar::r_Verb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CChar::r_Verb");
	if ( !pSrc )
		return false;

	EXC_TRY("Verb");

	if ( IsClientActive() && GetClientActive()->r_Verb(s, pSrc) )
		return true;

    if (CEntity::r_Verb(s, pSrc))
    {
        return true;
    }

	EXC_SET_BLOCK("Verb-statement");

	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys)-1 );
	if ( index < 0 )
    {
		return ( (m_pNPC && NPC_OnVerb(s, pSrc)) || (m_pPlayer && Player_OnVerb(s, pSrc)) || CObjBase::r_Verb(s, pSrc) );
    }

	CChar * pCharSrc = pSrc->GetChar();

	switch ( index )
	{
		case CHV_AFK:
			// toggle ?
			{
				bool fAFK = ( Skill_GetActive() == NPCACT_NAPPING );
				bool fMode;
				if ( s.HasArgs())
					fMode = ( s.GetArgVal() != 0 );
				else
					fMode = ! fAFK;
				if ( fMode != fAFK )
				{
					if ( fMode )
					{
						SysMessageDefault(DEFMSG_CMDAFK_ENTER);
						m_Act_p = GetTopPoint();
						Skill_Start( NPCACT_NAPPING );
					}
					else
					{
						SysMessageDefault(DEFMSG_CMDAFK_LEAVE);
						Skill_Start( SKILL_NONE );
					}
				}
			}
			break;

		case CHV_ALLSKILLS:
			{
				ushort uiVal = s.GetArgUSVal();
				for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
				{
					if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex((SKILL_TYPE)i) )
						continue;

					Skill_SetBase((SKILL_TYPE)i, uiVal );
				}
			}
			break;
		case CHV_ANIM:
			// ANIM, ANIM_TYPE action, bool fBackward = false, byte iFrameDelay = 1
			{
				int64 Arg_piCmd[3];		// Maximum parameters in one line
				int Arg_Qty = Str_ParseCmds(s.GetArgRaw(), Arg_piCmd, CountOf(Arg_piCmd));
				if ( !Arg_Qty )
					return false;
				return UpdateAnimate((ANIM_TYPE)(Arg_piCmd[0]), true, false,
					(Arg_Qty > 1) ? (uchar)(Arg_piCmd[1]) : 1,
					(Arg_Qty > 2) ? (uchar)(Arg_piCmd[2]) : 1);
			}
			break;
		case CHV_ATTACK:
			Fight_Attack(CUID::CharFindFromUID(s.GetArgVal()), true);
			break;
		case CHV_BANK:
			// Open the bank box for this person
			if ( pCharSrc == nullptr || ! pCharSrc->IsClientActive() )
				return false;
			pCharSrc->GetClientActive()->addBankOpen( this, ((s.HasArgs()) ? (LAYER_TYPE)(s.GetArgVal()) : LAYER_BANKBOX ) );
			break;
		case CHV_BARK:	// This plays creature-specific sounds (CRESND_TYPE). Use CHV_SOUND to play a precise sound ID (SOUND_TYPE) instead.
			SoundChar(s.HasArgs() ? (CRESND_TYPE)s.GetArgVal() : CRESND_RAND);
			break;
		case CHV_BOUNCE: // uid
			return ItemBounce( CUID::ItemFindFromUID( s.GetArgVal()) );
		case CHV_BOW:
			if (s.HasArgs())
				UpdateDir( CUID::ObjFindFromUID(s.GetArgVal()) );
			UpdateAnimate(ANIM_BOW);
			break;

		case CHV_CONTROL: // Possess
			if ( pCharSrc == nullptr || ! pCharSrc->IsClientActive())
				return false;
			return pCharSrc->GetClientActive()->Cmd_Control(this);

		case CHV_CONSUME:
			{
				CResourceQtyArray Resources;
				Resources.Load( s.GetArgStr() );
				ResourceConsume( &Resources, 1, false );
			}
			break;
		case CHV_CRIMINAL:
			if (s.HasArgs() && !s.GetArgVal())
			{
				CItem * pMemoryCriminal = LayerFind(LAYER_FLAG_Criminal);
                if (pMemoryCriminal)
                {
                    pMemoryCriminal->Delete();
                }
                else //if (IsStatFlag(STATF_CRIMINAL))
                {
                    // Otherwise clear it manually if there's no memory set
                    NotoSave_Update();
                    if (m_pClient)
                    {
                        m_pClient->removeBuff(BI_CRIMINALSTATUS);
                    }
                }
                StatFlag_Clear(STATF_CRIMINAL);
			}
			else
				Noto_Criminal();
			break;
		case CHV_DISCONNECT:
			// Push a player char off line. CLIENTLINGER thing
			if ( IsClientActive())
				return GetClientActive()->addKick( pSrc, false );
			SetDisconnected();
			break;
		case CHV_DROP:	// uid
			return ItemDrop( CUID::ItemFindFromUID(s.GetArgVal()), GetTopPoint() );
		case CHV_DUPE:	// = dupe a creature !
			{
				CChar * pChar = CreateNPC( GetID() );
				pChar->MoveTo( GetTopPoint() );
				pChar->DupeFrom(this, s.GetArgVal() < 1 ? true : false);
				pChar->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
				pChar->_iCreatedResScriptLine = s.m_iLineNum;
			}
			break;
		case CHV_EQUIP:	// uid
			return ItemEquip( CUID::ItemFindFromUID(s.GetArgVal()) );
		case CHV_EQUIPHALO:
			{
				// equip a halo light
				CItem * pItem = CItem::CreateScript(ITEMID_LIGHT_SRC, this);
				ASSERT(pItem);
				if ( s.HasArgs())	// how long to last ?
				{
					int64 iTimer = s.GetArgLLVal();
					if ( iTimer > 0 )
						pItem->SetTimeout(iTimer);

					//pItem->Item_GetDef()->m_ttNormal.m_tData4 = 0; // why would we alter the itemdef data?
				}
				pItem->SetAttr(ATTR_MOVE_NEVER);
				LayerAdd( pItem, LAYER_HAND2 );
			}
			return true;
		case CHV_EQUIPARMOR:
			return ItemEquipArmor(false);
		case CHV_EQUIPWEAPON:
			// find my best waepon for my skill and equip it.
			return ItemEquipWeapon(false);
		case CHV_FACE:
		{
			lpctstr pszVerbArg = s.GetArgStr();
			GETNONWHITESPACE(pszVerbArg);
			if (*pszVerbArg == '\0')
			{
				UpdateDir(dynamic_cast<CObjBase*>(pCharSrc));
				return true;
			}
			else if (IsStrNumeric(pszVerbArg))
			{
				CObjBase* pTowards = CUID::ObjFindFromUID(s.GetArgVal());
				if (pTowards != nullptr)
				{
					UpdateDir(pTowards);
					return true;
				}
			}
			else
			{
                CPointMap pt;
				pt.Read(s.GetArgStr());
				if (pt.IsValidPoint())
				{
					UpdateDir(pt);
					return true;
				}
			}
			return false;
		}
		case CHV_FIXWEIGHT:
			FixWeight();
			break;
		case CHV_FORGIVE:
			Jail( pSrc, false, 0 );
			break;
		case CHV_GOCHAR:	// uid
			return TeleportToObj( 1, s.GetArgStr());
		case CHV_GOCHARID:
			return TeleportToObj( 3, s.GetArgStr());
		case CHV_GOCLI:	// enum clients
			return TeleportToCli( 1, s.GetArgVal());
		case CHV_GOITEMID:
			return TeleportToObj( 4, s.GetArgStr());
		case CHV_GONAME:
			return TeleportToObj( 0, s.GetArgStr());
		case CHV_GO:
			return Spell_Teleport( g_Cfg.GetRegionPoint(s.GetArgStr()), true, false );
		case CHV_GOSOCK:	// sockid
			return TeleportToCli( 0, s.GetArgVal());
		case CHV_GOTYPE:
			return TeleportToObj( 2, s.GetArgStr());
		case CHV_GOUID:	// uid
			if ( s.HasArgs() )
			{
				const CObjBaseTemplate * pObj = CUID::ObjFindFromUID(s.GetArgVal());
				if ( pObj == nullptr )
					return false;
				pObj = pObj->GetTopLevelObj();
				Spell_Teleport( pObj->GetTopPoint(), true, false );
				return true;
			}
			return false;
		case CHV_HEAR:
			// NPC will hear this command but no-one else.
			if ( m_pPlayer )
				SysMessage(s.GetArgStr());
			else
				NPC_OnHear(s.GetArgStr(), pSrc->GetChar());
			break;
		case CHV_HUNGRY:	// How hungry are we ?
			if ( pCharSrc )
			{
				tchar *z = Str_GetTemp();
				if ( pCharSrc == this )
					snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_SELF), Food_GetLevelMessage( false, false ));
				else
					snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_FOOD_LVL_OTHER), GetName(), Food_GetLevelMessage( false, false ));
				pCharSrc->ObjMessage(z, this);
			}
			break;
		case CHV_INVIS:
			if ( pSrc )
			{
                _uiStatFlag = s.GetArgLLFlag( _uiStatFlag, STATF_INSUBSTANTIAL );
				UpdateMode(nullptr, true);
				if ( IsStatFlag(STATF_INSUBSTANTIAL) )
				{
					if ( IsClientActive() )
						GetClientActive()->addBuff(BI_HIDDEN, 1075655, 1075656);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVIS_ON));
				}
				else
				{
					if ( IsClientActive() && !IsStatFlag(STATF_HIDDEN) )
						GetClientActive()->removeBuff(BI_HIDDEN);
					if ( IsSetOF(OF_Command_Sysmsgs) )
						pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVIS_OFF));
				}
			}
			break;
		case CHV_INVUL:
			if ( pSrc )
			{
                _uiStatFlag = s.GetArgLLFlag( _uiStatFlag, STATF_INVUL );
				NotoSave_Update();
				if ( IsSetOF( OF_Command_Sysmsgs ) )
					pSrc->SysMessage( IsStatFlag( STATF_INVUL )? g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVUL_ON) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_INVUL_OFF) );
			}
			break;
		case CHV_JAIL:	// i am being jailed
			Jail( pSrc, true, s.GetArgVal() );
			break;
		case CHV_KILL:
			{
				Effect( EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc );
				OnTakeDamage( 10000, pCharSrc, DAMAGE_GOD );
				Stat_SetVal( STAT_STR, 0 );
				g_Log.Event( LOGL_EVENT|LOGM_KILLS|LOGM_GM_CMDS, "'%s' was KILLed by '%s'\n", GetName(), pSrc->GetName());
			}
			break;
		case CHV_MAKEITEM:
		{
			tchar *psTmp = Str_GetTemp();
			Str_CopyLimitNull( psTmp, s.GetArgRaw(), STR_TEMPLENGTH );
			GETNONWHITESPACE( psTmp );
			tchar * ttVal[2];
			int iTmp = 1;
			int iArg = Str_ParseCmds( psTmp, ttVal, CountOf( ttVal ), " ,\t" );
			if (!iArg)
			{
				return false;
			}
			if ( iArg == 2 )
			{
				if ( IsDigit( ttVal[1][0] ) )
					iTmp = Str_ToI( ttVal[1] );
			}
			//DEBUG_ERR(( "CHV_MAKEITEM iTmp is %d, arg was %s\n",iTmp,psTmp ));

			if ( IsClientActive() )
				m_Act_UID = m_pClient->m_Targ_UID;

			return Skill_MakeItem(
				(ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ttVal[0] )),
				m_Act_UID, SKTRIG_START, false, iTmp );
		}

		case CHV_MOUNT:
			{
				CChar *pChar = CUID::CharFindFromUID(s.GetArgVal());
				if ( pChar )
					Horse_Mount(pChar);
			}
			break;

		case CHV_NEWBIESKILL:
			{
				CResourceLock t;
				if ( !g_Cfg.ResourceLock( t, RES_NEWBIE, s.GetArgStr()) )
					return false;
				ReadScriptReduced(t);
			}
			break;

		case CHV_NEWGOLD:
        {
            // Usage: NEWGOLD amount, pile(1: the new gold is stacked on the existing pile in the pack; 0: stacked in a new pile)
            int64 piCmd[2];
            int iQty = Str_ParseCmds(s.GetArgRaw(), piCmd, CountOf(piCmd));
            if (iQty < 1)
                return false;

            int64 iGold = piCmd[0];
            if (iGold <= 0)
                return false;
            if (iGold > INT32_MAX)
                iGold = INT32_MAX;

            const bool fStackNewPile = (iQty >= 2) ? !((bool)piCmd[1]) : true;
			AddGoldToPack((int)iGold, GetPackSafe(), fStackNewPile);
        }
			break;

		case CHV_NEWLOOT:
			{
				if ( m_pNPC && !m_pPlayer && !IsStatFlag(STATF_CONJURED) )
				{
					CItem *pItem = CItem::CreateHeader(s.GetArgStr(), nullptr, false, this);
					if ( !pItem )
                    {
						g_World.m_uidNew.ClearUID();
                    }
					else
					{
                        pItem->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
                        pItem->_iCreatedResScriptLine = s.m_iLineNum;

						ItemEquip(pItem);
						g_World.m_uidNew = pItem->GetUID();
					}
				}
			} break;
		case CHV_NOTOCLEAR:
			NotoSave_Clear();
			break;
		case CHV_NOTOUPDATE:
			NotoSave_Update();
			break;

        case CHV_OWNER:
        {
            if (!s.HasArgs())   // If there are no args, direct call on NPC_PetSetOwner.
                return NPC_PetSetOwner(pCharSrc);

			CChar * pChar = CUID::CharFindFromUID(s.GetArgDWVal()); // otherwise we try to run it from the CChar with the given UID.
            if (pChar)
                return pChar->NPC_PetSetOwner(this);
            return false;   // Something went wrong, giving a warning of it.
        }

		case CHV_PACK:
			if ( pCharSrc == nullptr || ! pCharSrc->IsClientActive())
				return false;
			pCharSrc->m_pClient->addBankOpen( this, LAYER_PACK ); // Send Backpack (with items)
			break;

		case CHV_POISON:
		{
			int iSkill = s.GetArgVal();
			int iTicks = iSkill / 50;
			int64 piCmd[2];
			if (Str_ParseCmds(s.GetArgRaw(), piCmd, CountOf(piCmd)) > 1)
				iTicks = (int)(piCmd[1]);

			SetPoison(iSkill, iTicks, pSrc->GetChar());
		}
		break;

		case CHV_POLY:	// result of poly spell script choice. (casting a spell)
		{
			const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(SPELL_Polymorph);
			if ( !pSpellDef )
				return false;

			m_atMagery.m_iSpell = SPELL_Polymorph;
			m_atMagery.m_iSummonID = (CREID_TYPE)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));
			m_Act_UID = GetUID();
			m_Act_Prv_UID = GetUID();

			if ( m_pClient && IsSetMagicFlags(MAGICF_PRECAST) && !pSpellDef->IsSpellType(SPELLFLAG_NOPRECAST) )
			{
				Spell_CastDone();
				break;
			}

			int skill;
			if ( !pSpellDef->GetPrimarySkill(&skill, nullptr) )
				return false;

			Skill_Start((SKILL_TYPE)skill);
			break;
		}

		case CHV_PRIVSET:
			return SetPrivLevel( pSrc, s.GetArgStr());

		case CHV_DESTROY:	// remove this char from the world and bypass trigger's return value.
		case CHV_REMOVE:	// remove this char from the world instantly.
			if ( m_pPlayer )
			{
				if ( s.GetArgRaw()[0] != '1' || pSrc->GetPrivLevel() < PLEVEL_Admin )
				{
					pSrc->SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_CMD_REMOVE_PLAYER) );
					return false;
				}
				if ( IsClientActive() )
					GetClientActive()->addObjectRemove(this);
			}
			Delete((index == CHV_DESTROY));
			break;
		case CHV_RESURRECT:
			{
				if ( !s.GetArgVal() )
					return OnSpellEffect( SPELL_Resurrection, pCharSrc, 1000, nullptr );
				else
					return Spell_Resurrection( nullptr, pCharSrc, true );
			}
		case CHV_REVEAL:
			Reveal( s.GetArgDWVal());
			break;
		case CHV_SALUTE:	//	salute to player
		{
			if (s.HasArgs())
				UpdateDir( CUID::ObjFindFromUID(s.GetArgVal()) );
			UpdateAnimate(ANIM_SALUTE);
			break;
		}
		case CHV_SKILL:
			Skill_Start( g_Cfg.FindSkillKey( s.GetArgStr()));
			break;
		case CHV_SKILLGAIN:
			{
				if ( s.HasArgs() )
				{
					tchar * ppArgs[2];
					if ( Str_ParseCmds( s.GetArgRaw(), ppArgs, CountOf( ppArgs )) > 0 )
					{
						SKILL_TYPE iSkill = g_Cfg.FindSkillKey( ppArgs[0] );
						if ( iSkill == SKILL_NONE )
							return false;
						Skill_Experience( iSkill, Exp_GetVal( ppArgs[1] ));
					}
				}
			}
			return true;
		case CHV_SLEEP:
			SleepStart( s.GetArgVal() != 0 );
			break;
		case CHV_SUICIDE:
			Memory_ClearTypes( MEMORY_FIGHT ); // Clear the list of people who get credit for your death
			UpdateAnimate( ANIM_SALUTE );
			Stat_SetVal( STAT_STR, 0 );
			break;
		case CHV_SUMMONCAGE: // i just got summoned
			if ( pCharSrc != nullptr )
			{
				// Let's make a cage to put the player in
				ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, "i_multi_cage" ));
				if ( id < 0 )
					return false;
				CItemMulti * pItem = dynamic_cast <CItemMulti*>( CItem::CreateBase( id ));
				if ( pItem == nullptr )
					return false;
				CPointMap pt = pCharSrc->GetTopPoint();
				pt.MoveN( pCharSrc->m_dirFace, 3 );
				pItem->MoveToDecay( pt, 10*60* MSECS_PER_SEC);	// make the cage vanish after 10 minutes.
				pItem->Multi_Setup( nullptr, UID_CLEAR );
				Spell_Teleport( pt, true, false );
				break;
			}
			return false;
		case CHV_SUMMONTO:	// i just got summoned
			if ( !pCharSrc )
				return false;
			Spell_Teleport( pCharSrc->GetTopPoint(), true, false );
			break;
		case CHV_SMSG:
		case CHV_SMSGL:
		case CHV_SMSGLEX:
		case CHV_SMSGU:
		case CHV_SYSMESSAGE:
		case CHV_SYSMESSAGELOC:
		case CHV_SYSMESSAGELOCEX:
		case CHV_SYSMESSAGEUA:
			// just eat this if it's not a client.
			break;
		case CHV_TARGETCLOSE:
			if ( IsClientActive() )
				GetClientActive()->addTargetCancel();
			break;
		case CHV_UNDERWEAR:
			if ( ! IsPlayableCharacter())
				return false;
			SetHue( GetHue() ^ HUE_UNDERWEAR /*, false, pSrc*/ ); //call @Dye on underwear?
			UpdateMode();
			break;
		case CHV_UNEQUIP:	// uid
			return ItemBounce( CUID::ItemFindFromUID(s.GetArgVal()) );
		case CHV_WHERE:
			if ( pCharSrc )
			{
				tchar *z = Str_GetTemp();
				if ( m_pArea )
				{
					if (m_pArea->GetResourceID().IsUIDItem())  // Inside a multi region
					{
						snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
					}
					else
					{
						const CRegion * pRoom = GetTopPoint().GetRegion( REGION_TYPE_ROOM );
						if ( ! pRoom )
							snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_AREA), m_pArea->GetName(), GetTopPoint().WriteUsed());
						else
							snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE_ROOM), m_pArea->GetName(), pRoom->GetName(), GetTopPoint().WriteUsed());
					}
				}
				else
				{
					snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_MSG_WHERE), GetTopPoint().WriteUsed());
				}
				pCharSrc->ObjMessage(z, this);
			}
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CChar::OnTriggerSpeech( bool bIsPet, lpctstr pszText, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CChar::OnTriggerSpeech");

	lpctstr pszName;

	if ( bIsPet && !g_Cfg.m_sSpeechPet.IsEmpty() )
		pszName = static_cast<lpctstr>(g_Cfg.m_sSpeechPet);
	else if ( !bIsPet && !g_Cfg.m_sSpeechSelf.IsEmpty() )
		pszName = static_cast<lpctstr>(g_Cfg.m_sSpeechSelf);
	else
		goto lbl_cchar_ontriggerspeech;

	{
		CScriptObj * pDef = g_Cfg.ResourceGetDefByName( RES_SPEECH, pszName );
		if ( pDef )
		{
			CResourceLink * pLink	= dynamic_cast <CResourceLink *>( pDef );
			if ( pLink )
			{
				CResourceLock	s;
				if ( pLink->ResourceLock(s) && pLink->HasTrigger(XTRIG_UNKNOWN) )
				{
					TRIGRET_TYPE iRet = OnHearTrigger(s, pszText, pSrc, mode, wHue);
					if ( iRet == TRIGRET_RET_TRUE )
						return true;
					else if ( iRet == TRIGRET_RET_HALFBAKED )
						return false;
				}
				else
					DEBUG_ERR(("TriggerSpeech: couldn't run script for speech %s\n", pszName));
			}
			else
				DEBUG_ERR(("TriggerSpeech: couldn't find speech %s\n", pszName));
		}
		else
			DEBUG_ERR(("TriggerSpeech: couldn't find speech resource %s\n", pszName));
	}


lbl_cchar_ontriggerspeech:
	if ( bIsPet )
		return false;

	if ( !m_pPlayer )
		return false;

	if (!m_pPlayer->m_Speech.empty())
	{
		for ( size_t i = 0; i < m_pPlayer->m_Speech.size(); ++i )
		{
			CResourceLink * pLinkDSpeech = m_pPlayer->m_Speech[i].GetRef();
			if ( !pLinkDSpeech )
				continue;

			CResourceLock sDSpeech;
			if ( !pLinkDSpeech->ResourceLock(sDSpeech) )
				continue;

			TRIGRET_TYPE iRet = OnHearTrigger( sDSpeech, pszText, pSrc, mode, wHue );
			if ( iRet == TRIGRET_RET_TRUE )
				return true;
			else if ( iRet == TRIGRET_RET_HALFBAKED )
				break;
		}
	}

	return false;
}

// Gaining exp
uint Calc_ExpGet_Exp(uint level)
{
    if (level <= 1)
        return 0;
    if (g_Cfg.m_iLevelMode == LEVEL_MODE_LINEAR)
    {
        return ((level-1) * g_Cfg.m_iLevelNextAt);
    }
    else // if (g_Cfg.m_iLevelMode == LEVEL_MODE_DOUBLE) // default
    {
        uint exp = 0;
        for ( uint lev = 1; lev < level; ++lev )
        {
            exp += (g_Cfg.m_iLevelNextAt * (lev + 1));
        }
        return exp;
    }
}

// Increasing level
uint Calc_ExpGet_Level(uint exp)
{
	if (g_Cfg.m_iLevelNextAt < 1)	//Must do this check in case ini's LevelNextAt is not set or server will freeze because exp will never decrease in the while.
    {
        g_Log.EventError("Invalid LextNevelAt value.\n");
		return 0;
    }

    if (g_Cfg.m_iLevelMode == LEVEL_MODE_LINEAR)
    {
        return 1 + (exp / g_Cfg.m_iLevelNextAt);
    }
    else // if (g_Cfg.m_iLevelMode == LEVEL_MODE_DOUBLE) // default
    {
        uint level = 0;
        uint iNextLevelReq = 0;
        while (exp >= iNextLevelReq)
        {
            // reduce xp and raise level
            exp -= iNextLevelReq;
            ++level;

            // calculate requirement for next level
            iNextLevelReq = (g_Cfg.m_iLevelNextAt * (level+1));
        }
        return level;
    }
}

void CChar::ChangeExperience(llong delta, CChar *pCharDead)
{
	ADDTOCALLSTACK("CChar::ChangeExperience");

	if (!g_Cfg.m_iExperienceMode)
		return;

	static uint const keyWords[] =
	{
		DEFMSG_MSG_EXP_CHANGE_1,		// 0
		DEFMSG_MSG_EXP_CHANGE_2,
		DEFMSG_MSG_EXP_CHANGE_3,
		DEFMSG_MSG_EXP_CHANGE_4,
		DEFMSG_MSG_EXP_CHANGE_5,
		DEFMSG_MSG_EXP_CHANGE_6,		// 5
		DEFMSG_MSG_EXP_CHANGE_7,
		DEFMSG_MSG_EXP_CHANGE_8
	};

	if (delta != 0 || pCharDead)//	zero call will sync the exp level
	{
		if (delta < 0)
		{
			if (!(g_Cfg.m_iExperienceMode&EXP_MODE_ALLOW_DOWN))	// do not allow changes to minus
				return;
			// limiting delta to current level? check if delta goes out of level
			if (g_Cfg.m_bLevelSystem && g_Cfg.m_iExperienceMode&EXP_MODE_DOWN_NOLEVEL)
			{
				uint exp = Calc_ExpGet_Exp(m_level);
				if (delta + m_exp < exp)
					delta = (llong)exp - m_exp;
			}
		}

		if ((g_Cfg.m_iDebugFlags & DEBUGF_EXP) && (delta != 0))
		{
			g_Log.EventDebug("%s %s experience change (was %u, delta %lld, now %u)\n",
				(m_pNPC ? "NPC" : "Player"), GetName(), m_exp, delta, (uint)(m_exp + delta));
		}

		bool fShowMsg = (m_pClient != nullptr);

		if (IsTrigUsed(TRIGGER_EXPCHANGE))
		{
			CScriptTriggerArgs args(delta, fShowMsg);
			args.m_pO1 = pCharDead;
			if (OnTrigger(CTRIG_ExpChange, this, &args) == TRIGRET_RET_TRUE)
				return;
			delta = args.m_iN1;
			fShowMsg = (args.m_iN2 != 0);
		}

		// Do not allow an underflow due to negative Exp Change.
		if( delta < 0 && m_exp < abs(delta) )
			m_exp = 0;
		else
			m_exp = (uint)(m_exp + delta);

		if (m_pClient && fShowMsg && delta)
		{
			int iWord = 0;
			llong absval = abs(delta);
			llong maxval = (g_Cfg.m_bLevelSystem && g_Cfg.m_iLevelNextAt) ? maximum(g_Cfg.m_iLevelNextAt, 1000) : 1000;

			if (absval >= maxval)				// 100%
				iWord = 7;
			else if (absval >= (maxval * 2) / 3)//  66%
				iWord = 6;
			else if (absval >= maxval / 2)		//  50%
				iWord = 5;
			else if (absval >= maxval / 3)		//  33%
				iWord = 4;
			else if (absval >= maxval / 5)		//  20%
				iWord = 3;
			else if (absval >= maxval / 7)		//  14%
				iWord = 2;
			else if (absval >= maxval / 14)		//   7%
				iWord = 1;

			m_pClient->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_0),
				(delta > 0) ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_GAIN) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_CHANGE_LOST),
				g_Cfg.GetDefaultMsg(keyWords[iWord]));
		}
	}

	if (g_Cfg.m_bLevelSystem)
	{
		llong level = Calc_ExpGet_Level(m_exp);

		if (level != m_level)
		{
			delta = level - m_level;

			bool fShowMsg = (m_pClient != nullptr);

			if (IsTrigUsed(TRIGGER_EXPLEVELCHANGE))
			{
				CScriptTriggerArgs args(delta);
				if (OnTrigger(CTRIG_ExpLevelChange, this, &args) == TRIGRET_RET_TRUE)
					return;
				delta = args.m_iN1;
				fShowMsg = (args.m_iN2 != 0);
			}

			level = delta + m_level;

			// Prevent integer underflow due to negative level change
			if (delta < 0 && abs(delta) > m_level)
			{
				level = 0;
			}

			if (g_Cfg.m_iDebugFlags & DEBUGF_LEVEL)
			{
				g_Log.EventDebug("%s %s level change (was %u, delta %lld, now %u)\n",
					(m_pNPC ? "NPC" : "Player"), GetName(), m_level, delta, (uint)level);
			}
			m_level = (uint)level;

			if (m_pClient && fShowMsg)
			{
				m_pClient->SysMessagef(
					((abs(delta) == 1) ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_0)    : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_1)),
					((delta > 0)       ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_GAIN) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_EXP_LVLCHANGE_LOST))
				);
			}
		}
	}
}

// returns <SkillTotal>
uint CChar::GetSkillTotal(int what, bool how)
{
	ADDTOCALLSTACK("CChar::GetSkillTotal");
	uint	uiTotal = 0;
	ushort	uiBase;

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; ++i )
	{
		uiBase = Skill_GetBase((SKILL_TYPE)i);
		if ( how )
		{
			if ( what < 0 )
			{
				if ( uiBase >= -what )
				    continue;
			}
			else if ( uiBase < what )
				continue;
		}
		else
		{
			// check group flags
			const CSkillDef * pSkill = g_Cfg.GetSkillDef((SKILL_TYPE)i);
			if ( !pSkill )
				continue;
			if ( !( pSkill->m_dwGroup & what ) )
				continue;
		}
		uiTotal += uiBase;
	}

	return uiTotal;
}

