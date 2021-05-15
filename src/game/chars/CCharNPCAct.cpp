
// Actions specific to an NPC.

#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../network/receive.h"
#include "../clients/CClient.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../triggers.h"
#include "CCharNPC.h"

//////////////////////////
// CChar

enum NV_TYPE
{
	NV_BUY,
	NV_BYE,
	NV_FLEE,
	NV_GOTO,
	NV_HIRE,
	NV_LEAVE,
	NV_PETRETRIEVE,
	NV_PETSTABLE,
	NV_RELEASE,
	NV_RESTOCK,
	NV_RUN,
	NV_RUNTO,
	NV_SELL,
	NV_SHRINK,
	NV_TRAIN,
	NV_WALK,
	NV_QTY
};

lpctstr const CCharNPC::sm_szVerbKeys[NV_QTY+1] =
{
	"BUY",
	"BYE",
	"FLEE",
	"GOTO",
	"HIRE",
	"LEAVE",
	"PETRETRIEVE",
	"PETSTABLE",
	"RELEASE",
	"RESTOCK",
	"RUN",
	"RUNTO",
	"SELL",
	"SHRINK",
	"TRAIN",
	"WALK",
	nullptr
};

void CChar::Action_StartSpecial( CREID_TYPE id )
{
	ADDTOCALLSTACK("CChar::Action_StartSpecial");
	ASSERT(m_pNPC);
	// Take the special creature action.
	// lay egg, breath weapon (fire, lightning, acid, code, paralyze),
	//  create web, fire patch, fire ball,
	// steal, teleport, level drain, absorb magic, curse items,
	// rust items, stealing, charge, hiding, grab, regenerate, play dead.
	// Water = put out fire !

	if ( !g_Cfg.IsSkillFlag( Skill_GetActive(), SKF_NOANIM ) )
		UpdateAnimate( ANIM_CAST_AREA );

	switch ( id )
	{
		case CREID_FIRE_ELEM:
		{
			// Leave a fire path
			CItem * pItem = CItem::CreateScript( Calc_GetRandVal(2) ? ITEMID_FX_FIRE_F_EW : ITEMID_FX_FIRE_F_NS, this );
			ASSERT(pItem);
			pItem->SetType(IT_FIRE);
			pItem->m_itSpell.m_spell = (word)(SPELL_Fire_Field);
			pItem->m_itSpell.m_spelllevel = (word)(100 + Calc_GetRandVal(500));
			pItem->m_itSpell.m_spellcharges = 1;
			pItem->m_uidLink = GetUID();
			pItem->MoveToDecay( GetTopPoint(), 10 + Calc_GetRandVal(50)*MSECS_PER_SEC);
		}
		break;

		case CREID_GIANT_SPIDER:
		{
			// Leave a web path
			CItem * pItem = CItem::CreateScript( (ITEMID_TYPE)(Calc_GetRandVal2(ITEMID_WEB1_1, ITEMID_WEB1_4)), this );
			ASSERT(pItem);
			pItem->SetType(IT_WEB);
			pItem->MoveToDecay( GetTopPoint(), 10 + Calc_GetRandVal(170)*MSECS_PER_SEC);
		}
		break;

		default:
			SysMessage( "You have no special abilities" );
			return;
	}

	UpdateStatVal( STAT_DEX, (ushort)(-(5 + Calc_GetRandVal(5)) ));	// the stamina cost
}

bool CChar::NPC_OnVerb( CScript &s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CChar::NPC_OnVerb");
	ASSERT(m_pNPC);
	// Stuff that only NPC's do.

	EXC_TRY("OnVerb");
	CChar * pCharSrc = pSrc->GetChar();

	switch ( FindTableSorted( s.GetKey(), CCharNPC::sm_szVerbKeys, CountOf(CCharNPC::sm_szVerbKeys)-1 ))
	{
	case NV_BUY:
	{
		// Open up the buy dialog.
		if ( pCharSrc == nullptr || !pCharSrc->IsClientActive())
			return false;

		CClient * pClientSrc = pCharSrc->GetClientActive();
		ASSERT(pClientSrc != nullptr);
		if ( !pClientSrc->addShopMenuBuy(this) )
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NO_GOODS));
		else
			pClientSrc->m_TagDefs.SetNum("BUYSELLTIME", CWorldGameTime::GetCurrentTime().GetTimeRaw());
		break;
	}
	case NV_BYE:
		Skill_Start( SKILL_NONE );
		m_Act_UID.InitUID();
		break;
	case NV_LEAVE:
	case NV_FLEE:
		// Short amount of fleeing.
		m_atFlee.m_iStepsMax = s.GetArgVal();	// how long should it take to get there.
		if ( ! m_atFlee.m_iStepsMax )
			m_atFlee.m_iStepsMax = 20;

		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
		break;
	case NV_GOTO:
		m_Act_p = g_Cfg.GetRegionPoint( s.GetArgStr());
		Skill_Start( NPCACT_GOTO );
		break;
	case NV_HIRE:
		return NPC_OnHireHear( pCharSrc);
	case NV_PETRETRIEVE:
		return( NPC_StablePetRetrieve( pCharSrc ));
	case NV_PETSTABLE:
		return( NPC_StablePetSelect( pCharSrc ));
	case NV_RELEASE:
		NPC_PetRelease();
		break;
	case NV_RESTOCK:	// individual restock command.
		return NPC_Vendor_Restock(true, s.GetArgVal() != 0);
	case NV_RUN:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( true );
		break;
	case NV_RUNTO:
		m_Act_p = g_Cfg.GetRegionPoint( s.GetArgStr());
		Skill_Start( NPCACT_RUNTO );
		break;
	case NV_SELL:
	{
		// Open up the sell dialog.
		if ( pCharSrc == nullptr || !pCharSrc->IsClientActive() )
			return false;

		CClient * pClientSrc = pCharSrc->GetClientActive();
		ASSERT(pClientSrc != nullptr);
		if ( ! pClientSrc->addShopMenuSell( this ))
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_NOTHING_BUY));
		else
			pClientSrc->m_TagDefs.SetNum("BUYSELLTIME", CWorldGameTime::GetCurrentTime().GetTimeRaw());
		break;
	}
	case NV_SHRINK:
		{
			// we must own it.
			if ( !NPC_IsOwnedBy( pCharSrc ))
				return false;
			CItem * pItem = NPC_Shrink(); // this deletes the char !!!
			if ( pItem )
				pCharSrc->m_Act_UID = pItem->GetUID();
			if (s.HasArgs())
				pCharSrc->ItemBounce(pItem);

			return ( pItem != nullptr );
		}
	case NV_TRAIN:
		return( NPC_OnTrainHear( pCharSrc, s.GetArgStr()));
	case NV_WALK:
		m_Act_p = GetTopPoint();
		m_Act_p.Move( GetDirStr( s.GetArgRaw()));
		NPC_WalkToPoint( false );
		break;
	default:
		// Eat all the CClient::sm_szVerbKeys and CCharPlayer::sm_szVerbKeys verbs ?
		//if ( FindTableSorted(s.GetKey(), CClient::sm_szVerbKeys, CountOf(sm_szVerbKeys)-1) < 0 )
		return false;
	}

	EXC_CATCH;
	return true;
}

void CChar::NPC_ActStart_SpeakTo( CChar * pSrc )
{
	ADDTOCALLSTACK("CChar::NPC_ActStart_SpeakTo");
	ASSERT(m_pNPC);
	// My new action is that i am speaking to this person.
	// Or just update the amount of time i will wait for this person.
	m_Act_UID = pSrc->GetUID();
	m_atTalk.m_dwWaitCount = 20;
	m_atTalk.m_dwHearUnknown = 0;

	Skill_Start( ( pSrc->GetFame() > 7000 ) ? NPCACT_TALK_FOLLOW : NPCACT_TALK );
	_SetTimeoutS(3);
	UpdateDir(pSrc);
}

void CChar::NPC_OnHear( lpctstr pszCmd, CChar * pSrc, bool fAllPets )
{
	ADDTOCALLSTACK("CChar::NPC_OnHear");
	ASSERT(m_pNPC);
	// This CChar has heard you say something.
	if ( !pSrc )
		return;

	// Pets always have a basic set of actions.
	if ( NPC_OnHearPetCmd(pszCmd, pSrc, fAllPets) || !NPC_CanSpeak() )
		return;

	// What where we doing ?
	// too busy to talk ?

	switch ( Skill_GetActive())
	{
		case SKILL_BEGGING: // busy begging. (hack)
			if ( !g_Cfg.IsSkillFlag( SKILL_BEGGING, SKF_SCRIPTED ) )
				return;
			break;
		case NPCACT_TALK:
		case NPCACT_TALK_FOLLOW:
			// Was NPC talking to someone else ?
			if ( m_Act_UID != pSrc->GetUID())
			{
				if ( NPC_Act_Talk() )
				{
					CChar * pCharOld = m_Act_UID.CharFind();
					if (pCharOld != nullptr)
					{
						tchar * z = Str_GetTemp();
						snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_INTERRUPT), pCharOld->GetName(), pSrc->GetName());
						Speak(z);
					}
				}
			}
			break;
		default:
			break;
	}

	// I've heard them for the first time.
	CItemMemory * pMemory = Memory_FindObjTypes( pSrc, MEMORY_SPEAK );
	if ( pMemory == nullptr )
	{
		// This or CTRIG_SeeNewPlayer will be our first contact with people.
		if ( IsTrigUsed(TRIGGER_NPCHEARGREETING) )
		{
			if ( OnTrigger( CTRIG_NPCHearGreeting, pSrc ) == TRIGRET_RET_TRUE )
				return;
		}

		// record that we attempted to speak to them.
		pMemory = Memory_AddObjTypes(pSrc, MEMORY_SPEAK);
		if ( pMemory )
			pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
	}

	// Do the scripts want me to take some action based on this speech.
	SKILL_TYPE skill = m_Act_SkillCurrent;

	TALKMODE_TYPE mode = TALKMODE_SAY;
	for ( size_t i = 0; i < m_pNPC->m_Speech.size(); ++i )
	{
		CResourceLink * pLink = m_pNPC->m_Speech[i].GetRef();
		if ( !pLink )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) || !pLink->HasTrigger(XTRIG_UNKNOWN) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	CCharBase * pCharDef = Char_GetDef();
	ASSERT(pCharDef != nullptr);
	for ( size_t i = 0; i < pCharDef->m_Speech.size(); ++i )
	{
		CResourceLink * pLink = pCharDef->m_Speech[i].GetRef();
		if ( !pLink )
			continue;
		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;
		TRIGRET_TYPE iRet = OnHearTrigger( s, pszCmd, pSrc, mode );
		if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
			continue;
		if ( iRet == TRIGRET_RET_DEFAULT && skill == m_Act_SkillCurrent )
		{
			// You are the new speaking target.
			NPC_ActStart_SpeakTo( pSrc );
		}
		return;
	}

	// hard code some default reactions.
	if ( m_pNPC->m_Brain == NPCBRAIN_HEALER	|| Skill_GetBase( SKILL_SPIRITSPEAK ) >= 1000 )
	{
		if ( NPC_LookAtChar( pSrc, 1 ))
			return;
	}

	// can't figure you out.
	if ( IsTrigUsed(TRIGGER_NPCHEARUNKNOWN) )
	{
		if ( OnTrigger( CTRIG_NPCHearUnknown, pSrc ) == TRIGRET_RET_TRUE )
			return;
	}

	if ( (Skill_GetActive() == NPCACT_TALK) || (Skill_GetActive() == NPCACT_TALK_FOLLOW) )
	{
		++ m_atTalk.m_dwHearUnknown;
		uint iMaxUnk = 4;
		if ( GetDist( pSrc ) > 4 )
			iMaxUnk = 1;
		if ( m_atTalk.m_dwHearUnknown > iMaxUnk )
		{
			Skill_Start( SKILL_NONE ); // say good by
		}
	}
}

void CChar::NPC_OnNoticeSnoop( const CChar * pCharThief, const CChar * pCharMark )
{
	ADDTOCALLSTACK("CChar::NPC_OnNoticeSnoop");
	ASSERT(m_pNPC);

	// start making them angry at you.
	static uint const sm_szTextSnoop[] =
	{
		DEFMSG_NPC_GENERIC_SNOOPED_1,
		DEFMSG_NPC_GENERIC_SNOOPED_2,
		DEFMSG_NPC_GENERIC_SNOOPED_3,
		DEFMSG_NPC_GENERIC_SNOOPED_4
	};

	if ( pCharMark != this )	// not me so who cares.
		return;

	if ( NPC_CanSpeak())
	{
		Speak( g_Cfg.GetDefaultMsg(sm_szTextSnoop[ Calc_GetRandVal( CountOf( sm_szTextSnoop )) ]));
	}
	if ( ! Calc_GetRandVal(4))
	{
		m_Act_UID = pCharThief->GetUID();
		m_atFlee.m_iStepsMax = 20;	// how long should it take to get there.
		m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
		Skill_Start( NPCACT_FLEE );
	}
}

int CChar::NPC_WalkToPoint( bool fRun )
{
	ADDTOCALLSTACK("CChar::NPC_WalkToPoint");
	ASSERT(m_pNPC);
	// Move toward my target .
	//
	// RETURN:
	//  0 = we are here.
	//  1 = took the step.
	//  2 = can't take this step right now. (obstacle)

	if (Can(CAN_C_NONMOVER))
		return 0;

    const CCharBase	*pCharDef = Char_GetDef();
    const CPointMap	pTarg = m_Act_p;
	CPointMap	pMe = GetTopPoint();
    int iDex = Stat_GetAdjusted(STAT_DEX);
    int iInt = Stat_GetAdjusted(STAT_INT);
	DIR_TYPE	Dir = pMe.GetDir(pTarg);
	bool		fUsePathfinding = false;

	EXC_TRY("NPC_WalkToPoint");
	if ( Dir >= DIR_QTY )
		return 0;		// we are already in the spot
	if ( iDex <= 0 )
		return 2;			// we cannot move now

	EXC_SET_BLOCK("NPC_AI_PATH");
	//	Use pathfinding
	if ( NPC_GetAiFlags() & NPC_AI_PATH )
	{
		NPC_Pathfinding();

		//	walk the saved path
		CPointMap local;
		local.m_x = m_pNPC->m_nextX[0];
		local.m_y = m_pNPC->m_nextY[0];
		local.m_map = pMe.m_map;

		// no steps available yet, or pathfinding not usable in this situation
		// so, use default movements
		if (local.m_x > 0 && local.m_y > 0)
		{
            fUsePathfinding = true;

			if ( pMe.GetDist(local) != 1 )
			{
				// The next step is too far away, pathfinding route has become invalid
				m_pNPC->m_nextPt.InitPoint();
				m_pNPC->m_nextX[0] = 0;
				m_pNPC->m_nextY[0] = 0;
			}
			else
			{
				// Update our heading to the way we need to go
				Dir = pMe.GetDir(local);
				ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);

				EXC_TRYSUB("Array Shift");
				//	also shift the steps array
				for ( int j = 0; j < (MAX_NPC_PATH_STORAGE_SIZE - 1); ++j )
				{
					m_pNPC->m_nextX[j] = m_pNPC->m_nextX[j+1];
					m_pNPC->m_nextY[j] = m_pNPC->m_nextY[j+1];
				}
				m_pNPC->m_nextX[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				m_pNPC->m_nextY[MAX_NPC_PATH_STORAGE_SIZE - 1] = 0;
				EXC_CATCHSUB("NPCAI");
			}
		}
	}

	EXC_SET_BLOCK("Non-Advanced pathfinding");
	pMe.Move( Dir );
	if ( ! CanMoveWalkTo(pMe, true, false, Dir ) )
	{
		CPointMap	ptFirstTry = pMe;

		// try to step around it ?
		int iDiff = 0;
		int iRand = Calc_GetRandVal( 100 );
		if ( iRand < 30 )	// do nothing.
		{
			// whilst pathfinding we should keep trying to find new ways to our destination
			if ( fUsePathfinding == true )
			{
				_SetTimeoutD( 5 ); // wait a moment before finding a new route
				return 1;
			}
			return 2;
		}

		if ( iRand < 35 )		iDiff = 4;	// 5
		else if ( iRand < 40 )	iDiff = 3;	// 10
		else if ( iRand < 65 )	iDiff = 2;
		else					iDiff = 1;
		if ( iRand & 1 )		iDiff = -iDiff;
		pMe = GetTopPoint();
		Dir = GetDirTurn( Dir, iDiff );
		pMe.Move( Dir );
		if ( ! CanMoveWalkTo(pMe, true, false, Dir ))
		{
			bool fClearedWay = false;
			// Some object in my way that i could move ? Try to move it.
			if ( !Can(CAN_C_USEHANDS) || Can(CAN_C_STATUE) || IsStatFlag(STATF_DEAD|STATF_SLEEPING|STATF_FREEZE|STATF_STONE) )
                ;   // i cannot use hands or i am frozen, so cannot move objects
			else if ( (NPC_GetAiFlags() & NPC_AI_MOVEOBSTACLES) && (iInt > iRand) )
			{
				int			i;
				CPointMap	point;
				for ( i = 0; i < 2; ++i )
				{
					if ( !i )	point = pMe;
					else		point = ptFirstTry;

					//	Scan point for items that could be moved by me and move them to my position
					CWorldSearch	AreaItems(point);
					for (;;)
					{
						CItem *pItem = AreaItems.GetItem();
						if ( !pItem )	break;
						else if ( abs(pItem->GetTopZ() - pMe.m_z) > 3 )		continue;		// item is too high
						else if ( !pItem->Can(CAN_I_BLOCK) )				continue;		// this item not blocking me
						else if ( !CanMove(pItem) || !CanCarry(pItem) )		fClearedWay = false;
						else
						{
							//	move this item to the position I am currently in
							pItem->MoveToUpdate(GetTopPoint());
                            fClearedWay = true;
							break;
						}
					}

					if ( fClearedWay )
						break;
					//	If not cleared the way still, but I am still clever enough
					//	I should try to move in the first step I was trying to move to
					else if ( iInt < iRand*3 )
						break;
				}

				//	we have just cleared our way
				if ( fClearedWay )
				{
					if ( point == ptFirstTry )
					{
						Dir = GetTopPoint().GetDir(m_Act_p);
						ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);
						if (Dir >= DIR_QTY)
                            fClearedWay = false;
					}
				}
			}
			if ( !fClearedWay )
			{
				// whilst pathfinding we should keep trying to find new ways to our destination
				if ( fUsePathfinding )
				{
					_SetTimeoutD( 5 ); // wait a moment before finding a new route
					return 1;
				}
				return 2;
			}
		}
	}

	EXC_SET_BLOCK("Finishing Move Action a");
	//Finish Move Action

	// ??? Make sure we are not facing a wall.
	ASSERT(Dir > DIR_INVALID && Dir < DIR_QTY);
	m_dirFace = Dir;	// Face this direction.
	if ( fRun && ( ! Can(CAN_C_RUN|CAN_C_FLY) || Stat_GetVal(STAT_DEX) <= 1 ))
		fRun = false;

	EXC_SET_BLOCK("StatFlag");
	StatFlag_Mod(STATF_FLY, fRun);

	EXC_SET_BLOCK("Old Top Point");
	const CPointMap ptOld = GetTopPoint();

	EXC_SET_BLOCK("Reveal");
	CheckRevealOnMove();

	EXC_SET_BLOCK("MoveToChar");
	if (!MoveToChar(pMe, false, true))
        return 2;

	EXC_SET_BLOCK("Move Update");
	UpdateMove(ptOld);

	EXC_SET_BLOCK("Speed counting");
	// How fast can they move.
	int64 iTickNext;

	// TAG.OVERRIDE.MOVERATE
	int64 tTick;
    CVarDefCont * pVal = GetKey("OVERRIDE.MOVEDELAY", true);
    if (pVal)
    {
        iTickNext = pVal->GetValNum();  // foot walking speed
        if (IsStatFlag(STATF_ONHORSE | STATF_HOVERING)) // On Mount
        {
            if (IsStatFlag(STATF_FLY))  // Running
            {
                iTickNext /= 4; // 4 times faster when running while it's on a mount
            }
            else
            {
                iTickNext /= 2; // 2 times faster when walking while it's on a mount
            }
        }
        else
        {
            if (IsStatFlag(STATF_FLY))
            {
                iTickNext /= 2; // 2 times faster when running.
            }
        }
    }
    else
    {
        CVarDefCont * pValue = GetKey("OVERRIDE.MOVERATE", true);
        if (pValue)
            tTick = pValue->GetValNum();	//Taking value from tag.override.moverate
        else
            tTick = pCharDef->m_iMoveRate;	//no tag.override.moverate, we get default moverate (created from ini's one).
        // END TAG.OVERRIDE.MOVERATE
        if (fRun)
        {
            if (IsStatFlag(STATF_PET))	// pets run a little faster.
            {
                if (iDex < 75)
                    iDex = 75;
            }
            iTickNext = MSECS_PER_SEC / 4 + Calc_GetRandLLVal((100 - (iDex*tTick) / 100) / 5) * MSECS_PER_SEC / 10;   // TODO MSEC to TICK? custom timers for npc's movement?
        }
        else
            iTickNext = MSECS_PER_SEC + Calc_GetRandLLVal((100 - (iDex*tTick) / 100) / 3) * MSECS_PER_SEC / 10;
    }

	if (iTickNext < MSECS_PER_TENTH) // Do not allow less than a tenth of second. This may be decreased in the future to allow more precise timers, at the cost of cpu.
		iTickNext = MSECS_PER_TENTH;
	else if (iTickNext > 5 * MSECS_PER_SEC)  // neither more than 5 seconds.
		iTickNext = 5 * MSECS_PER_SEC;

	_SetTimeout(iTickNext);
	EXC_CATCH;
	return 1;
}

bool CChar::NPC_LookAtCharGuard( CChar * pChar, bool bFromTrigger )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharGuard");
	ASSERT(m_pNPC);

	// Does the guard hate the target ?
	//	do not waste time on invul+dead, non-criminal and jailed chars
	if ( ((pChar->IsStatFlag(STATF_INVUL|STATF_DEAD) || pChar->Can(CAN_C_STATUE) || pChar->IsPriv(PRIV_JAILED)) && !bFromTrigger) || !(pChar->Noto_IsCriminal() || pChar->Noto_IsEvil()) )
		return false;

	if ( ! pChar->m_pArea->IsGuarded())
	{
		static uint const sm_szSpeakGuardJeer[] =
		{
			DEFMSG_NPC_GUARD_THREAT_1,
			DEFMSG_NPC_GUARD_THREAT_2,
			DEFMSG_NPC_GUARD_THREAT_3,
			DEFMSG_NPC_GUARD_THREAT_4,
			DEFMSG_NPC_GUARD_THREAT_5
		};

		// At least jeer at the criminal.
		if ( Calc_GetRandVal(10))
			return false;

		tchar *pszMsg = Str_GetTemp();
		snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(sm_szSpeakGuardJeer[ Calc_GetRandVal( CountOf( sm_szSpeakGuardJeer )) ]), pChar->GetName());
		Speak(pszMsg);
		UpdateDir(pChar);
		return false;
	}

	static uint const sm_szSpeakGuardStrike[] =
	{
		DEFMSG_NPC_GUARD_STRIKE_1,
		DEFMSG_NPC_GUARD_STRIKE_2,
		DEFMSG_NPC_GUARD_STRIKE_3,
		DEFMSG_NPC_GUARD_STRIKE_4,
		DEFMSG_NPC_GUARD_STRIKE_5
	};

	if ( GetTopDist3D(pChar) > 1 )
	{
		if ( g_Cfg.m_fGuardsInstantKill || pChar->Skill_GetBase(SKILL_MAGERY) )
			Spell_Teleport(pChar->GetTopPoint(), false, false);

		// If we got intant kill guards enabled, allow the guards to swing immidiately
		if ( g_Cfg.m_fGuardsInstantKill )
		{
			pChar->Stat_SetVal(STAT_STR, 1);
			Fight_Hit(pChar);
		}
	}
	if ( !IsStatFlag(STATF_WAR) || m_Act_UID != pChar->GetUID() )
	{
		Speak(g_Cfg.GetDefaultMsg(sm_szSpeakGuardStrike[Calc_GetRandVal(CountOf(sm_szSpeakGuardStrike))]));
		Fight_Attack(pChar, true);
	}
	return true;
}

bool CChar::NPC_LookAtCharMonster( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharMonster");
	ASSERT(m_pNPC);
	// return:
	//   true = take new action.
	//   false = continue with any previous action.
	//  motivation level =
	//  0 = not at all.
	//  100 = definitly.
	//

	int iFoodLevel = Food_GetLevelPercent();

	// Attacks those not of my kind.
	if ( ! Noto_IsCriminal() && (iFoodLevel > 40) )	// Am I not evil ?
		return NPC_LookAtCharHuman( pChar );

    // Attack if i am stronger, if it's the same target i was attacking, or i'm just stupid.
    int iActMotivation = NPC_GetAttackMotivation(pChar);
    if (iActMotivation <= 0)
        return false;
	if ( iActMotivation < m_pNPC->m_Act_Motivation )
		return false;

	int iDist = GetTopDist3D( pChar );
	if ( IsStatFlag( STATF_HIDDEN ) && ! NPC_FightMayCast() && (iDist > 1) )
		return false;	// element of suprise.

	if ( Fight_Attack( pChar ) == false )
		return false;

	m_pNPC->m_Act_Motivation = (uchar)iActMotivation;
	return true;
}

bool CChar::NPC_LookAtCharHuman( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHuman");
	ASSERT(m_pNPC);

	if ( IsStatFlag(STATF_DEAD) || pChar->IsStatFlag(STATF_DEAD) || pChar->Can(CAN_C_STATUE) )
		return false;

	if ( Noto_IsEvil())		// I am evil.
	{
		// Attack others if we are evil.
		return NPC_LookAtCharMonster( pChar );
	}

	if (( ! pChar->Noto_IsEvil() && g_Cfg.m_fGuardsOnMurderers) && (! pChar->IsStatFlag( STATF_CRIMINAL ))) 	// not interesting.
		return false;

	// Yell for guard if we see someone evil.
	if (m_pArea->IsGuarded())
	{
		if (m_pNPC->m_Brain == NPCBRAIN_GUARD)
        {
			return NPC_LookAtCharGuard(pChar);
        }
		else if (NPC_CanSpeak() && !Calc_GetRandVal(3))
		{
			// Find a guard.
            if (CallGuards(pChar))
            {
                Speak(pChar->IsStatFlag(STATF_CRIMINAL) ?
                    g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SEECRIM) :
                    g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_SEEMONS));   // only speak if can really call the guards.
            }
			if (IsStatFlag(STATF_WAR))
				return false;

			// run away like a coward.
			m_Act_UID = pChar->GetUID();
			m_atFlee.m_iStepsMax = 20;		// how long should it take to get there.
			m_atFlee.m_iStepsCurrent = 0;	// how long has it taken ?
			Skill_Start(NPCACT_FLEE);
			m_pNPC->m_Act_Motivation = 80;
			return true;
		}
	}
	// Attack an evil creature ?
	return false;
}

bool CChar::NPC_LookAtCharHealer( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtCharHealer");
	ASSERT(m_pNPC);

	if ( !pChar->IsStatFlag(STATF_DEAD) || pChar->IsStatFlag(STATF_STONE) || pChar->Can(CAN_C_STATUE) || (pChar->m_pNPC && pChar->m_pNPC->m_bonded) )
		return false;

	static lpctstr const sm_szHealerRefuseEvils[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_EVIL_3 )
	};
	static lpctstr const sm_szHealerRefuseCriminals[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_CRIM_3 )
	};
	static lpctstr const sm_szHealerRefuseGoods[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_REF_GOOD_3 )
	};
	static lpctstr const sm_szHealer[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RES_5 )
	};

	UpdateDir( pChar );

	lpctstr pszRefuseMsg;

	int iDist = GetDist( pChar );
	if ( pChar->IsStatFlag( STATF_INSUBSTANTIAL ))
	{
		pszRefuseMsg = g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_MANIFEST );
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if ( iDist > 3 )
	{
		if ( Calc_GetRandVal(5))
			return false;
		Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_RANGE ) );
		return true;
	}

	// What noto is this char to me ?
	bool ImEvil = Noto_IsEvil();
	bool ImNeutral = Noto_IsNeutral();
	NOTO_TYPE NotoThem = pChar->Noto_GetFlag( this, true );

	if ( !IsStatFlag( STATF_CRIMINAL ) && NotoThem == NOTO_CRIMINAL )
	{
		pszRefuseMsg = sm_szHealerRefuseCriminals[ Calc_GetRandVal( CountOf( sm_szHealerRefuseCriminals )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if (( !ImNeutral && !ImEvil) && NotoThem >= NOTO_NEUTRAL )
	{
		pszRefuseMsg = sm_szHealerRefuseEvils[ Calc_GetRandVal( CountOf( sm_szHealerRefuseEvils )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	if (( ImNeutral || ImEvil ) && NotoThem == NOTO_GOOD )
	{
		pszRefuseMsg = sm_szHealerRefuseGoods[ Calc_GetRandVal( CountOf( sm_szHealerRefuseGoods )) ];
		if ( Calc_GetRandVal(5) || iDist > 3 )
			return false;
		Speak( pszRefuseMsg );
		return true;
	}

	// Attempt to res.
	Speak( sm_szHealer[ Calc_GetRandVal( CountOf( sm_szHealer )) ] );
	UpdateAnimate( ANIM_CAST_AREA );
	if ( ! pChar->OnSpellEffect( SPELL_Resurrection, this, 1000, nullptr ))
	{
		if ( Calc_GetRandVal(2))
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_1 ) );
		else
			Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_HEALER_FAIL_2 ) );
	}

	return true;
}

// I might want to go pickup this item ?
bool CChar::NPC_LookAtItem( CItem * pItem, int iDist )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtItem");
	ASSERT(m_pNPC);

	if ( !Can(CAN_C_USEHANDS) || !CanSee(pItem) )
		return false;

	int iWantThisItem = NPC_WantThisItem(pItem);
	if ( IsTrigUsed(TRIGGER_NPCLOOKATITEM) )
	{
		if ( IsTrigUsed(TRIGGER_NPCLOOKATITEM) && !pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_LOCKEDDOWN|ATTR_SECURE) )
		{

			CScriptTriggerArgs	Args(iDist, iWantThisItem, pItem);
			switch ( OnTrigger(CTRIG_NPCLookAtItem, this, &Args) )
			{
				case  TRIGRET_RET_TRUE:		return true;
				case  TRIGRET_RET_FALSE:	return false;
				default:					break;
			}
			iWantThisItem = (int)(Args.m_iN2);
		}
	}

	// Loot nearby items on ground
	if ( iWantThisItem > Calc_GetRandVal(100) )
	{
		m_Act_UID = pItem->GetUID();
		NPC_Act_Looting();
		return true;
	}

	// Loot nearby corpses
	if ( pItem->IsType(IT_CORPSE) && (NPC_GetAiFlags() & NPC_AI_LOOTING) && (Memory_FindObj(pItem) == nullptr) )
	{
		m_Act_UID = pItem->GetUID();
		NPC_Act_Looting();
		return true;
	}

	// Check for doors we can open
	if ( pItem->IsType(IT_DOOR) && GetDist(pItem) <= 1 && CanTouch(pItem) && !Calc_GetRandVal(2) )
	{
		if ( pItem->IsDoorOpen() )	// door is already open
			return false;

		UpdateDir(pItem);
		if ( !Use_Item(pItem) )	// try to open it
			return false;

		// Walk through it
		CPointMap pt = GetTopPoint();
		pt.MoveN(GetDir(pItem), 2);
		if ( CanMoveWalkTo(pt) )
		{
			m_Act_p = pt;
			NPC_WalkToPoint();
			return true;
		}
	}

	return false;
}

bool CChar::NPC_LookAtChar( CChar * pChar, int iDist )
{
	ADDTOCALLSTACK("CChar::NPC_LookAtChar");
	ASSERT(m_pNPC);
	// I see a char.
	// Do I want to do something to this char (more that what i'm already doing ?)
	// RETURN:
	//   true = yes i do want to take a new action.

	// Don't call this function frequently, since CanSeeLOS is an expensive function (lot of calculations but
	//	most importantly it has to read the UO files, and file I/O is slow)
	if ( !pChar || ( pChar == this ) || !CanSeeLOS(pChar,LOS_NB_WINDOWS) ) //Flag - we can attack through a window
		return false;

	if ( IsTrigUsed(TRIGGER_NPCLOOKATCHAR) )
	{
		switch ( OnTrigger(CTRIG_NPCLookAtChar, pChar) )
		{
			case  TRIGRET_RET_TRUE:		return true;
			case  TRIGRET_RET_FALSE:	return false;
			default:					break;
		}
	}

	if (m_pNPC->m_Brain != NPCBRAIN_BERSERK)
	{
		if ( NPC_IsOwnedBy( pChar, false ))
		{
			// follow my owner again. (Default action)
			m_Act_UID = pChar->GetUID();
			m_pNPC->m_Act_Motivation = 50;
			Skill_Start( (Skill_GetActive() == NPCACT_FOLLOW_TARG) ? NPCACT_FOLLOW_TARG : NPCACT_GUARD_TARG);
			return true;
		}
		else
		{
			// initiate a conversation ?
			if ( ! IsStatFlag( STATF_WAR ) &&
				( (Skill_GetActive() == SKILL_NONE) || (Skill_GetActive() == NPCACT_WANDER) ) && // I'm idle
				pChar->m_pPlayer &&
				! Memory_FindObjTypes( pChar, MEMORY_SPEAK ))
			{
				if ( IsTrigUsed(TRIGGER_NPCSEENEWPLAYER) )
				{
					if ( OnTrigger( CTRIG_NPCSeeNewPlayer, pChar ) != TRIGRET_RET_TRUE )
					{
						// record that we attempted to speak to them.
						CItemMemory * pMemory = Memory_AddObjTypes( pChar, MEMORY_SPEAK );
						if ( pMemory )
							pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_FIRSTSPEAK;
						// m_Act_Hear_Unknown = 0;
					}
				}
			}
		}
	}
	
	if (IsStatFlag(STATF_DEAD))
		return false;

	switch ( m_pNPC->m_Brain )	// my type of brain
	{
		case NPCBRAIN_GUARD:
			// Guards should look around for criminals or nasty creatures.
			if ( NPC_LookAtCharGuard( pChar ))
				return true;
			break;

		case NPCBRAIN_MONSTER:
		case NPCBRAIN_DRAGON:
			if ( NPC_LookAtCharMonster( pChar ))
				return true;
			break;

		case NPCBRAIN_BERSERK:
			// Blade Spirits or Energy Vortex.
			// Attack everyone you see!
			if ( Fight_IsActive()) // Is this a better target than my last ?
			{
				CChar * pCharTarg = m_Act_UID.CharFind();
				if ( pCharTarg && (GetTopDist3D(pCharTarg) <= iDist) )
						return true;
			}
			if ( Fight_Attack( pChar ) )
				return true;
			break;

		case NPCBRAIN_HEALER:
			// Healers should look around for ghosts.
			if ( NPC_LookAtCharHealer( pChar ))
				return true;
			if ( NPC_LookAtCharHuman( pChar ))
				return true;
			break;

		case NPCBRAIN_BANKER:
		case NPCBRAIN_VENDOR:
		case NPCBRAIN_STABLE:
		case NPCBRAIN_ANIMAL:
		case NPCBRAIN_HUMAN:
			if ( NPC_LookAtCharHuman(pChar) )
				return true;
			break;

		default:
			break;
	}

	return false;
}

bool CChar::NPC_LookAround( bool fForceCheckItems )
{
	ADDTOCALLSTACK("CChar::NPC_LookAround");
	ASSERT(m_pNPC);
	// Take a look around for other people/chars.
	// We may be doing something already. So check current action motivation level.
	// RETURN:
	//   true = found something better to do.

	CSector *pSector = GetTopSector();
	if ( !m_pNPC || !pSector )
		return false;

    // Call the rand function once, since repeated calls can be expensive (and this function is called a LOT of times, if there are lots of active NPCs)
    const int iRand = Calc_GetRandVal(UO_MAP_VIEW_RADAR);
    const CPointMap& ptTop = GetTopPoint();
	
    int iRange = GetVisualRange();
    if (iRange > UO_MAP_VIEW_RADAR)
        iRange = UO_MAP_VIEW_RADAR;
	int iRangeBlur = UO_MAP_VIEW_SIGHT;

	// If I can't move don't look too far.
	if ( Can(CAN_C_NONMOVER) || !Can(CAN_C_MOVEMENTCAPABLEMASK) || IsStatFlag(STATF_FREEZE|STATF_STONE) )
	{
		if ( !NPC_FightMayCast() )	// And i have no distance attack.
			iRange = iRangeBlur = 2;
	}
	else
	{
		// I'm mobile. do basic check if i would move here first.
		if ( !NPC_CheckWalkHere(ptTop, m_pArea) )
		{
			// I should move. Someone lit a fire under me.
			m_Act_p = ptTop;
			m_Act_p.Move((DIR_TYPE)(iRand % DIR_QTY));
			NPC_WalkToPoint(true);
			SoundChar(CRESND_NOTICE);
			return true;
		}

		if ( Stat_GetAdjusted(STAT_INT) < 50 )
			iRangeBlur /= 2;
	}

	// If sector is too complex, lower the number of chars we look at to keep some performance
	if ( pSector->GetCharComplexity() > (g_Cfg.m_iMaxCharComplexity / 2) )
		iRange /= 4;

	// Any interesting chars here ?
	int iDist = 0;
	CChar *pChar = nullptr;
	CWorldSearch AreaChars(ptTop, iRange);
	for (;;)
	{
		pChar = AreaChars.GetChar();
		if ( !pChar )
			break;
		if ( pChar == this )	// just myself.
			continue;

		iDist = GetTopDist3D(pChar);
		if ( iDist > iRangeBlur )
		{
			if (iRand % iDist )
				continue;	// can't see them.
		}
		if ( NPC_LookAtChar(pChar, iDist) )		// expensive function call
		{
			SoundChar(CRESND_NOTICE);
			return true;
		}
	}

	// Check the ground for good stuff.
	if ( !fForceCheckItems && (Stat_GetAdjusted(STAT_INT) > 10) && !IsSkillBase(Skill_GetActive()) && !(iRand % 3) )
		fForceCheckItems = true;

	if ( fForceCheckItems )
	{
		CItem *pItem = nullptr;
		CWorldSearch AreaItems(ptTop, iRange);
		for (;;)
		{
			pItem = AreaItems.GetItem();
			if ( !pItem )
				break;

			iDist = GetTopDist3D(pItem);
			if ( iDist > iRangeBlur )
			{
				if ( iRand % iDist )
					continue;	// can't see them.
			}
			if ( NPC_LookAtItem(pItem, iDist) )
			{
				if (!IsPlayableCharacter())
					SoundChar(CRESND_NOTICE);
				return true;
			}
		}
	}

	if ( !IsPlayableCharacter() && ( (m_pNPC->m_Brain == NPCBRAIN_BERSERK) || !(iRand % 6) ) )
		SoundChar(CRESND_IDLE);

	// Move stuff that is in our way ? (chests etc.)
	return false;
}

void CChar::NPC_Act_Wander()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Wander");
	ASSERT(m_pNPC);
	// NPCACT_WANDER
	// just wander aimlessly. (but within bounds)
	// Stop wandering and re-eval frequently

	if ( Can(CAN_C_NONMOVER) )
		return;

    // Call the rand function once, since repeated calls can be expensive (and this function is called a LOT of times, if there are lots of active NPCs)
    const int iRand = Calc_GetRandVal(UINT16_MAX);
	int iStopWandering = 0;

	if ( !(iRand % (7 + (Stat_GetVal(STAT_DEX) / 30))) )
		iStopWandering = 1;			// i'm stopping to wander "for the dexterity". 

	if ( !(iRand % (2 + TICKS_PER_SEC/2)) )
	{
		// NPC_LookAround() is very expensive, so since NPC_Act_Wander is called every tick for every char with ACTION == NPCACT_WANDER,
		//	don't look around every time.
		if ( NPC_LookAround() )
			iStopWandering = 2;		// i'm stopping to wander because i have seen something interesting 
	}

	// Staggering Walk around.
	m_Act_p = GetTopPoint();
	m_Act_p.Move( GetDirTurn(m_dirFace, 1 - (iRand % 3)) );

	int iReturnToHome = 0;

	if ( m_pNPC->m_Home_Dist_Wander && m_ptHome.IsValidPoint() )
	{
		if ( m_Act_p.GetDist(m_ptHome) > m_pNPC->m_Home_Dist_Wander )
			iReturnToHome = 1;
	}

	if (IsTrigUsed(TRIGGER_NPCACTWANDER))
	{
		CScriptTriggerArgs Args(iStopWandering, iReturnToHome);
		if (OnTrigger(CTRIG_NPCActWander, this, &Args) == TRIGRET_RET_TRUE)
			return;

		iStopWandering = (int)Args.m_iN1;
		iReturnToHome = (int)Args.m_iN2;
	}

	if (iStopWandering)
		Skill_Start( SKILL_NONE );
	else
	{
		if (iReturnToHome)
			Skill_Start(NPCACT_GO_HOME);
		else
			NPC_WalkToPoint();
	}
}

void CChar::NPC_Act_Guard()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Guard");
	ASSERT(m_pNPC);
	// Protect our target or owner. (m_Act_UID)

	CChar * pChar = m_Act_UID.CharFind();
	if ( pChar != nullptr && pChar != this && CanSeeLOS(pChar, LOS_NB_WINDOWS) )
	{
		if ( pChar->Fight_IsActive() )	// protect the target if they're in a fight
		{
			if ( Fight_Attack( pChar->m_Fight_Targ_UID.CharFind() ))
				return;
		}
	}

	// Target is out of range or doesn't need protecting, so just follow for now
	//NPC_LookAtChar(pChar, 1);
	NPC_Act_Follow();
}

bool CChar::NPC_Act_Follow(bool fFlee, int maxDistance, bool fMoveAway)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Follow");
	ASSERT(m_pNPC);
	// Follow our target or owner (m_Act_UID), we may be fighting (m_Fight_Targ_UID).
	// false = can't follow any more, give up.

	if (Can(CAN_C_NONMOVER))
		return false;

	EXC_TRY("NPC_Act_Follow");

	/*
	* Replaced the Fight_IsActive() check with a check on m_fight_targ_UID.
	* Red npcs usually never interact "in a peaceful way" with the players and thus m_act_UID is usually never set preventing the creature from fleeing and putting it in an immobile "state".
	* Fight_IsActive returns true if the character is actively fighting (using a combat skill) in this case it's false because of the NPC'sFleeing
	* Action and thus it will never pass the Fight_IsActive() check
	*/
	//CChar * pChar =  Fight_IsActive() ? m_Fight_Targ_UID.CharFind() : m_Act_UID.CharFind();

	CChar* pChar = nullptr;
	//If the NPC action is following somebody, directly assign the character from  the m_Act_UID value. 
	if (Skill_GetActive() == NPCACT_FOLLOW_TARG)
		pChar = m_Act_UID.CharFind();
	else
		pChar = m_Fight_Targ_UID.IsValidUID() ? m_Fight_Targ_UID.CharFind() : m_Act_UID.CharFind();
	if (pChar == nullptr)
	{
		// free to do as i wish !
		Skill_Start(SKILL_NONE);
		return false;
	}

	EXC_SET_BLOCK("Trigger");
	if (IsTrigUsed(TRIGGER_NPCACTFOLLOW))
	{
		CScriptTriggerArgs Args(fFlee, maxDistance, fMoveAway);
		switch (OnTrigger(CTRIG_NPCActFollow, pChar, &Args))
		{
		    case TRIGRET_RET_TRUE:
            {
                return false;
            }
		    case TRIGRET_RET_FALSE:
            {
                return true;
            }
		    default:
            {
                break;
            }
		}

		fFlee = (Args.m_iN1 != 0);
		maxDistance = static_cast<int>(Args.m_iN2);
		fMoveAway = (Args.m_iN3 != 0);
	}

	EXC_SET_BLOCK("CanSee");
	// Have to be able to see target to follow.
	if (CanSee(pChar))
	{
		m_Act_p = pChar->GetTopPoint();
	}
	else
	{
		// Monster may get confused because he can't see you.
		// There is a chance they could forget about you if hidden for a while.
        if (fFlee || !Calc_GetRandVal(1 + ((100 - Stat_GetAdjusted(STAT_INT)) / 20)))
        {
            return false;
        }
	}

	EXC_SET_BLOCK("Distance checks");
	const CPointMap& ptMe = GetTopPoint();
	int dist = ptMe.GetDist(m_Act_p);
    if (dist > UO_MAP_VIEW_RADAR)		// too far away ?
    {
        return false;
    }

	if (fMoveAway)
	{
        if (dist < maxDistance)
        {
            fFlee = true;	// start moving away
        }
	}
	else
	{
		if (fFlee)
		{
            if (dist >= maxDistance)
            {
                return false;
            }
		}
        else if (dist <= maxDistance)
        {
            return true;
        }
	}

	EXC_SET_BLOCK("Fleeing");
	if (fFlee)
	{
		CPointMap ptOld = m_Act_p;
		m_Act_p = ptMe;
		m_Act_p.Move(GetDirTurn(m_Act_p.GetDir(ptOld), 4 + 1 - Calc_GetRandVal(3)));
		int iRet = NPC_WalkToPoint(dist > 3);
		m_Act_p = ptOld;	// last known point of the enemy.
		return (iRet < 2);  // 2 = fail
	}

	EXC_SET_BLOCK("WalkToPoint 1");
	return (NPC_WalkToPoint(IsStatFlag(STATF_WAR) ? true : (dist > 3)) < 2);    // 2 = fail

	EXC_CATCH;
	return false;
}

bool CChar::NPC_Act_Talk()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Talk");
	ASSERT(m_pNPC);
	// NPCACT_TALK:
	// NPCACT_TALK_FOLLOW
	// RETURN:
	//  false = do something else. go Idle
	//  true = just keep waiting.

	CChar * pChar = m_Act_UID.CharFind();
	if ( pChar == nullptr )	// they are gone ?
		return false;

	// too far away.
	int iDist = GetTopDist3D( pChar );
	if (( iDist >= UO_MAP_VIEW_SIGHT ) || ( m_ptHome.GetDist3D( pChar->GetTopPoint() ) > m_pNPC->m_Home_Dist_Wander ))	// give up.
		return false;

	// can't see them
	if ( !CanSee( pChar ) )
		return false;

	if ( Skill_GetActive() == NPCACT_TALK_FOLLOW && iDist > 3 )
	{
		// try to move closer.
		if ( ! NPC_Act_Follow( false, 4, false ) )
			return false;
	}

	if ( m_atTalk.m_dwWaitCount <= 1 )
	{
		if ( NPC_CanSpeak() )
		{
			static lpctstr const sm_szText[] =
			{
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_1 ),
				g_Cfg.GetDefaultMsg( DEFMSG_NPC_GENERIC_GONE_2 )
			};
			tchar *pszMsg = Str_GetTemp();
			snprintf(pszMsg, STR_TEMPLENGTH, sm_szText[ Calc_GetRandVal(CountOf(sm_szText)) ], pChar->GetName() );
			Speak(pszMsg);
		}
		return false;
	}

	--m_atTalk.m_dwWaitCount;
	return true;	// just keep waiting.
}

void CChar::NPC_Act_GoHome()
{
	ADDTOCALLSTACK("CChar::NPC_Act_GoHome");
	ASSERT(m_pNPC);
	// NPCACT_GO_HOME
	// If our home is not valid then

	if ( !Calc_GetRandVal(3) && NPC_LookAround())
		return;

	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )
	{
		// Had to change this guards were still roaming the forests
		// this goes hand in hand with the change that guards arent
		// called if the criminal makes it outside guarded territory.

		const CRegion * pAreaHome = m_ptHome.GetRegion( REGION_TYPE_AREA );
		if ( pAreaHome && pAreaHome->IsGuarded())
		{
			if ( !m_pArea || !m_pArea->IsGuarded() )
			{
				if ( Spell_Teleport(m_ptHome, false, false) )
				{
					Skill_Start(SKILL_NONE);
					return;
				}
			}
		}
		else if (!IsSetOF(OF_GuardOutsideGuardedArea))
		{
			g_Log.Event( LOGL_WARN, "Guard 0%x '%s' has no guard post (%s)! Removing it.\n", (dword)GetUID(), GetName(), GetTopPoint().WriteUsed());

			// If we arent conjured and still got no valid home
			// then set our status to conjured and take our life.
			if ( ! IsStatFlag(STATF_CONJURED))
			{
				StatFlag_Set( STATF_CONJURED );
				Stat_SetVal(STAT_STR, 0);
				return;
			}
            // else we are conjured and probably a timer started already.
		}
	}

    const CPointMap ptCurrent = GetTopPoint();
	if ( !m_ptHome.IsValidPoint() || !ptCurrent.IsValidPoint() || ( ptCurrent.GetDist(m_ptHome) < m_pNPC->m_Home_Dist_Wander ))
	{
   		Skill_Start(SKILL_NONE);
		return;
	}

	if ( g_Cfg.m_iLostNPCTeleport )
	{
		const int iDistance = m_ptHome.GetDist( ptCurrent );
		if ( (iDistance > g_Cfg.m_iLostNPCTeleport) && (iDistance > m_pNPC->m_Home_Dist_Wander) )
		{
			if ( IsTrigUsed(TRIGGER_NPCLOSTTELEPORT) )
			{
				CScriptTriggerArgs Args(iDistance);	// ARGN1 - distance
				if ( OnTrigger(CTRIG_NPCLostTeleport, this, &Args) != TRIGRET_RET_TRUE )
					Spell_Teleport(m_ptHome, true, false);
			}
			else
			{
				Spell_Teleport(m_ptHome, true, false);
			}
		}
	}

   	m_Act_p = m_ptHome;
   	if ( !NPC_WalkToPoint() ) // get there
   	{
   		Skill_Start(SKILL_NONE);
		return;
	}
}

void CChar::NPC_LootMemory( CItem * pItem )
{
	ADDTOCALLSTACK("CChar::NPC_LootMemory");
	ASSERT(m_pNPC);
	// Create a memory of this item.
	// I have already looked at it.

	CItem * pMemory = Memory_AddObjTypes( pItem, MEMORY_SPEAK );
	pMemory->m_itEqMemory.m_Action = NPC_MEM_ACT_IGNORE;

	// If the item is set to decay.
	if (pItem->IsTimerSet())
	{
		const int64 iTimerDiff = pItem->GetTimerAdjusted();
		if (iTimerDiff > 0)
			pMemory->SetTimeout(iTimerDiff);		// forget about this once the item is gone
	}
}

void CChar::NPC_Act_Looting()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Looting");
	ASSERT(m_pNPC);
	// We killed something, let's take a look on the corpse.
	// Or we find something interesting on ground
	//
	// m_Act_UID = UID of the item/corpse that we trying to loot

	if ( m_pArea->IsFlag(REGION_FLAG_SAFE|REGION_FLAG_GUARDED) )
		return;
	if (!(NPC_GetAiFlags() & NPC_AI_LOOTING))
		return;
	if (m_pNPC->m_Brain != NPCBRAIN_MONSTER || !Can(CAN_C_USEHANDS) || IsStatFlag(STATF_CONJURED | STATF_PET) || (GetKeyNum("DEATHFLAGS") & DEATH_NOCORPSE))
		return;

	CItem * pItem = m_Act_UID.ItemFind();
	if ( pItem == nullptr )
		return;

	if ( GetDist(pItem) > 2 )	// move toward it
	{
		NPC_WalkToPoint();
		return;
	}

	CItemCorpse * pCorpse = dynamic_cast<CItemCorpse *>(pItem);
	if ( pCorpse && !pCorpse->IsContainerEmpty() )
		pItem = static_cast<CItem*>( pCorpse->GetContentIndex(Calc_GetRandVal( (int)pCorpse->GetContentCount() )) );

	if ( !CanTouch(pItem) || !CanMove(pItem) || !CanCarry(pItem) )
	{
		NPC_LootMemory(pItem);
		return;
	}

	if ( IsTrigUsed(TRIGGER_NPCSEEWANTITEM) )
	{
		CScriptTriggerArgs Args(pItem);
		if ( OnTrigger(CTRIG_NPCSeeWantItem, this, &Args) == TRIGRET_RET_TRUE )
			return;
	}

	if ( pCorpse )
		Speak(g_Cfg.GetDefaultMsg(DEFMSG_LOOT_RUMMAGE), HUE_TEXT_DEF, TALKMODE_EMOTE);

	ItemBounce(pItem, false);
}

void CChar::NPC_Act_Flee()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Flee");
	ASSERT(m_pNPC);
	// NPCACT_FLEE
	// I should move faster this way.
	// ??? turn to strike if they are close.
	if ( ++ m_atFlee.m_iStepsCurrent >= m_atFlee.m_iStepsMax )
	{
		Skill_Start( SKILL_NONE );
		return;
	}
	if ( ! NPC_Act_Follow( true, m_atFlee.m_iStepsMax ))
	{
		Skill_Start( SKILL_NONE );
		return;
	}
}

void CChar::NPC_Act_Runto(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Runto");
	ASSERT(m_pNPC);
	// NPCACT_RUNTO:
	// Still trying to get to this point.

	switch ( NPC_WalkToPoint(true))
	{
		case 0:
			// We reached our destination
			NPC_Act_Idle();	// look for something new to do.
			break;
		case 1:
			// Took a step....keep trying to get there.
			break;
		case 2:
			// Give it up...
			// Go directly there...
			if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )
			{
				const CPointMap& ptMe = GetTopPoint();
				if (!ptMe.IsValidPoint())
				{
					--iDist;
				}
				else
				{
					const int iPDist = m_Act_p.GetDist(ptMe);
					iDist = iDist > iPDist ? iPDist : iDist - 1;
				}

				if (iDist)
					NPC_Act_Runto(iDist);
				else
					NPC_Act_Idle();
			}
			else
			{
				if (m_Act_p.IsValidPoint() && IsPlayableCharacter() && !IsStatFlag(STATF_FREEZE | STATF_STONE))
					Spell_Teleport(m_Act_p, true, false);
				else
					NPC_Act_Idle();
			}
			break;
	}
}

void CChar::NPC_Act_Goto(int iDist)
{
	ADDTOCALLSTACK("CChar::NPC_Act_Goto");
	ASSERT(m_pNPC);
	// NPCACT_GOTO:
	// Still trying to get to this point.

	switch ( NPC_WalkToPoint())
	{
		case 0:
			// We reached our destination
			NPC_Act_Idle();	// look for something new to do.
			break;
		case 1:
			// Took a step....keep trying to get there.
			break;
		case 2:
			// Give it up...
			// Go directly there...
			if ( NPC_GetAiFlags() & NPC_AI_PERSISTENTPATH )
			{
				const CPointMap& ptMe = GetTopPoint();
				if (!ptMe.IsValidPoint())
				{
					--iDist;
				}
				else
				{
					const int iPDist = m_Act_p.GetDist(ptMe);
					iDist = iDist > iPDist ? iPDist : iDist - 1;
				}

				if (iDist)
					NPC_Act_Runto(iDist);
				else
					NPC_Act_Idle();
			}
			else
			{
				if (m_Act_p.IsValidPoint() && IsPlayableCharacter() && !IsStatFlag(STATF_FREEZE | STATF_STONE))
					Spell_Teleport(m_Act_p, true, false);
				else
					NPC_Act_Idle();	// look for something new to do.
			}
			break;
	}
}

bool CChar::NPC_Act_Food()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Food");
	ASSERT(m_pNPC);

	const int iFood = Stat_GetVal(STAT_FOOD);
	const int iFoodLevel = Food_GetLevelPercent();
	if ( iFood >= 10 )
		return false;							//	search for food is starving or very hungry
	if ( iFoodLevel > 40 )
		return false;							// and it is at least 60% hungry

	m_pNPC->m_Act_Motivation = (byte)(50 - (iFoodLevel / 2));

    const int   iMyZ = GetTopPoint().m_z;
	ushort  uiEatAmount = 1;
	int     iSearchDistance = 2;
	CItem   *pClosestFood = nullptr;
	int     iClosestFood = 100;
	
	bool    fSearchGrass = false;
	CItem   *pCropItem = nullptr;

	CItemContainer	*pPack = GetPack();
	if ( pPack )
	{
		for (CSObjContRec* pObjRec : *pPack)
		{
			CItem* pFood = static_cast<CItem*>(pObjRec);
			// I have some food personaly, so no need to search for something
			if ( pFood->IsType(IT_FOOD) )
			{
				if ( (uiEatAmount = Food_CanEat(pFood)) > 0 )
				{
					Use_EatQty(pFood, uiEatAmount);
					return true;
				}
			}
		}
	}

	// Search for food nearby
	iSearchDistance = (UO_MAP_VIEW_SIGHT * ( 100 - iFoodLevel ) ) / 100;
	CWorldSearch AreaItems(GetTopPoint(), minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
	for (;;)
	{
		CItem * pItem = AreaItems.GetItem();
		if ( !pItem )
			break;
		if ( !CanSee(pItem) )
			continue;

		if ( pItem->IsType(IT_CROPS) || pItem->IsType(IT_FOLIAGE) )
		{
			// is it ripe?
			const CItemBase * checkItemBase = pItem->Item_GetDef();
			if ( checkItemBase->m_ttNormal.m_tData3 )
			{
				// remember this, just in case we do not find any suitable food
				pCropItem = pItem;
				continue;
			}
		}

        const CPointMap& ptItem = pItem->GetTopPoint();
		if (ptItem.m_z > (iMyZ + 10) || ptItem.m_z < (iMyZ - 1) )
			continue;
		if ( pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) )
			continue;

		if ( (uiEatAmount = Food_CanEat(pItem)) > 0 )
		{
			const int iDist = GetDist(pItem);
			if ( pClosestFood )
			{
				if ( iDist < iClosestFood )
				{
					pClosestFood = pItem;
					iClosestFood = iDist;
				}
			}
			else
			{
				pClosestFood = pItem;
				iClosestFood = iDist;
			}
		}
	}

	if ( pClosestFood )
	{
		if ( iClosestFood <= 1 )
		{
			//	can take and eat just in place
			ushort uiEaten = (ushort)(pClosestFood->ConsumeAmount(uiEatAmount));
			EatAnim(pClosestFood->GetName(), uiEaten);
			if ( !pClosestFood->GetAmount() )
			{
				pClosestFood->Plant_CropReset();	// set growth if this is a plant
			}
		}
		else
		{
			//	move towards this item
			switch ( m_Act_SkillCurrent )
			{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_GO_HOME:
				case NPCACT_NAPPING:
				case NPCACT_FLEE:
					{
						CPointMap pt = pClosestFood->GetTopPoint();
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							Skill_Start(NPCACT_GOTO);
							return true;
							//NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
						}
						break;
					}
				default:
					break;
			}
		}
	}
	else
	{
        // no food around, but maybe i am ok with grass? Or shall I try to pick crops?

		const NPCBRAIN_TYPE brain = GetNPCBrainGroup();
		if ( brain == NPCBRAIN_ANIMAL )						// animals eat grass always
			fSearchGrass = true;
		//else if (( brain == NPCBRAIN_HUMAN ) && !iFood )	// human eat grass if starving nearly to death
		//	fSearchGrass = true;

		// found any crops or foliage at least (nearby, of course)?
		if ( pCropItem )
		{
			if ( GetDist(pCropItem) < 5 )
			{
				Use_Item(pCropItem);
				fSearchGrass = false;	// no need to eat grass if at next tick we can eat better stuff
			}
		}
	}
	if ( fSearchGrass )
	{
        const CCharBase *pCharDef = Char_GetDef();
        const CResourceID rid = CResourceID(RES_TYPEDEF, IT_GRASS);

		if ( pCharDef->m_FoodType.ContainsResourceID(rid) ) // do I accept grass as food?
		{
			CItem *pResBit = CWorldMap::CheckNaturalResource(GetTopPoint(), IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				ushort uiEaten = pResBit->ConsumeAmount(10);
				EatAnim("grass", uiEaten/10);

				//	the bit is not needed in a worldsave, timeout of 10 minutes
				pResBit->m_TagDefs.SetNum("NOSAVE", 1);
				pResBit->SetTimeoutS(60*10);
				//DEBUG_ERR(("Starting skill food\n"));
				Skill_Start( NPCACT_FOOD );
				_SetTimeoutS(5);
				return true;
			}
			else									//	search for grass nearby
			{
				CPointMap pt = CWorldMap::FindTypeNear_Top(GetTopPoint(), IT_GRASS, minimum(iSearchDistance,m_pNPC->m_Home_Dist_Wander));
				if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
				{
					if (( pt.m_x != GetTopPoint().m_x ) && ( pt.m_y != GetTopPoint().m_y ) && ( pt.m_map == GetTopPoint().m_map ))
					{
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							//DEBUG_ERR(("NPCACT_GOTO started; pt.x %d pt.y %d\n",pt.m_x,pt.m_y));
							Skill_Start(NPCACT_GOTO);
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

void CChar::NPC_Act_Idle()
{
	ADDTOCALLSTACK("CChar::NPC_Act_Idle");
	ASSERT(m_pNPC);
	// Free to do as we please. decide what we want to do next.
	// Idle NPC's should try to take some action.

	m_pNPC->m_Act_Motivation = 0;	// we have no motivation to do anything.

	if ( NPC_GetAiFlags()&NPC_AI_INTFOOD )
	{
		bool fFood = NPC_Act_Food();
		//DEBUG_ERR(("fFood %d\n",fFood));
		if ( fFood ) //are we hungry?
			return;
	}

	if ( NPC_LookAround() )
		return;			// found something interesting

	// ---------- If we found nothing else to do. do this. -----------

	// If guards are found outside guarded territories, do the following.
	if ( m_pNPC->m_Brain == NPCBRAIN_GUARD && !m_pArea->IsGuarded() && m_ptHome.IsValidPoint())
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	// Specific creature random actions.
	if ( Stat_GetVal(STAT_DEX) >= Stat_GetAdjusted(STAT_DEX) && !Calc_GetRandVal(3) )
	{
		if ( IsTrigUsed(TRIGGER_NPCSPECIALACTION) )
		{
			if ( OnTrigger( CTRIG_NPCSpecialAction, this ) == TRIGRET_RET_TRUE )
				return;
		}

		switch ( GetDispID())
		{
			case CREID_FIRE_ELEM:
				if ( !CWorldMap::IsItemTypeNear(GetTopPoint(), IT_FIRE, 0, false) )
				{
					Action_StartSpecial(CREID_FIRE_ELEM);
					return;
				}
				break;

			default:
				// TAG.OVERRIDE.SPIDERWEB
				CVarDefCont * pValue = GetKey("OVERRIDE.SPIDERWEB",true);
				if ( pValue )
				{
					if ( GetDispID() != CREID_GIANT_SPIDER )
					{
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				} else {
					if ( GetDispID() == CREID_GIANT_SPIDER )
					{
						Action_StartSpecial(CREID_GIANT_SPIDER);
						return;
					}
				}
		}
	}

	// Periodically head home.
	if ( m_ptHome.IsValidPoint() && ! Calc_GetRandVal( 15 ))
	{
		Skill_Start(NPCACT_GO_HOME);
		return;
	}

	//	periodically use hiding skill
	if ( Skill_GetBase(SKILL_HIDING) > 30 &&
		! Calc_GetRandVal( 15 - Skill_GetBase(SKILL_HIDING)/100) &&
		!m_pArea->IsGuarded())
	{
		// Just hide here.
		if ( !IsStatFlag(STATF_HIDDEN) )
		{
			Skill_Start(SKILL_HIDING);
			return;
		}
	}

	if ( Calc_GetRandVal( 100 - Stat_GetAdjusted(STAT_DEX)) < 25 )
	{
		// dex determines how jumpy they are.
		// Decide to wander about ?
		Skill_Start( NPCACT_WANDER );
		return;
	}

	// just stand here for a bit.
	Skill_Start(SKILL_NONE);
	_SetTimeoutS(1 + Calc_GetRandLLVal(2));
}

bool CChar::NPC_OnItemGive( CChar *pCharSrc, CItem *pItem )
{
	ADDTOCALLSTACK("CChar::NPC_OnItemGive");
	ASSERT(m_pNPC);
	// Someone (Player) is giving me an item.
	// return true = accept

	if ( !pCharSrc )
		return false;

	CScriptTriggerArgs Args(pItem);
	if ( IsTrigUsed(TRIGGER_RECEIVEITEM) )
	{
		if ( OnTrigger(CTRIG_ReceiveItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

    if ( pItem->IsType(IT_GOLD) )
    {
        CItemMemory *pMemory = Memory_FindObj(pCharSrc);
        if ( pMemory )
        {
            switch ( pMemory->m_itEqMemory.m_Action )
            {
                case NPC_MEM_ACT_SPEAK_TRAIN:
                    return NPC_OnTrainPay(pCharSrc, pMemory, pItem);
                case NPC_MEM_ACT_SPEAK_HIRE:
                    return NPC_OnHirePay(pCharSrc, pMemory, pItem);
                default:
                    break;
            }
        }
    }

	// Giving item to own pet
	if ( NPC_IsOwnedBy(pCharSrc) )
	{
		if ( NPC_IsVendor() )
		{
			if ( pItem->IsType(IT_GOLD) )
			{
                uint uiWage = Char_GetDef()->GetHireDayWage();
                uiWage = (uint)pCharSrc->PayGold(this, uiWage, nullptr, PAYGOLD_HIRE);
                if (uiWage > 0)
                {
                    Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_MONEY));
                    NPC_OnHirePayMore(pItem, uiWage);
                    return true;
                }
			}
			else
			{
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_SELL));
				GetBank(LAYER_VENDOR_STOCK)->ContentAdd(pItem);
                return true;
			}
		}

		if ( Food_CanEat(pItem) )
		{
			if ( Use_Eat(pItem, pItem->GetAmount()) )
			{
				if ( NPC_CanSpeak() )
					Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_TY));

				if ( !pItem->IsDeleted() )		// if the NPC don't eat the full stack, bounce back the remaining amount on player backpack
					pCharSrc->ItemBounce(pItem);
				return true;
			}

			if ( NPC_CanSpeak() )
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_FOOD_NO));
			return false;
		}

		if ( pCharSrc->IsPriv(PRIV_GM) )
			return ItemBounce(pItem);

		if ( !CanCarry(pItem) )
		{
			if ( NPC_CanSpeak() )
				Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_PET_WEAK));
			return false;
		}

		// Place item on backpack
		CItemContainer *pPack = GetPack();
		if ( !pPack )
			return false;
		pPack->ContentAdd(pItem);
		return true;
	}

	if ( pItem->IsType(IT_GOLD) )
	{
		if ( m_pNPC->m_Brain == NPCBRAIN_BANKER )
		{
			CItemContainer *pBankBox = pCharSrc->GetPackSafe();
			if ( !pBankBox )
				return false;

			if ( NPC_CanSpeak() )
			{
				tchar *pszMsg = Str_GetTemp();
				snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_NPC_BANKER_DEPOSIT), pItem->GetAmount());
				Speak(pszMsg);
			}

			pBankBox->ContentAdd(pItem);
			return true;
		}
        return false;
	}

	if ( NPC_IsVendor() && !IsStatFlag(STATF_PET) )
	{
		// Dropping item on vendor means quick sell
		if ( pCharSrc->IsClientActive() )
		{
			VendorItem item;
			item.m_serial = pItem->GetUID();
			item.m_vcAmount = pItem->GetAmount();
			pCharSrc->GetClientActive()->Event_VendorSell(this, &item, 1);
		}
		return false;
	}

	if ( !NPC_WantThisItem(pItem) )
	{
		if ( IsTrigUsed(TRIGGER_NPCREFUSEITEM) )
		{
			if ( OnTrigger(CTRIG_NPCRefuseItem, pCharSrc, &Args) != TRIGRET_RET_TRUE )
			{
				pCharSrc->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_GENERIC_DONTWANT));
				return false;
			}
		}
		else
			return false;
	}

	if ( IsTrigUsed(TRIGGER_NPCACCEPTITEM) )
	{
		if ( OnTrigger(CTRIG_NPCAcceptItem, pCharSrc, &Args) == TRIGRET_RET_TRUE )
			return false;
	}

	// Place item on backpack
	CItemContainer *pPack = GetPack();
	if ( !pPack )
		return false;
	pPack->ContentAdd(pItem);
	return true;
}

void CChar::NPC_OnTickAction()
{
	ADDTOCALLSTACK("CChar::NPC_OnTickAction");
	ASSERT(m_pNPC);
	// Our action timer has expired. last skill or task might be complete ?
	// What action should we take now ?
	EXC_TRY("NPC_TickAction");

	const SKILL_TYPE iSkillActive = Skill_GetActive();
    if (!m_pArea)
    {
        const CPointMap& pt = GetUnkPoint();
		if (pt.IsValidPoint())
		{
			if (iSkillActive != NPCACT_RIDDEN)
			{
				g_Log.EventWarn("Trying to Tick Action on an NPC placed in an invalid area (P=%s). UID=0% " PRIx32 ", defname=%s.\n", pt.WriteUsed(), GetUID().GetObjUID(), GetResourceName());
			}
		}
		else
		{
			g_Log.EventWarn("Trying to Tick Action on unplaced NPC. UID=0% " PRIx32 ", defname=%s.\n", GetUID().GetObjUID(), GetResourceName());
		}
        return;
    }

	
    bool fSkillFight = false;
	if ( g_Cfg.IsSkillFlag( iSkillActive, SKF_SCRIPTED ) )
	{
		// SCRIPTED SKILL OnTickAction
	}
	else if (g_Cfg.IsSkillFlag(iSkillActive, SKF_FIGHT))
	{
		EXC_SET_BLOCK("fighting");
        fSkillFight = true;
		NPC_Act_Fight();
	}
	else if (g_Cfg.IsSkillFlag(iSkillActive, SKF_MAGIC))
	{
		EXC_SET_BLOCK("fighting-magic");
		NPC_Act_Fight();
	}
	else
	{
		switch ( iSkillActive )
		{
			case SKILL_NONE:
				// We should try to do something new.
				EXC_SET_BLOCK("idle: Skill_None");
				NPC_Act_Idle();
				break;

			case SKILL_STEALTH:
			case SKILL_HIDING:
				// We are currently hidden.
				EXC_SET_BLOCK("look around");
				if ( NPC_LookAround())
					break;
				// just remain hidden unless we find something new to do.
				if ( Calc_GetRandVal( Skill_GetBase(SKILL_HIDING)))
					break;
				EXC_SET_BLOCK("idle: Hidding");
				NPC_Act_Idle();
				break;

			case SKILL_ARCHERY:
			case SKILL_FENCING:
			case SKILL_MACEFIGHTING:
			case SKILL_SWORDSMANSHIP:
			case SKILL_WRESTLING:
			case SKILL_THROWING:
				// If we are fighting . Periodically review our targets.
				EXC_SET_BLOCK("fight");
				NPC_Act_Fight();
				break;
            case SKILL_MAGERY:
            case SKILL_NECROMANCY:
            case SKILL_MYSTICISM:
            case SKILL_CHIVALRY:
            case SKILL_SPELLWEAVING:
                EXC_SET_BLOCK("magic");
                NPC_Act_Fight();    // May be we can split Fight and Magic from here?
                break;

			case NPCACT_GUARD_TARG:
				// fight with the target, or follow it
				EXC_SET_BLOCK("guard");
				NPC_Act_Guard();
				break;

			case NPCACT_FOLLOW_TARG:
				// continue to follow our target.
				EXC_SET_BLOCK("look at char");
				NPC_LookAtChar( m_Act_UID.CharFind(), 1 );
				EXC_SET_BLOCK("follow char");
				NPC_Act_Follow();
				break;
			case NPCACT_STAY:
				// Just stay here til told to do otherwise.
				break;
			case NPCACT_GOTO:
				EXC_SET_BLOCK("goto");
				NPC_Act_Goto();
				break;
			case NPCACT_WANDER:
				EXC_SET_BLOCK("wander");
				NPC_Act_Wander();
				break;
			case NPCACT_FLEE:
				EXC_SET_BLOCK("flee");
				NPC_Act_Flee();
				break;
			case NPCACT_TALK:
			case NPCACT_TALK_FOLLOW:
				// Got bored just talking to you.
				EXC_SET_BLOCK("talk");
				if ( ! NPC_Act_Talk())
				{
					EXC_SET_BLOCK("idle: Talk");
					NPC_Act_Idle();	// look for something new to do.
				}
				break;
			case NPCACT_GO_HOME:
				EXC_SET_BLOCK("go home");
				NPC_Act_GoHome();
				break;
			case NPCACT_LOOKING:
				EXC_SET_BLOCK("looking");
				if ( NPC_LookAround( true ) )
					break;
				EXC_SET_BLOCK("idle: Looking");
				NPC_Act_Idle();
				break;
			case NPCACT_FOOD:
				EXC_SET_BLOCK("Food Skill");
				if ( NPC_GetAiFlags() & NPC_AI_INTFOOD )
				{
					if ( ! NPC_Act_Food() )
						Skill_Start(SKILL_NONE);
				}
				break;
			case NPCACT_RUNTO:
				EXC_SET_BLOCK("Run To");
				NPC_Act_Runto();
				break;

			default:
				if ( !IsSkillBase(iSkillActive) )	// unassigned skill ? that's weird
				{
					Skill_Start(SKILL_NONE);
				}
				break;
		}
	}

	EXC_SET_BLOCK("timer expired (NPC)");
	if ( _IsTimerExpired() && !fSkillFight) // If i'm fighting, i don't want to wait to start another swing
	{
		int64 timeout = (150-Stat_GetAdjusted(STAT_DEX))/2;
		timeout = maximum(timeout, 0);
		timeout = Calc_GetRandLLVal2(timeout/2, timeout);
		// default next brain/move tick
		_SetTimeoutD(1 + timeout);   // In Tenths of Second.
	}

	//	vendors restock periodically
	if ( NPC_IsVendor() )
		NPC_Vendor_Restock();

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' [0%x]\n", GetName(), (dword)GetUID());
	EXC_DEBUG_END;
}

void CChar::NPC_Pathfinding()
{
	ADDTOCALLSTACK("CChar::NPC_Pathfinding");
	ASSERT(m_pNPC);
	const CPointMap ptLocal = GetTopPoint();

	EXC_TRY("Pathfinding");
	EXC_SET_BLOCK("pre-checking");

    const CPointMap ptTarg = m_Act_p;
    const int dist = ptLocal.GetDist(ptTarg);
	// If NPC_AI_ALWAYSINT is set, just make it as smart as possible.
	const int iInt = ( NPC_GetAiFlags() & NPC_AI_ALWAYSINT ) ? 300 : Stat_GetAdjusted(STAT_INT);
	
	//	do we really need to find the path?
	if ( iInt < 30 ) // too dumb
        return;					
	if ( m_pNPC->m_nextPt == ptTarg ) // we have path to that position already saved in m_NextX/Y
        return;			
	if ( !ptTarg.IsValidPoint() ) // invalid point
        return;
	if (( ptTarg.m_x == ptLocal.m_x ) && ( ptTarg.m_y == ptLocal.m_y )) // same spot
        return; 
	if ( ptTarg.m_map != ptLocal.m_map ) // cannot just move to another map
        return;
	if ( dist >= MAX_NPC_PATH_STORAGE_SIZE/2 ) // skip too far locations which should be too slow
        return;
	if ( dist < 2 ) // skip too low distance (1 step) - good in default
        return;

	// pathfinding is buggy near the edges of the map,
	// so do not use it there
	if ((ptLocal.m_x <= MAX_NPC_PATH_STORAGE_SIZE / 2) || (ptLocal.m_y <= MAX_NPC_PATH_STORAGE_SIZE / 2) ||
		(ptLocal.m_x >= (g_MapList.GetMapSizeX(ptLocal.m_map) - MAX_NPC_PATH_STORAGE_SIZE / 2)) ||
		(ptLocal.m_y >= (g_MapList.GetMapSizeY(ptLocal.m_map) - MAX_NPC_PATH_STORAGE_SIZE / 2)))
	{
		return;
	}

	// need 300 int at least to pathfind each step, but always
	// search if this is a first step
	if (( Calc_GetRandVal(300) > iInt ) && ( m_pNPC->m_nextX[0] ))
        return;

	//	clear saved steps list
	EXC_SET_BLOCK("clearing last steps");
	memset(m_pNPC->m_nextX, 0, sizeof(m_pNPC->m_nextX));
	memset(m_pNPC->m_nextY, 0, sizeof(m_pNPC->m_nextY));

	//	proceed with the pathfinding
	EXC_SET_BLOCK("filling the map");
    // The pathfinder class is big, it's better to store that on the heap, instead of on the stack.
    std::unique_ptr<CPathFinder> path = std::make_unique<CPathFinder>(this, ptTarg);

	EXC_SET_BLOCK("searching the path");
	if ( !path->FindPath() )
		return;

	//	save the found path
	EXC_SET_BLOCK("saving found path");

	// Don't read the first step, it's the same as the current position, so i = 1
	for ( size_t i = 1, sz = path->LastPathSize(); (i != sz) && (i < MAX_NPC_PATH_STORAGE_SIZE /* Don't overflow*/ ); ++i )
	{
        const CPointMap& ptNext = path->ReadStep(i);
		m_pNPC->m_nextX[i - 1] = ptNext.m_x;
		m_pNPC->m_nextY[i - 1] = ptNext.m_y;
	}
	m_pNPC->m_nextPt = ptTarg;
	path->ClearLastPath(); // !! Use explicitly when using one CPathFinder object for more NPCs

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("'%s' point '%d,%d,%d,%d' [0%x]\n", GetName(), ptLocal.m_x, ptLocal.m_y, ptLocal.m_z, ptLocal.m_map, (dword)GetUID());
	EXC_DEBUG_END;
}

void CChar::NPC_Food()
{
	ADDTOCALLSTACK("CChar::NPC_Food");
	ASSERT(m_pNPC);
	EXC_TRY("FoodAI");

	const CPointMap& ptMe = GetTopPoint();
	int		iFood = Stat_GetVal(STAT_FOOD);
	int		iFoodLevel = Food_GetLevelPercent();
	ushort	uiEatAmount = 1;
	int		iSearchDistance = 2;
	CItem	*pClosestFood = nullptr;
	int		iClosestFood = 100;
	int		iMyZ = ptMe.m_z;
	bool	fSearchGrass = false;

	if ( iFood >= 10 )
        return;						//	search for food is starving or very hungry
	if ( iFoodLevel > 40 )
        return;						// and it is at least 60% hungry

	CItemContainer	*pPack = GetPack();
	if ( pPack )
	{
		EXC_SET_BLOCK("searching in pack");
		for (CSObjContRec* pObjRec : *pPack)
		{
			CItem* pFood = static_cast<CItem*>(pObjRec);
			// i have some food personaly, so no need to search for something
			if ( pFood->IsType(IT_FOOD) )
			{
				if ( (uiEatAmount = Food_CanEat(pFood)) > 0 )
				{
					EXC_SET_BLOCK("eating from pack");
					Use_EatQty(pFood, uiEatAmount);
					return;
				}
			}
		}
	}

	// Search for food nearby
	EXC_SET_BLOCK("searching nearby");
	iSearchDistance = (UO_MAP_VIEW_SIGHT * ( 100 - iFoodLevel ) ) / 100;
	CWorldSearch AreaItems(ptMe, minimum(iSearchDistance, m_pNPC->m_Home_Dist_Wander));
	for (;;)
	{
		CItem *pItem = AreaItems.GetItem();
		if ( !pItem )
            break;
		if ( !CanSee(pItem) || pItem->IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) )
            continue;
		if ( (pItem->GetTopPoint().m_z < iMyZ) || (pItem->GetTopPoint().m_z > (iMyZ + (m_height / 2))) )
			continue;

		if ( (uiEatAmount = Food_CanEat(pItem)) > 0 )
		{
			int iDist = GetDist(pItem);
			if ( pClosestFood )
			{
				if ( iDist < iClosestFood )
				{
					pClosestFood = pItem;
					iClosestFood = iDist;
				}
			}
			else
			{
				pClosestFood = pItem;
				iClosestFood = iDist;
			}
		}
	}

	if ( pClosestFood )
	{
		if ( iClosestFood <= 1 )
		{
			//	can take and eat just in place
			EXC_SET_BLOCK("eating nearby");
			ushort uiEaten = pClosestFood->ConsumeAmount(uiEatAmount);
			EatAnim(pClosestFood->GetName(), uiEaten);
			if ( !pClosestFood->GetAmount() )
			{
				pClosestFood->Plant_CropReset();	// set growth if this is a plant
			}
		}
		else
		{
			//	move towards this item
			switch ( m_Act_SkillCurrent )
			{
				case NPCACT_STAY:
				case NPCACT_GOTO:
				case NPCACT_WANDER:
				case NPCACT_LOOKING:
				case NPCACT_GO_HOME:
				case NPCACT_NAPPING:
				case NPCACT_FLEE:
					{
						EXC_SET_BLOCK("walking to desired");
						CPointMap pt = pClosestFood->GetTopPoint();
						if ( CanMoveWalkTo(pt) )
						{
							m_Act_p = pt;
							Skill_Start(NPCACT_GOTO);
							//NPC_WalkToPoint((iFoodLevel < 5) ? true : false);
						}
						break;
					}
				default:
					break;
			}
		}
	}
					// no food around, but maybe i am ok with grass?
	else
	{
		const NPCBRAIN_TYPE brain = GetNPCBrainGroup();
		if ( brain == NPCBRAIN_ANIMAL )						// animals eat grass always
			fSearchGrass = true;
		else if (( brain == NPCBRAIN_HUMAN ) && !iFood )	// human eat grass if starving nearly dead
			fSearchGrass = true;
	}

	if ( fSearchGrass )
	{
		const CCharBase *pCharDef = Char_GetDef();
        const CResourceID rid = CResourceID(RES_TYPEDEF, IT_GRASS);

		EXC_SET_BLOCK("searching grass");
		if ( pCharDef->m_FoodType.ContainsResourceID(rid) ) // do I accept grass as a food?
		{
			CItem *pResBit = CWorldMap::CheckNaturalResource(ptMe, IT_GRASS, true, this);
			if ( pResBit && pResBit->GetAmount() && ( pResBit->GetTopPoint().m_z == iMyZ ) )
			{
				EXC_SET_BLOCK("eating grass");
				const ushort uiEaten = pResBit->ConsumeAmount(15);
				EatAnim("grass", uiEaten/10);

				//	the bit is not needed in a worldsave, timeout of 10 minutes
				pResBit->m_TagDefs.SetNum("NOSAVE", 1);
				pResBit->SetTimeoutS(60*10);
				return;
			}
			else									//	search for grass nearby
			{
				switch ( m_Act_SkillCurrent )
				{
					case NPCACT_STAY:
					case NPCACT_GOTO:
					case NPCACT_WANDER:
					case NPCACT_LOOKING:
					case NPCACT_GO_HOME:
					case NPCACT_NAPPING:
					case NPCACT_FLEE:
						{
							EXC_SET_BLOCK("searching grass nearby");
							CPointMap pt = CWorldMap::FindTypeNear_Top(ptMe, IT_GRASS, minimum(iSearchDistance, m_pNPC->m_Home_Dist_Wander));
							if (( pt.m_x >= 1 ) && ( pt.m_y >= 1 ))
							{
								// we found grass nearby, but has it already been consumed?
								pResBit = CWorldMap::CheckNaturalResource(pt, IT_GRASS, false, this);
								if ( pResBit != nullptr && pResBit->GetAmount() && CanMoveWalkTo(pt) )
								{
									EXC_SET_BLOCK("walking to grass");
									pResBit->m_TagDefs.SetNum("NOSAVE", 1);
									pResBit->SetTimeoutS(60*10);
									m_Act_p = pt;
									Skill_Start(NPCACT_GOTO);
									return;
								}
							}
							break;
						}
					default:
						break;
				}
			}
		}
	}
	EXC_CATCH;
}

void CChar::NPC_ExtraAI()
{
	ADDTOCALLSTACK("CChar::NPC_ExtraAI");
	ASSERT(m_pNPC);
	EXC_TRY("ExtraAI");

	if ( !m_pNPC )
		return;
	if ( GetNPCBrainGroup() != NPCBRAIN_HUMAN )
		return;

	EXC_SET_BLOCK("init");
	if ( IsTrigUsed(TRIGGER_NPCACTION) )
	{
		if ( OnTrigger( CTRIG_NPCAction, this ) == TRIGRET_RET_TRUE )
			return;
	}

    if (!Can(CAN_C_EQUIP) && !Can(CAN_C_USEHANDS))
    {
        // These are checked when trying to equip the item, so avoid further processing if we know from the start
        //  that we won't be able to use those items.
        return;
    }

	// Equip weapons if possible
	EXC_SET_BLOCK("weapon/shield");
	if ( IsStatFlag(STATF_WAR) )
	{
		CItem *pWeapon = LayerFind(LAYER_HAND1);
		if ( !pWeapon || !pWeapon->IsTypeWeapon() )
			ItemEquipWeapon(false);

		CItem *pShield = LayerFind(LAYER_HAND2);
		if ( !pShield || !pShield->IsTypeArmor() )
		{
			const CItemContainer * pPack = GetPack();
			if (pPack)
			{
				pShield = pPack->ContentFind(CResourceID(RES_TYPEDEF, IT_SHIELD));
				if (pShield)
					ItemEquip(pShield);
			}
		}
		return;
	}

	// Equip lightsource at night time
	EXC_SET_BLOCK("light source");
	const CPointMap& pt = GetTopPoint();
	const CSector *pSector = pt.GetSector();
	if ( pSector && pSector->IsDark() )
	{
		const CItem *pLightSourceCheck = LayerFind(LAYER_HAND2);
		if ( !(pLightSourceCheck && (pLightSourceCheck->IsType(IT_LIGHT_OUT) || pLightSourceCheck->IsType(IT_LIGHT_LIT))) )
		{
			CItem *pLightSource = ContentFind(CResourceID(RES_TYPEDEF, IT_LIGHT_OUT));
			if ( pLightSource )
			{
				ItemEquip(pLightSource);
				Use_Obj(pLightSource, false);
			}
		}
	}
	else
	{
		CItem *pLightSource = LayerFind(LAYER_HAND2);
		if ( pLightSource && (pLightSource->IsType(IT_LIGHT_OUT) || pLightSource->IsType(IT_LIGHT_LIT)) )
			ItemBounce(pLightSource, false);
	}

	EXC_CATCH;
}
