// Actions specific to an NPC.

#include "../../common/sphere_library/CSRand.h"
#include "../../common/CExpression.h"
#include "../clients/CClient.h"
#include "../components/CCSpawn.h"
#include "../items/CItemContainer.h"
#include "../items/CItemMemory.h"
#include "../items/CItemVendable.h"
#include "../CServerTime.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

void CChar::NPC_OnPetCommand( bool fSuccess, CChar * pMaster )
{
	ADDTOCALLSTACK("CChar::NPC_OnPetCommand");
	ASSERT(m_pNPC);

	if ( !g_Cfg.m_sSpeechPet.IsEmpty() )
		return;			// if using a custom SPEECH and still want sound, just put BARK on each command

	if ( !CanSee( pMaster ) )
		return;

	// i take a command from my master.
	if ( NPC_CanSpeak() )
		Speak( fSuccess ? g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_SUCCESS ) : g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_FAILURE ) );
	else
		SoundChar( fSuccess ? CRESND_NOTICE : CRESND_IDLE );
}

enum PC_TYPE
{
	PC_ATTACK,
	PC_BOUGHT,
	PC_CASH,
	PC_COME,
	PC_DROP,
	PC_DROP_ALL,
	PC_EQUIP,
	PC_FOLLOW,
	PC_FOLLOW_ME,
	PC_FRIEND,
	PC_GO,
	PC_GUARD,
	PC_GUARD_ME,
	PC_KILL,
	PC_PRICE,
	PC_RELEASE,
	PC_SAMPLES,
	PC_SPEAK,
	PC_STATUS,
	PC_STAY,
	PC_STOCK,
	PC_STOP,
	PC_TRANSFER,
	PC_UNFRIEND,
	PC_QTY
};

bool CChar::NPC_OnHearPetCmd( lpctstr pszCmd, CChar *pSrc, bool fAllPets )
{
	ADDTOCALLSTACK("CChar::NPC_OnHearPetCmd");
	ASSERT(m_pNPC);
	// This should just be another speech block !!!

	// We own this char (pet or hireling)
	// pObjTarget = the m_ActTarg has been set for them to attack.
	// RETURN:
	//  true = we understand this. though we may not do what we are told.
	//  false = this is not a command we know.
	//  if ( GetTargMode() == CLIMODE_TARG_PET_CMD ) it needs a target.

	if ( !pSrc->IsClientActive() || Can(CAN_C_STATUE))
		return false;

	m_fIgnoreNextPetCmd = false;	// We clear this incase it's true from previous pet cmds.
	TALKMODE_TYPE mode = TALKMODE_SAY;
	if ( OnTriggerSpeech(true, pszCmd, pSrc, mode) )
	{
		m_fIgnoreNextPetCmd = !fAllPets;
		return true;
	}
	if ( (m_pNPC->m_Brain == NPCBRAIN_BERSERK) && !pSrc->IsPriv(PRIV_GM) )
		return false;	// Berserk npcs do not listen to any command (except if src is a GM)

	static lpctstr const sm_Pet_table[PC_QTY+1] =
	{
		"ATTACK",
		"BOUGHT",
		"CASH",
		"COME",
		"DROP",	// "GIVE" ?
		"DROP ALL",
		"EQUIP",
		"FOLLOW",
		"FOLLOW ME",
		"FRIEND",
		"GO",
		"GUARD",
		"GUARD ME",
		"KILL",
		"PRICE",	// may have args ?
		"RELEASE",
		"SAMPLES",
		"SPEAK",
		"STATUS",
		"STAY",
		"STOCK",
		"STOP",
		"TRANSFER",
		"UNFRIEND",
        nullptr
	};

	PC_TYPE iCmd = (PC_TYPE)FindTableSorted(pszCmd, sm_Pet_table, ARRAY_COUNT(sm_Pet_table) - 1);
	if ( iCmd < 0 )
	{
		if ( !strnicmp(pszCmd, sm_Pet_table[PC_PRICE], 5) )
			iCmd = PC_PRICE;
		else
			return false;
	}

	// Check if pSrc can use the commands on this pet
	switch ( iCmd )
	{
		case PC_FOLLOW:
		case PC_STAY:
		case PC_STOP:
		{
			// If it's a conjured monster, it can be controlled (accepts commands) only by its summoner.
			// This is just a security check, because conjured NPCs "friend" command is blocked.
			if  (IsStatFlag(STATF_CONJURED))
			{
				if (NPC_IsOwnedBy(pSrc, true))
					break;
				else
					return false;
			}
			// Pet friends can use only these commands
			if ( Memory_FindObjTypes(pSrc, MEMORY_FRIEND) )
				break;
		}
		default:
		{
			// All others commands are avaible only to pet owner
			if ( !NPC_IsOwnedBy(pSrc, true) )
				return true;
		}
	}

	if ( IsStatFlag(STATF_DEAD) )
	{
		// Bonded NPCs still placed on world even when dead.
		// They can listen to commands, but not to these commands below
		if ( (iCmd == PC_GUARD) || (iCmd == PC_GUARD_ME) || (iCmd == PC_ATTACK) || (iCmd == PC_KILL) || (iCmd == PC_TRANSFER) || (iCmd == PC_DROP) || (iCmd == PC_DROP_ALL) )
			return true;
	}

	bool bTargAllowGround = false;
	bool bCheckCrime = false;
	lpctstr pTargPrompt = nullptr;
	CCharBase *pCharDef = Char_GetDef();

	switch ( iCmd )
	{
		case PC_ATTACK:
		case PC_KILL:
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_ATT);
			bCheckCrime = true;
			break;

        case PC_GUARD_ME:
            m_Act_UID = pSrc->GetUID();
            Skill_Start(NPCACT_GUARD_TARG);
            break;

		case PC_COME:
		case PC_FOLLOW_ME:
			m_Act_UID = pSrc->GetUID();
			Skill_Start(NPCACT_FOLLOW_TARG);
			break;

		case PC_FOLLOW:
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FOLLOW);
			break;

		case PC_FRIEND:
			if ( IsStatFlag(STATF_CONJURED) )
			{
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FRIEND_SUMMONED));
				return false;
			}
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FRIEND);
			break;

		case PC_UNFRIEND:
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_UNFRIEND);
			break;

		case PC_GO:
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_GO);
			bTargAllowGround = true;
			break;

		case PC_GUARD:
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_GUARD);
			bCheckCrime = true;
			break;

		case PC_STAY:
		case PC_STOP:
			Skill_Start(NPCACT_STAY);
			break;

		case PC_TRANSFER:
			if ( IsStatFlag(STATF_CONJURED) )
			{
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_TRANSFER_SUMMONED));
				return false;
			}
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_TRANSFER);
			break;

		case PC_RELEASE:
			if (IsValidResourceDef("d_pet_release"))
				pSrc->m_pClient->Dialog_Setup(CLIMODE_DIALOG, g_Cfg.ResourceGetIDType(RES_DIALOG, "d_pet_release"), 0, this);
			else
				NPC_PetRelease();
			break;

		case PC_DROP:
		{
			// Drop backpack items on ground
			// NOTE: This is also called on pet release
			CItemContainer *pPack = GetPack();
			if ( pPack )
			{
				pPack->ContentsDump(GetTopPoint(), ATTR_OWNED);
				break;
			}
			if ( NPC_CanSpeak() )
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_CARRYNOTHING));
			return true;
		}

		case PC_DROP_ALL:
			DropAll(nullptr, ATTR_OWNED);
			break;

		case PC_SPEAK:
			NPC_OnPetCommand(true, pSrc);
			return true;

		case PC_EQUIP:
			ItemEquipWeapon(false);
			ItemEquipArmor(false);
			break;

		case PC_STATUS:
		{
			if ( !NPC_CanSpeak() )
				break;

			uint iWage = pCharDef->GetHireDayWage();
			CItemContainer *pBank = GetBank();
			tchar *pszMsg = Str_GetTemp();
			if ( NPC_IsVendor() )
			{
				CItemContainer *pCont = GetBank(LAYER_VENDOR_STOCK);
				tchar *pszTemp1 = Str_GetTemp();
				tchar *pszTemp2 = Str_GetTemp();
				tchar *pszTemp3 = Str_GetTemp();
				if ( iWage )
				{
					snprintf(pszTemp1, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_1), pBank->m_itEqBankBox.m_Check_Amount);
					snprintf(pszTemp2, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_2), pBank->m_itEqBankBox.m_Check_Amount / iWage);
					snprintf(pszTemp3, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_3), (int)(pCont->GetContentCount()));
				}
				else
				{
					snprintf(pszTemp1, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_1), pBank->m_itEqBankBox.m_Check_Amount);
					snprintf(pszTemp2, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_4), pBank->m_itEqBankBox.m_Check_Restock, pBank->GetTimerAdjusted() / 60);
					snprintf(pszTemp3, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_STAT_GOLD_3), (int)(pCont->GetContentCount()));
				}
				snprintf(pszMsg, Str_TempLength(), "%s %s %s", pszTemp1, pszTemp2, pszTemp3);
			}
			else if ( iWage )
			{
				snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_DAYS_LEFT), pBank->m_itEqBankBox.m_Check_Amount / iWage);
			}
			Speak(pszMsg);
			return true;
		}

		case PC_CASH:
		{
			// Give up my cash total.
			if ( !NPC_IsVendor() )
				return false;

			CItemContainer *pBank = GetBank();
			if ( pBank )
			{
				const uint uiWage = pCharDef->GetHireDayWage();
				tchar *pszMsg = Str_GetTemp();
				if ( pBank->m_itEqBankBox.m_Check_Amount > uiWage )
				{
					snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_GETGOLD_1), pBank->m_itEqBankBox.m_Check_Amount - uiWage);
					pSrc->AddGoldToPack(pBank->m_itEqBankBox.m_Check_Amount - uiWage);
					pBank->m_itEqBankBox.m_Check_Amount = uiWage;
				}
				else
					snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_GETGOLD_2), pBank->m_itEqBankBox.m_Check_Amount);
				Speak(pszMsg);
			}
			return true;
		}

		case PC_BOUGHT:
			if ( !NPC_IsVendor() )
				return false;
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_ITEMS_BUY));
			pSrc->GetClientActive()->addBankOpen(this, LAYER_VENDOR_EXTRA);
			break;

		case PC_PRICE:
			if ( !NPC_IsVendor() )
				return false;
			pTargPrompt = g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_SETPRICE);
			break;

		case PC_SAMPLES:
			if ( !NPC_IsVendor() )
				return false;
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_ITEMS_SAMPLE));
			pSrc->GetClientActive()->addBankOpen(this, LAYER_VENDOR_BUYS);
			break;

		case PC_STOCK:
			// Magic restocking container.
			if ( !NPC_IsVendor() )
				return false;
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_ITEMS_SELL));
			pSrc->GetClientActive()->addBankOpen(this, LAYER_VENDOR_STOCK);
			break;

		default:
			return false;
	}

	if ( pTargPrompt )
	{
		pszCmd += strlen(sm_Pet_table[iCmd]);
		GETNONWHITESPACE(pszCmd);
		pSrc->m_pClient->m_tmPetCmd.m_iCmd = iCmd;
		pSrc->m_pClient->m_tmPetCmd.m_fAllPets = fAllPets;
		pSrc->m_pClient->m_Targ_UID = GetUID();
		pSrc->m_pClient->m_Targ_Text = pszCmd;
		pSrc->m_pClient->addTarget(CLIMODE_TARG_PET_CMD, pTargPrompt, bTargAllowGround, bCheckCrime);
		return true;
	}

	// Make some sound to confirm we heard it
	NPC_OnPetCommand(true, pSrc);
	return true;
}

bool CChar::NPC_OnHearPetCmdTarg( int iCmd, CChar *pSrc, CObjBase *pObj, const CPointMap &pt, lpctstr pszArgs )
{
	ADDTOCALLSTACK("CChar::NPC_OnHearPetCmdTarg");
	ASSERT(m_pNPC);
	// Pet commands that required a target.
	// First NPC_OnHearPetCmd is called, then a target is requested. After you select the target object, then this function is called.
	// It's wise to check again if we can use this command (it's already done in NPC_OnHearPetCmd), because something (like the ownership)
	// could have changed after the pet heared the command but before we select the target.

	if ( m_fIgnoreNextPetCmd )
	{
		m_fIgnoreNextPetCmd = false;
		return false;
	}
	if ( (m_pNPC->m_Brain == NPCBRAIN_BERSERK) && !(pSrc->IsPriv(PRIV_GM) && (pSrc->GetPrivLevel() > GetPrivLevel())) )
		return false;	// Berserk npcs do not listen to any command (except if src is a GM)

	// Check if pSrc can use the commands on this pet
	switch ( iCmd )
	{
		case PC_FOLLOW:
		case PC_STAY:
		case PC_STOP:
			// If it's a conjured monster, it can be controlled (accepts commands) only by its summoner.
			// This is just a security check, because conjured NPCs "friend" command is blocked.
			if  (IsStatFlag(STATF_CONJURED))
			{
				if (NPC_IsOwnedBy(pSrc, true))
					break;
				else
					return false;
			}
			// Pet friends can use only these commands
			if ( Memory_FindObjTypes(pSrc, MEMORY_FRIEND) )
				break;
		case PC_TRANSFER:
			// Can't transfer ownership of a conjured monster, it can be controlled only by its summoner
			if (IsStatFlag(STATF_CONJURED))
			{	
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_TRANSFER_SUMMONED));
				return false;
			}
			break;
		default:
		{
			// All others commands are avaible only to pet owner
			if ( !NPC_IsOwnedBy(pSrc, true) )
				return false;
		}
	}

	if ( IsStatFlag(STATF_DEAD) )
	{
		// Bonded NPCs still placed on world even when dead.
		// They can listen to commands, but not to these commands below
		if ( (iCmd == PC_GUARD) || (iCmd == PC_GUARD_ME) || (iCmd == PC_ATTACK) || (iCmd == PC_KILL) || (iCmd == PC_TRANSFER) || (iCmd == PC_DROP) || (iCmd == PC_DROP_ALL) )
			return true;
	}

    bool fSuccess = false;
	CItem *pItemTarg = dynamic_cast<CItem *>(pObj);
	CChar *pCharTarg = dynamic_cast<CChar *>(pObj);

	switch ( iCmd )
	{
		case PC_ATTACK:
		case PC_KILL:
		{
			if ( !pCharTarg || pCharTarg == pSrc || pCharTarg == this )
				break;
            fSuccess = pCharTarg->OnAttackedBy(pSrc, true);	// we know who told them to do this.
            if ( fSuccess )
                fSuccess = Fight_Attack(pCharTarg, true);
			break;
		}

		case PC_FOLLOW:
			if ( !pCharTarg )
				break;
			m_Act_UID = pCharTarg->GetUID();
            fSuccess = Skill_Start(NPCACT_FOLLOW_TARG);
			break;

		case PC_FRIEND:
		{
			if ( !pCharTarg || !pCharTarg->m_pPlayer || (pCharTarg == pSrc) )
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_CONFUSED));
				break;
			}
			CItemMemory *pMemory = Memory_FindObjTypes(pCharTarg, MEMORY_FRIEND);
			if ( pMemory )
			{
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FRIEND_ALREADY));
				break;
			}
			pSrc->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FRIEND_SUCCESS1), GetName(), pCharTarg->GetName());
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_FRIEND_SUCCESS2), pSrc->GetName(), GetName());
			Memory_AddObjTypes(pCharTarg, MEMORY_FRIEND);

			m_Act_UID = pCharTarg->GetUID();
            fSuccess = Skill_Start(NPCACT_FOLLOW_TARG);
			break;
		}

		case PC_UNFRIEND:
		{
			if ( !pCharTarg || !pCharTarg->m_pPlayer )
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_CONFUSED));
				break;
			}
			CItemMemory *pMemory = Memory_FindObjTypes(pCharTarg, MEMORY_FRIEND);
			if ( !pMemory )
			{
				pSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_UNFRIEND_NOTFRIEND));
				break;
			}
			pSrc->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_UNFRIEND_SUCCESS1), GetName(), pCharTarg->GetName());
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_UNFRIEND_SUCCESS2), pSrc->GetName(), GetName());
			pMemory->Delete();

			m_Act_UID = pSrc->GetUID();
            fSuccess = Skill_Start(NPCACT_FOLLOW_TARG);
			break;
		}

		case PC_GO:
			if ( !pt.IsValidPoint() )
				break;
			m_Act_p = pt;
            fSuccess = Skill_Start(NPCACT_GOTO);
			break;

		case PC_GUARD:
			if ( !pCharTarg )
				break;
			pCharTarg->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_TARG_GUARD_SUCCESS), GetName());
			m_Act_UID = pCharTarg->GetUID();
            fSuccess = Skill_Start(NPCACT_GUARD_TARG);
			break;

		case PC_TRANSFER:
			if ( !pCharTarg || !pCharTarg->IsClientActive() )
				break;
			if ( IsSetOF(OF_PetSlots) )
			{
                short iFollowerSlots = GetFollowerSlots();
                if ( !pCharTarg->FollowersUpdate(this, iFollowerSlots, true) )
				{
					pSrc->SysMessageDefault(DEFMSG_PETSLOTS_TRY_TRANSFER);
					break;
				}
			}
            fSuccess = NPC_PetSetOwner( pCharTarg );
			break;

		case PC_PRICE:	// "PRICE" the vendor item.
			if ( !pItemTarg || !NPC_IsVendor() || !pSrc->IsClientActive() )
				break;
			if ( IsDigit(pszArgs[0]) )	// did they name a price
				return NPC_SetVendorPrice(pItemTarg, atoi(pszArgs));
			if ( !NPC_SetVendorPrice(pItemTarg, -1) )	// test if it can be priced
				return false;
			pSrc->m_pClient->addPromptConsole(CLIMODE_PROMPT_VENDOR_PRICE, g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SETPRICE_2), pItemTarg->GetUID(), GetUID());
			return true;

		default:
			break;
	}

	// Make some sound to confirm we heard it
    NPC_OnPetCommand(fSuccess, pSrc);
    return fSuccess;
}

void CChar::NPC_PetClearOwners()
{
	ADDTOCALLSTACK("CChar::NPC_PetClearOwners");
	ASSERT(m_pNPC);

	CChar * pOwner = NPC_PetGetOwner();
	Memory_ClearTypes(MEMORY_IPET|MEMORY_FRIEND);
	m_pNPC->m_bonded = 0;	// pets without owner cannot be bonded

	if ( NPC_IsVendor() )
	{
		StatFlag_Clear(STATF_INVUL);
		if ( pOwner )	// give back to NPC owner all the stuff we are trying to sell
		{
			CItemContainer * pBankVendor = GetBank();
			CItemContainer * pBankOwner = pOwner->GetBank();
			pOwner->AddGoldToPack( pBankVendor->m_itEqBankBox.m_Check_Amount, pBankOwner );
			pBankVendor->m_itEqBankBox.m_Check_Amount = 0;

			for ( size_t i = 0; i < ARRAY_COUNT(sm_VendorLayers); ++i )
			{
				CItemContainer * pCont = GetBank( sm_VendorLayers[i] );
				if ( !pCont )
					continue;

				for (CSObjContRec* pObjRec : pCont->GetIterationSafeCont())
				{
					CItem* pItem = static_cast<CItem*>(pObjRec);
					pBankOwner->ContentAdd(pItem);
				}
			}
		}
	}

	if ( IsStatFlag(STATF_RIDDEN) )
	{
		CChar *pCharRider = Horse_GetMountChar();
		if ( pCharRider )
			pCharRider->Horse_UnMount();
	}

	if ( pOwner && IsSetOF(OF_PetSlots) )
	{
        const short iFollowerSlots = GetFollowerSlots();
        pOwner->FollowersUpdate(this, -iFollowerSlots);
	}
}

bool CChar::NPC_PetSetOwner( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_PetSetOwner");
	//ASSERT(m_pNPC); // m_pNPC may not be set yet if this is a conjured creature.

	if ( m_pPlayer || !pChar || (pChar == this) )
		return false;

    CChar * pOwner = NPC_PetGetOwner();
	if ( pOwner == pChar )
		return false;

	m_ptHome.InitPoint();	// no longer homed
    NPC_PetClearOwners();	// clear previous owner before set the new owner
    CCSpawn* pSpawn = GetSpawn();
	if ( pSpawn )
		pSpawn->DelObj( GetUID() );

	Memory_AddObjTypes(pChar, MEMORY_IPET);
	NPC_Act_Follow();

	if ( NPC_IsVendor() )
	{
		// Clear my cash total.
		CItemContainer * pBank = GetBank();
		pBank->m_itEqBankBox.m_Check_Amount = 0;
		StatFlag_Set(STATF_INVUL);
	}

	if ( IsSetOF(OF_PetSlots) )
	{
        const short iFollowerSlots = GetFollowerSlots();
        pChar->FollowersUpdate(this, iFollowerSlots);
	}

	return true;
}

// Hirelings...

bool CChar::NPC_CheckHirelingStatus()
{
	ADDTOCALLSTACK("CChar::NPC_CheckHirelingStatus");
	ASSERT(m_pNPC);
	//  Am i happy at the moment ?
	//  If not then free myself.
	//
	// RETURN:
	//  true = happy.

	if ( ! IsStatFlag( STATF_PET ))
		return true;

	CCharBase * pCharDef = Char_GetDef();
	int64 iFoodConsumeRate = g_Cfg.m_iRegenRate[STAT_FOOD] / MSECS_PER_SEC;

    uint uiWage = pCharDef->GetHireDayWage();
    if ( ! uiWage || ! iFoodConsumeRate )
		return true;

	// I am hired for money not for food.
    uint uiPeriodWage = (uint)IMulDivLL( uiWage, iFoodConsumeRate, 24 * 60 * g_Cfg.m_iGameMinuteLength );
    if ( uiPeriodWage <= 0 )
        uiPeriodWage = 1;

	CItemContainer * pBank = GetBank();
    if ( pBank->m_itEqBankBox.m_Check_Amount > uiPeriodWage )
	{
        pBank->m_itEqBankBox.m_Check_Amount -= uiPeriodWage;
	}
	else
	{
		tchar* pszMsg = Str_GetTemp();
        snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_WAGE_COST ), uiWage);
		Speak(pszMsg);

		CChar * pOwner = NPC_PetGetOwner();
		if ( pOwner )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_HIRE_TIMEUP ) );

			CItem * pMemory = Memory_AddObjTypes( pOwner, MEMORY_SPEAK );
			if ( pMemory )
				pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_HIRE;

			NPC_PetDesert();
			return false;
		}

		// Some sort of strange bug to get here.
		Memory_ClearTypes( MEMORY_IPET );
		StatFlag_Clear( STATF_PET );
	}

	return true;
}

void CChar::NPC_OnHirePayMore( CItem * pGold, uint uiWage, bool fHire )
{
	ADDTOCALLSTACK("CChar::NPC_OnHirePayMore");
	ASSERT(m_pNPC);
	// We have been handed money.
	// similar to PC_STATUS

	CItemContainer	*pBank = GetBank();
	if ( !uiWage || !pBank )
		return;

	if ( pGold )
	{
		if ( fHire )
		{
			pBank->m_itEqBankBox.m_Check_Amount = 0;	// zero any previous balance.
		}

		pBank->m_itEqBankBox.m_Check_Amount += pGold->GetAmount();
		Sound( pGold->GetDropSound( nullptr ));
		pGold->Delete();
	}

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_HIRE_TIME), int(pBank->m_itEqBankBox.m_Check_Amount / uiWage));
	Speak(pszMsg);
}

bool CChar::NPC_OnHirePay( CChar * pCharSrc, CItemMemory * pMemory, CItem * pGold )
{
	ADDTOCALLSTACK("CChar::NPC_OnHirePay");
	ASSERT(m_pNPC);

	if ( !pCharSrc || !pMemory )
		return false;

    uint uiWage = Char_GetDef()->GetHireDayWage();
	if ( IsStatFlag( STATF_PET ))
	{
		if ( ! pMemory->IsMemoryTypes(MEMORY_IPET|MEMORY_FRIEND))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_EMPLOYED ) );
			return false;
		}
	}
	else
	{
		if ( uiWage <= 0 )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_NOT_FOR_HIRE ) );
			return false;
		}
        uiWage = (uint)pCharSrc->PayGold(this, uiWage, pGold, PAYGOLD_HIRE);
		if ( pGold->GetAmount() < uiWage )
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_NOT_ENOUGH ) );
			return false;
		}
		// NOTO_TYPE ?
		if ( pMemory->IsMemoryTypes( MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_NOT_WORK ) );
			return false;
		}

		// Put all my loot cash away.
		ContentConsume( CResourceID(RES_TYPEDEF,IT_GOLD), INT32_MAX, 0 );
		// Mark all my stuff ATTR_OWNED - i won't give it away.
		ContentAttrMod( ATTR_OWNED, true );

		NPC_PetSetOwner( pCharSrc );
	}

	pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;
	NPC_OnHirePayMore( pGold, uiWage, true );
	return true;
}

bool CChar::NPC_OnHireHear( CChar * pCharSrc )
{
	ADDTOCALLSTACK("CChar::NPC_OnHireHear");
	ASSERT(m_pNPC);

	CCharBase * pCharDef = Char_GetDef();
	uint uiWage = pCharDef->GetHireDayWage();
	if ( ! uiWage )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_NOT_FOR_HIRE ) );
		return false;
	}
    uiWage = (uint)pCharSrc->PayGold(this, uiWage, nullptr, PAYGOLD_HIRE);
	CItemMemory * pMemory = Memory_FindObj( pCharSrc );
	if ( pMemory )
	{
		if ( pMemory->IsMemoryTypes(MEMORY_IPET|MEMORY_FRIEND))
		{
			// Next gold i get goes toward hire.
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_HIRE;
			NPC_OnHirePayMore( nullptr, uiWage, false );
			return true;
		}
		if ( pMemory->IsMemoryTypes( MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY ))
		{
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_NOT_WORK ) );
			return false;
		}
	}
	if ( IsStatFlag( STATF_PET ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_EMPLOYED ) );
		return false;
	}

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, Str_TempLength(), g_Rand.GetVal(2) ?
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_HIRE_AMNT ) :
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_HIRE_RATE ), (int)uiWage );
	Speak(pszMsg);

	pMemory = Memory_AddObjTypes( pCharSrc, MEMORY_SPEAK );
	if ( pMemory )
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_HIRE;
	return true;
}

bool CChar::NPC_SetVendorPrice( CItem * pItem, int iPrice )
{
	ADDTOCALLSTACK("CChar::NPC_SetVendorPrice");
	ASSERT(m_pNPC);
	// player vendors.
	// CLIMODE_PROMPT_VENDOR_PRICE
	// This does not check who is setting the price if if it is valid for them to do so.

	if ( ! NPC_IsVendor())
		return false;

	if ( pItem == nullptr ||
		pItem->GetTopLevelObj() != this ||
		pItem->GetParent() == this )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_INV_ONLY ) );
		return false;
	}

	CItemVendable * pVendItem = dynamic_cast <CItemVendable *> (pItem);
	if ( pVendItem == nullptr )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_PET_CANTSELL ) );
		return false;
	}

	if ( iPrice < 0 )	// just a test.
		return true;

	tchar *pszMsg = Str_GetTemp();
	snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SETPRICE_1), pVendItem->GetName(), iPrice);
	Speak(pszMsg);

	pVendItem->SetPlayerVendorPrice( iPrice );
	return true;
}

void CChar::NPC_PetRelease()
{
	ADDTOCALLSTACK("CChar::NPC_PetRelease");
	if (!m_pNPC)
		return;

	if (IsStatFlag(STATF_CONJURED) || (m_pNPC->m_bonded && IsStatFlag(STATF_DEAD)))
	{
		Effect(EFFECT_XYZ, ITEMID_FX_TELE_VANISH, this, 10, 15);
		Sound(SOUND_TELEPORT);
		Delete();
		return;
	}

	if (Skill_GetActive() == NPCACT_RIDDEN)
	{
		CChar* pCharRider = Horse_GetMountChar();
		if (pCharRider)
			pCharRider->Horse_UnMount();
	}

	SoundChar(CRESND_NOTICE);
	if (Skill_GetActive() != NPCACT_RIDDEN)
		Skill_Start(SKILL_NONE);
	NPC_PetClearOwners();
	UpdatePropertyFlag();
}

void CChar::NPC_PetDesert()
{
	ADDTOCALLSTACK("CChar::NPC_PetDesert");
	ASSERT(m_pNPC);
	// If the monster has brain_berserk (i.e. energy vortex): when the player summons it, his CurFollower property rises.
	//	If the master attacks the berserk pet, the pet deserts him and the master's CurFollower goes down. If we don't prevent
	//	berserk monsters to desert the master, he can do this trick to summon a lot of energy vortexes without any cost to his followers property.

	if (m_pNPC->m_Brain == NPCBRAIN_BERSERK)
		return;
	CChar * pCharOwn = NPC_PetGetOwner();
	if ( !pCharOwn )
		return;

	if ( IsTrigUsed(TRIGGER_PETDESERT) )
	{
		if ( OnTrigger( CTRIG_PetDesert, pCharOwn, nullptr ) == TRIGRET_RET_TRUE )
			return;
	}

	if ( ! pCharOwn->CanSee(this))
		pCharOwn->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_DESERTED), GetName());

	tchar	*pszMsg = Str_GetTemp();
	snprintf(pszMsg, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_DECIDE_MASTER), GetName());
	Speak(pszMsg);

	// free to do as i wish !
	NPC_PetRelease();
}
