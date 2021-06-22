// Actions specific to an NPC.
#include "../clients/CClient.h"
#include "../triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

// I'm a murderer?
bool CChar::Noto_IsMurderer() const
{
	ADDTOCALLSTACK("CChar::Noto_IsMurderer");
	return ( m_pPlayer && (m_pPlayer->m_wMurders > g_Cfg.m_iMurderMinCount) );
}

// I'm evil?
bool CChar::Noto_IsEvil() const
{
	ADDTOCALLSTACK("CChar::Noto_IsEvil");
	short iKarma = GetKarma();

	//	guarded areas could be both RED and BLUE ones.
	if ( m_pArea && m_pArea->IsGuarded() && m_pArea->m_TagDefs.GetKeyNum("RED") )
	{
		//	red zone is opposite to blue - murders are considered normal here
		//	while people with 0 kills and good karma are considered bad here

		if ( Noto_IsMurderer() )
			return false;

		if ( m_pPlayer )
		{
			if ( iKarma < g_Cfg.m_iPlayerKarmaEvil )
				return true;
		}
		else
		{
			if ( iKarma < 0 )
				return true;
		}

		return false;
	}

	// animals and humans given more leeway.
	if ( Noto_IsMurderer() )
		return true;
	switch ( GetNPCBrainGroup() )
	{
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_DRAGON:
			return ( iKarma < 0 );
		case NPCBRAIN_BERSERK:
			return true;
		case NPCBRAIN_ANIMAL:
			return ( iKarma <= -800 );
		default:
			break;
	}
	if ( m_pPlayer )
	{
		return ( iKarma < g_Cfg.m_iPlayerKarmaEvil );
	}
	return ( iKarma <= -3000 );
}

bool CChar::Noto_IsCriminal() const
{
    ADDTOCALLSTACK("CChar::Noto_IsCriminal");
    // do the guards hate me ?
    if ( IsStatFlag( STATF_CRIMINAL ) )
        return true;
    return Noto_IsEvil();
}

bool CChar::Noto_IsNeutral() const
{
	ADDTOCALLSTACK("CChar::Noto_IsNeutral");
	// Should neutrality change in guarded areas ?
	short iKarma = GetKarma();
	switch ( GetNPCBrainGroup() )
	{
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_BERSERK:
			return ( iKarma <= 0 );
		case NPCBRAIN_ANIMAL:
			return ( iKarma <= 100 );
		default:
			break;
	}
	if ( m_pPlayer )
	{
		return ( iKarma < g_Cfg.m_iPlayerKarmaNeutral );
	}
	return ( iKarma < 0 );
}

NOTO_TYPE CChar::Noto_GetFlag(const CChar * pCharViewer, bool fAllowIncog, bool fAllowInvul, bool fOnlyColor) const
{
	ADDTOCALLSTACK("CChar::Noto_GetFlag");
	CChar * pThis = const_cast<CChar*>(this);
	CChar * pTarget = const_cast<CChar*>(pCharViewer);
	NOTO_TYPE iNoto = NOTO_INVALID;
	NOTO_TYPE iColor = NOTO_INVALID;

	if ( ! pThis->m_notoSaves.empty() )
	{
		int id = -1;
		if (pThis->m_pNPC && pThis->NPC_PetGetOwner() && (g_Cfg.m_iPetsInheritNotoriety != 0))	// If I'm a pet and have owner I redirect noto to him.
			pThis = pThis->NPC_PetGetOwner();

		id = pThis->NotoSave_GetID(pTarget);

		if (id != -1)
			return pThis->NotoSave_GetValue(id, fOnlyColor);
	}

	if (IsTrigUsed(TRIGGER_NOTOSEND))
	{
		CScriptTriggerArgs args;
		pThis->OnTrigger(CTRIG_NotoSend, pTarget, &args);
		iNoto = (NOTO_TYPE)(args.m_iN1);
		iColor = (NOTO_TYPE)(args.m_iN2);
	}

	if (iNoto == NOTO_INVALID)
		iNoto = Noto_CalcFlag(pCharViewer, fAllowIncog, fAllowInvul);
	if (iColor == NOTO_INVALID)
		iColor = iNoto;
	pThis->NotoSave_Add(pTarget, iNoto, iColor);

	return fOnlyColor ? iColor : iNoto;
}

// NOTO_GOOD            1
// NOTO_GUILD_SAME      2
// NOTO_NEUTRAL         3
// NOTO_CRIMINAL        4
// NOTO_GUILD_WAR       5
// NOTO_EVIL            6
// NOTO_INVUL           7
NOTO_TYPE CChar::Noto_CalcFlag(const CChar * pCharViewer, bool fAllowIncog, bool fAllowInvul) const
{
	ADDTOCALLSTACK("CChar::Noto_GetFlag");
	// What is this char to the viewer ?
	// This allows the noto attack check in the client.
	// NOTO_GOOD = it is criminal to attack me.

	NOTO_TYPE iNotoFlag = (NOTO_TYPE)(m_TagDefs.GetKeyNum("OVERRIDE.NOTO"));
	if (iNotoFlag != NOTO_INVALID)
		return iNotoFlag;

	if (fAllowIncog && IsStatFlag(STATF_INCOGNITO))
		return NOTO_NEUTRAL;

	if (fAllowInvul && IsStatFlag(STATF_INVUL))
		return NOTO_INVUL;

	if (m_pArea && m_pArea->IsFlag(REGION_FLAG_ARENA))	// everyone is neutral here.
		return NOTO_NEUTRAL;

	if (this != pCharViewer)	// Am I checking myself?
	{
		if (m_pNPC)
		{
			if (g_Cfg.m_iPetsInheritNotoriety != 0)
			{
				// Do we have a master to inherit notoriety from?
				CChar* pMaster = NPC_PetGetOwnerRecursive();
				if (pMaster && (pMaster != pCharViewer)) // master doesn't want to see their own status
				{
					// return master's notoriety
					NOTO_TYPE notoMaster = pMaster->Noto_GetFlag(pCharViewer, fAllowIncog, fAllowInvul);

					// check if notoriety is inheritable based on bitmask setting:
					//		NOTO_GOOD		= 0x01
					//		NOTO_GUILD_SAME	= 0x02
					//		NOTO_NEUTRAL	= 0x04
					//		NOTO_CRIMINAL	= 0x08
					//		NOTO_GUILD_WAR	= 0x10
					//		NOTO_EVIL		= 0x20
					int iPetNotoFlag = 1 << (notoMaster - 1);
					if ((g_Cfg.m_iPetsInheritNotoriety & iPetNotoFlag) == iPetNotoFlag)
						return notoMaster;
				}
			}

			if (NPC_IsOwnedBy(pCharViewer, false))	// All pets are neutral to their owners.
				return NOTO_NEUTRAL;
		}

		// Are we in the same party ?
		if (m_pParty && (m_pParty == pCharViewer->m_pParty) )
		{
			//if (m_pParty->GetLootFlag(this))
				return NOTO_GUILD_SAME;
		}

		if (m_pPlayer)
		{
			// Check the guild/town stuff
			const CItemStone * pMyTown = Guild_Find(MEMORY_TOWN);
			const CItemStone * pMyGuild = Guild_Find(MEMORY_GUILD);
			if (pMyGuild || pMyTown)
			{
				const CItemStone * pViewerGuild = pCharViewer->Guild_Find(MEMORY_GUILD);
				const CItemStone * pViewerTown = pCharViewer->Guild_Find(MEMORY_TOWN);
				// Are we both in a guild?
				if (pViewerGuild || pViewerTown)
				{
					if (pMyGuild && pMyGuild->IsPrivMember(this))
					{
						if (pViewerGuild && pViewerGuild->IsPrivMember(pCharViewer))
						{
							if (pViewerGuild == pMyGuild) // Same guild?
								return NOTO_GUILD_SAME; // return green
							if (pMyGuild->IsAlliedWith(pViewerGuild))
								return NOTO_GUILD_SAME;
							// Are we in different guilds but at war? (not actually a crime right?)
							if (pMyGuild->IsAtWarWith(pViewerGuild))
								return NOTO_GUILD_WAR; // return orange
						}
						if (pMyGuild->IsAtWarWith(pViewerTown))
							return NOTO_GUILD_WAR; // return orange
					}
					if (pMyTown && pMyTown->IsPrivMember(this))
					{
						if (pViewerGuild && pViewerGuild->IsPrivMember(pCharViewer))
						{
							if (pMyTown->IsAtWarWith(pViewerGuild))
								return NOTO_GUILD_WAR; // return orange
						}
						if (pMyTown->IsAtWarWith(pViewerTown))
							return NOTO_GUILD_WAR; // return orange
					}
				}
			}
		}

	}

	if (Noto_IsEvil())
		return NOTO_EVIL;

	if (this != pCharViewer) // Am I checking myself?
	{
		// If they saw me commit a crime or I am their aggressor then criminal to just them.
		const CItemMemory * pMemory = pCharViewer->Memory_FindObjTypes(this, MEMORY_SAWCRIME | MEMORY_AGGREIVED);
		if (pMemory != nullptr)
			return NOTO_CRIMINAL;
	}

	if (IsStatFlag(STATF_CRIMINAL))	// criminal to everyone.
		return NOTO_CRIMINAL;

	if (Noto_IsNeutral() || m_TagDefs.GetKeyNum("NOTO.PERMAGREY"))
		return NOTO_NEUTRAL;

	return NOTO_GOOD;
}

HUE_TYPE CChar::Noto_GetHue(const CChar * pCharViewer, bool fIncog) const
{
	ADDTOCALLSTACK("CChar::Noto_GetHue");
	const CVarDefCont * sVal = GetKey("NAME.HUE", true);
	if (sVal)
		return (HUE_TYPE)(sVal->GetValNum());

	NOTO_TYPE color = Noto_GetFlag(pCharViewer, fIncog, true, true);
	const CChar *pChar = GetOwner();
	if (!pChar)
		pChar = this;
	switch (color)
	{
	case NOTO_GOOD:			return (HUE_TYPE)(g_Cfg.m_iColorNotoGood);		// Blue
	case NOTO_GUILD_SAME:	return (HUE_TYPE)(g_Cfg.m_iColorNotoGuildSame);	// Green (same guild)
	case NOTO_NEUTRAL:		return (HUE_TYPE)(g_Cfg.m_iColorNotoNeutral);	// Grey (someone that can be attacked)
	case NOTO_CRIMINAL:		return (HUE_TYPE)(g_Cfg.m_iColorNotoCriminal);	// Grey (criminal)
	case NOTO_GUILD_WAR:	return (HUE_TYPE)(g_Cfg.m_iColorNotoGuildWar);	// Orange (enemy guild)
	case NOTO_EVIL:			return (HUE_TYPE)(g_Cfg.m_iColorNotoEvil);		// Red
	case NOTO_INVUL:		return pChar->IsPriv(PRIV_GM) ? (HUE_TYPE)g_Cfg.m_iColorNotoInvulGameMaster : (HUE_TYPE)g_Cfg.m_iColorNotoInvul;		// Purple / Yellow
	default:				return ((HUE_TYPE)color > NOTO_INVUL ? (HUE_TYPE)color : g_Cfg.m_iColorNotoDefault);	// Grey
	}
}

lpctstr CChar::Noto_GetFameTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetFameTitle");
	if ( IsStatFlag(STATF_INCOGNITO|STATF_POLYMORPH) )
		return "";

	if ( !IsPriv(PRIV_PRIV_NOSHOW) )	// PRIVSHOW is on
	{
		if ( IsPriv(PRIV_GM) )			// GM mode is on
		{
			switch ( GetPrivLevel() )
			{
				case PLEVEL_Owner:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_OWNER );	//"Owner ";
				case PLEVEL_Admin:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ADMIN );	//"Admin ";
				case PLEVEL_Dev:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_DEV );	//"Dev ";
				case PLEVEL_GM:
					return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_GM );	//"GM ";
				default:
					break;
			}
		}
		switch ( GetPrivLevel() )
		{
			case PLEVEL_Seer:
                return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_SEER );	//"Seer ";
			case PLEVEL_Counsel:
                return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_COUNSEL );	//"Counselor ";
			default:
                break;
		}
	}

	if (( GetFame() > 9900 ) && (m_pPlayer || !g_Cfg.m_NPCNoFameTitle))
		return Char_GetDef()->IsFemale() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_LADY ) : g_Cfg.GetDefaultMsg( DEFMSG_TITLE_LORD );	//"Lady " : "Lord ";

	return "";
}

int CChar::Noto_GetLevel() const
{
	ADDTOCALLSTACK("CChar::Noto_GetLevel");

	size_t i = 0;
	short iKarma = GetKarma();
	for ( ; i < g_Cfg.m_NotoKarmaLevels.size() && iKarma < g_Cfg.m_NotoKarmaLevels[i]; ++i )
		;

	size_t j = 0;
	const ushort uiFame = GetFame();
	for ( ; j < g_Cfg.m_NotoFameLevels.size() && uiFame > g_Cfg.m_NotoFameLevels[j]; ++j )
		;

	return (int)( ( i * (g_Cfg.m_NotoFameLevels.size() + 1) ) + j );
}

lpctstr CChar::Noto_GetTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetTitle");

	lpctstr pTitle = Noto_IsMurderer() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_MURDERER ) : ( IsStatFlag(STATF_CRIMINAL) ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_CRIMINAL ) :  g_Cfg.GetNotoTitle(Noto_GetLevel(), Char_GetDef()->IsFemale()) );
	lpctstr pFameTitle = GetKeyStr("NAME.PREFIX");
	if ( !*pFameTitle )
		pFameTitle = Noto_GetFameTitle();

	tchar * pTemp = Str_GetTemp();
	snprintf( pTemp, STR_TEMPLENGTH, "%s%s%s%s%s%s",
		(pTitle[0]) ? ( Char_GetDef()->IsFemale() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ARTICLE_FEMALE ) : g_Cfg.GetDefaultMsg( DEFMSG_TITLE_ARTICLE_MALE ) )  : "",
		pTitle,
		(pTitle[0]) ? " " : "",
		pFameTitle,
		GetName(),
		GetKeyStr("NAME.SUFFIX"));

	return pTemp;
}

void CChar::Noto_Murder()
{
	ADDTOCALLSTACK("CChar::Noto_Murder");
	if ( Noto_IsMurderer() )
		SysMessageDefault(DEFMSG_MSG_MURDERER);

	if ( m_pPlayer && m_pPlayer->m_wMurders )
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Murders, g_Cfg.GetSpellEffect(SPELL_NONE, 0), (int)(g_Cfg.m_iMurderDecayTime/MSECS_PER_TENTH), nullptr);
}

bool CChar::Noto_Criminal( CChar * pCharViewer, bool fFromSawCrime )
{
	ADDTOCALLSTACK("CChar::Noto_Criminal");
	if ( m_pNPC || IsPriv(PRIV_GM) )
		return false;

	int64 decay = g_Cfg.m_iCriminalTimer; // in the ini is in minutes, but it's stored internally in msecs

    TRIGRET_TYPE retCriminal = TRIGRET_RET_DEFAULT;
	if ( IsTrigUsed(TRIGGER_CRIMINAL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = decay / (60*MSECS_PER_SEC);   // convert in minutes
        Args.m_iN2 = fFromSawCrime;
		Args.m_pO1 = pCharViewer;
        retCriminal = OnTrigger(CTRIG_Criminal, this, &Args);
		decay = (Args.m_iN1 * (60*MSECS_PER_SEC)); // back in ms
	}

    if (retCriminal == TRIGRET_RET_TRUE)
    {
        // Return 1: prevent the char to be flagged as criminal and do not remove the SawCrime memory
        return false;
    }

    if (retCriminal != TRIGRET_RET_FALSE)
    {
        // Return != 0 and 1: flag the char as criminal
        if (!IsStatFlag(STATF_CRIMINAL))
            SysMessageDefault(DEFMSG_MSG_GUARDS);
        if (decay)
            Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Criminal, g_Cfg.GetSpellEffect(SPELL_NONE, 0), decay/MSECS_PER_TENTH);
    }

    // Return != 1: remove the SawCrime memory, since i made him criminal to myself and also i may call the guards
    if (pCharViewer)
    {
        CItemMemory *pMemorySawCrime = pCharViewer->Memory_FindObjTypes(this, MEMORY_SAWCRIME);
        if (pMemorySawCrime)
            pCharViewer->Memory_ClearTypes(pMemorySawCrime, MEMORY_SAWCRIME);
    }

	return (retCriminal == TRIGRET_RET_FALSE);
}

void CChar::Noto_ChangeDeltaMsg( int iDelta, lpctstr pszType )
{
	ADDTOCALLSTACK("CChar::Noto_ChangeDeltaMsg");
	if ( !iDelta )
		return;

#define	NOTO_DEGREES	8
#define	NOTO_FACTOR		(300/NOTO_DEGREES)

	static uint const sm_DegreeTable[8] =
	{
		DEFMSG_MSG_NOTO_CHANGE_1,
		DEFMSG_MSG_NOTO_CHANGE_2,
		DEFMSG_MSG_NOTO_CHANGE_3,
		DEFMSG_MSG_NOTO_CHANGE_4,
		DEFMSG_MSG_NOTO_CHANGE_5,
		DEFMSG_MSG_NOTO_CHANGE_6,
		DEFMSG_MSG_NOTO_CHANGE_7,
		DEFMSG_MSG_NOTO_CHANGE_8		// 300 = huge
	};

	int iDegree = minimum(abs(iDelta) / NOTO_FACTOR, 7);

	tchar *pszMsg = Str_GetTemp();
	snprintf( pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_0 ),
		( iDelta < 0 ) ? g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_LOST ) : g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_GAIN ),
		g_Cfg.GetDefaultMsg(sm_DegreeTable[iDegree]), pszType );

	SysMessage( pszMsg );
}

void CChar::Noto_ChangeNewMsg( int iPrvLevel )
{
	ADDTOCALLSTACK("CChar::Noto_ChangeNewMsg");
	if ( iPrvLevel != Noto_GetLevel())
	{
		// reached a new title level ?
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_GETTITLE ), Noto_GetTitle());
	}
}

void CChar::Noto_Fame( int iFameChange )
{
	ADDTOCALLSTACK("CChar::Noto_Fame");

	if ( ! iFameChange )
		return;

	const int iFame = GetFame();
	if ( iFameChange > 0 )
	{
		if ( iFame + iFameChange > g_Cfg.m_iMaxFame )
			iFameChange = (g_Cfg.m_iMaxFame - iFame);
	}
	else
	{
		if ( iFame + iFameChange < 0 )
			iFameChange = -iFame;
	}

	if ( IsTrigUsed(TRIGGER_FAMECHANGE) )
	{
		CScriptTriggerArgs Args(iFameChange);	// ARGN1 - Fame change modifier
		TRIGRET_TYPE retType = OnTrigger(CTRIG_FameChange, this, &Args);

		if ( retType == TRIGRET_RET_TRUE )
			return;
		iFameChange = (int)(Args.m_iN1);
	}

	if ( ! iFameChange )
		return;

	SetFame((ushort)(iFame + iFameChange));
    Noto_ChangeDeltaMsg( (int)GetFame() - iFame, g_Cfg.GetDefaultMsg( DEFMSG_NOTO_FAME ) );
}

void CChar::Noto_Karma( int iKarmaChange, int iBottom, bool fMessage )
{
	ADDTOCALLSTACK("CChar::Noto_Karma");

	const int iKarma = GetKarma();
	iKarmaChange = g_Cfg.Calc_KarmaScale( iKarma, iKarmaChange );

	if ( iKarmaChange > 0 )
	{
		if ( iKarma + iKarmaChange > g_Cfg.m_iMaxKarma )
			iKarmaChange = g_Cfg.m_iMaxKarma - iKarma;
	}
	else
	{
		if (iBottom == INT32_MIN)
			iBottom = g_Cfg.m_iMinKarma;
		if ( iKarma + iKarmaChange < iBottom )
			iKarmaChange = iBottom - iKarma;
	}

	if ( IsTrigUsed(TRIGGER_KARMACHANGE) )
	{
		CScriptTriggerArgs Args(iKarmaChange);	// ARGN1 - Karma change modifier
		TRIGRET_TYPE retType = OnTrigger(CTRIG_KarmaChange, this, &Args);

		if ( retType == TRIGRET_RET_TRUE )
			return;
		iKarmaChange = (int)(Args.m_iN1);
	}

	if ( ! iKarmaChange )
		return;

    SetKarma((short)(iKarma + iKarmaChange));
    Noto_ChangeDeltaMsg( (int)GetKarma() - iKarma, g_Cfg.GetDefaultMsg( DEFMSG_NOTO_KARMA ) );
	NotoSave_Update();
	if ( fMessage == true )
	{
		Noto_ChangeNewMsg( Noto_GetLevel() );
	}
}

void CChar::Noto_Kill(CChar * pKill, int iTotalKillers)
{
	ADDTOCALLSTACK("CChar::Noto_Kill");

	if ( !pKill )
		return;

	// What was their noto to me ?
	NOTO_TYPE NotoThem = pKill->Noto_GetFlag( this, false );

	// Fight is over now that i have won. (if i was fighting at all )
	// ie. Magery cast might not be a "fight"
	Fight_Clear(pKill);
	if ( pKill == this )
		return;

	if ( m_pNPC )
	{
		if ( pKill->m_pNPC )
		{
            if (m_pNPC->m_Brain == NPCBRAIN_GUARD)	// don't create corpse if NPC got killed by a guard
            {
                pKill->StatFlag_Set(STATF_CONJURED);
                return;
            }
		}
	}
	else if ( NotoThem < NOTO_GUILD_SAME )
	{
		// I'm a murderer !
		if ( !IsPriv(PRIV_GM) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = m_pPlayer->m_wMurders + 1LL;
			args.m_iN2 = true;
			args.m_iN3 = false;

			if ( IsTrigUsed(TRIGGER_MURDERMARK) )
			{
				OnTrigger(CTRIG_MurderMark, this, &args);
				if (args.m_iN1 < 0)
					args.m_iN1 = 0;
			}

			if ( args.m_iN3 < 1 )
			{
				m_pPlayer->m_wMurders = (word)(args.m_iN1);
				if (args.m_iN2)
					Noto_Criminal();

				Noto_Murder();
			}

			NotoSave_Update();
		}
	}

	// No fame/karma/exp gain on these conditions
	if ( NotoThem == NOTO_GUILD_SAME || pKill->IsStatFlag(STATF_CONJURED) )
		return;

	int iPrvLevel = Noto_GetLevel();	// store title before fame/karma changes to check if it got changed
	Noto_Fame(g_Cfg.Calc_FameKill(pKill) / iTotalKillers);
	Noto_Karma(g_Cfg.Calc_KarmaKill(pKill, NotoThem) / iTotalKillers);

	if ( g_Cfg.m_bExperienceSystem && (g_Cfg.m_iExperienceMode & EXP_MODE_RAISE_COMBAT) )
	{
		int change = (pKill->m_exp / 10) / iTotalKillers;
		if ( change )
		{
			if ( m_pPlayer && pKill->m_pPlayer )
				change = (change * g_Cfg.m_iExperienceKoefPVP) / 100;
			else
				change = (change * g_Cfg.m_iExperienceKoefPVM) / 100;
		}

		if ( change )
		{
			//	bonuses of different experiences
			if ( (m_exp * 4) < pKill->m_exp )		// 200%		[exp < 1/4 of killed]
				change *= 2;
			else if ( (m_exp * 2) < pKill->m_exp )	// 150%		[exp < 1/2 of killed]
				change = (change * 3) / 2;
			else if ( m_exp <= pKill->m_exp )		// 100%		[exp <= killed]
				;
			else if ( m_exp < (pKill->m_exp * 2) )	//  50%		[exp < 2 * killed]
				change /= 2;
			else if ( m_exp < (pKill->m_exp * 3) )	//  25%		[exp < 3 * killed]
				change /= 4;
			else									//  10%		[exp >= 3 * killed]
				change /= 10;
		}

		ChangeExperience(change, pKill);
	}

	Noto_ChangeNewMsg(iPrvLevel);	// inform any title changes
}

void CChar::NotoSave_Add( CChar * pChar, NOTO_TYPE value, NOTO_TYPE color  )
{
	ADDTOCALLSTACK("CChar::NotoSave_Add");
	if ( !pChar )
		return;
	const CUID& uid = pChar->GetUID();
	if  ( !m_notoSaves.empty() )	// Checking if I already have him in the list, only if there 's any list.
	{
		for (std::vector<NotoSaves>::iterator it = m_notoSaves.begin(), end = m_notoSaves.end(); it != end; ++it)
		{
			NotoSaves & refNoto = *it;
			if ( refNoto.charUID == uid.GetObjUID() )
			{
				// Found him, no actions needed so I forget about him...
				// or should I update data ?

				refNoto.value = value;
				refNoto.color = color;
				return;
			}
		}
	}
	NotoSaves refNoto;
	refNoto.charUID = pChar->GetUID().GetObjUID();
	refNoto.time = 0;
	refNoto.value = value;
	refNoto.color = color;
	m_notoSaves.emplace_back(std::move(refNoto));
}

NOTO_TYPE CChar::NotoSave_GetValue( int id, bool bGetColor )
{
	ADDTOCALLSTACK("CChar::NotoSave_GetValue");
	if ( m_notoSaves.empty() )
		return NOTO_INVALID;
	if ( id < 0 )
		return NOTO_INVALID;
	if ( (int)(m_notoSaves.size()) <= id )
		return NOTO_INVALID;
	NotoSaves & refNotoSave = m_notoSaves[id];
	if (bGetColor && refNotoSave.color != 0 )	// retrieving color if requested... only if a color is greater than 0 (to avoid possible crashes).
		return refNotoSave.color;
	else
		return refNotoSave.value;
}

int64 CChar::NotoSave_GetTime( int id )
{
	ADDTOCALLSTACK("CChar::NotoSave_GetTime");
	if ( m_notoSaves.empty() )
		return -1;
	if ( id < 0 )
		return NOTO_INVALID;
	if ( (int)(m_notoSaves.size()) <= id )
		return -1;
	NotoSaves & refNotoSave = m_notoSaves[id];
	return refNotoSave.time;
}

void CChar::NotoSave_Clear()
{
	ADDTOCALLSTACK("CChar::NotoSave_Clear");
	if ( !m_notoSaves.empty() )
		m_notoSaves.clear();
}

void CChar::NotoSave_Update()
{
	ADDTOCALLSTACK("CChar::NotoSave_Update");
	NotoSave_Clear();
	UpdateMode();
	UpdatePropertyFlag();
}

void CChar::NotoSave_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::NotoSave_CheckTimeout");
	if (!m_notoSaves.empty())
	{
        std::vector<CChar*> vToResend;

        EXC_TRY("Loop");
		for (std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end();)
		{
			NotoSaves & refNoto = *it;
            ++refNoto.time;
            if ((refNoto.time > g_Cfg.m_iNotoTimeout) && (g_Cfg.m_iNotoTimeout > 0))
            {
                vToResend.emplace_back(CUID::CharFindFromUID(refNoto.charUID));
                it = m_notoSaves.erase(it);
            }
            else
			    ++it;
		}
        EXC_CATCH;

        for (CChar* pChar : vToResend)
        {
            NotoSave_Resend(pChar);
        }
	}
}

void CChar::NotoSave_Resend(CChar * pChar)
{
	ADDTOCALLSTACK("CChar::NotoSave_Resend");
    if (pChar)
    {
        const CObjBaseTemplate* pObj = pChar->GetTopLevelObj();
        if (GetDist(pObj) < pChar->GetVisualRange())
            Noto_GetFlag(pChar, true, true);
    }
}

int CChar::NotoSave_GetID( CChar * pChar ) const
{
	ADDTOCALLSTACK("CChar::NotoSave_GetID(CChar)");
	if ( !pChar || m_notoSaves.empty() )
		return -1;
	if ( !m_notoSaves.empty() )
	{
		int id = 0;
		for (const auto& it : m_notoSaves)
		{
			const CUID uid(it.charUID);
			if ( uid.CharFind() && (uid == pChar->GetUID()) )
				return id;
			++id;
		}
	}
	return -1;
}

bool CChar::NotoSave_Delete( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NotoSave_Delete");
	if ( ! pChar )
		return false;
	if ( !m_notoSaves.empty() )
	{
		for (std::vector<NotoSaves>::iterator it = m_notoSaves.begin(), end = m_notoSaves.end(); it != end; ++it)
		{
			if (it->charUID == pChar->GetUID().GetObjUID() )
			{
				m_notoSaves.erase(it);
				return true;
			}
		}
	}
	return false;
}
