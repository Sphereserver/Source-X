// Actions specific to an NPC.

#include "../graysvr/CPathFinder.h"
#include "../network/receive.h"
#include "CChar.h"
#include "CCharNPC.h"
#include "CClient.h"
#include "CServTime.h"
#include "graysvr.h"
#include "Triggers.h"

//////////////////////////
// CChar

const LAYER_TYPE CChar::sm_VendorLayers[] = // static
{
	LAYER_VENDOR_STOCK, LAYER_VENDOR_EXTRA, LAYER_VENDOR_BUYS,
};

bool CChar::NPC_IsVendor() const
{
	return (m_pNPC && m_pNPC->IsVendor());
}

int CChar::NPC_GetAiFlags()
{
	if(m_pNPC == NULL)
		return 0;
	return (m_pNPC->GetNpcAiFlags(this));
}

bool CChar::NPC_Vendor_Restock(bool bForce, bool bFillStock)
{
	ADDTOCALLSTACK("CChar::NPC_Vendor_Restock");
	// Restock this NPC char.
	// Then Set the next restock time for this .

	if ( m_pNPC == NULL )
		return false;

	// Make sure that we're a vendor and not a pet
	if ( IsStatFlag(STATF_Pet) || !NPC_IsVendor() )
		return false;

	bool bRestockNow = true;

	if ( !bForce && m_pNPC->m_timeRestock.IsTimeValid() )
	{
		// Restock occurs every 10 minutes of inactivity (unless
		// region tag specifies different time)
		CRegionWorld *region = GetRegion();
		INT64 restockIn = 10 * 60 * TICK_PER_SEC;
		if( region != NULL )
		{
			CVarDefCont *vardef = region->m_TagDefs.GetKey("RestockVendors");
			if( vardef != NULL )
				restockIn = vardef->GetValNum();
			if ( region->m_TagDefs.GetKey("NoRestock") != NULL )
				bRestockNow = false;
		}
		if ( m_TagDefs.GetKey("NoRestock") != NULL )
			bRestockNow = false;
		
		if (bRestockNow)
			bRestockNow = ( CServTime::GetCurrentTime().GetTimeDiff(m_pNPC->m_timeRestock) > restockIn );
	}

	// At restock the containers are actually emptied
	if ( bRestockNow )
	{
		m_pNPC->m_timeRestock.Init();

		for ( size_t i = 0; i < COUNTOF(sm_VendorLayers); ++i )
		{
			CItemContainer *pCont = GetBank(sm_VendorLayers[i]);
			if ( !pCont )
				return false;

			pCont->Empty();
		}
	}

	if ( bFillStock )
	{
		// An invalid restock time means that the containers are
		// waiting to be filled
		if ( !m_pNPC->m_timeRestock.IsTimeValid() )
		{
			if ( IsTrigUsed(TRIGGER_NPCRESTOCK) )
			{
				CCharBase *pCharDef = Char_GetDef();
				ReadScriptTrig(pCharDef, CTRIG_NPCRestock, true);
			}

			//	we need restock vendor money as well
			GetBank()->Restock();
		}

		// remember that the stock was filled (or considered up-to-date)
		m_pNPC->m_timeRestock.SetCurrentTime();
	}
	return true;
}

bool CChar::NPC_StablePetSelect( CChar * pCharPlayer )
{
	ADDTOCALLSTACK("CChar::NPC_StablePetSelect");
	// I am a stable master.
	// I will stable a pet for the player.

	if ( pCharPlayer == NULL )
		return( false );
	if ( ! pCharPlayer->IsClient())
		return( false );

	// Might have too many pets already ?
	int iCount = 0;
	CItemContainer * pBank = GetBank();
	if ( pBank->GetCount() >= MAX_ITEMS_CONT )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_FULL ) );
		return( false );
	}

	// Calculate the max limit of pets that the NPC can hold for the player
	double iSkillTaming = pCharPlayer->Skill_GetAdjusted(SKILL_TAMING);
	double iSkillAnimalLore = pCharPlayer->Skill_GetAdjusted(SKILL_ANIMALLORE);
	double iSkillVeterinary = pCharPlayer->Skill_GetAdjusted(SKILL_VETERINARY);
	double iSkillSum = iSkillTaming + iSkillAnimalLore + iSkillVeterinary;

	int iPetMax;
	if ( iSkillSum >= 240.0 )
		iPetMax = 5;
	else if ( iSkillSum >= 200.0 )
		iPetMax = 4;
	else if ( iSkillSum >= 160.0 )
		iPetMax = 3;
	else
		iPetMax = 2;

	if ( iSkillTaming >= 100.0 )
		iPetMax += (int)((iSkillTaming - 90.0) / 10);

	if ( iSkillAnimalLore >= 100.0 )
		iPetMax += (int)((iSkillAnimalLore - 90.0) / 10);

	if ( iSkillVeterinary >= 100.0 )
		iPetMax += (int)((iSkillVeterinary - 90.0) / 10);

	if ( m_TagDefs.GetKey("MAXPLAYERPETS") )
		iPetMax = static_cast<int>(m_TagDefs.GetKeyNum("MAXPLAYERPETS"));

	for ( CItem *pItem = pBank->GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( pItem->IsType(IT_FIGURINE) && pItem->m_uidLink == pCharPlayer->GetUID() )
			iCount++;
	}
	if ( iCount >= iPetMax )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TOOMANY ) );
		return( false );
	}

	pCharPlayer->m_pClient->m_Targ_PrvUID = GetUID();
	pCharPlayer->m_pClient->addTarget( CLIMODE_TARG_PET_STABLE, g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG ) );
	return( true );
}

bool CChar::NPC_StablePetRetrieve( CChar * pCharPlayer )
{
	ADDTOCALLSTACK("CChar::NPC_StablePetRetrieve");
	// Get pets for this person from my inventory.
	// May want to put up a menu ???

	if ( !m_pNPC || m_pNPC->m_Brain != NPCBRAIN_STABLE )
		return false;

	int iCount = 0;
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetBank()->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( pItem->IsType(IT_FIGURINE) && pItem->m_uidLink == pCharPlayer->GetUID() )
		{
			if ( !pCharPlayer->Use_Figurine(pItem) )
			{
				TCHAR *pszTemp = Str_GetTemp();
				sprintf(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_CLAIM_FOLLOWER), pItem->GetName());
				Speak(pszTemp);
				return true;
			}

			pItem->Delete();
			iCount++;
		}
	}

	Speak(g_Cfg.GetDefaultMsg((iCount > 0) ? DEFMSG_NPC_STABLEMASTER_CLAIM : DEFMSG_NPC_STABLEMASTER_CLAIM_NOPETS));
	return true;
}

int CChar::NPC_OnTrainCheck( CChar * pCharSrc, SKILL_TYPE Skill )
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainCheck");
	// Can we train in this skill ?
	// RETURN: Amount of skill we can train.
	//

	if ( !IsSkillBase(Skill) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_DUNNO_1));
		return 0;
	}

	int iSkillSrcVal = pCharSrc->Skill_GetBase(Skill);
	int iSkillVal = Skill_GetBase(Skill);
	int iTrainVal = NPC_GetTrainMax(pCharSrc, Skill) - iSkillSrcVal;

	// Train npc skill cap
	int iMaxDecrease = 0;
	if ( (pCharSrc->GetSkillTotal() + iTrainVal) > pCharSrc->Skill_GetMax(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) )
	{	
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
				continue;

			if ( pCharSrc->Skill_GetLock(static_cast<SKILL_TYPE>(i)) == SKILLLOCK_DOWN )
				iMaxDecrease += pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
		}
		iMaxDecrease = minimum(iTrainVal, iMaxDecrease);
	}
	else
	{
		iMaxDecrease = iTrainVal;
	}

	LPCTSTR pszMsg;
	if ( iSkillVal <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_2 );
	}
	else if ( iSkillSrcVal > iSkillVal )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_3 );
	}
	else if ( iMaxDecrease <= 0 )
	{
		pszMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_DUNNO_4 );
	}
	else
	{
		return( iMaxDecrease );
	}

	char	*z = Str_GetTemp();
	sprintf(z, pszMsg, g_Cfg.GetSkillKey(Skill));
	Speak(z);
	return 0;
}

bool CChar::NPC_OnTrainPay(CChar *pCharSrc, CItemMemory *pMemory, CItem * pGold)
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainPay");
	SKILL_TYPE skill = static_cast<SKILL_TYPE>(pMemory->m_itEqMemory.m_Skill);
	if ( !IsSkillBase(skill) || !g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) )
	{
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_FORGOT));
		return false;
	}

	int iTrainCost = NPC_OnTrainCheck(pCharSrc, skill) * g_Cfg.m_iTrainSkillCost;
	if (( iTrainCost <= 0 ) || !pGold )
		return false;

	Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_SUCCESS ) );

	// Consume as much money as we can train for.
	if ( pGold->GetAmount() < iTrainCost )
	{
		iTrainCost = pGold->GetAmount();
	}
	else if ( pGold->GetAmount() == iTrainCost )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_1 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;
	}
	else
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_2 ) );
		pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_NONE;

		// Give change back.
		pGold->UnStackSplit( iTrainCost, pCharSrc );
	}
	GetPackSafe()->ContentAdd( pGold );	// take my cash.

	// Give credit for training.
	NPC_TrainSkill( pCharSrc, skill, iTrainCost );
	return( true );
}

bool CChar::NPC_TrainSkill( CChar * pCharSrc, SKILL_TYPE skill, int toTrain )
{
	ADDTOCALLSTACK("CChar::NPC_TrainSkill");
	int iTrain = toTrain;
	if ( (pCharSrc->GetSkillTotal() + toTrain) > pCharSrc->Skill_GetMax(static_cast<SKILL_TYPE>(g_Cfg.m_iMaxSkill)) )
	{	
		for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
		{
			if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
				continue;

			if ( toTrain < 1 )
			{
				pCharSrc->Skill_SetBase(skill, iTrain + pCharSrc->Skill_GetBase(skill));
				break;
			}

			if ( pCharSrc->Skill_GetLock(static_cast<SKILL_TYPE>(i)) == SKILLLOCK_DOWN )
			{
				if ( pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i)) > toTrain )
				{
					pCharSrc->Skill_SetBase(static_cast<SKILL_TYPE>(i), pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i)) - toTrain);
					toTrain = 0;
				}
				else
				{
					toTrain -= pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
					pCharSrc->Skill_SetBase(static_cast<SKILL_TYPE>(i), 0);
				}
			}
		}
	}
	else
	{
		pCharSrc->Skill_SetBase(skill, iTrain + pCharSrc->Skill_GetBase(skill));
	}

	return true;
}

bool CChar::NPC_OnTrainHear( CChar * pCharSrc, LPCTSTR pszCmd )
{
	ADDTOCALLSTACK("CChar::NPC_OnTrainHear");
	// We are asking for training ?

	if ( ! m_pNPC )
		return( false );

	// Check the NPC is capable of teaching
	if ( (m_pNPC->m_Brain < NPCBRAIN_HUMAN) || (m_pNPC->m_Brain > NPCBRAIN_STABLE) || (m_pNPC->m_Brain == NPCBRAIN_GUARD) )
		return( false );

	// Check the NPC isn't busy fighting
	if ( Memory_FindObjTypes( pCharSrc, MEMORY_FIGHT|MEMORY_HARMEDBY|MEMORY_IRRITATEDBY|MEMORY_AGGREIVED ))
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_ENEMY ) );
		return true;
	}

	// Did they mention a skill name i recognize ?
	TemporaryString pszMsg;

	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
			continue;

		LPCTSTR pSkillKey = g_Cfg.GetSkillKey(static_cast<SKILL_TYPE>(i));
		if ( FindStrWord( pszCmd, pSkillKey ) <= 0)
			continue;

		// Can we train in this ?
		int iTrainCost = NPC_OnTrainCheck(pCharSrc, static_cast<SKILL_TYPE>(i)) * g_Cfg.m_iTrainSkillCost;
		if ( iTrainCost <= 0 )
			return true;

		sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_NPC_TRAINER_PRICE), iTrainCost, static_cast<LPCTSTR>(pSkillKey));
		Speak(pszMsg);
		CItemMemory * pMemory = Memory_AddObjTypes( pCharSrc, MEMORY_SPEAK );
		if ( pMemory )
		{
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_SPEAK_TRAIN;
			pMemory->m_itEqMemory.m_Skill = static_cast<WORD>(i);
		}
		return true;
	}

	// What can he teach me about ?
	// Just tell them what we can teach them or set up a memory to train.
	strcpy( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_1 ) );

	LPCTSTR pPrvSkill = NULL;

	size_t iCount = 0;
	for ( size_t i = 0; i < g_Cfg.m_iMaxSkill; i++ )
	{
		if ( !g_Cfg.m_SkillIndexDefs.IsValidIndex(static_cast<SKILL_TYPE>(i)) )
			continue;

		int iDiff = NPC_GetTrainMax(pCharSrc, static_cast<SKILL_TYPE>(i)) - pCharSrc->Skill_GetBase(static_cast<SKILL_TYPE>(i));
		if ( iDiff <= 0 )
			continue;

		if ( iCount > 6 )
		{
			pPrvSkill = g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_2 );
			break;
		}
		if ( iCount > 1 )
		{
			strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_PRICE_3 ) );
		}
		if ( pPrvSkill )
		{
			strcat( pszMsg, pPrvSkill );
		}

		pPrvSkill = g_Cfg.GetSkillKey(static_cast<SKILL_TYPE>(i));
		iCount++;
	}

	if ( iCount == 0 )
	{
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_3 ) );
		return true;
	}
	if ( iCount > 1 )
	{
		strcat( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_NPC_TRAINER_THATSALL_4 ) );
	}

	strcat( pszMsg, pPrvSkill );
	strcat( pszMsg, "." );
	Speak( pszMsg );
	return( true );
}
