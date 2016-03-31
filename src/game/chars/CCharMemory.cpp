// Actions specific to an NPC.
#include "../clients/CClient.h"
#include "../common/CUIDExtra.h"
#include "../network/send.h"
#include "../CServTime.h"
#include "CChar.h"
#include "CCharNPC.h"

// Get my guild stone for my guild. even if i'm just a STONEPRIV_CANDIDATE ?
// ARGS:
//  MemType == MEMORY_GUILD or MEMORY_TOWN
CItemStone * CChar::Guild_Find( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_Find");
	if ( ! m_pPlayer )
		return( NULL );
	CItemMemory * pMyGMem = Memory_FindTypes((word)(MemType));
	if ( ! pMyGMem )
		return( NULL );
	CItemStone * pMyStone = dynamic_cast <CItemStone*>( pMyGMem->m_uidLink.ItemFind());
	if ( pMyStone == NULL )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes((word)(MemType)); 	// Make them forget they were ever in this guild....again!
		return( NULL );
	}
	return( pMyStone );
}

// Get my member record for my guild.
CStoneMember * CChar::Guild_FindMember( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_FindMember");
	CItemStone * pMyStone = Guild_Find(MemType);
	if ( pMyStone == NULL )
		return( NULL );
	CStoneMember * pMember = pMyStone->GetMember( this );
	if ( pMember == NULL )
	{
		// Some sort of mislink ! fix it.
		const_cast <CChar*>(this)->Memory_ClearTypes((word)(MemType)); 	// Make them forget they were ever in this guild....again!
		return( NULL );
	}
	return( pMember );
}

// response to "I resign from my guild" or "town"
// Are we in an active battle ?
void CChar::Guild_Resign( MEMORY_TYPE MemType )
{
	ADDTOCALLSTACK("CChar::Guild_Resign");

	if ( IsStatFlag( STATF_DEAD ))
		return;

	CStoneMember * pMember = Guild_FindMember(MemType);
	if ( pMember == NULL )
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
	if ( pMember == NULL )
		return( NULL );
	if ( ! pMember->IsAbbrevOn())
		return( NULL );
	CItemStone * pMyStone = pMember->GetParentStone();
	if ( pMyStone == NULL ||
		! pMyStone->GetAbbrev()[0] )
		return( NULL );
	return( pMyStone->GetAbbrev());
}

// Get my [guild abbrev] if i have chosen to turn it on.
lpctstr CChar::Guild_AbbrevBracket( MEMORY_TYPE MemType ) const
{
	ADDTOCALLSTACK("CChar::Guild_AbbrevBracket");
	lpctstr pszAbbrev = Guild_Abbrev(MemType);
	if ( pszAbbrev == NULL )
		return( NULL );
	tchar * pszTemp = Str_GetTemp();
	sprintf( pszTemp, " [%s]", pszAbbrev );
	return( pszTemp );
}

//***************************************************************
// Memory this char has about something in the world.

// Reset the check timer based on the type of memory.
bool CChar::Memory_UpdateFlags( CItemMemory * pMemory )
{
	ADDTOCALLSTACK("CChar::Memory_UpdateFlags");

	ASSERT(pMemory);
	ASSERT(pMemory->IsType(IT_EQ_MEMORY_OBJ));

	word wMemTypes = pMemory->GetMemoryTypes();

	if ( !wMemTypes )	// No memories here anymore so kill it.
	{
		return false;
	}

	int64 iCheckTime;
	if ( wMemTypes & MEMORY_IPET )
	{
		StatFlag_Set( STATF_Pet );
	}
	if ( wMemTypes & MEMORY_FIGHT )	// update more often to check for retreat.
		iCheckTime = 30*TICK_PER_SEC;
	else if ( wMemTypes & ( MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN ))
		iCheckTime = -1;	// never go away.
	else if ( m_pNPC )	// MEMORY_SPEAK
		iCheckTime = 5*60*TICK_PER_SEC;
	else
		iCheckTime = 20*60*TICK_PER_SEC;
	pMemory->SetTimeout( iCheckTime );	// update it's decay time.
	return( true );
}

// Just clear these flags but do not delete the memory.
// RETURN: true = still useful memory.
bool CChar::Memory_UpdateClearTypes( CItemMemory * pMemory, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_UpdateClearTypes");
	ASSERT(pMemory);

	word wPrvMemTypes = pMemory->GetMemoryTypes();
	bool fMore = ( pMemory->SetMemoryTypes( wPrvMemTypes &~ MemTypes ) != 0);

	MemTypes &= wPrvMemTypes;	// Which actually got turned off ?

	if ( MemTypes & MEMORY_IPET )
	{
		// Am i still a pet of some sort ?
		if ( Memory_FindTypes( MEMORY_IPET ) == NULL )
		{
			StatFlag_Clear( STATF_Pet );
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
		pMemory->m_itEqMemory.m_pt = GetTopPoint();	// Where did the fight start ?
		pMemory->SetTimeStamp(CServTime::GetCurrentTime().GetTimeRaw());
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
CItemMemory * CChar::Memory_CreateObj( CUID uid, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_CreateObj");

	CItemMemory * pMemory = dynamic_cast <CItemMemory *>(CItem::CreateBase( ITEMID_MEMORY ));
	if ( pMemory == NULL )
		return( NULL );

	pMemory->SetType(IT_EQ_MEMORY_OBJ);
	pMemory->SetAttr(ATTR_NEWBIE);
	pMemory->m_uidLink = uid;

	Memory_AddTypes( pMemory, MemTypes );
	LayerAdd( pMemory, LAYER_SPECIAL );
	return( pMemory );
}


CItemMemory * CChar::Memory_CreateObj( const CObjBase * pObj, word MemTypes )
{
	ASSERT(pObj);
	return Memory_CreateObj( pObj->GetUID(), MemTypes );
}

// Remove all the memories of this type.
void CChar::Memory_ClearTypes( word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_ClearTypes");
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		CItemMemory * pMemory = dynamic_cast <CItemMemory *>(pItem);
		if ( !pMemory )
			continue;
		Memory_ClearTypes(pMemory, MemTypes);
	}
}

// Do I have a memory / link for this object ?
CItemMemory * CChar::Memory_FindObj( CUID uid ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindObj");
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsType(IT_EQ_MEMORY_OBJ) )
			continue;
		if ( pItem->m_uidLink != uid )
			continue;
		return dynamic_cast<CItemMemory *>(pItem);
	}
	return NULL;
}

CItemMemory * CChar::Memory_FindObj( const CObjBase * pObj ) const
{
	if ( pObj == NULL )
		return( NULL );
	return Memory_FindObj( pObj->GetUID());
}

// Do we have a certain type of memory.
// Just find the first one.
CItemMemory * CChar::Memory_FindTypes( word MemTypes ) const
{
	ADDTOCALLSTACK("CChar::Memory_FindTypes");
	if ( !MemTypes )
		return NULL;

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
	{
		if ( !pItem->IsMemoryTypes(MemTypes) )
			continue;
		return dynamic_cast<CItemMemory *>(pItem);
	}
	return NULL;
}

CItemMemory * CChar::Memory_FindObjTypes( const CObjBase * pObj, word MemTypes ) const
{
	CItemMemory * pMemory = Memory_FindObj(pObj);
	if ( pMemory == NULL )
		return( NULL );
	if ( ! pMemory->IsMemoryTypes( MemTypes ))
		return( NULL );
	return( pMemory );
}

CItemMemory * CChar::Memory_AddObj( CUID uid, word MemTypes )
{
	return Memory_CreateObj( uid, MemTypes );
}

CItemMemory * CChar::Memory_AddObj( const CObjBase * pObj, word MemTypes )
{
	return Memory_CreateObj( pObj, MemTypes );
}

// Looping through all memories ( ForCharMemoryType ).
TRIGRET_TYPE CChar::OnCharTrigForMemTypeLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CGString * pResult, word wMemType )
{
	ADDTOCALLSTACK("CChar::OnCharTrigForMemTypeLoop");
	CScriptLineContext StartContext = s.GetContext();
	CScriptLineContext EndContext = StartContext;

	if ( wMemType )
	{
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
		{
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
	if ( EndContext.m_lOffset <= StartContext.m_lOffset )
	{
		// just skip to the end.
		TRIGRET_TYPE iRet = OnTriggerRun( s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult );
		if ( iRet != TRIGRET_ENDIF )
			return( iRet );
	}
	else
		s.SeekContext( EndContext );
	return( TRIGRET_ENDIF );
}

// Adding a new value for this memory, updating notoriety
CItemMemory * CChar::Memory_AddObjTypes( CUID uid, word MemTypes )
{
	ADDTOCALLSTACK("CChar::Memory_AddObjTypes");
	CItemMemory * pMemory = Memory_FindObj( uid );
	if ( pMemory == NULL )
	{
		return Memory_CreateObj( uid, MemTypes );
	}
	Memory_AddTypes( pMemory, MemTypes );
	NotoSave_Delete( uid.CharFind() );
	return( pMemory );
}

CItemMemory * CChar::Memory_AddObjTypes( const CObjBase * pObj, word MemTypes )
{
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
	if ( pObj == NULL )
		return( false );

	if ( pMemory->IsMemoryTypes( MEMORY_FIGHT ))
	{
		// Is the fight still valid ?
		return Memory_Fight_OnTick( pMemory );
	}

	if ( pMemory->IsMemoryTypes( MEMORY_IPET | MEMORY_GUARD | MEMORY_GUILD | MEMORY_TOWN ))
		return( true );	// never go away.

	return( false );	// kill it?.
}

// The fight is over because somebody ran away.
void CChar::Memory_Fight_Retreat( CChar * pTarg, CItemMemory * pFight )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Retreat");
	if ( pTarg == NULL || pTarg->IsStatFlag( STATF_DEAD ))
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
	if ( pTarg == NULL )
		return( false );	// They are gone for some reason ?

	int64 elapsed = Attacker_GetElapsed(Attacker_GetID(pTarg));
	// Attacker.Elapsed = -1 means no combat end for this attacker.
	// g_Cfg.m_iAttackerTimeout = 0 means attackers doesnt decay. (but cleared when the attacker is killed or the char dies)

	if ( GetDist(pTarg) > UO_MAP_VIEW_RADAR || ( g_Cfg.m_iAttackerTimeout != 0 && elapsed > (int64)(g_Cfg.m_iAttackerTimeout) && elapsed >= 0 ) )
	{
		Memory_Fight_Retreat( pTarg, pMemory );
	clearit:
		Memory_ClearTypes( pMemory, MEMORY_FIGHT|MEMORY_IAGGRESSOR|MEMORY_AGGREIVED );
		return( true );
	}

	int64 iTimeDiff = - g_World.GetTimeDiff( pMemory->GetTimeStamp() );

	// If am fully healthy then it's not much of a fight.
	if ( iTimeDiff > 60*60*TICK_PER_SEC )
		goto clearit;
	if ( pTarg->GetHealthPercent() >= 100 && iTimeDiff > 2*60*TICK_PER_SEC )
		goto clearit;

	pMemory->SetTimeout(20*TICK_PER_SEC);
	return( true );	// reschedule it.
}

void CChar::Memory_Fight_Start( const CChar * pTarg )
{
	ADDTOCALLSTACK("CChar::Memory_Fight_Start");
	// I am attacking this creature.
	// i might be the aggressor or just retaliating.
	// This is just the "Intent" to fight. Maybe No damage done yet.

	ASSERT(pTarg);
	if (Fight_IsActive() && m_Fight_Targ == pTarg->GetUID())
	{
		// quick check that things are ok.
		return;
	}

	word MemTypes;
	CItemMemory * pMemory = Memory_FindObj( pTarg );
	if ( pMemory == NULL )
	{
		// I have no memory of them yet.
		// There was no fight. Am I the aggressor ?
		CItemMemory * pTargMemory = pTarg->Memory_FindObj( this );
		if ( pTargMemory != NULL )	// My target remembers me.
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
		pMemory = Memory_CreateObj( pTarg, MEMORY_FIGHT|MEMORY_WAR_TARG|MemTypes );
	}
	else
	{
		// I have a memory of them.
		bool fMemPrvType = pMemory->IsMemoryTypes(MEMORY_WAR_TARG);
		if ( fMemPrvType )
			return;
		if ( pMemory->IsMemoryTypes(MEMORY_HARMEDBY|MEMORY_SAWCRIME|MEMORY_AGGREIVED))
			MemTypes = 0;	// I am defending myself rightly.
		else
			MemTypes = MEMORY_IAGGRESSOR;

		// Update the fights status
		Memory_AddTypes( pMemory, MEMORY_FIGHT|MEMORY_WAR_TARG|MemTypes );
	}
	//Attacker_Add(const_cast<CChar*>(pTarg));
	if ( IsClient() && m_Fight_Targ == pTarg->GetUID() && !IsSetCombatFlags(COMBAT_NODIRCHANGE))
	{
		// This may be a useless command. How do i say the fight is over ?
		// This causes the funny turn to the target during combat !
		new PacketSwing(GetClient(), pTarg);
	}
	else
	{
		if ( m_pNPC && m_pNPC->m_Brain == NPCBRAIN_BERSERK ) // it will attack everything.
			return;
	}
}
