
// Actions specific to an NPC.

#include "../../network/send.h"
#include "../clients/CClient.h"
#include "../CWorldGameTime.h"
#include "CChar.h"
#include "CCharNPC.h"

// Get my guild stone for my guild. even if i'm just a STONEPRIV_CANDIDATE ?
// ARGS:
//  MemType == MEMORY_GUILD or MEMORY_TOWN
CItemStone * CChar::Guild_Find( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_Find");
	if ( ! m_pPlayer )
		return nullptr;
	CItemMemory * pMyGMem = Memory_FindTypes((word)(MemType));
	if ( ! pMyGMem )
		return nullptr;
	CItemStone * pMyStone = dynamic_cast <CItemStone*>( pMyGMem->m_uidLink.ItemFind());
	if ( pMyStone == nullptr )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes((word)(MemType)); 	// Make them forget they were ever in this guild....again!
		return nullptr;
	}
	return pMyStone;
}

// Get my member record for my guild.
CStoneMember * CChar::Guild_FindMember( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_FindMember");
	CItemStone * pMyStone = Guild_Find(MemType);
	if ( pMyStone == nullptr )
		return nullptr;
	CStoneMember * pMember = pMyStone->GetMember( this );
	if ( pMember == nullptr )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes((word)(MemType)); 	// Make them forget they were ever in this guild....again!
		return nullptr;
	}
	return pMember;
}

// response to "I resign from my guild" or "town"
// Are we in an active battle ?
void CChar::Guild_Resign( MEMORY_TYPE MemType )
{
	ADDTOCALLSTACK("CChar::Guild_Resign");

	if ( IsStatFlag( STATF_DEAD ))
		return;

	CStoneMember * pMember = Guild_FindMember(MemType);
	if ( pMember == nullptr )
		return ;

	if ( pMember->IsPrivMember())
	{
		CItemMemory * pMemFight = Memory_FindTypes( MEMORY_FIGHT );
		if ( pMemFight )
		{
			CItemStone * pMyStone = pMember->GetParentStone();
			ASSERT(pMyStone);
			SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_GUILDRESIGN ), static_cast<lpctstr>(pMyStone->GetTypeName()) );
		}
	}

	delete pMember;
}

// Get my guild abbrev if i have chosen to turn it on.
lpctstr CChar::Guild_Abbrev( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_Abbrev");
	CStoneMember * pMember = Guild_FindMember(MemType);
	if ( pMember == nullptr )
		return nullptr;
	if ( ! pMember->IsAbbrevOn())
		return nullptr;
	CItemStone * pMyStone = pMember->GetParentStone();
	if ( pMyStone == nullptr ||
		! pMyStone->GetAbbrev()[0] )
		return nullptr;
	return pMyStone->GetAbbrev();
}

// Get my [guild abbrev] if i have chosen to turn it on.
lpctstr CChar::Guild_AbbrevBracket( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_AbbrevBracket");
	lpctstr pszAbbrev = Guild_Abbrev(MemType);
	if ( pszAbbrev == nullptr )
		return nullptr;
	tchar * pszTemp = Str_GetTemp();
	snprintf( pszTemp, Str_TempLength(), " [%s]", pszAbbrev );
	return pszTemp;
}

//***************************************************************
// Memory this char has about something in the world.

// Reset the check timer based on the type of memory.
bool CChar::Memory_UpdateFlags(CItemMemory * pMemory)
{
	ADDTOCALLSTACK("CChar::Memory_UpdateFlags");

	ASSERT(pMemory);
	ASSERT(pMemory->IsType(IT_EQ_MEMORY_OBJ));

	word wMemTypes = pMemory->GetMemoryTypes();

	if (!wMemTypes)	// No memories here anymore so kill it.
		return false;

	int64 iCheckTime;
	if (wMemTypes & MEMORY_IPET)
		StatFlag_Set(STATF_PET);

	if (wMemTypes & MEMORY_FIGHT)	// update more often to check for retreat.
		iCheckTime = 30 * MSECS_PER_SEC;
	else if (wMemTypes & (MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN))
		iCheckTime = -1;	// never go away.
    //else if ((wMemTypes & MEMORY_SAWCRIME) && (g_Cfg.m_iCriminalTimer > 0))
    //    iCheckTime = g_Cfg.m_iCriminalTimer * 60;
	else if (m_pNPC)	// MEMORY_SPEAK
		iCheckTime = 5 * 60 * MSECS_PER_SEC;
	else
		iCheckTime = 20 * 60 * MSECS_PER_SEC;

	pMemory->SetTimeout(iCheckTime);	// update its decay time.	
	CChar * pCharLink = pMemory->m_uidLink.CharFind();
	if (pCharLink)
	{
		pCharLink->NotoSave_Update();	// Clear my notoriety from the target.
		NotoSave_Update();				// iAggressor is stored in the other char, so the call should be reverted.
	}
	return true;
}

// Just clear these flags but do not delete the memory.
// RETURN: true = still useful memory.
bool CChar::Memory_UpdateClearTypes( CItemMemory * pMemory, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_UpdateClearTypes");
	ASSERT(pMemory);

	const word wPrvMemTypes = pMemory->GetMemoryTypes();
	const bool fMore = ( pMemory->SetMemoryTypes( wPrvMemTypes &~ MemTypes ) != 0);

	MemTypes &= wPrvMemTypes;	// Which actually got turned off ?

	if ( MemTypes & MEMORY_IPET )
	{
		// Am i still a pet of some sort ?
		if ( Memory_FindTypes( MEMORY_IPET ) == nullptr )
		{
			StatFlag_Clear( STATF_PET );
		}
	}

	return fMore && Memory_UpdateFlags( pMemory );
}

// Adding a new flag to the given pMemory
void CChar::Memory_AddTypes( CItemMemory * pMemory, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_AddTypes");
	if ( pMemory )
	{
		pMemory->SetMemoryTypes( pMemory->GetMemoryTypes() | MemTypes );
		pMemory->m_itEqMemory.m_pt = static_cast<CPointBase const&>(GetTopPoint());	// Where did the fight start ?
		pMemory->SetTimeStampS(CWorldGameTime::GetCurrentTime().GetTimeRaw());
		Memory_UpdateFlags( pMemory );
	}
}

// Clear the memory object of this type.
bool CChar::Memory_ClearTypes( CItemMemory * pMemory, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	if ( pMemory )
	{
		if ( Memory_UpdateClearTypes( pMemory, MemTypes ))
			return true;
		pMemory->Delete();
	}
	return false;
}

// Create a memory about this object.
// NOTE: Does not check if object already has a memory.!!!
//  Assume it does not !
CItemMemory * CChar::Memory_CreateObj(const CUID& uid, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_CreateObj");

	CItemMemory * pMemory = dynamic_cast <CItemMemory *>(CItem::CreateBase( ITEMID_MEMORY ));
	if ( pMemory == nullptr )
		return nullptr;

	pMemory->SetType(IT_EQ_MEMORY_OBJ);
	pMemory->SetAttr(ATTR_NEWBIE);
	pMemory->m_uidLink = uid;

	Memory_AddTypes( pMemory, MemTypes );
	LayerAdd( pMemory, LAYER_SPECIAL );
	return pMemory;
}


CItemMemory * CChar::Memory_CreateObj( const CObjBase * pObj, word MemTypes )
{
	ASSERT(pObj);
	return Memory_CreateObj( pObj->GetUID(), MemTypes );
}

// Remove all the memories of this type.
void CChar::Memory_ClearTypes( word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes(type)");
	for (CSObjContRec* pObjRec : GetIterationSafeCont())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		CItemMemory * pMemory = dynamic_cast <CItemMemory *>(pItem);
		if ( !pMemory )
			continue;
		Memory_ClearTypes(pMemory, MemTypes);
	}
}

// Do I have a memory / link for this object ?
CItemMemory * CChar::Memory_FindObj( const CUID& uid ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindObj(UID)");
	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) )
			continue;
		if ( pItem->m_uidLink != uid )
			continue;
		return dynamic_cast<CItemMemory *>(pItem);
	}
	return nullptr;
}

CItemMemory * CChar::Memory_FindObj( const CObjBase * pObj ) const
{
    ADDTOCALLSTACK("CChar::Memory_FindObj");
	if ( pObj == nullptr )
		return nullptr;
	return Memory_FindObj( pObj->GetUID());
}

// Do we have a certain type of memory.
// Just find the first one.
CItemMemory * CChar::Memory_FindTypes( word MemTypes ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindTypes");
	if ( !MemTypes )
		return nullptr;

	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		return dynamic_cast<CItemMemory *>(pItem);
	}
	return nullptr;
}

CItemMemory * CChar::Memory_FindObjTypes( const CObjBase * pObj, word MemTypes ) const
{
    ADDTOCALLSTACK("CChar::Memory_FindObjTypes");
	CItemMemory * pMemory = Memory_FindObj(pObj);
	if ( pMemory == nullptr )
		return nullptr;
	if ( ! pMemory->IsMemoryTypes( MemTypes ))
		return nullptr;
	return pMemory;
}

CItemMemory * CChar::Memory_AddObj(const CUID& uid, word MemTypes )
{
	return Memory_CreateObj( uid, MemTypes );
}

CItemMemory * CChar::Memory_AddObj( const CObjBase * pObj, word MemTypes )
{
	return Memory_CreateObj( pObj, MemTypes );
}

// Looping through all memories ( ForCharMemoryType ).
TRIGRET_TYPE CChar::OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, word wMemType )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForMemTypeLoop");
	const CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	if ( wMemType )
	{
		for (CSObjContRec* pObjRec : GetIterationSafeCont())
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( !pItem->IsMemoryTypes(wMemType) )
				continue;
			TRIGRET_TYPE iRet = pItem->OnTriggerRun( s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult );
			if ( iRet == TRIGRET_BREAK )
			{
				EndContext = StartContext;
				break;
			}
			if (( iRet != TRIGRET_ENDIF ) && ( iRet != TRIGRET_CONTINUE ))
				return( iRet );
			if ( iRet == TRIGRET_CONTINUE )
				EndContext = StartContext;
			else
				EndContext = s.GetContext();
			s.SeekContext( StartContext );
		}
	}
	if ( EndContext.m_iOffset <= StartContext.m_iOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
		s.SeekContext( EndContext );
	return TRIGRET_ENDIF;
}

// Adding a new value for this memory, updating notoriety
CItemMemory * CChar::Memory_AddObjTypes(const CUID& uid, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_AddObjTypes(UID)");
	CItemMemory * pMemory = Memory_FindObj( uid );
	if ( pMemory == nullptr )
		return Memory_CreateObj( uid, MemTypes );

	Memory_AddTypes( pMemory, MemTypes );
	NotoSave_Delete( uid.CharFind() );
	return pMemory;
}

CItemMemory * CChar::Memory_AddObjTypes( const CObjBase * pObj, word MemTypes )
{
    ADDTOCALLSTACK("CChar::Memory_AddObjTypes");
	ASSERT(pObj);
	return Memory_AddObjTypes( pObj->GetUID(), MemTypes );
}

// NOTE: Do not return true unless u update the timer !
// RETURN: false = done with this memory.
bool CChar::Memory_OnTick( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_OnTick");
	ASSERT(pMemory);

	CObjBase * pObj = pMemory->m_uidLink.ObjFind();
	if ( pObj == nullptr )
		return false;

	if ( pMemory->IsMemoryTypes( MEMORY_FIGHT ))
	{
		// Is the fight still valid ?
		return Memory_Fight_OnTick( pMemory );
	}

	if ( pMemory->IsMemoryTypes( MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN ))
		return true;	// never go away.

	return false;	// kill it?.
}

// The fight is over because somebody ran away.
void CChar::Memory_Fight_Retreat( CChar * pTarg, CItemMemory * pFight )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Retreat");
	if ( pTarg == nullptr || pTarg->IsStatFlag( STATF_DEAD ))
		return;

	ASSERT(pFight);
	int iMyDistFromBattle = GetTopPoint().GetDist( pFight->m_itEqMemory.m_pt );
	int iHisDistFromBattle = pTarg->GetTopPoint().GetDist( pFight->m_itEqMemory.m_pt );

	bool fCowardice = (iMyDistFromBattle > iHisDistFromBattle);
	Attacker_Delete(pTarg, false, ATTACKER_CLEAR_DISTANCE);

	if ( fCowardice && ! pFight->IsMemoryTypes( MEMORY_IAGGRESSOR ))
	{
		// cowardice is ok if i was attacked.
		return;
	}

	SysMessagef(fCowardice ? g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_1) : g_Cfg.GetDefaultMsg(DEFMSG_MSG_COWARD_2), pTarg->GetName());

	// Lose some fame.
	if ( fCowardice )
		Noto_Fame( -1 );
}

// Check on the status of the fight.
// return: false = delete the memory completely.
//  true = skip it.
bool CChar::Memory_Fight_OnTick( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_OnTick");

	ASSERT(pMemory);
	CChar * pTarg = pMemory->m_uidLink.CharFind();
	if ( pTarg == nullptr )
		return false;	// They are gone for some reason ?

	int64 elapsed = Attacker_GetElapsed(Attacker_GetID(pTarg));
	// Attacker.Elapsed = -1 means no combat end for this attacker.
	// g_Cfg.m_iAttackerTimeout = 0 means attackers doesnt decay. (but cleared when the attacker is killed or the char dies)

	if  (GetDist(pTarg) > g_Cfg.m_iMapViewRadar || ((g_Cfg.m_iAttackerTimeout != 0) && (elapsed >= 0) && (elapsed > g_Cfg.m_iAttackerTimeout)) )
	{
		Memory_Fight_Retreat( pTarg, pMemory );
	clearit:
		Memory_ClearTypes( pMemory, MEMORY_FIGHT|MEMORY_IAGGRESSOR|MEMORY_AGGREIVED );
		return true;
	}

	const int64 iTimeDiff = CWorldGameTime::GetCurrentTime().GetTimeDiff( pMemory->GetTimeStampS() );

	// If am fully healthy then it's not much of a fight.
	if ( iTimeDiff > 60*60*MSECS_PER_SEC )
		goto clearit;
	if ( (pTarg->GetStatPercent(STAT_STR) >= 100) && (iTimeDiff > 2*60*MSECS_PER_SEC) )
		goto clearit;

	pMemory->SetTimeoutD(20);
	return true;	// reschedule it.
}

void CChar::Memory_Fight_Start( const CChar * pTarg )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Start");
	// I am attacking this creature.
	// i might be the aggressor or just retaliating.
	// This is just the "Intent" to fight. Maybe No damage done yet.

	ASSERT(pTarg);
	if ( Fight_IsActive() && (m_Fight_Targ_UID == pTarg->GetUID()) )
	{
		// quick check that things are ok.
		return;
	}

	word MemTypes;
	CItemMemory * pMemory = Memory_FindObj( pTarg );
	if ( pMemory == nullptr )
	{
		// I have no memory of them yet.
		// There was no fight. Am I the aggressor ?
		CItemMemory * pTargMemory = pTarg->Memory_FindObj( this );
		if ( pTargMemory != nullptr )	// My target remembers me.
		{
			if ( pTargMemory->IsMemoryTypes( MEMORY_IAGGRESSOR ))
				MemTypes = MEMORY_HARMEDBY;
			else if ( pTargMemory->IsMemoryTypes( MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED ))
				MemTypes = MEMORY_IAGGRESSOR;
			else
				MemTypes = 0;
		}
		else
		{
			// Hmm, I must have started this i guess.
			MemTypes = MEMORY_IAGGRESSOR;
		}
		pMemory = Memory_CreateObj( pTarg, MEMORY_FIGHT|MemTypes );
	}
	else
	{
		if (Attacker_GetID(pTarg->GetUID()) >= 0)	// I'm already in fight against pTarg, no need of more code
			return;
		if ( pMemory->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED))
			MemTypes = 0;	// I am defending myself rightly.
		else
			MemTypes = MEMORY_IAGGRESSOR;

		Memory_AddTypes(pMemory, MEMORY_FIGHT | MemTypes); // Update the fight status.
	}

	if ( IsClientActive() && (m_Fight_Targ_UID == pTarg->GetUID()) && !IsSetCombatFlags(COMBAT_NODIRCHANGE))
	{
		// This may be a useless command. How do i say the fight is over ?
		// This causes the funny turn to the target during combat !
		new PacketSwing(GetClientActive(), pTarg);
	}
	else
	{
		if ( m_pNPC && (m_pNPC->m_Brain == NPCBRAIN_BERSERK) ) // it will attack everything.
			return;
	}
}
