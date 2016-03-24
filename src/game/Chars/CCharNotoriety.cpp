// Actions specific to an NPC.
#include "../common/CGrayUIDextra.h"
#include "../clients/CClient.h"
#include "../Triggers.h"
#include "CChar.h"
#include "CCharNPC.h"

// I'm a murderer?
bool CChar::Noto_IsMurderer() const
{
	ADDTOCALLSTACK("CChar::Noto_IsMurderer");
	return( m_pPlayer && m_pPlayer->m_wMurders > g_Cfg.m_iMurderMinCount );
}

// I'm evil?
bool CChar::Noto_IsEvil() const
{
	ADDTOCALLSTACK("CChar::Noto_IsEvil");
	int		iKarma = Stat_GetAdjusted(STAT_KARMA);

	//	guarded areas could be both RED and BLUE ones.
	if ( m_pArea && m_pArea->IsGuarded() && m_pArea->m_TagDefs.GetKeyNum("RED", true) )
	{
		//	red zone is opposite to blue - murders are considered normal here
		//	while people with 0 kills and good karma are considered bad here

		if ( Noto_IsMurderer() )
			return false;

		if ( m_pPlayer )
		{
			if ( iKarma > g_Cfg.m_iPlayerKarmaEvil )
				return true;
		}
		else
		{
			if ( iKarma > 0 )
				return true;
		}

		return false;
	}

	// animals and humans given more leeway.
	if ( Noto_IsMurderer() )
		return true;
	switch ( GetNPCBrain() )
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

bool CChar::Noto_IsNeutral() const
{
	ADDTOCALLSTACK("CChar::Noto_IsNeutral");
	// Should neutrality change in guarded areas ?
	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	switch ( GetNPCBrain() )
	{
		case NPCBRAIN_MONSTER:
		case NPCBRAIN_BERSERK:
			return( iKarma<= 0 );
		case NPCBRAIN_ANIMAL:
			return( iKarma<= 100 );
		default:
			break;
	}
	if ( m_pPlayer )
	{
		return( iKarma<g_Cfg.m_iPlayerKarmaNeutral );
	}
	return( iKarma<0 );
}

NOTO_TYPE CChar::Noto_GetFlag( const CChar * pCharViewer, bool fAllowIncog, bool fAllowInvul, bool bOnlyColor ) const
{
	ADDTOCALLSTACK("CChar::Noto_GetFlag");
	CChar * pThis = const_cast<CChar*>(this);
	CChar * pTarget = const_cast<CChar*>(pCharViewer);
	NOTO_TYPE Noto;
	NOTO_TYPE color = NOTO_INVALID;
	if ( pThis->m_notoSaves.size() )
	{
		int id = -1;
		if (pThis->m_pNPC && pThis->NPC_PetGetOwner() && g_Cfg.m_iPetsInheritNotoriety != 0)	// If I'm a pet and have owner I redirect noto to him.
			pThis = pThis->NPC_PetGetOwner();

		id = pThis->NotoSave_GetID(pTarget);

		if ( id != -1 )
			return pThis->NotoSave_GetValue( id, bOnlyColor );
	}
	if (IsTrigUsed(TRIGGER_NOTOSEND))
	{
		CScriptTriggerArgs args;
		pThis->OnTrigger(CTRIG_NotoSend, pTarget, &args);
		Noto = static_cast<NOTO_TYPE>(args.m_iN1);
		color = static_cast<NOTO_TYPE>(args.m_iN2);
		if (Noto > NOTO_INVALID && Noto <= NOTO_INVUL) // if Notoriety is between NOTO_GOOD and NOTO_INVUL its ok
		{
			pThis->NotoSave_Add( pTarget, Noto, color);
			goto NotoReturn;
		}
	}
	Noto = Noto_CalcFlag( pCharViewer, fAllowIncog, fAllowInvul);
	pThis->NotoSave_Add(pTarget, Noto, color);
NotoReturn:
	if (bOnlyColor && color != 0)
		return color;
	else
		return Noto;
}

NOTO_TYPE CChar::Noto_CalcFlag( const CChar * pCharViewer, bool fAllowIncog, bool fAllowInvul ) const
{
	ADDTOCALLSTACK("CChar::Noto_CalcFlag");
	NOTO_TYPE iNotoFlag = static_cast<NOTO_TYPE>(m_TagDefs.GetKeyNum("OVERRIDE.NOTO", true));
	if ( iNotoFlag != NOTO_INVALID )
		return iNotoFlag;

	if ( this != pCharViewer )
	{
		if ( g_Cfg.m_iPetsInheritNotoriety != 0 )
		{
			// Do we have a master to inherit notoriety from?
			CChar* pMaster = NPC_PetGetOwner();
			if (pMaster != NULL && pMaster != pCharViewer) // master doesn't want to see their own status
			{
				// protect against infinite loop
				static int sm_iReentrant = 0;
				if (sm_iReentrant < 32)
				{
					// return master's notoriety
					++sm_iReentrant;
					NOTO_TYPE notoMaster = pMaster->Noto_GetFlag(pCharViewer, fAllowIncog, fAllowInvul);
					--sm_iReentrant;

					// check if notoriety is inheritable based on bitmask setting:
					//		NOTO_GOOD		= 0x01
					//		NOTO_GUILD_SAME	= 0x02
					//		NOTO_NEUTRAL	= 0x04
					//		NOTO_CRIMINAL	= 0x08
					//		NOTO_GUILD_WAR	= 0x10
					//		NOTO_EVIL		= 0x20
					//		NOTO_INVUL		= 0x40
					int iNotoFlags = 1 << (notoMaster - 1);
					if ( (g_Cfg.m_iPetsInheritNotoriety & iNotoFlags) == iNotoFlags )
						return notoMaster;
				}
				else
				{
					DEBUG_ERR(("Too many owners (circular ownership?) to continue acquiring notoriety towards %s uid=0%x\n", pMaster->GetName(), pMaster->GetUID().GetPrivateUID()));
					// too many owners, return the notoriety for however far we got down the chain
				}
			}
		}
	}

	if ( fAllowInvul && IsStatFlag(STATF_INVUL))
		return NOTO_INVUL;
	if ( IsStatFlag(STATF_Criminal))	// criminal to everyone.
		return NOTO_CRIMINAL;
	if ( fAllowIncog && IsStatFlag(STATF_Incognito))
		return NOTO_NEUTRAL;
	if ( m_pArea && m_pArea->IsFlag(REGION_FLAG_ARENA))
		return NOTO_NEUTRAL;			// everyone is neutral here.
	if ( Noto_IsEvil())
		return NOTO_EVIL;
	if ( Noto_IsNeutral())
		return NOTO_NEUTRAL;

	if ( this != pCharViewer ) // Am I checking myself?
	{
		// If they saw me commit a crime or I am their aggressor then criminal to just them.
		CItemMemory * pMemory = pCharViewer->Memory_FindObjTypes(this, MEMORY_SAWCRIME|MEMORY_AGGREIVED|MEMORY_HARMEDBY);
		if ( pMemory != NULL )
			return( NOTO_CRIMINAL );

		// Are we in the same party ?
		if ( m_pParty && m_pParty == pCharViewer->m_pParty )
		{
			if ( m_pParty->GetLootFlag(this))
				return(NOTO_GUILD_SAME);
		}

		// Check the guild stuff
		CItemStone * pMyGuild = Guild_Find(MEMORY_GUILD);
		if ( pMyGuild )
		{
			CItemStone * pViewerGuild = pCharViewer->Guild_Find(MEMORY_GUILD);
			if ( pViewerGuild )
			{
				if ( pViewerGuild == pMyGuild )
					return NOTO_GUILD_SAME;
				if ( pMyGuild->IsAlliedWith(pViewerGuild))
					return NOTO_GUILD_SAME;
				if ( pMyGuild->IsAtWarWith(pViewerGuild))
					return NOTO_GUILD_WAR;
			}
		}

		// Check the town stuff
		CItemStone * pMyTown = Guild_Find(MEMORY_TOWN);
		if ( pMyTown )
		{
			CItemStone * pViewerTown = pCharViewer->Guild_Find(MEMORY_TOWN);
			if ( pViewerTown )
			{
				if ( pMyTown->IsAtWarWith(pViewerTown))
					return NOTO_GUILD_WAR;
			}
		}
	}

	return( NOTO_GOOD );
}

HUE_TYPE CChar::Noto_GetHue( const CChar * pCharViewer, bool fIncog ) const
{
	ADDTOCALLSTACK("CChar::Noto_GetHue");
	CVarDefCont * sVal = GetKey( "NAME.HUE", true );
	if ( sVal )
		return  static_cast<HUE_TYPE>(sVal->GetValNum());

	NOTO_TYPE color = Noto_GetFlag(pCharViewer, fIncog, true,true);
	switch ( color )
	{
		case NOTO_GOOD:			return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoGood);		// Blue
		case NOTO_GUILD_SAME:		return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoGuildSame);	// Green (same guild)
		case NOTO_NEUTRAL:		return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoNeutral);	// Grey (someone that can be attacked)
		case NOTO_CRIMINAL:		return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoCriminal);	// Grey (criminal)
		case NOTO_GUILD_WAR:		return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoGuildWar);	// Orange (enemy guild)
		case NOTO_EVIL:			return static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoEvil);		// Red
		case NOTO_INVUL:		return IsPriv(PRIV_GM)? static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoInvulGameMaster) : static_cast<HUE_TYPE>(g_Cfg.m_iColorNotoInvul);		// Purple / Yellow
		default:			return static_cast<HUE_TYPE>(color > NOTO_INVUL ? color : g_Cfg.m_iColorNotoDefault);	// Grey
	}
}


lpctstr CChar::Noto_GetFameTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetFameTitle");
	if ( IsStatFlag(STATF_Incognito|STATF_Polymorph) )
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
			case PLEVEL_Seer: return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_SEER );	//"Seer ";
			case PLEVEL_Counsel: return g_Cfg.GetDefaultMsg( DEFMSG_TITLE_COUNSEL );	//"Counselor ";
			default: break;
		}
	}

	if (( Stat_GetAdjusted(STAT_FAME) > 9900 ) && (m_pPlayer || !g_Cfg.m_NPCNoFameTitle))
		return Char_GetDef()->IsFemale() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_LADY ) : g_Cfg.GetDefaultMsg( DEFMSG_TITLE_LORD );	//"Lady " : "Lord ";

	return "";
}

int CChar::Noto_GetLevel() const
{
	ADDTOCALLSTACK("CChar::Noto_GetLevel");

	size_t i = 0;
	int iKarma = Stat_GetAdjusted(STAT_KARMA);
	for ( ; i < g_Cfg.m_NotoKarmaLevels.GetCount() && iKarma < g_Cfg.m_NotoKarmaLevels.GetAt(i); i++ )
		;

	size_t j = 0;
	int iFame = Stat_GetAdjusted(STAT_FAME);
	for ( ; j < g_Cfg.m_NotoFameLevels.GetCount() && iFame > g_Cfg.m_NotoFameLevels.GetAt(j); j++ )
		;

	return( ( i * (g_Cfg.m_NotoFameLevels.GetCount() + 1) ) + j );
}

lpctstr CChar::Noto_GetTitle() const
{
	ADDTOCALLSTACK("CChar::Noto_GetTitle");

	lpctstr pTitle = Noto_IsMurderer() ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_MURDERER ) : ( IsStatFlag(STATF_Criminal) ? g_Cfg.GetDefaultMsg( DEFMSG_TITLE_CRIMINAL ) :  g_Cfg.GetNotoTitle(Noto_GetLevel(), Char_GetDef()->IsFemale()) );
	lpctstr pFameTitle = GetKeyStr("NAME.PREFIX");
	if ( !*pFameTitle )
		pFameTitle = Noto_GetFameTitle();

	tchar * pTemp = Str_GetTemp();
	sprintf( pTemp, "%s%s%s%s%s%s",
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
		Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Murders, 0, g_Cfg.m_iMurderDecayTime, NULL);
}

bool CChar::Noto_Criminal( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::Noto_Criminal");
	if ( m_pNPC || IsPriv(PRIV_GM) )
		return false;

	int decay = g_Cfg.m_iCriminalTimer;

	if ( IsTrigUsed(TRIGGER_CRIMINAL) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = decay;
		Args.m_pO1 = pChar;
		if ( OnTrigger(CTRIG_Criminal, this, &Args) == TRIGRET_RET_TRUE )
			return false;

		decay = static_cast<int>(Args.m_iN1);
	}

	if ( !IsStatFlag(STATF_Criminal) )
		SysMessageDefault(DEFMSG_MSG_GUARDS);

	Spell_Effect_Create(SPELL_NONE, LAYER_FLAG_Criminal, 0, decay);
	return true;
}

void CChar::Noto_ChangeDeltaMsg( int iDelta, lpctstr pszType )
{
	ADDTOCALLSTACK("CChar::Noto_ChangeDeltaMsg");
	if ( !iDelta )
		return;

#define	NOTO_DEGREES	8
#define	NOTO_FACTOR		(300/NOTO_DEGREES)

	static UINT const sm_DegreeTable[8] =
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
	sprintf( pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_CHANGE_0 ), 
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
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_MSG_NOTO_GETTITLE ), static_cast<lpctstr>(Noto_GetTitle()));
	}
}

void CChar::Noto_Fame( int iFameChange )
{
	ADDTOCALLSTACK("CChar::Noto_Fame");

	if ( ! iFameChange )
		return;

	int iFame = maximum(Stat_GetAdjusted(STAT_FAME), 0);
	if ( iFameChange > 0 )
	{
		if ( iFame + iFameChange > g_Cfg.m_iMaxFame )
			iFameChange = g_Cfg.m_iMaxFame - iFame;
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
		iFameChange = static_cast<int>(Args.m_iN1);
	}

	if ( ! iFameChange )
		return;

	iFame += iFameChange;
	Noto_ChangeDeltaMsg( iFame - Stat_GetAdjusted(STAT_FAME), g_Cfg.GetDefaultMsg( DEFMSG_NOTO_FAME ) );
	Stat_SetBase(STAT_FAME, static_cast<short>(iFame));
}

void CChar::Noto_Karma( int iKarmaChange, int iBottom, bool bMessage )
{
	ADDTOCALLSTACK("CChar::Noto_Karma");

	int	iKarma = Stat_GetAdjusted(STAT_KARMA);
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
		iKarmaChange = static_cast<int>(Args.m_iN1);
	}

	if ( ! iKarmaChange )
		return;

	iKarma += iKarmaChange;
	Noto_ChangeDeltaMsg( iKarma - Stat_GetAdjusted(STAT_KARMA), g_Cfg.GetDefaultMsg( DEFMSG_NOTO_KARMA ) );
	Stat_SetBase(STAT_KARMA, static_cast<short>(iKarma));
	NotoSave_Update();
	if ( bMessage == true )
	{
		int iPrvLevel = Noto_GetLevel();
		Noto_ChangeNewMsg( iPrvLevel );
	}
}

extern uint Calc_ExpGet_Exp(uint);

void CChar::Noto_Kill(CChar * pKill, bool fPetKill, int iTotalKillers)
{
	ADDTOCALLSTACK("CChar::Noto_Kill");

	if ( !pKill )
		return;

	// What was there noto to me ?
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
			if ( m_pNPC->m_Brain == NPCBRAIN_GUARD )	// don't create corpse if NPC got killed by a guard
				pKill->StatFlag_Set(STATF_Conjured);
			return;
		}
	}
	else if ( NotoThem < NOTO_GUILD_SAME )
	{
		ASSERT(m_pPlayer);
		// I'm a murderer !
		if ( !IsPriv(PRIV_GM) )
		{
			CScriptTriggerArgs args;
			args.m_iN1 = m_pPlayer->m_wMurders+1;
			args.m_iN2 = true;

			if ( IsTrigUsed(TRIGGER_MURDERMARK) )
			{
				OnTrigger(CTRIG_MurderMark, this, &args);
				if ( args.m_iN1 < 0 )
					args.m_iN1 = 0;
			}

			m_pPlayer->m_wMurders = static_cast<word>(args.m_iN1);
			NotoSave_Update();
			if ( args.m_iN2 )
				Noto_Criminal();

			Noto_Murder();
		}
	}

	// No fame/karma/exp gain on these conditions
	if ( fPetKill || NotoThem == NOTO_GUILD_SAME || pKill->IsStatFlag(STATF_Conjured) || (pKill->m_pNPC && pKill->m_pNPC->m_bonded) )
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

int CChar::NotoSave() 
{ 
	ADDTOCALLSTACK("CChar::NotoSave");
	return static_cast<int>(m_notoSaves.size());
}
void CChar::NotoSave_Add( CChar * pChar, NOTO_TYPE value, NOTO_TYPE color  )
{
	ADDTOCALLSTACK("CChar::NotoSave_Add");
	if ( !pChar )
		return;
	CGrayUID uid = pChar->GetUID();
	if  ( m_notoSaves.size() )	// Checking if I already have him in the list, only if there 's any list.
	{
		for (std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it)
		{
			NotoSaves & refNoto = *it;
			if ( refNoto.charUID == uid )
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
	refNoto.charUID = pChar->GetUID();
	refNoto.time = 0;
	refNoto.value = value;
	refNoto.color = color;
	m_notoSaves.push_back(refNoto);
}

NOTO_TYPE CChar::NotoSave_GetValue( int id, bool bGetColor )
{
	ADDTOCALLSTACK("CChar::NotoSave_GetValue");
	if ( !m_notoSaves.size() )
		return NOTO_INVALID;
	if ( id < 0 )
		return NOTO_INVALID;
	if ( static_cast<int>(m_notoSaves.size()) <= id )
		return NOTO_INVALID;
	NotoSaves & refNotoSave = m_notoSaves.at(id);
	if (bGetColor && refNotoSave.color != 0 )	// retrieving color if requested... only if a color is greater than 0 (to avoid possible crashes).
		return refNotoSave.color;
	else
		return refNotoSave.value;
}

INT64 CChar::NotoSave_GetTime( int id )
{
	ADDTOCALLSTACK("CChar::NotoSave_GetTime");
	if ( !m_notoSaves.size() )
		return -1;
	if ( id < 0 )
		return NOTO_INVALID;
	if ( static_cast<int>(m_notoSaves.size()) <= id )
		return -1;
	NotoSaves & refNotoSave = m_notoSaves.at(id);
	return refNotoSave.time;
}

void CChar::NotoSave_Clear()
{
	ADDTOCALLSTACK("CChar::NotoSave_Clear");
	if ( m_notoSaves.size() )
		m_notoSaves.clear();
}

void CChar::NotoSave_Update()
{
	ADDTOCALLSTACK("CChar::NotoSave_Update");
	NotoSave_Clear();
	UpdateMode();
	ResendTooltip();
}

void CChar::NotoSave_CheckTimeout()
{
	ADDTOCALLSTACK("CChar::NotoSave_CheckTimeout");
	if (g_Cfg.m_iNotoTimeout <= 0)	// No value = no expiration.
		return;
	if (m_notoSaves.size())
	{
		int count = 0;
		for (std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it)
		{
			NotoSaves & refNoto = *it;
			if (++(refNoto.time) > g_Cfg.m_iNotoTimeout)	// updating timer while checking ini's value.
			{
				//m_notoSaves.erase(it);
				NotoSave_Resend(count);
				break;
			}
			count++;
		}
	}
}

void CChar::NotoSave_Resend( int id )
{
	ADDTOCALLSTACK("CChar::NotoSave_Resend()");
	if ( !m_notoSaves.size() )
		return;
	if ( static_cast<int>(m_notoSaves.size()) <= id )
		return;
	NotoSaves & refNotoSave = m_notoSaves.at( id );
	CGrayUID uid = refNotoSave.charUID;
	CChar * pChar = uid.CharFind();
	if ( ! pChar )
		return;
	NotoSave_Delete( pChar );
	CObjBaseTemplate *pObj = pChar->GetTopLevelObj();
	if (  GetDist( pObj ) < UO_MAP_VIEW_SIGHT )
		Noto_GetFlag( pChar, true , true );
}

int CChar::NotoSave_GetID( CChar * pChar )
{
	ADDTOCALLSTACK("CChar::NotoSave_GetID(CChar)");
	if ( !pChar || !m_notoSaves.size() )
		return -1;
	if ( NotoSave() )
	{
		int count = 0;
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves & refNotoSave = m_notoSaves.at(count);
			CGrayUID uid = refNotoSave.charUID;
			if ( uid.CharFind() && uid == static_cast<dword>(pChar->GetUID()) )
				return count;
			count++;
		}
	}
	return -1;
}

bool CChar::NotoSave_Delete( CChar * pChar )
{		
	ADDTOCALLSTACK("CChar::NotoSave_Delete");
	if ( ! pChar )
		return false;
	if ( NotoSave() )
	{
		int count = 0;
		for ( std::vector<NotoSaves>::iterator it = m_notoSaves.begin(); it != m_notoSaves.end(); ++it )
		{
			NotoSaves & refNotoSave = m_notoSaves.at(count);
			CGrayUID uid = refNotoSave.charUID;
			if ( uid.CharFind() && uid == static_cast<dword>(pChar->GetUID()) )
			{
				m_notoSaves.erase(it);
				return true;
			}
			count++;
		}
	}
	return false;
}
