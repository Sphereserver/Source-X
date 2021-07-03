
// Game server messages. (No login stuff)

#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../items/CItemMap.h"
#include "../items/CItemMessage.h"
#include "../items/CItemMultiCustom.h"
#include "../components/CCSpawn.h"
#include "../CSector.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldTickingList.h"
#include "../spheresvr.h"
#include "../triggers.h"
#include "CClient.h"


/////////////////////////////////////////////////////////////////
// -CClient stuff.

void CClient::resendBuffs() const
{
	// These checks are in addBuff too, but it would be useless to call it so many times
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	CChar *pChar = GetChar();
	ASSERT(pChar);

	// NOTE: If the player logout and login again without close the client, buffs with remaining
	// time will stay cached on client, making it not display the remaining time if the server
	// send this same buff again. To avoid this, we must remove the buff before send it.

	if ( pChar->IsStatFlag(STATF_HIDDEN | STATF_INSUBSTANTIAL) )
		addBuff(BI_HIDDEN, 1075655, 1075656);

	const CItem *pStuck = pChar->LayerFind(LAYER_FLAG_Stuck);
	if ( pStuck )
	{
		removeBuff(BI_PARALYZE);
		addBuff(BI_PARALYZE, 1075827, 1075828, (word)(pStuck->GetTimerSAdjusted()));
	}

	// Spells
	tchar NumBuff[7][8];
	lpctstr pNumBuff[7] = { NumBuff[0], NumBuff[1], NumBuff[2], NumBuff[3], NumBuff[4], NumBuff[5], NumBuff[6] };

	word wStatEffect = 0;
	word wTimerEffect = 0;

	for (const CSObjContRec* pObjRec : *pChar)
	{
		const CItem* pItem = static_cast<const CItem*>(pObjRec);
		if ( !pItem->IsType(IT_SPELL) )
			continue;

		wStatEffect = pItem->m_itSpell.m_spelllevel;
        int64 iTimerEffectSigned = pItem->GetTimerSAdjusted();
		wTimerEffect = (word)(maximum(iTimerEffectSigned, 0));

		switch ( pItem->m_itSpell.m_spell )
		{
			case SPELL_Night_Sight:
				removeBuff(BI_NIGHTSIGHT);
				addBuff(BI_NIGHTSIGHT, 1075643, 1075644, wTimerEffect);
				break;
			case SPELL_Clumsy:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_CLUMSY);
				addBuff(BI_CLUMSY, 1075831, 1075832, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Weaken:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_WEAKEN);
				addBuff(BI_WEAKEN, 1075837, 1075838, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Feeblemind:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_FEEBLEMIND);
				addBuff(BI_FEEBLEMIND, 1075833, 1075834, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Curse:
			{
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					Str_FromI(wStatEffect, NumBuff[idx], sizeof(NumBuff[0]), 10);
				for ( int idx = 3; idx < 7; ++idx )
					Str_FromI(10, NumBuff[idx], sizeof(NumBuff[0]), 10);

				removeBuff(BI_CURSE);
				addBuff(BI_CURSE, 1075835, 1075836, wTimerEffect, pNumBuff, 7);
				break;
			}
			case SPELL_Mass_Curse:
			{
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					Str_FromI(wStatEffect, NumBuff[idx], sizeof(NumBuff[0]), 10);

				removeBuff(BI_MASSCURSE);
				addBuff(BI_MASSCURSE, 1075839, 1075840, wTimerEffect, pNumBuff, 3);
				break;
			}
			case SPELL_Strength:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_STRENGTH);
				addBuff(BI_STRENGTH, 1075845, 1075846, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Agility:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_AGILITY);
				addBuff(BI_AGILITY, 1075841, 1075842, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Cunning:
				Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
				removeBuff(BI_CUNNING);
				addBuff(BI_CUNNING, 1075843, 1075844, wTimerEffect, pNumBuff, 1);
				break;
			case SPELL_Bless:
			{
				for ( int idx = STAT_STR; idx < STAT_BASE_QTY; ++idx )
					Str_FromI(wStatEffect, NumBuff[idx], sizeof(NumBuff[0]), 10);

				removeBuff(BI_BLESS);
				addBuff(BI_BLESS, 1075847, 1075848, wTimerEffect, pNumBuff, STAT_BASE_QTY);
				break;
			}
			case SPELL_Reactive_Armor:
			{
				removeBuff(BI_REACTIVEARMOR);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					Str_FromI(wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
					for ( int idx = 1; idx < 5; ++idx )
						Str_FromI(5, NumBuff[idx], sizeof(NumBuff[0]), 10);

					addBuff(BI_REACTIVEARMOR, 1075812, 1075813, wTimerEffect, pNumBuff, 5);
				}
				else
				{
					addBuff(BI_REACTIVEARMOR, 1075812, 1070722, wTimerEffect);
				}
				break;
			}
			case SPELL_Protection:
			case SPELL_Arch_Prot:
			{
				BUFF_ICONS BuffIcon = BI_PROTECTION;
				dword BuffCliloc = 1075814;
				if ( pItem->m_itSpell.m_spell == SPELL_Arch_Prot )
				{
					BuffIcon = BI_ARCHPROTECTION;
					BuffCliloc = 1075816;
				}

				removeBuff(BuffIcon);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					Str_FromI(-pItem->m_itSpell.m_PolyStr, NumBuff[0], sizeof(NumBuff[0]), 10);
					Str_FromI(-pItem->m_itSpell.m_PolyDex / 10, NumBuff[1], sizeof(NumBuff[0]), 10);
					addBuff(BuffIcon, BuffCliloc, 1075815, wTimerEffect, pNumBuff, 2);
				}
				else
				{
					addBuff(BuffIcon, BuffCliloc, 1070722, wTimerEffect);
				}
				break;
			}
			case SPELL_Poison:
				removeBuff(BI_POISON);
				addBuff(BI_POISON, 1017383, 1070722, wTimerEffect);
				break;
			case SPELL_Incognito:
				removeBuff(BI_INCOGNITO);
				addBuff(BI_INCOGNITO, 1075819, 1075820, wTimerEffect);
				break;
			case SPELL_Paralyze:
				removeBuff(BI_PARALYZE);
				addBuff(BI_PARALYZE, 1075827, 1075828, wTimerEffect);
				break;
			case SPELL_Magic_Reflect:
			{
				removeBuff(BI_MAGICREFLECTION);
				if ( IsSetCombatFlags(COMBAT_ELEMENTAL_ENGINE) )
				{
					Str_FromI(-wStatEffect, NumBuff[0], sizeof(NumBuff[0]), 10);
					for ( int idx = 1; idx < 5; ++idx )
						Str_FromI(10, NumBuff[idx], sizeof(NumBuff[0]), 10);

					addBuff(BI_MAGICREFLECTION, 1075817, 1075818, wTimerEffect, pNumBuff, 5);
				}
				else
				{
					addBuff(BI_MAGICREFLECTION, 1075817, 1070722, wTimerEffect);
				}
				break;
			}
			case SPELL_Invis:
				removeBuff(BI_INVISIBILITY);
				addBuff(BI_INVISIBILITY, 1075825, 1075826, wTimerEffect);
				break;
			default:
				break;
		}

	}
}

void CClient::addBuff( const BUFF_ICONS IconId, const dword ClilocOne, const dword ClilocTwo, const word durationSeconds, lpctstr* pArgs, uint uiArgCount) const
{
	ADDTOCALLSTACK("CClient::addBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	new PacketBuff(this, IconId, ClilocOne, ClilocTwo, durationSeconds, pArgs, uiArgCount);
}

void CClient::removeBuff(const BUFF_ICONS IconId) const
{
	ADDTOCALLSTACK("CClient::removeBuff");
	if ( !IsSetOF(OF_Buffs) )
		return;
	if ( PacketBuff::CanSendTo(GetNetState()) == false )
		return;

	new PacketBuff(this, IconId);
}


bool CClient::addDeleteErr(byte code, dword iSlot) const
{
	ADDTOCALLSTACK("CClient::addDeleteErr");
	// code
	if ( code == PacketDeleteError::Success )
		return true;
	const CChar *pChar = (code == PacketDeleteError::NotExist) ? nullptr : m_tmSetupCharList[iSlot].CharFind();
	g_Log.EventWarn("%x:Bad Char Delete Attempted %d (acct='%s', char='%s', IP='%s')\n", GetSocketID(), code, GetAccount()->GetName(), (pChar ? pChar->GetName() : "INVALID"), GetPeerStr());
	new PacketDeleteError(this, static_cast<PacketDeleteError::Reason>(code));
	return false;
}

void CClient::addTime( bool fCurrent ) const
{
	ADDTOCALLSTACK("CClient::addTime");
	// Send time. (real or game time ??? why ?)

	if ( fCurrent )
	{
		const llong lCurrentTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();
		new PacketGameTime(this,
								( lCurrentTime / ( 60*60*MSECS_PER_SEC)) % 24,
								( lCurrentTime / ( 60*MSECS_PER_SEC)) % 60,
								( lCurrentTime / (MSECS_PER_SEC)) % 60);
	}
	else
	{
		new PacketGameTime(this);
	}
}

void CClient::addObjectRemoveCantSee( const CUID& uid, lpctstr pszName ) const
{
	ADDTOCALLSTACK("CClient::addObjectRemoveCantSee");
	// Seems this object got out of sync some how.
	if ( pszName == nullptr )
        pszName = "it";
	SysMessagef( "You can't see %s", pszName );
	addObjectRemove( uid );
}

void CClient::closeContainer( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CClient::closeContainer");
	new PacketCloseContainer(this, pObj);
}

void CClient::closeUIWindow( const CObjBase* pObj, PacketCloseUIWindow::UIWindow windowType ) const
{
	ADDTOCALLSTACK("CClient::closeUIWindow");
	new PacketCloseUIWindow(this, pObj, windowType);
}

void CClient::addObjectRemove( const CUID& uid ) const
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	// Tell the client to remove the item or char
	new PacketRemoveObject(this, uid);
}

void CClient::addObjectRemove( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CClient::addObjectRemove");
	addObjectRemove( pObj->GetUID());
}

void CClient::addRemoveAll( bool fItems, bool fChars )
{
	ADDTOCALLSTACK("CClient::addRemoveAll");
	if ( fItems )
	{
		// Remove any multi objects first ! or client will hang
		CWorldSearch AreaItems(GetChar()->GetTopPoint(), UO_MAP_VIEW_RADAR);
		AreaItems.SetSearchSquare(true);
		for (;;)
		{
			CItem * pItem = AreaItems.GetItem();
			if ( pItem == nullptr )
				break;
			addObjectRemove(pItem);
		}
	}
	if ( fChars )
	{
		CChar * pCharSrc = GetChar();
		CWorldSearch AreaChars(GetChar()->GetTopPoint(), GetChar()->GetVisualRange());
		AreaChars.SetAllShow(IsPriv(PRIV_ALLSHOW));
		AreaChars.SetSearchSquare(true);
		for (;;)
		{
			CChar * pChar = AreaChars.GetChar();
			if ( pChar == nullptr )
				break;
			if ( pChar == pCharSrc )
				continue;
			addObjectRemove(pChar);
		}
	}
}

void CClient::addItem_OnGround( CItem * pItem ) // Send items (on ground)
{
	ADDTOCALLSTACK("CClient::addItem_OnGround");
	if ( !pItem )
		return;

    const bool fCorpse = (pItem->GetDispID() == ITEMID_CORPSE);
    // The new packet doesn't show the right direction for corpses. Use the old one (which still works with old clients, for items with item graphics <= 0x3FFF).
	if ( !fCorpse && PacketItemWorldNew::CanSendTo(GetNetState()) )
		new PacketItemWorldNew(this, pItem);
	else
		new PacketItemWorld(this, pItem);

	// send KR drop confirmation
	if ( PacketDropAccepted::CanSendTo(GetNetState()) )
		new PacketDropAccepted(this);

	// send item sound
	if (pItem->IsType(IT_SOUND))
		addSound((SOUND_TYPE)(pItem->m_itSound.m_dwSound), pItem, pItem->m_itSound.m_iRepeat );

	// send corpse clothing
	if ( !IsPriv(PRIV_DEBUG) && (fCorpse && CCharBase::IsPlayableID(pItem->GetCorpseType())) )	// cloths on corpse
	{
		const CItemCorpse *pCorpse = static_cast<const CItemCorpse *>(pItem);
		if ( pCorpse )
		{
			addContainerContents( pCorpse, false, true );	// send all corpse items
			addContainerContents( pCorpse, true, true );	// equip proper items on corpse
		}
	}

	// send item tooltip
	addAOSTooltip(pItem);

	if ( (pItem->IsType(IT_MULTI_CUSTOM)) && m_pChar->CanSee(pItem) )
	{
		// send house design version
		const CItemMultiCustom *pItemMulti = dynamic_cast<const CItemMultiCustom *>(pItem);
        ASSERT(pItemMulti);
        pItemMulti->SendVersionTo(this);
	}
}

void CClient::addItem_Equipped( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem_Equipped");
	ASSERT(pItem);
	// Equip a single item on a CChar.
	CChar * pChar = dynamic_cast <CChar*> (pItem->GetParent());
	ASSERT( pChar != nullptr );

	if ( ! m_pChar->CanSeeItem( pItem ) && m_pChar != pChar )
		return;

	new PacketItemEquipped(this, pItem);

	//addAOSTooltip(pItem);		// tooltips for equipped items are handled on packet 0x78 (PacketCharacter)
}

void CClient::addItem_InContainer( const CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem_InContainer");
	ASSERT(pItem);
	CItemContainer * pCont = dynamic_cast <CItemContainer*> (pItem->GetParent());
	if ( pCont == nullptr )
		return;

	new PacketItemContainer(this, pItem);

	if ( PacketDropAccepted::CanSendTo(GetNetState()) )
		new PacketDropAccepted(this);

	//addAOSTooltip(pItem);		// tooltips for items inside containers are handled on packet 0x3C (PacketItemContents)
}

void CClient::addItem( CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItem");
	if ( pItem == nullptr )
		return;
	if ( pItem->IsTopLevel())
		addItem_OnGround( pItem );
	else if ( pItem->IsItemEquipped())
		addItem_Equipped( pItem );
	else if ( pItem->IsItemInContainer())
		addItem_InContainer( pItem );
}

void CClient::addContainerContents( const CItemContainer * pContainer, bool fCorpseEquip, bool fCorpseFilter, bool fShop ) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContainerContents");
	// NOTE: We needed to send the header for this FIRST !!!
	// 1 = equip a corpse
	// 0 = contents.

	if (fCorpseEquip == true)
		new PacketCorpseEquipment(this, pContainer);
	else
		new PacketItemContents(this, pContainer, fShop, fCorpseFilter);

	return;
}

void CClient::addOpenGump( const CObjBase * pContainer, GUMP_TYPE gump ) const
{
	ADDTOCALLSTACK("CClient::addOpenGump");
	// NOTE: if pContainer has not already been sent to the client
	//  this will crash client.
	new PacketContainerOpen(this, pContainer, gump);
}

bool CClient::addContainerSetup( const CItemContainer * pContainer ) // Send Backpack (with items)
{
	ADDTOCALLSTACK("CClient::addContainerSetup");
	ASSERT(pContainer);
	ASSERT(pContainer->IsItem());

	// open the container with the proper GUMP.
	CItemBase * pItemDef = pContainer->Item_GetDef();
	if (!pItemDef)
		return false;

	GUMP_TYPE gump = pItemDef->IsTypeContainer();
	if ( gump <= GUMP_RESERVED )
		return false;

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);

	addOpenGump(pContainer, gump);
	addContainerContents(pContainer);

	LogOpenedContainer(pContainer);
	return true;
}

void CClient::LogOpenedContainer(const CItemContainer* pContainer) // record a container in opened container list
{
	ADDTOCALLSTACK("CClient::LogOpenedContainer");
	if (pContainer == nullptr)
		return;

	const CObjBaseTemplate * pTopMostContainer = pContainer->GetTopLevelObj();
	const CObjBase * pTopContainer = pContainer->GetContainer();

	dword dwTopMostContainerUID = pTopMostContainer->GetUID().GetPrivateUID();
	dword dwTopContainerUID = 0;

	if ( pTopContainer != nullptr )
		dwTopContainerUID = pTopContainer->GetUID().GetPrivateUID();
	else
		dwTopContainerUID = dwTopMostContainerUID;

	m_openedContainers[pContainer->GetUID().GetPrivateUID()] = std::make_pair(
																std::make_pair( dwTopContainerUID, dwTopMostContainerUID ),
																pTopMostContainer->GetTopPoint()
															    );
}

void CClient::addSeason(SEASON_TYPE season)
{
	ADDTOCALLSTACK("CClient::addSeason");
	if ( m_pChar->IsStatFlag(STATF_DEAD) )		// everything looks like this when dead.
		season = SEASON_Desolate;
	if ( season == m_Env.m_Season )	// the season i saw last.
		return;

	m_Env.m_Season = season;

	new PacketSeason(this, season, true);

	// client resets light level on season change, so resend light here too
	m_Env.m_Light = UINT8_MAX;
	addLight();
}

void CClient::addWeather( WEATHER_TYPE weather ) // Send new weather to player
{
	ADDTOCALLSTACK("CClient::addWeather");
	ASSERT( m_pChar );

	if ( g_Cfg.m_fNoWeather )
		return;

	if ( weather == WEATHER_DEFAULT )
		weather = m_pChar->GetTopSector()->GetWeather();

	if ( weather == m_Env.m_Weather )
		return;

	m_Env.m_Weather = weather;
	new PacketWeather(this, weather, Calc_GetRandVal2(10, 70), 0x10);
}

void CClient::addLight() const
{
	ADDTOCALLSTACK("CClient::addLight");
	// NOTE: This could just be a flash of light.
	// Global light level.
	ASSERT(m_pChar);
	ASSERT(m_pChar->m_pPlayer);

	if (m_pChar->IsStatFlag(STATF_NIGHTSIGHT|STATF_DEAD))
	{
		new PacketGlobalLight(this, LIGHT_BRIGHT);
		return;
	}

	byte iLight = UINT8_MAX;

	if ( m_pChar->m_pPlayer->m_LocalLight )			// personal light override
		iLight = m_pChar->m_pPlayer->m_LocalLight;

	if ( iLight == UINT8_MAX )
		iLight = m_pChar->GetLightLevel();

	// Scale light level for non-t2a.
	//if ( iLight < LIGHT_BRIGHT )
	//	iLight = LIGHT_BRIGHT;
	if ( iLight > LIGHT_DARK )
		iLight = LIGHT_DARK;

	if ( iLight == m_Env.m_Light )
		return;

	new PacketGlobalLight(this, iLight);
}

void CClient::addArrowQuest( int x, int y, int id ) const
{
	ADDTOCALLSTACK("CClient::addArrowQuest");

	new PacketArrowQuest(this, x, y, id);
}

void CClient::addMusic( MIDI_TYPE id ) const
{
	ADDTOCALLSTACK("CClient::addMusic");
	// Music is ussually appropriate for the region.
	new PacketPlayMusic(this, id);
}

bool CClient::addKick( CTextConsole * pSrc, bool fBlock )
{
	ADDTOCALLSTACK("CClient::addKick");
	// Kick me out.
	ASSERT( pSrc );
	if ( GetAccount() == nullptr )
	{
		GetNetState()->markReadClosed();
		return true;
	}

	if ( ! GetAccount()->Kick( pSrc, fBlock ))
		return false;

	lpctstr pszAction = fBlock ? "KICK" : "DISCONNECT";
	SysMessagef("You have been %sed by '%s'", pszAction, pSrc->GetName());

	if ( IsConnectTypePacket() )
	{
		new PacketKick(this);
	}

	GetNetState()->markReadClosed();
	return true;
}

void CClient::addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase, int iOnce ) const
{
	ADDTOCALLSTACK("CClient::addSound");
	if ( !g_Cfg.m_fGenericSounds )
		return;

	CPointMap pt;
	if ( pBase )
	{
		pBase = pBase->GetTopLevelObj();
		pt = pBase->GetTopPoint();
	}
	else
		pt = m_pChar->GetTopPoint();

	if (( id > 0 ) && !iOnce && !pBase )
		return;

	new PacketPlaySound(this, id, iOnce, 0, pt);
}

void CClient::addBarkUNICODE( const nchar * pwText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang ) const
{
	ADDTOCALLSTACK("CClient::addBarkUNICODE");
	if ( pwText == nullptr )
		return;

	if ( ! IsConnectTypePacket() )
	{
		// Need to convert back from unicode !
		//SysMessage(pwText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SAY;
		pSrc = nullptr;
	}

	new PacketMessageUNICODE(this, pwText, pSrc, wHue, mode, font, lang);
}

void CClient::addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, lpctstr pArgs ) const
{
	ADDTOCALLSTACK("CClient::addBarkLocalized");
	if ( iClilocId <= 0 )
		return;

	if ( !IsConnectTypePacket() )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SAY;
		pSrc = nullptr;
	}

	new PacketMessageLocalised(this, iClilocId, pSrc, wHue, mode, font, pArgs);
}

void CClient::addBarkLocalizedEx( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affix, lpctstr pAffix, lpctstr pArgs ) const
{
	ADDTOCALLSTACK("CClient::addBarkLocalizedEx");
	if ( iClilocId <= 0 )
		return;

	if ( !IsConnectTypePacket() )
		return;

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SAY;
		pSrc = nullptr;
	}

	new PacketMessageLocalisedEx(this, iClilocId, pSrc, wHue, mode, font, affix, pAffix, pArgs);
}

void CClient::addBarkParse( lpctstr pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool fUnicode, lpctstr name) const
{
	ADDTOCALLSTACK("CClient::addBarkParse");
	if ( !pszText )
		return;

	HUE_TYPE defaultHue = HUE_TEXT_DEF;
	FONT_TYPE defaultFont = FONT_NORMAL;
	bool defaultUnicode = false;
    bool fUseSpeechHueOverride = false;
    bool fUseEmoteHueOverride = false;
	const CChar * pSrcChar = nullptr;
	if (pSrc && pSrc->IsChar())
	    pSrcChar = static_cast<const CChar *>(pSrc);

	switch ( mode )
	{
		case TALKMODE_SYSTEM:
		{
		talkmode_system:
			defaultHue = (HUE_TYPE)(g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_COLOR"));
			defaultFont = (FONT_TYPE)(g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_FONT"));
			defaultUnicode = g_Exp.m_VarDefs.GetKeyNum("SMSG_DEF_UNICODE") > 0 ? true : false;
			break;
		}
		case TALKMODE_EMOTE:
		{
			fUseEmoteHueOverride = true;
			defaultHue = (HUE_TYPE)(g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_COLOR"));
			defaultFont = (FONT_TYPE)(g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_FONT"));
			defaultUnicode = g_Exp.m_VarDefs.GetKeyNum("EMOTE_DEF_UNICODE") > 0 ? true : false;
			break;
		}
		case TALKMODE_SAY:
		{
            if (pSrc == nullptr)
            {
                // It's a SYSMESSAGE
                goto talkmode_system;
            }
            fUseSpeechHueOverride = true;
			defaultHue = (HUE_TYPE)(g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_COLOR"));
			defaultFont = (FONT_TYPE)(g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_FONT"));
			defaultUnicode = g_Exp.m_VarDefs.GetKeyNum("SAY_DEF_UNICODE") > 0 ? true : false;
			break;
		}
		case TALKMODE_ITEM:
		{
			if ( pSrc && pSrc->IsChar() )
			{
				defaultFont = (FONT_TYPE)(g_Exp.m_VarDefs.GetKeyNum("CMSG_DEF_FONT"));
				defaultUnicode = g_Exp.m_VarDefs.GetKeyNum("CMSG_DEF_UNICODE") > 0 ? true : false;
			}
			else
			{
				defaultHue = (HUE_TYPE)(g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_COLOR"));
				defaultFont = (FONT_TYPE)(g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_FONT"));
				defaultUnicode = g_Exp.m_VarDefs.GetKeyNum("IMSG_DEF_UNICODE") > 0 ? true : false;
			}
		}
		break;

		default:
			break;
	}

	word Args[] = { (word)wHue, (word)font, (word)fUnicode };
    lptstr ptcBarkBuffer = Str_GetTemp();  // Be sure to init this before the goto instruction

	if ( *pszText == '@' )
	{
		++pszText;
		if ( *pszText == '@' ) // @@ = just a @ symbol
			goto bark_default;

		const char *s = pszText;
		pszText	= strchr( s, ' ' );

		if ( !pszText )
			return;

		for ( int i = 0; ( s < pszText ) && ( i < 3 ); )
		{
			if ( *s == ',' ) // default value, skip it
			{
				++i;
				++s;
				continue;
			}
			Args[i] = (Exp_GetWVal(s));
			++i;

			if ( *s == ',' )
				++s;
			else
				break;	// no more args here!
		}
		++pszText;
		if ( Args[1] > FONT_QTY )
			Args[1] = (word)FONT_NORMAL;
	}

	if (fUseSpeechHueOverride && pSrcChar)
	{
        if (pSrcChar->m_SpeechHueOverride)
            Args[0] = (word)pSrcChar->m_SpeechHueOverride;
	}
	else if (fUseEmoteHueOverride && pSrcChar)
	{
		if (pSrcChar->m_EmoteHueOverride)
			Args[0] = (word)pSrcChar->m_EmoteHueOverride;
	}
	/*
	else if (mode <= TALKMODE_YELL)
	I removed else from the beginning of the condition, because if pSrcChar->m_SpeechHueOverride or pSrcChar->m_EmoteHueOverride not set, we still need to set default value.
	It should not block the overrides, because Args[0] changed another value from "HUE_TEXT_DEF".

	- xwerswoodx
	*/
    if (mode <= TALKMODE_YELL)
    {
        if (Args[0] == HUE_TEXT_DEF)
            Args[0] = (word)defaultHue;
    }

    if (mode != TALKMODE_SPELL)
    {
        if ( Args[1] == FONT_NORMAL )
            Args[1] = (word)defaultFont;
    }

	if ( Args[2] == 0 )
		Args[2] = (word)defaultUnicode;

	Str_CopyLimitNull(	ptcBarkBuffer, name,	STR_TEMPLENGTH);
	Str_ConcatLimitNull(ptcBarkBuffer, pszText, STR_TEMPLENGTH);

	switch ( Args[2] )
	{
		case 3:	// Extended localized message (with affixed ASCII text)
		{
            tchar * ppArgs[256];
			int iQty = Str_ParseCmds(ptcBarkBuffer, ppArgs, CountOf(ppArgs), "," );
			int iClilocId = Exp_GetVal( ppArgs[0] );
			int iAffixType = Exp_GetVal( ppArgs[1] );
			CSString CArgs;
			for (int i = 3; i < iQty; ++i )
			{
				if ( CArgs.GetLength() )
					CArgs += "\t";
				CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
			}

			addBarkLocalizedEx( iClilocId, pSrc, (HUE_TYPE)(Args[0]), mode, (FONT_TYPE)(Args[1]), (AFFIX_TYPE)(iAffixType), ppArgs[2], CArgs.GetBuffer());
			break;
		}

		case 2:	// Localized
		{
            tchar * ppArgs[256];
			int iQty = Str_ParseCmds(ptcBarkBuffer, ppArgs, CountOf(ppArgs), "," );
			int iClilocId = Exp_GetVal( ppArgs[0] );
			CSString CArgs;
			for ( int i = 1; i < iQty; ++i )
			{
				if ( CArgs.GetLength() )
					CArgs += "\t";
				CArgs += ( !strcmp(ppArgs[i], "NULL") ? " " : ppArgs[i] );
			}

			addBarkLocalized( iClilocId, pSrc, (HUE_TYPE)(Args[0]), mode, (FONT_TYPE)(Args[1]), CArgs.GetBuffer());
			break;
		}

		case 1:	// Unicode
		{
			nchar szBuffer[ MAX_TALK_BUFFER ];
			CvtSystemToNUNICODE( szBuffer, CountOf(szBuffer), ptcBarkBuffer, -1 );
			addBarkUNICODE( szBuffer, pSrc, (HUE_TYPE)(Args[0]), mode, (FONT_TYPE)(Args[1]), 0 );
			break;
		}

		case 0:	// Ascii
		default:
		{
bark_default:
			if (ptcBarkBuffer[0] == '\0')
			{
				Str_CopyLimitNull(ptcBarkBuffer, name, STR_TEMPLENGTH);
				Str_ConcatLimitNull(ptcBarkBuffer, pszText, STR_TEMPLENGTH);
			}

			addBark(ptcBarkBuffer, pSrc, (HUE_TYPE)(Args[0]), mode, (FONT_TYPE)(Args[1]));
			break;
		}
	}
}

void CClient::addBark( lpctstr pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font ) const
{
	ADDTOCALLSTACK("CClient::addBark");
	if ( pszText == nullptr )
		return;

	if ( ! IsConnectTypePacket() )
	{
		SysMessage(pszText);
		return;
	}

	if ( mode == TALKMODE_BROADCAST )
	{
		mode = TALKMODE_SAY;
		pSrc = nullptr;
	}

	new PacketMessageASCII(this, pszText, pSrc, wHue, mode, font);
}

void CClient::addObjMessage( lpctstr pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode ) // The message when an item is clicked
{
	ADDTOCALLSTACK("CClient::addObjMessage");
	if ( !pMsg )
		return;

	if ( IsSetOF(OF_Flood_Protection) && ( GetPrivLevel() <= PLEVEL_Player )  )
	{
		if ( !strnicmp(pMsg, m_zLastObjMessage, SCRIPT_MAX_LINE_LEN) )
			return;

		Str_CopyLimitNull(m_zLastObjMessage, pMsg, SCRIPT_MAX_LINE_LEN);
	}

	addBarkParse(pMsg, pSrc, wHue, mode);
}

void CClient::addEffect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc,
    byte bSpeedMsecs, byte bLoop, bool fExplode, dword color, dword render, word effectid, dword explodeid, word explodesound, dword effectuid, byte type) const
{
	ADDTOCALLSTACK("CClient::addEffect");
	// bSpeedSeconds = tenths of second = 0=very fast, 7=slow.

	ASSERT(m_pChar);
	ASSERT(pDst);

	if ( (pSrc == nullptr) && (motion == EFFECT_BOLT) ) // source required for bolt effect
		return;

	if (effectid || explodeid)
		new PacketEffect(this, motion, id, pDst, pSrc, bSpeedMsecs, bLoop, fExplode, color, render, effectid, explodeid, explodesound, effectuid, type);
	else if (color || render)
		new PacketEffect(this, motion, id, pDst, pSrc, bSpeedMsecs, bLoop, fExplode, color, render);
	else
		new PacketEffect(this, motion, id, pDst, pSrc, bSpeedMsecs, bLoop, fExplode);
}

/* Effect at a Map Point instead of an Object */
void CClient::addEffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptSrc, const CPointMap *ptDest,
    byte bSpeedMsecs, byte bLoop, bool fExplode, dword color, dword render, word effectid, dword explodeid, word explodesound, dword effectuid, byte type) const
{
	ADDTOCALLSTACK("CClient::addEffect");
	// bSpeedSeconds = tenth of second = 0=very fast, 7=slow.

	ASSERT(m_pChar);

    const CPointMap *ptSrcToUse = (ptDest && (motion == EFFECT_BOLT)) ? ptSrc : ptDest;

	if (effectid || explodeid)
		new PacketEffect(this, motion, id, ptDest, ptSrcToUse, bSpeedMsecs, bLoop, fExplode, color, render, effectid, explodeid, explodesound, effectuid, type);
	else if (color || render)
		new PacketEffect(this, motion, id, ptDest, ptSrcToUse, bSpeedMsecs, bLoop, fExplode, color, render);
	else
		new PacketEffect(this, motion, id, ptDest, ptSrcToUse, bSpeedMsecs, bLoop, fExplode);
}

void CClient::GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE & wHue ) const
{
	ADDTOCALLSTACK("CClient::GetAdjustedItemID");
	// An equipped item.
	ASSERT( pChar );

	id = pItem->GetDispID();
	wHue = pItem->GetHue();

	const CItemBase * pItemDef = pItem->Item_GetDef();
    const uchar uiResDisp = GetResDisp();

	if ( pItem->IsType(IT_EQ_HORSE) )
	{
		// check the reslevel of the ridden horse
		CREID_TYPE idHorse = pItem->m_itFigurine.m_ID;
		const CCharBase * pCharDef = CCharBase::FindCharBase(idHorse);
		if ( pCharDef && (uiResDisp < pCharDef->GetResLevel() ) )
		{
			idHorse = (CREID_TYPE)(pCharDef->GetResDispDnId());
			wHue = pCharDef->GetResDispDnHue();

			// adjust the item to display the mount item associated with
			// the resdispdnid of the mount's chardef
			if ( idHorse != pItem->m_itFigurine.m_ID )
			{
				tchar * sMountDefname = Str_GetTemp();
				sprintf(sMountDefname, "mount_0x%x", idHorse);
				ITEMID_TYPE idMountItem = (ITEMID_TYPE)(g_Exp.m_VarDefs.GetKeyNum(sMountDefname));
				if ( idMountItem > ITEMID_NOTHING )
				{
					id = idMountItem;
					pItemDef = CItemBase::FindItemBase(id);
				}
			}
		}
	}

	if ( m_pChar->IsStatFlag( STATF_HALLUCINATING ))
		wHue = (HUE_TYPE)(Calc_GetRandVal( HUE_DYE_HIGH ));

	else if ( pChar->IsStatFlag(STATF_STONE)) //Client do not have stone state. So we must send the hue we want. (Affect the paperdoll hue as well)
		wHue = HUE_STONE;

	// Normaly, client automaticly change the anim color in grey when someone is insubtantial, hidden or invisible. Paperdoll are never affected by this.
	// The next 3 state overwrite the client process and force ITEM color to have a specific color. It cause that Paperdoll AND anim get the color.
	else if ( pChar->IsStatFlag(STATF_INSUBSTANTIAL) && g_Cfg.m_iColorInvis)
		wHue = g_Cfg.m_iColorInvis;
	else if (pChar->IsStatFlag(STATF_HIDDEN) && g_Cfg.m_iColorHidden)
		wHue = g_Cfg.m_iColorHidden;
	else if (pChar->IsStatFlag(STATF_INVISIBLE) && g_Cfg.m_iColorInvisSpell)
		wHue = g_Cfg.m_iColorInvisSpell;

	else
	{
		if ( pItemDef && (uiResDisp < pItemDef->GetResLevel() ) )
			if ( pItemDef->GetResDispDnHue() != HUE_DEFAULT )
				wHue = pItemDef->GetResDispDnHue();

		if (( wHue & HUE_MASK_HI ) > HUE_QTY )
			wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		else
			wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;

	}

	if ( pItemDef && (uiResDisp < pItemDef->GetResLevel() ) )
		id = (ITEMID_TYPE)(pItemDef->GetResDispDnId());
}

void CClient::GetAdjustedCharID( const CChar * pChar, CREID_TYPE &id, HUE_TYPE &wHue ) const
{
	ADDTOCALLSTACK("CClient::GetAdjustedCharID");
	// Some clients can't see all creature artwork and colors.
	// try to do the translation here,.

	ASSERT( GetAccount() );
	ASSERT( pChar );

	if ( IsPriv(PRIV_DEBUG) )
	{
		id = CREID_MAN;
		wHue = 0;
		return;
	}

	id = pChar->GetDispID();
	CCharBase * pCharDef = pChar->Char_GetDef();

	if ( m_pChar->IsStatFlag(STATF_HALLUCINATING) )
	{
		if ( pChar != m_pChar )
		{
			// Get a random creature from the artwork.
			pCharDef = nullptr;
			while ( pCharDef == nullptr )
			{
				id = (CREID_TYPE)(Calc_GetRandVal(CREID_EQUIP_GM_ROBE));
				if ( id != CREID_SEA_CREATURE )		// skip this chardef, it can crash many clients
					pCharDef = CCharBase::FindCharBase(id);
			}
		}

		wHue = (HUE_TYPE)(Calc_GetRandVal(HUE_DYE_HIGH));
	}
	else
	{
		if ( pChar->IsStatFlag(STATF_STONE) )	//Client do not have stone state. So we must send the hue we want. (Affect the paperdoll hue as well)
			wHue = HUE_STONE;

		// Normaly, client automaticly change the anim color in grey when someone is insubtantial, hidden or invisible. Paperdoll are never affected by this.
		// The next 3 state overwrite the client process and force SKIN color to have a specific color. It cause that Paperdoll AND anim get the color.
		else if ( pChar->IsStatFlag(STATF_INSUBSTANTIAL) && g_Cfg.m_iColorInvis)
			wHue = g_Cfg.m_iColorInvis;
		else if ( pChar->IsStatFlag(STATF_HIDDEN) && g_Cfg.m_iColorHidden)
			wHue = g_Cfg.m_iColorHidden;
		else if ( pChar->IsStatFlag(STATF_INVISIBLE) && g_Cfg.m_iColorInvisSpell)
			wHue = g_Cfg.m_iColorInvisSpell;
		else
		{
			wHue = pChar->GetHue();
			// Allow transparency and underwear colors
			if ( (wHue & HUE_MASK_HI) > HUE_QTY )
				wHue &= HUE_MASK_LO | HUE_UNDERWEAR | HUE_TRANSLUCENT;
			else
				wHue &= HUE_MASK_HI | HUE_UNDERWEAR | HUE_TRANSLUCENT;
		}
	}

	if ( pCharDef && (GetResDisp() < pCharDef->GetResLevel()) )
	{
		id = (CREID_TYPE)(pCharDef->GetResDispDnId());
		if ( pCharDef->GetResDispDnHue() != HUE_DEFAULT )
			wHue = pCharDef->GetResDispDnHue();
	}
}

void CClient::addCharMove( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CClient::addCharMove");

	addCharMove(pChar, pChar->GetDirFlag());
}

void CClient::addCharMove( const CChar * pChar, byte iCharDirFlag ) const
{
	ADDTOCALLSTACK("CClient::addCharMove (DirFlag)");
	// This char has just moved on screen.
	// or changed in a subtle way like "hidden"
	// NOTE: If i have been turned this will NOT update myself.

	new PacketCharacterMove(this, pChar, iCharDirFlag);
}

void CClient::addChar( CChar * pChar, bool fFull )
{
	ADDTOCALLSTACK("CClient::addChar");
	// Full update about a char.
	EXC_TRY("addChar");

    if (fFull)
	    new PacketCharacter(this, pChar);
    else
        addCharMove(pChar);

    const bool fStatue = pChar->Can(CAN_C_STATUE);
    if (fStatue)
    {
        const int iAnim = (int)pChar->GetKeyNum("STATUE_ANIM", true);
        const int iFrame = (int)pChar->GetKeyNum("STATUE_FRAME", true);
        new PacketStatueAnimation(this, pChar, iAnim, iFrame);
    }

	EXC_SET_BLOCK("Wake sector");
	pChar->GetTopPoint().GetSector()->SetSectorWakeStatus();	// if it can be seen then wake it.

	EXC_SET_BLOCK("Health bar color");
	addHealthBarUpdate( pChar );

    if (fFull && !fStatue)
    {
        if ( pChar->m_pNPC && pChar->m_pNPC->m_bonded && pChar->IsStatFlag(STATF_DEAD) )
        {
            EXC_SET_BLOCK("Bonded status");
            addBondedStatus(pChar, true);
        }

        EXC_SET_BLOCK("AOSToolTip adding (end)");
        addAOSTooltip( pChar );
    }

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("m_dirFace (0%x)\n", pChar->m_dirFace);
	EXC_DEBUG_END;
}

void CClient::addItemName( CItem * pItem )
{
	ADDTOCALLSTACK("CClient::addItemName");
	// NOTE: CanSee() has already been called.
	if ( !pItem )
		return;

	bool fIdentified = ( IsPriv(PRIV_GM) || pItem->IsAttr( ATTR_IDENTIFIED ));
	lpctstr pszNameFull = pItem->GetNameFull( fIdentified );

	tchar szName[ MAX_ITEM_NAME_SIZE * 2 ];
	size_t len = Str_CopyLimitNull( szName, pszNameFull, CountOf(szName) );

	const CContainer* pCont = dynamic_cast<const CContainer*>(pItem);
	if ( pCont != nullptr )
	{
		// ??? Corpses show hair as an item !!
		len += snprintf( szName+len, sizeof(szName) - len,
			g_Cfg.GetDefaultMsg(DEFMSG_CONT_ITEMS), pCont->GetContentCount(), pCont->GetTotalWeight() / WEIGHT_UNITS);
	}

	// obviously damaged ?
	else if ( pItem->IsTypeArmorWeapon())
	{
		const int iPercent = pItem->Armor_GetRepairPercent();
		if ( iPercent < 50 &&
			( m_pChar->Skill_GetAdjusted( SKILL_ARMSLORE ) / 10 > iPercent ))
		{
			len += snprintf( szName+len, sizeof(szName) - len, " (%s)", pItem->Armor_GetRepairDesc());
		}
	}

	// Show the priced value
	CItemContainer * pMyCont = dynamic_cast <CItemContainer *>( pItem->GetParent());
	if ( pMyCont != nullptr && pMyCont->IsType(IT_EQ_VENDOR_BOX))
	{
		const CItemVendable * pVendItem = dynamic_cast <const CItemVendable *> (pItem);
		if ( pVendItem )
		{
			len += snprintf( szName+len, sizeof(szName) - len, " (%u gp)", pVendItem->GetBasePrice());
		}
	}

	HUE_TYPE wHue = HUE_TEXT_DEF;
	const CVarDefCont* sVal = pItem->GetKey("NAME.HUE", true);
	if (sVal)
		wHue = (HUE_TYPE)(sVal->GetValNum());

	const CItemCorpse * pCorpseItem = dynamic_cast <const CItemCorpse *>(pItem);
	if ( pCorpseItem )
	{
		CChar * pCharCorpse = pCorpseItem->m_uidLink.CharFind();
		if ( pCharCorpse )
		{
			wHue = pCharCorpse->Noto_GetHue( m_pChar, true );
		}
	}

	if ( IsPriv( PRIV_GM ))
	{
		if ( pItem->IsAttr(ATTR_INVIS ))
		{
			len += Str_CopyLen( szName+len, " (invis)" );
		}
		// Show the restock count
		if ( pMyCont != nullptr && pMyCont->IsType(IT_EQ_VENDOR_BOX) )
		{
			len += snprintf( szName+len, sizeof(szName) - len, " (%d restock)", pItem->GetContainedLayer());
		}
		switch ( pItem->GetType() )
		{
			case IT_SPAWN_CHAR:
			case IT_SPAWN_ITEM:
				{
					CCSpawn *pSpawn = pItem->GetSpawn();
					if ( pSpawn )
						len += pSpawn->WriteName(szName + len);
				}
				break;

			case IT_TREE:
			case IT_GRASS:
			case IT_ROCK:
			case IT_WATER:
				{
					CResourceDef *pResDef = g_Cfg.ResourceGetDef(pItem->m_itResource.m_ridRes);
					if ( pResDef )
						len += snprintf(szName + len, sizeof(szName) - len, " (%s)", pResDef->GetName());
				}
				break;

			default:
				break;
		}
	}
	if ( IsPriv(PRIV_DEBUG) )
		len += snprintf(szName+len, sizeof(szName) - len, " [0%x]", (dword) pItem->GetUID());

	if (( IsTrigUsed(TRIGGER_AFTERCLICK) ) || ( IsTrigUsed(TRIGGER_ITEMAFTERCLICK) ))
	{
		CScriptTriggerArgs Args( this );
		Args.m_VarsLocal.SetStrNew("ClickMsgText", &szName[0]);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", (int64)(wHue));

		TRIGRET_TYPE ret = pItem->OnTrigger( "@AfterClick", m_pChar, &Args );	// CTRIG_AfterClick, ITRIG_AfterClick

		if ( ret == TRIGRET_RET_TRUE )
			return;

		lpctstr pNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");

		if ( pNewStr != nullptr )
			Str_CopyLimitNull(szName, pNewStr, CountOf(szName));

		wHue = (HUE_TYPE)(Args.m_VarsLocal.GetKeyNum("ClickMsgHue"));
	}

	addObjMessage( szName, pItem, wHue, TALKMODE_ITEM );
}

void CClient::addCharName( const CChar * pChar ) // Singleclick text for a character
{
	ADDTOCALLSTACK("CClient::addCharName");
	// Karma wHue codes ?
	ASSERT( pChar );

	HUE_TYPE wHue = pChar->Noto_GetHue( m_pChar, true );

	tchar *pszTemp = Str_GetTemp();
	lpctstr prefix = pChar->GetKeyStr( "NAME.PREFIX" );
	if ( ! *prefix )
		prefix = pChar->Noto_GetFameTitle();

    Str_CopyLimitNull( pszTemp, prefix, STR_TEMPLENGTH );
    Str_ConcatLimitNull( pszTemp, pChar->GetName(), STR_TEMPLENGTH );
    Str_ConcatLimitNull( pszTemp, pChar->GetKeyStr( "NAME.SUFFIX" ), STR_TEMPLENGTH );

	if ( !pChar->IsStatFlag(STATF_INCOGNITO) || ( GetPrivLevel() > pChar->GetPrivLevel() ))
	{
		// Guild abbrev.
		lpctstr pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_TOWN);
		if ( pAbbrev )
		{
            Str_ConcatLimitNull( pszTemp, pAbbrev, STR_TEMPLENGTH );
		}
		pAbbrev = pChar->Guild_AbbrevBracket(MEMORY_GUILD);
		if ( pAbbrev )
		{
            Str_ConcatLimitNull( pszTemp, pAbbrev, STR_TEMPLENGTH );
		}
	}
	else
        Str_CopyLimitNull( pszTemp, pChar->GetName(), STR_TEMPLENGTH );

	if ( pChar->m_pNPC && g_Cfg.m_fVendorTradeTitle )
	{
		if ( pChar->GetNPCBrainGroup() == NPCBRAIN_HUMAN )
		{
			lpctstr title = pChar->GetTradeTitle();
			if ( *title )
			{
                Str_ConcatLimitNull( pszTemp, " ", STR_TEMPLENGTH );
                Str_ConcatLimitNull( pszTemp, title, STR_TEMPLENGTH );
			}
		}
	}

	bool fAllShow = IsPriv(PRIV_DEBUG|PRIV_ALLSHOW);

	if ( g_Cfg.m_fCharTags || fAllShow )
	{
		if ( pChar->m_pNPC )
		{
			if ( pChar->IsPlayableCharacter())
                Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_NPC), STR_TEMPLENGTH );
			if ( pChar->IsStatFlag( STATF_CONJURED ))
                Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SUMMONED), STR_TEMPLENGTH );
			else if ( pChar->IsStatFlag( STATF_PET ))
                Str_ConcatLimitNull( pszTemp, (pChar->m_pNPC->m_bonded) ? g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_BONDED) : g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_TAME), STR_TEMPLENGTH );
		}
		if ( pChar->IsStatFlag( STATF_INVUL ) && ! pChar->IsStatFlag( STATF_INCOGNITO ) && ! pChar->IsPriv( PRIV_PRIV_NOSHOW ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_INVUL), STR_TEMPLENGTH );
		if ( pChar->IsStatFlag( STATF_STONE ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_STONE), STR_TEMPLENGTH );
		if ( pChar->IsStatFlag( STATF_FREEZE ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_FROZEN), STR_TEMPLENGTH );
		if ( pChar->IsStatFlag( STATF_INSUBSTANTIAL | STATF_INVISIBLE | STATF_HIDDEN ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HIDDEN), STR_TEMPLENGTH );
		if ( pChar->IsStatFlag( STATF_SLEEPING ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SLEEPING), STR_TEMPLENGTH );
		if ( pChar->IsStatFlag( STATF_HALLUCINATING ))
            Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_HALLU), STR_TEMPLENGTH );

		if ( fAllShow )
		{
			if ( pChar->IsStatFlag(STATF_SPAWNED) )
				Str_ConcatLimitNull(pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_SPAWN), STR_TEMPLENGTH);

			if (IsPriv(PRIV_DEBUG))
			{
				const size_t uiSLen = strlen(pszTemp);
				snprintf(pszTemp + uiSLen, STR_TEMPLENGTH - uiSLen, " [0%x]", (dword)pChar->GetUID());
			}
		}
	}
	if ( ! fAllShow && pChar->Skill_GetActive() == NPCACT_NAPPING )
        Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_AFK), STR_TEMPLENGTH );
	if ( pChar->GetPrivLevel() <= PLEVEL_Guest )
        Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_GUEST), STR_TEMPLENGTH );
	if ( pChar->IsPriv( PRIV_JAILED ))
        Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_JAILED), STR_TEMPLENGTH );
	if ( pChar->IsDisconnected())
        Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_LOGOUT), STR_TEMPLENGTH );
	if (( fAllShow || pChar == m_pChar ) && pChar->IsStatFlag( STATF_CRIMINAL ))
        Str_ConcatLimitNull( pszTemp, g_Cfg.GetDefaultMsg(DEFMSG_CHARINFO_CRIMINAL), STR_TEMPLENGTH );
	if ( fAllShow || ( IsPriv(PRIV_GM) && ( g_Cfg.m_iDebugFlags & DEBUGF_NPC_EMOTE )))
	{
        Str_ConcatLimitNull( pszTemp, " [", STR_TEMPLENGTH );
        Str_ConcatLimitNull( pszTemp, pChar->Skill_GetName(), STR_TEMPLENGTH);
        Str_ConcatLimitNull( pszTemp, "]", STR_TEMPLENGTH );
	}

	if ( IsTrigUsed(TRIGGER_AFTERCLICK) )
	{
		CScriptTriggerArgs Args( this );
		Args.m_VarsLocal.SetStrNew("ClickMsgText", pszTemp);
		Args.m_VarsLocal.SetNumNew("ClickMsgHue", (int64)(wHue));

		TRIGRET_TYPE ret = const_cast<CChar*>(pChar)->OnTrigger( "@AfterClick", m_pChar, &Args );	// CTRIG_AfterClick, ITRIG_AfterClick

		if ( ret == TRIGRET_RET_TRUE )
			return;

		lpctstr pNewStr = Args.m_VarsLocal.GetKeyStr("ClickMsgText");

		if ( pNewStr != nullptr )
			Str_CopyLimitNull(pszTemp, pNewStr, STR_TEMPLENGTH);

		wHue = (HUE_TYPE)(Args.m_VarsLocal.GetKeyNum("ClickMsgHue"));
	}

	addObjMessage( pszTemp, pChar, wHue, TALKMODE_ITEM );
}

void CClient::addPlayerStart( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addPlayerStart");

	if ( m_pChar != pChar )	// death option just uses this as a reload.
	{
		// This must be a CONTROL command ?
		CharDisconnect();
		if ( pChar->IsClientActive())	// not sure why this would happen but take care of it anyhow.
			pChar->GetClientActive()->CharDisconnect();
		m_pChar = pChar;
		m_pChar->ClientAttach( this );
	}
	ASSERT(m_pChar->m_pPlayer);
	ASSERT(m_pAccount);

	CItem * pItemChange = m_pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( pItemChange != nullptr )
		pItemChange->Delete();

	if ( g_Cfg.m_bAutoResDisp )
		m_pAccount->SetAutoResDisp(this);

	ClearTargMode();	// clear death menu mode. etc. ready to walk about. cancel any previous modes

    const CPointMap pt = m_pChar->GetTopPoint();
	m_Env.SetInvalid();

/*
	CExtData ExtData;
	ExtData.Party_Enable.m_state = 1;
	addExtData( EXTDATA_Party_Enable, &ExtData, sizeof(ExtData.Party_Enable));
*/

	new PacketPlayerStart(this);
	addMapDiff();
	m_pChar->MoveToChar(pt, true, false, false, false); // make sure we are in active list
	m_pChar->Update();
	addPlayerWarMode();
	addLoginComplete();
	addTime(true);
	if ( CSector *pSector = pt.GetSector() )
		addSeason(pSector->GetSeason());
	if (pChar->m_pParty)
		pChar->m_pParty->SendAddList(nullptr);

	addKRToolbar(pChar->m_pPlayer->getKrToolbarStatus());
	resendBuffs();
}

void CClient::addPlayerWarMode() const
{
	ADDTOCALLSTACK("CClient::addPlayerWarMode");

	new PacketWarMode(this, m_pChar);
}

void CClient::addToolTip( const CObjBase * pObj, lpctstr pszText ) const
{
	ADDTOCALLSTACK("CClient::addToolTip");
	if ( pObj == nullptr )
		return;
	if ( pObj->IsChar())
		return; // no tips on chars.

	new PacketTooltip(this, pObj, pszText);
}

bool CClient::addBookOpen( CItem * pBook ) const
{
	ADDTOCALLSTACK("CClient::addBookOpen");
	// word wrap is done when typed in the client. (it has a var size font)
	if (pBook == nullptr)
		return false;

	word wPagesNow = 0;
	bool bNewPacket = PacketDisplayBookNew::CanSendTo(GetNetState());

	if (pBook->IsBookSystem() == false)
	{
		// User written book.
		CItemMessage *pMsgItem = static_cast<CItemMessage *>(pBook);

		if (pMsgItem->IsBookWritable())
            wPagesNow = pMsgItem->GetPageCount(); // for some reason we must send them now
	}

	if (bNewPacket)
		new PacketDisplayBookNew(this, pBook);
	else
		new PacketDisplayBook(this, pBook);

	// We could just send all the pages now if we want.
	if (wPagesNow > 0)
		addBookPage(pBook, 1, wPagesNow);

	return true;
}

void CClient::addBookPage( const CItem * pBook, word wPage, word wCount ) const
{
	ADDTOCALLSTACK("CClient::addBookPage");
	// ARGS:
	//  wPage = 1 based page.
	if ( wPage <= 0 )
		return;
	if ( wCount < 1 )
		wCount = 1;

	new PacketBookPageContent(this, pBook, wPage, wCount );
}

uint CClient::Setup_FillCharList(Packet* pPacket, const CChar * pCharFirst)
{
	ADDTOCALLSTACK("CClient::Setup_FillCharList");
	// list available chars for your account that are idle.
	ASSERT(pPacket);
	const CAccount * pAccount = GetAccount();
	ASSERT( pAccount );

	size_t count = 0;

	if ( pCharFirst && pAccount->IsMyAccountChar( pCharFirst ))
	{
		m_tmSetupCharList[0] = pCharFirst->GetUID();

		pPacket->writeStringFixedASCII(pCharFirst->GetName(), MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);

		++count;
	}

	const size_t iAcctCharCount = pAccount->m_Chars.GetCharCount(), iAcctMaxChars = pAccount->GetMaxChars();
	const uint iMax = (uint)minimum(maximum(iAcctCharCount,iAcctMaxChars), MAX_CHARS_PER_ACCT);

	uint iQty = (uint)iAcctCharCount;
	if (iQty > iMax)
		iQty = iMax;

	for (uint i = 0; i < iQty; ++i)
	{
		const CUID uid(pAccount->m_Chars.GetChar(i));
		const CChar* pChar = uid.CharFind();
		if ( pChar == nullptr )
			continue;
		if ( pCharFirst == pChar )
			continue;
		if ( count >= iMax )
			break;

		m_tmSetupCharList[count] = uid;

		pPacket->writeStringFixedASCII(pChar->GetName(), MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);

		++count;
	}

	// always show max count for some stupid reason. (client bug)
	// pad out the rest of the chars.
	uint iClientMin = 5;
	if (GetNetState()->isClientVersion(MINCLIVER_PADCHARLIST) || !GetNetState()->getCryptVersion())
		iClientMin = maximum(iQty, 5);

	for ( ; count < iClientMin; ++count)
	{
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
		pPacket->writeStringFixedASCII("", MAX_NAME_SIZE);
	}

	return (uint)count;
}

void CClient::SetTargMode( CLIMODE_TYPE targmode, lpctstr pPrompt, int64 iTimeout )
{
	ADDTOCALLSTACK("CClient::SetTargMode");
	// ??? Get rid of menu stuff if previous targ mode.
	// Can i close a menu ?
	// Cancel a cursor input.

	bool fSuppressCancelMessage = false;
	CChar *pCharThis = GetChar();
	ASSERT(pCharThis);

	const CLIMODE_TYPE curTargMode = GetTargMode();
	switch (curTargMode)
	{
		case CLIMODE_TARG_OBJ_FUNC:
		{
			if ( IsTrigUsed(TRIGGER_TARGON_CANCEL) )
			{
				CScriptTriggerArgs Args;
				Args.m_s1 =  m_Targ_Text;
				if (pCharThis->OnTrigger( CTRIG_Targon_Cancel, pCharThis, &Args ) == TRIGRET_RET_TRUE )
					fSuppressCancelMessage = true;
			}
		} break;
		case CLIMODE_TARG_USE_ITEM:
		{
			CItem * pItemUse = m_Targ_UID.ItemFind();
			if (pItemUse && (IsTrigUsed(TRIGGER_TARGON_CANCEL) || IsTrigUsed(TRIGGER_ITEMTARGON_CANCEL)))
			{
				if ( pItemUse->OnTrigger( ITRIG_TARGON_CANCEL, pCharThis) == TRIGRET_RET_TRUE )
					fSuppressCancelMessage = true;
			}
		} break;

		case CLIMODE_TARG_SKILL_MAGERY:
		{
			const CSpellDef* pSpellDef = g_Cfg.GetSpellDef(m_tmSkillMagery.m_iSpell);
			if (pSpellDef)
			{
				CScriptTriggerArgs Args(m_tmSkillMagery.m_iSpell, 0, m_Targ_Prv_UID.ObjFind());

				if ( IsTrigUsed(TRIGGER_SPELLTARGETCANCEL) )
				{
					if (pCharThis->OnTrigger( CTRIG_SpellTargetCancel, this, &Args ) == TRIGRET_RET_TRUE )
					{
						fSuppressCancelMessage = true;
						break;
					}
				}

				if ( IsTrigUsed(TRIGGER_TARGETCANCEL) )
				{
					if (pCharThis->Spell_OnTrigger( m_tmSkillMagery.m_iSpell, SPTRIG_TARGETCANCEL, pCharThis, &Args ) == TRIGRET_RET_TRUE )
						fSuppressCancelMessage = true;
				}
			}
		} break;

		case CLIMODE_TARG_SKILL:
		case CLIMODE_TARG_SKILL_HERD_DEST:
		case CLIMODE_TARG_SKILL_PROVOKE:
		case CLIMODE_TARG_SKILL_POISON:
		{
			SKILL_TYPE action = SKILL_NONE;
			switch (curTargMode)
			{
				case CLIMODE_TARG_SKILL:
					action = m_tmSkillTarg.m_iSkill;
					break;
				case CLIMODE_TARG_SKILL_HERD_DEST:
					action = SKILL_HERDING;
					break;
				case CLIMODE_TARG_SKILL_PROVOKE:
					action = SKILL_PROVOCATION;
					break;
				case CLIMODE_TARG_SKILL_POISON:
					action = SKILL_POISONING;
					break;
				default:
					break;
			}

			if (action != SKILL_NONE)
			{
				if ( IsTrigUsed(TRIGGER_SKILLTARGETCANCEL) )
				{
					if (pCharThis->Skill_OnCharTrigger(action, CTRIG_SkillTargetCancel) == TRIGRET_RET_TRUE )
					{
						fSuppressCancelMessage = true;
						break;
					}
				}
				if ( IsTrigUsed(TRIGGER_TARGETCANCEL) )
				{
					if (pCharThis->Skill_OnTrigger(action, SKTRIG_TARGETCANCEL) == TRIGRET_RET_TRUE )
						fSuppressCancelMessage = true;
				}
			}
		} break;

		default:
			break;
	}

	// determine timeout time
    if (iTimeout > 0)
        m_Targ_Timeout = CWorldGameTime::GetCurrentTime().GetTimeRaw() + iTimeout;
    else
        m_Targ_Timeout = 0;

	if (curTargMode == targmode)
		return;

	if ((curTargMode != CLIMODE_NORMAL) && (targmode != CLIMODE_NORMAL))
	{
		//If there's any item in LAYER_DRAGGING we remove it from view and then bounce it
		CItem * pItem = pCharThis->LayerFind( LAYER_DRAGGING );
		if (pItem != nullptr)
		{
			pItem->RemoveFromView();		//Removing from view to avoid seeing it in the cursor
			pCharThis->ItemBounce(pItem);
			// Just clear the old target mode
			if (fSuppressCancelMessage == false)
				addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_2));
		}
	}

	m_Targ_Mode = targmode;
	if ( targmode == CLIMODE_NORMAL && fSuppressCancelMessage == false )
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_TARGET_CANCEL_1) );
	else if ( pPrompt && *pPrompt ) // Check that the message is not blank.
		addSysMessage( pPrompt );
}

void CClient::addPromptConsole( CLIMODE_TYPE mode, lpctstr pPrompt, CUID context1, CUID context2, bool bUnicode )
{
	ADDTOCALLSTACK("CClient::addPromptConsole");

	m_Prompt_Uid = context1;
	m_Prompt_Mode = mode;

	// Check that the message is not blank.
	if ( pPrompt && *pPrompt )
		addSysMessage( pPrompt );

	new PacketAddPrompt(this, context1, context2, bUnicode);
}

void CClient::addTarget( CLIMODE_TYPE targmode, lpctstr pPrompt, bool fAllowGround, bool fCheckCrime, int64 iTimeout) // Send targetting cursor to client
{
	ADDTOCALLSTACK("CClient::addTarget");
	// Send targetting cursor to client.
    // Expect XCMD_Target back.
	// ??? will this be selective for us ? objects only or chars only ? not on the ground (statics) ?

	SetTargMode( targmode, pPrompt, iTimeout);

	new PacketAddTarget(this,
						fAllowGround? PacketAddTarget::Ground : PacketAddTarget::Object,
						targmode,
						fCheckCrime? PacketAddTarget::Harmful : PacketAddTarget::None);
}

void CClient::addTargetDeed( const CItem * pDeed )
{
	ADDTOCALLSTACK("CClient::addTargetDeed");
	// Place an item from a deed. preview all the stuff

	ASSERT( m_Targ_UID == pDeed->GetUID());
	ITEMID_TYPE iddef = (ITEMID_TYPE)(RES_GET_INDEX(pDeed->m_itDeed.m_Type));
	m_tmUseItem.m_pParent = pDeed->GetParent();	// Cheat Verify.
	addTargetItems( CLIMODE_TARG_USE_ITEM, iddef, pDeed->GetHue() );
}

bool CClient::addTargetChars( CLIMODE_TYPE mode, CREID_TYPE baseID, bool fNotoCheck, int64 iTimeout)
{
	ADDTOCALLSTACK("CClient::addTargetChars");
	CCharBase * pBase = CCharBase::FindCharBase( baseID );
	if ( !pBase )
		return false;

	tchar * pszTemp = Str_GetTemp();
	snprintf(pszTemp, STR_TEMPLENGTH, "%s '%s'?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_SUMMON), pBase->GetTradeName());

	addTarget(mode, pszTemp, true, fNotoCheck, iTimeout);
	return true;
}

bool CClient::addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, HUE_TYPE color, bool fAllowGround )
{
	ADDTOCALLSTACK("CClient::addTargetItems");
	// Add a list of items to place at target.
	// preview all the stuff

	ASSERT(m_pChar);

	lpctstr pszName;
	CItemBase * pItemDef;
	if ( id < ITEMID_TEMPLATE )
	{
		pItemDef = CItemBase::FindItemBase( id );
		if ( pItemDef == nullptr )
			return false;
		pszName = pItemDef->GetName();

		if ( pItemDef->IsType(IT_STONE_GUILD) )
		{
			// Check if they are already in a guild first
			CItemStone * pStone = m_pChar->Guild_Find(MEMORY_GUILD);
			if (pStone)
			{
				addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_GUILD_ALREADY_MEMBER) );
				return false;
			}
		}
	}
	else
	{
		pItemDef = nullptr;
		pszName = "template";
	}

	tchar *pszTemp = Str_GetTemp();
	snprintf(pszTemp, STR_TEMPLENGTH, "%s %s?", g_Cfg.GetDefaultMsg(DEFMSG_WHERE_TO_PLACE), pszName);

	if ( CItemBase::IsID_Multi( id ) )	// a multi we get from Multi.mul
	{
		SetTargMode(targmode, pszTemp);
		new PacketAddTarget(this, fAllowGround? PacketAddTarget::Ground : PacketAddTarget::Object, targmode, PacketAddTarget::None, id, color);
		return true;
	}

	// preview not supported by this ver?
	addTarget(targmode, pszTemp, true);
	return true;
}

void CClient::addTargetCancel()
{
	ADDTOCALLSTACK("CClient::addTargetCancel");

	// handle the cancellation now, in an ideal world the client would normally respond to
	// the cancel target packet as if the user had pressed ESC, but we shouldn't rely on
	// this happening (older clients for example don't support the cancel command and will
	// bring up a new target cursor)
	SetTargMode();

	// tell the client to cancel their cursor
	new PacketAddTarget(this, PacketAddTarget::Object, 0, PacketAddTarget::Cancel);
}

void CClient::addCodexOfWisdom(dword dwTopicID, bool fForceOpen)
{
    ADDTOCALLSTACK("CClient::addCodexOfWisdom");
    // Open Codex of Wisdom

    new PacketCodexOfWisdom(this, dwTopicID, fForceOpen);
}

void CClient::addDyeOption( const CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addDyeOption");
	// Put up the wHue chart for the client.
	// This results in a Event_Item_Dye message. CLIMODE_DYE

	new PacketShowDyeWindow(this, pObj);

	SetTargMode( CLIMODE_DYE );
}

void CClient::addSkillWindow(SKILL_TYPE skill, bool fFromInfo) const // Opens the skills list
{
	ADDTOCALLSTACK("CClient::addSkillWindow");
	// Whos skills do we want to look at ?
	CChar* pChar = m_Prop_UID.CharFind();
	if (pChar == nullptr)
		pChar = m_pChar;

	bool fAllSkills = (skill >= (SKILL_TYPE)(g_Cfg.m_iMaxSkill));
	if (fAllSkills == false && g_Cfg.m_SkillIndexDefs.IsValidIndex(skill) == false)
		return;

	if ( IsTrigUsed(TRIGGER_USERSKILLS) )
	{
		CScriptTriggerArgs Args(fAllSkills? -1 : skill, fFromInfo);
		if (m_pChar->OnTrigger(CTRIG_UserSkills, pChar, &Args) == TRIGRET_RET_TRUE)
			return;
	}

	if (fAllSkills == false && skill >= SKILL_QTY)
		return;

	new PacketSkills(this, pChar, skill);
}

void CClient::addPlayerSee( const CPointMap & ptOld )
{
	ADDTOCALLSTACK("CClient::addPlayerSee");
	// Adjust to my new location, what do I now see here?

    const bool fOSIMultiSight = IsSetOF(OF_OSIMultiSight);
    const CChar *pCharThis = GetChar();
	const int iViewDist = pCharThis->GetVisualRange();
    const CPointMap& ptCharThis = pCharThis->GetTopPoint();
	const CRegion *pCurrentCharRegion = ptCharThis.GetRegion(REGION_TYPE_HOUSE);

	// Nearby items on ground
	uint iSeeCurrent = 0;
	uint iSeeMax = g_Cfg.m_iMaxItemComplexity * 30;

    std::vector<CItem*> vecMultis;
    std::vector<CItem*> vecItems;

    // ptOld: the point from where i moved (i can call this method when i'm moving to a new position),
    //  If ptOld is an invalid point, just send every object i can see.
	CWorldSearch AreaItems(ptCharThis, UO_MAP_VIEW_RADAR * 2);    // *2 to catch big multis
	AreaItems.SetSearchSquare(true);
	for (;;)
	{
        CItem* pItem = AreaItems.GetItem();
		if ( !pItem )
			break;

        const CPointMap& ptItemTop = pItem->GetTopLevelObj()->GetTopPoint();
        if ( pItem->IsTypeMulti() )		// incoming multi on radar view
		{
            const DIR_TYPE dirFace = pItem->GetDir(pCharThis);
            const CItemMulti* pMulti = static_cast<const CItemMulti*>(pItem);

            // This looks like the only way to make this thing work. Even if i send the worldobj packet with the commented code, the client
            //  will ignore it (and SpyUO 2 doesn't show that packet ?! it only shows packets that actually result in the generation of a world item, how weird)
            const int iOldDist = ptOld.GetDistSight(ptItemTop);  // handles also the case of an invalid point
            const int iCornerDistFromCenter = pMulti->GetSideDistanceFromCenter(dirFace);
            if ((iOldDist + iCornerDistFromCenter) > iViewDist)
            {
                //const int iCurDist = ptCharThis.GetDistSight(ptItemTop);
                //if (iCurDist + iCornerDistFromCenter <= iViewDist)
                    vecMultis.emplace_back(pItem);
            }

            /*
            const CPointMap ptMultiCorner = pMulti->GetRegion()->GetRegionCorner(dirFace);
            if ( ptOld.GetDistSight(ptMultiCorner) > UO_MAP_VIEW_RADAR )
            {
                if (ptCharThis.GetDistSight(ptMultiCorner) <= UO_MAP_VIEW_RADAR )
                {
                    // this just came into view
                    vecMultis.emplace_back(pItem);
                }
            }
            */

            continue;
		}

		if ( (iSeeCurrent > iSeeMax) || !pCharThis->CanSee(pItem) )
            continue;

        const int iOldDist = ptOld.GetDistSight(ptItemTop);  // handles also the case of an invalid point
		if ( fOSIMultiSight )
		{
            bool bSee = false;
            if (((ptOld.GetRegion(REGION_TYPE_HOUSE) != pCurrentCharRegion) || (ptOld.GetDistSight(ptItemTop) > iViewDist)) &&
                (ptItemTop.GetRegion(REGION_TYPE_HOUSE) == pCurrentCharRegion))
            {
                bSee = true;    // item is in same house as me
            }
            else if ((ptOld.GetDistSight(ptItemTop) > iViewDist) && (ptCharThis.GetDistSight(ptItemTop) <= iViewDist))	// item just came into view
            {
                if (!ptItemTop.GetRegion(REGION_TYPE_HOUSE)		// item is not in a house (ships are ok)
                    || (pItem->m_uidLink.IsValidUID() && pItem->m_uidLink.IsItem() && pItem->m_uidLink.ItemFind()->IsTypeMulti())	// item is linked to a multi
                    || pItem->IsTypeMulti()		        // item is an multi
                    || pItem->GetKeyNum("ALWAYSSEND"))  // item has ALWAYSSEND tag set
                {
                    bSee = true;
                }
            }
            if (bSee)
			{
				++iSeeCurrent;
				vecItems.emplace_back(pItem);
			}
		}
		else
		{
			if ( (iOldDist > iViewDist) && (ptCharThis.GetDistSight(ptItemTop) <= iViewDist) )		// item just came into view
			{
				++iSeeCurrent;
                vecItems.emplace_back(pItem);
			}
		}
	}

    // First send the multis
    for (CItem *pItem : vecMultis)
        addItem_OnGround(pItem);

    // Then the items
    for (CItem * pItem : vecItems)
        addItem_OnGround(pItem);

	// Nearby chars
	iSeeCurrent = 0;
	iSeeMax = g_Cfg.m_iMaxCharComplexity * 5;

	CWorldSearch AreaChars(pCharThis->GetTopPoint(), iViewDist);
	AreaChars.SetAllShow(IsPriv(PRIV_ALLSHOW));
	AreaChars.SetSearchSquare(true);
	for (;;)
	{
        CChar* pChar = AreaChars.GetChar();
		if ( !pChar || iSeeCurrent > iSeeMax )
			break;
		if ( pCharThis == pChar || !CanSee(pChar) )
			continue;

		if ( ptOld.GetDistSight(pChar->GetTopPoint()) > iViewDist )
		{
			++iSeeCurrent;
			addChar(pChar);
		}
	}
}

void CClient::addPlayerView( const CPointMap & pt, bool bFull )
{
	ADDTOCALLSTACK("CClient::addPlayerView");
	// I moved = Change my point of view. Teleport etc..

	addPlayerUpdate();

	if ( pt == m_pChar->GetTopPoint() )
		return;		// not a real move i guess. might just have been a change in face dir.

	m_Env.SetInvalid();		// must resend environ stuff

	if ( bFull )
		addPlayerSee(pt);
}

void CClient::addReSync()
{
	ADDTOCALLSTACK("CClient::addReSync");
    CChar* pChar = GetChar();
	if ( pChar == nullptr )
		return;
	// Reloads the client with all it needs.
	addMap();
	addChar(pChar);
	addPlayerView(CPointMap());
	addLight();		// Current light level where I am.
	addWeather();	// if any ...
	addSpeedMode(pChar->m_pPlayer->m_speedMode);
	addStatusWindow(pChar);
}

void CClient::addMap() const
{
	ADDTOCALLSTACK("CClient::addMap");
	if ( m_pChar == nullptr )
		return;

	CPointMap pt = m_pChar->GetTopPoint();
	new PacketMapChange(this, g_MapList.GetMapID(pt.m_map));
}

void CClient::addMapDiff() const
{
	ADDTOCALLSTACK("CClient::addMapDiff");
	// Enables map diff usage on the client. If the client is told to
	// enable diffs, and then logs back in without restarting, it will
	// continue to use the diffs even if not told to enable them - so
	// this packet should always be sent even if empty.

	new PacketEnableMapDiffs(this);
}

void CClient::addMapWaypoint(CObjBase *pObj, MAPWAYPOINT_TYPE type) const
{
    ADDTOCALLSTACK("CClient::addMapWaypoint");
    // Add/remove map waypoints on newer classic or every enhanced clients

    if (type)
    {
		// Classic clients only support MAPWAYPOINT_Remove and MAPWAYPOINT_Healer
		if ((type != MAPWAYPOINT_Healer) && !GetNetState()->isClientKR() && !GetNetState()->isClientEnhanced())
			return;

        if (PacketWaypointAdd::CanSendTo(GetNetState()))
            new PacketWaypointAdd(this, pObj, type);
    }
    else
    {
        if (PacketWaypointRemove::CanSendTo(GetNetState()))
            new PacketWaypointRemove(this, pObj);
    }
}

void CClient::addChangeServer() const
{
	ADDTOCALLSTACK("CClient::addChangeServer");
	CPointMap pt = m_pChar->GetTopPoint();

	new PacketZoneChange(this, pt);
}

void CClient::addPlayerUpdate() const
{
	ADDTOCALLSTACK("CClient::addPlayerUpdate");
	// Update player character on screen (id / hue / notoriety / position / dir).
	// NOTE: This will reset client-side walk sequence to 0, so reset it on server
	// side too, to prevent client request an unnecessary 'resync' (packet 0x22)
	// to server because client seq != server seq.

	new PacketPlayerUpdate(this);
	GetNetState()->m_sequence = 0;
}

void CClient::UpdateStats()
{
	ADDTOCALLSTACK("CClient::UpdateStats");
	if ( !m_fUpdateStats || !m_pChar )
		return;

	if ( m_fUpdateStats & SF_UPDATE_STATUS )
	{
		addStatusWindow( m_pChar);
		m_fUpdateStats = 0;
	}
	else
	{
		if ( m_fUpdateStats & SF_UPDATE_HITS )
		{
			addHitsUpdate( m_pChar);
			m_fUpdateStats &= ~SF_UPDATE_HITS;
		}
		if ( m_fUpdateStats & SF_UPDATE_MANA )
		{
			addManaUpdate( m_pChar );
			m_fUpdateStats &= ~SF_UPDATE_MANA;
		}

		if ( m_fUpdateStats & SF_UPDATE_STAM )
		{
			addStamUpdate( m_pChar );
			m_fUpdateStats &= ~SF_UPDATE_STAM;
		}
	}
}

void CClient::addStatusWindow( CObjBase *pObj, bool fRequested ) // Opens the status window
{
	ADDTOCALLSTACK("CClient::addStatusWindow");
	if ( !pObj )
		return;

	if ( IsTrigUsed(TRIGGER_USERSTATS) )
	{
		CScriptTriggerArgs Args(0, 0, pObj);
		Args.m_iN3 = fRequested;
		if ( m_pChar->OnTrigger(CTRIG_UserStats, dynamic_cast<CTextConsole *>(pObj), &Args) == TRIGRET_RET_TRUE )
			return;
	}

	new PacketObjectStatus(this, pObj);
	if ( pObj == m_pChar )
	{
		m_fUpdateStats = 0;
		if ( PacketStatLocks::CanSendTo(GetNetState()) )
			new PacketStatLocks(this, m_pChar);
	}
}

void CClient::addHitsUpdate( CChar *pChar )
{
	ADDTOCALLSTACK("CClient::addHitsUpdate");
	if ( !pChar )
		return;

	PacketHealthUpdate cmd(pChar, pChar == m_pChar);
	cmd.send(this);
}

void CClient::addManaUpdate( CChar *pChar )
{
	ADDTOCALLSTACK("CClient::addManaUpdate");
	if ( !pChar )
		return;

	PacketManaUpdate cmd(pChar, true);
	cmd.send(this);

	if ( pChar->m_pParty )
	{
		PacketManaUpdate cmd2(pChar, false);
		pChar->m_pParty->AddStatsUpdate(pChar, &cmd2);
	}
}

void CClient::addStamUpdate( CChar *pChar )
{
	ADDTOCALLSTACK("CClient::addStamUpdate");
	if ( !pChar )
		return;

	PacketStaminaUpdate cmd(pChar, true);
	cmd.send(this);

	if ( pChar->m_pParty )
	{
		PacketStaminaUpdate cmd2(pChar, false);
		pChar->m_pParty->AddStatsUpdate(pChar, &cmd2);
	}
}

void CClient::addHealthBarUpdate( const CChar * pChar ) const
{
	ADDTOCALLSTACK("CClient::addHealthBarUpdate");
	if ( pChar == nullptr )
		return;

    const CNetState* pNetState = GetNetState();
    if ( PacketHealthBarUpdateNew::CanSendTo(pNetState) )
        new PacketHealthBarUpdateNew(this, pChar);
    else if ( PacketHealthBarUpdate::CanSendTo(pNetState) )
        new PacketHealthBarUpdate(this, pChar);
}

void CClient::addBondedStatus( const CChar * pChar, bool bIsDead ) const
{
	ADDTOCALLSTACK("CClient::addBondedStatus");
	if ( pChar == nullptr )
		return;

	new PacketBondedStatus(this, pChar, bIsDead);
}

void CClient::addSpellbookOpen( CItem * pBook )
{
	ADDTOCALLSTACK("CClient::addSpellbookOpen");
    // NOTE: New spellbook types needs tooltip feature enabled to display gump content.
    //		 Enhanced clients need tooltip on all spellbook types otherwise it will crash.
    //		 Clients can also crash if open spellbook gump when spellbook item is not loaded yet.

	if ( !m_pChar )
		return;

	if ( IsTrigUsed(TRIGGER_SPELLBOOK) )
	{
		CScriptTriggerArgs	Args( 0, 0, pBook );
		if ( m_pChar->OnTrigger( CTRIG_SpellBook, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return;
	}

	if ( pBook->GetDispID() == ITEMID_SPELLBOOK2 )
	{
		// weird client bug.
		pBook->SetDispID( ITEMID_SPELLBOOK );
		pBook->Update();
		return;
	}

    // count what spells I have.
	int count = pBook->GetSpellcountInBook();
	if ( count == -1 )
		return;
	addItem(pBook); 	// NOTE: if the spellbook item is not present on the client it will crash.
	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump( pBook, GUMP_OPEN_SPELLBOOK );

	// New AOS spellbook packet required by client 4.0.0 and above.
	// Old packet is still required if both FEATURE_AOS_TOOLTIP and FEATURE_AOS_UPDATE aren't sent.
	if ( PacketSpellbookContent::CanSendTo(GetNetState()) && IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) )
	{
		// Handle new AOS spellbook stuff (old packets no longer work)
		new PacketSpellbookContent( this, pBook, (word)(pBook->Item_GetDef()->m_ttSpellbook.m_iOffset + 1) );
		return;
	}

	if (count <= 0)
		return;

	new PacketItemContents(this, pBook);
}


void CClient::addCustomSpellbookOpen( CItem * pBook, dword gumpID )
{
	ADDTOCALLSTACK("CClient::addCustomSpellbookOpen");
	const CItemContainer *pContainer = static_cast<CItemContainer *>(pBook);
	if ( !pContainer )
		return;

	int count = 0;
	for (const CSObjContRec* pObjRec : *pContainer)
	{
		auto pItem = static_cast<const CItem*>(pObjRec);
		if ( !pItem->IsType( IT_SCROLL ) )
			continue;
		++ count;
	}

	OpenPacketTransaction transaction(this, PacketSend::PRI_NORMAL);
	addOpenGump( pBook, GUMP_TYPE(gumpID));
	if (count <= 0)
		return;

	new PacketItemContents(this, pContainer);
}

void CClient::addScrollScript( CResourceLock &s, SCROLL_TYPE type, dword context, lpctstr pszHeader )
{
	ADDTOCALLSTACK("CClient::addScrollScript");

	new PacketOpenScroll(this, s, type, context, pszHeader);
}

void CClient::addScrollResource( lpctstr pszSec, SCROLL_TYPE type, dword scrollID )
{
	ADDTOCALLSTACK("CClient::addScrollResource");
	//
	// type = 0 = TIPS
	// type = 2 = UPDATES

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, RES_SCROLL, pszSec ))
		return;
	addScrollScript( s, type, scrollID );
}

void CClient::addVendorClose( const CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addVendorClose");
	// Clear the vendor display.

	new PacketCloseVendor(this, pVendor);
}

bool CClient::addShopMenuBuy( CChar * pVendor )
{
	ADDTOCALLSTACK("CClient::addShopMenuBuy");
	// Try to buy stuff that the vendor has.
	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	OpenPacketTransaction transaction(this, PacketSend::PRI_HIGH);

	// Non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag(STATF_PET) )
		pVendor->NPC_Vendor_Restock();

	CItemContainer *pContainer = pVendor->GetBank(LAYER_VENDOR_STOCK);
	CItemContainer *pContainerExtra = pVendor->GetBank(LAYER_VENDOR_EXTRA);
	if (!pContainer || !pContainerExtra)
	{
		DEBUG_MSG(("Vendor with no LAYER_VENDOR_STOCK or LAYER_VENDOR_EXTRA!\n"));
		return false;
	}

	// Send NPC layers 26 and 27 content
	new PacketItemEquipped(this, pContainer);
	new PacketItemEquipped(this, pContainerExtra);

	// Send item list and then price list first for layer 26, then for 27.
	new PacketItemContents(this, pContainer, true, false);
	PacketVendorBuyList *buyList = new PacketVendorBuyList();
	size_t buyListCount = buyList->fillBuyData(pContainer, pVendor->NPC_GetVendorMarkup());
	buyList->push(this);

	new PacketItemContents(this, pContainerExtra, true, false);
	PacketVendorBuyList *buyListExtra = new PacketVendorBuyList();
	size_t buyListExtraCount = buyListExtra->fillBuyData(pContainerExtra, pVendor->NPC_GetVendorMarkup());
	buyListExtra->push(this);

	if (!buyListCount && !buyListExtraCount)
		return false;

	// Open gump
	addOpenGump(pVendor, GUMP_VENDOR_RECT);

    // The Enhanced Client needs it always, for the Classic Client we send it always too, but we do that to have the updated 'Gold Available' value.
    new PacketObjectStatus(this, m_pChar);// the player is who needs to be sent.

	return true;
}

bool CClient::addShopMenuSell( CChar * pVendor )
{
	ADDTOCALLSTACK( "CClient::addShopMenuSell" );
	// Player selling to vendor.
	// What things do you have in your inventory that the vendor would want ?
	// Should end with a returned Event_VendorSell()

	if ( !pVendor || !pVendor->NPC_IsVendor() )
		return false;

	OpenPacketTransaction transaction( this, PacketSend::PRI_LOW );

	//	non-player vendors could be restocked on-the-fly
	if ( !pVendor->IsStatFlag( STATF_PET ) )
		pVendor->NPC_Vendor_Restock();

	CItemContainer *pContainer1 = pVendor->GetBank( LAYER_VENDOR_BUYS );
	CItemContainer *pContainer2 = pVendor->GetBank( LAYER_VENDOR_STOCK );
	addItem( pContainer1 );
	addItem( pContainer2 );

	// Classic clients will crash without extra packets, let's provide some empty packets specialy for them
	CItemContainer *pContainer3 = pVendor->GetBank( LAYER_VENDOR_EXTRA );
	addItem( pContainer3 );

	if ( pVendor->IsStatFlag( STATF_PET ) )	// player vendor
		pContainer2 = nullptr;		// no stock

	PacketVendorSellList cmd( pVendor );
	size_t count = cmd.fillSellList( this, m_pChar->GetPackSafe(), pContainer1, pContainer2, -pVendor->NPC_GetVendorMarkup() );
	if ( count <= 0 )
		return false;

	cmd.send( this );
	return true;
}

void CClient::addBankOpen( CChar * pChar, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CClient::addBankOpen");
	// open it up for this pChar.
	ASSERT( pChar );

	CItemContainer * pBankBox = pChar->GetBank(layer);
	ASSERT(pBankBox);
	addItem( pBankBox );	// may crash client if we dont do this.

	/*if ( pChar != GetChar())
	{
		// xbank verb on others needs this ?
		addChar( pChar );
	}*/

	pBankBox->OnOpenEvent( m_pChar, pChar );
	addContainerSetup( pBankBox );
}

void CClient::addDrawMap( CItemMap * pMap )
{
	ADDTOCALLSTACK("CClient::addDrawMap");
	// Make player drawn maps possible. (m_map_type=0) ???

	if ( pMap == nullptr )
	{
blank_map:
		addSysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MAP_IS_BLANK) );
		return;
	}
	if ( pMap->IsType(IT_MAP_BLANK))
		goto blank_map;

	CRectMap rect;
	rect.SetRect( pMap->m_itMap.m_left,
		pMap->m_itMap.m_top,
		pMap->m_itMap.m_right,
		pMap->m_itMap.m_bottom,
		g_MapList.GetMapID(pMap->m_itMap.m_map));

	if ( !rect.IsValid() )
		goto blank_map;
	if ( rect.IsRectEmpty() )
		goto blank_map;

	if ( PacketDisplayMapNew::CanSendTo(GetNetState()))
		new PacketDisplayMapNew(this, pMap, rect);
	else
		new PacketDisplayMap(this, pMap, rect);

	addMapMode( pMap, MAP_UNSENT, false );

	// Now show all the pins
	PacketMapPlot plot(pMap, MAP_ADD, false);
	for ( size_t i = 0; i < pMap->m_Pins.size(); ++i )
	{
		plot.setPin(pMap->m_Pins[i].m_x, pMap->m_Pins[i].m_y);
		plot.send(this);
	}
}

void CClient::addMapMode( CItemMap * pMap, MAPCMD_TYPE iType, bool fEdit )
{
	ADDTOCALLSTACK("CClient::addMapMode");
	// NOTE: MAPMODE_* depends on who is looking. Multi clients could interfere with each other ?
	if ( !pMap )
		return;

	pMap->m_fPlotMode = fEdit;

	new PacketMapPlot(this, pMap, iType, fEdit);
}

void CClient::addBulletinBoard( const CItemContainer * pBoard )
{
	ADDTOCALLSTACK("CClient::addBulletinBoard");
	// Open up the bulletin board and all it's messages
	// PacketBulletinBoardReq::onReceive
	if (pBoard == nullptr)
		return;

	// Give the bboard name.
	new PacketBulletinBoard(this, pBoard);

	// Send Content messages for all the items on the bboard.
	// Not sure what x,y are here, date/time maybe ?
	addContainerContents( pBoard );

	// The client will now ask for the headers it wants.
}

bool CClient::addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, const CUID& uidMsg )
{
	ADDTOCALLSTACK("CClient::addBBoardMessage");
	ASSERT(pBoard);

	CItemMessage *pMsgItem = static_cast<CItemMessage *>(uidMsg.ItemFind());
	if (pBoard->IsItemInside( pMsgItem ) == false)
		return false;

	// check author is properly linked
	if (pMsgItem->m_sAuthor.IsEmpty() == false && pMsgItem->m_uidLink.CharFind() == nullptr)
	{
		pMsgItem->Delete();
		return false;
	}

	// Send back the message header and/or body.
	new PacketBulletinBoard(this, flag, pBoard, pMsgItem);
	return true;
}

void CClient::addLoginComplete()
{
	ADDTOCALLSTACK("CClient::addLoginComplete");
	new PacketLoginComplete(this);
}

void CClient::addChatSystemMessage( CHATMSG_TYPE iType, lpctstr pszName1, lpctstr pszName2, CLanguageID lang )
{
	ADDTOCALLSTACK("CClient::addChatSystemMessage");

	new PacketChatMessage(this, iType, pszName1, pszName2, lang);
}

void CClient::addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, lpctstr pszName, lpctstr pszText )
{
	ADDTOCALLSTACK("CClient::addGumpTextDisp");
	// ??? how do we control where exactly the text goes ??

	new PacketSignGump(this, pObj, gump, pszName, pszText);
}

void CClient::addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, uint count, CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addItemMenu");
	// We must set GetTargMode() to show what mode we are in for menu select.
	// Will result in PacketMenuChoice::onReceive()

	if (count <= 0)
		return;

	if (pObj == nullptr)
		pObj = m_pChar;

	new PacketDisplayMenu(this, mode, item, count, pObj);

	m_tmMenu.m_UID = pObj->GetUID();
	SetTargMode( mode );
}

void CClient::addCharPaperdoll( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::addCharPaperdoll");
	if ( !pChar )
		return;

	new PacketPaperdoll(this, pChar);
}


void CClient::addShowDamage( int damage, dword uid_damage )
{
	ADDTOCALLSTACK("CClient::addShowDamage");
	if ( damage < 0 )
		damage = 0;

	if ( PacketCombatDamage::CanSendTo(GetNetState()) )
		new PacketCombatDamage(this, (word)(damage), static_cast<CUID>(uid_damage));
	else if ( PacketCombatDamageOld::CanSendTo(GetNetState()) )
		new PacketCombatDamageOld(this, (byte)(damage), static_cast<CUID>(uid_damage));
}

void CClient::addSpeedMode( byte speedMode )
{
	ADDTOCALLSTACK("CClient::addSpeedMode");

	new PacketSpeedMode(this, speedMode);
}

void CClient::addVisualRange( byte visualRange )
{
	ADDTOCALLSTACK("CClient::addVisualRange");

	//DEBUG_ERR(("addVisualRange called with argument %d\n", visualRange));

	new PacketVisualRange(this, visualRange);
}

void CClient::addIdleWarning( byte message )
{
	ADDTOCALLSTACK("CClient::addIdleWarning");

	new PacketWarningMessage(this, static_cast<PacketWarningMessage::Message>(message));
}

void CClient::addKRToolbar( bool bEnable )
{
	ADDTOCALLSTACK("CClient::addKRToolbar");
	if ( PacketToggleHotbar::CanSendTo(GetNetState()) == false || !IsResClient(RDS_KR) || ( GetConnectType() != CONNECT_GAME ))
		return;

	new PacketToggleHotbar(this, bEnable);
}


// --------------------------------------------------------------------
void CClient::SendPacket( tchar * ptcKey )
{
	ADDTOCALLSTACK("CClient::SendPacket");
	PacketSend* packet = new PacketSend(0, 0, PacketSend::PRI_NORMAL);
	packet->seek();

	while ( *ptcKey )
	{
		if ( packet->getLength() > SCRIPT_MAX_LINE_LEN - 4 )
		{	// we won't get here because this lenght is enforced in all scripts
			DEBUG_ERR(("SENDPACKET too big.\n"));

			delete packet;
			return;
		}

		GETNONWHITESPACE( ptcKey );

		if ( toupper(*ptcKey) == 'D' )
		{
			++ptcKey;
			dword iVal = Exp_GetVal(ptcKey);

			packet->writeInt32(iVal);
		}
		else if ( toupper(*ptcKey) == 'W' )
		{
			++ptcKey;
			word iVal = (Exp_GetWVal(ptcKey));

			packet->writeInt16(iVal);
		}
		else
		{
			if ( toupper(*ptcKey) == 'B' )
				ptcKey++;
			byte iVal = (Exp_GetBVal(ptcKey));

			packet->writeByte(iVal);
		}
	}

	packet->trim();
	packet->push(this);
}

// ---------------------------------------------------------------------
// Login type stuff.

byte CClient::Setup_Start( CChar * pChar ) // Send character startup stuff to player
{
	ADDTOCALLSTACK("CClient::Setup_Start");
	// Play this char.

	ASSERT( GetAccount() );
	ASSERT( pChar );

	CAccount* pAccount = GetAccount();

	CharDisconnect();	// I'm already logged in as someone else ?
	pAccount->m_uidLastChar = pChar->GetUID();

	g_Log.Event( LOGM_CLIENTS_LOG, "%x:Character startup for account '%s', char '%s'. IP='%s'.\n", GetSocketID(), pAccount->GetName(), pChar->GetName(), GetPeerStr() );

	if ( GetPrivLevel() > PLEVEL_Player )		// GMs should login with invul and without allshow flag set
	{
		ClearPrivFlags(PRIV_ALLSHOW);
		pChar->StatFlag_Set(STATF_INVUL);
	}

	addPlayerStart( pChar );
	ASSERT(m_pChar);

	bool fNoMessages = false;
	bool fQuickLogIn = pChar->LayerFind(LAYER_FLAG_ClientLinger);
	if ( IsTrigUsed(TRIGGER_LOGIN) )
	{
		CScriptTriggerArgs Args( fNoMessages, fQuickLogIn );
		if ( pChar->OnTrigger( CTRIG_LogIn, pChar, &Args ) == TRIGRET_RET_TRUE )
		{
			m_pChar->ClientDetach();
			pChar->SetDisconnected();
			return PacketLoginError::Blocked;
		}
		fNoMessages	= (Args.m_iN1 != 0);
		fQuickLogIn	= (Args.m_iN2 != 0);
	}

	tchar *z = Str_GetTemp();
	if ( !fQuickLogIn )
	{
		if ( !fNoMessages )
		{
			addBark(g_sServerDescription.c_str(), nullptr, HUE_YELLOW, TALKMODE_SAY, FONT_NORMAL);

			snprintf(z, STR_TEMPLENGTH, (g_Serv.StatGet(SERV_STAT_CLIENTS)==2) ?
				g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYER ) : g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_PLAYERS ),
				g_Serv.StatGet(SERV_STAT_CLIENTS)-1 );
			addSysMessage(z);

            const lpctstr ptcLastLogged = pAccount->m_TagDefs.GetKeyStr("LastLogged");
            if (!IsStrEmpty(ptcLastLogged))
            {
                snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg( DEFMSG_LOGIN_LASTLOGGED ), ptcLastLogged);
                addSysMessage(z);
            }
		}
		if ( m_pChar->m_pArea && m_pChar->m_pArea->IsGuarded() && !m_pChar->m_pArea->IsFlag(REGION_FLAG_ANNOUNCE) )
		{
			const CVarDefContStr * pVarStr = dynamic_cast <CVarDefContStr *>( m_pChar->m_pArea->m_TagDefs.GetKey("GUARDOWNER"));
			SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARDSP), (pVarStr ? pVarStr->GetValStr() : g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_GUARDSPT)) );
			if ( m_pChar->m_pArea->m_TagDefs.GetKeyNum("RED") )
				SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_REDDEF), g_Cfg.GetDefaultMsg(DEFMSG_MSG_REGION_REDENTER));
		}
	}

	if ( IsPriv(PRIV_GM_PAGE) && !g_World.m_GMPages.IsContainerEmpty() )
	{
		snprintf(z, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_GMPAGE_PENDING), (int)(g_World.m_GMPages.GetContentCount()), g_Cfg.m_cCommandPrefix);
		addSysMessage(z);
	}
	if ( IsPriv(PRIV_JAILED) )
		m_pChar->Jail(&g_Serv, true, (int)(pAccount->m_TagDefs.GetKeyNum("JailCell")));
	if ( g_Serv.m_timeShutdown > 0 )
		addBarkParse(g_Cfg.GetDefaultMsg(DEFMSG_MSG_SERV_SHUTDOWN_SOON), nullptr, HUE_TEXT_DEF, TALKMODE_SAY, FONT_BOLD);

	pAccount->m_TagDefs.DeleteKey("LastLogged");
	Announce(true);		// announce you to the world

	// Don't login on the water, bring us to nearest shore (unless I can swim)
	if ( !IsPriv(PRIV_GM) && !m_pChar->Can(CAN_C_SWIM) && m_pChar->IsSwimming() )
	{
		m_pChar->MoveToNearestShore();
	}

	/*
	* // If ever we want to change how timers are suspended...
	* 
	if (m_pChar->IsPlayer())
	{
		// When a character logs out, its timer and its contents' timers has to freeze.
		// The timeout is stored as server time (not real world time) in milliseconds.
		// When a char logs out, the logout server time is stored.
		// When the char logs in again, move forward its timers by the time it spent offline.
		if (m_pChar->m_pPlayer->_iTimeLastDisconnected > 0)
		{
			const int64 iDelta = CWorldGameTime::GetCurrentTime().GetTimeRaw() - m_pChar->m_pPlayer->_iTimeLastDisconnected;
			if (iDelta < 0)
			{
				g_Log.EventWarn("World Time was manually changed. The TIMERs belonging to the char '%s' (UID=0%x) couldn't be frozen during its logout.\n", m_pChar->GetName(), m_pChar->GetUID().GetObjUID());
			}
			else
			{
				m_pChar->TimeoutRecursiveResync(iDelta);
			}
		}
	}
	*/

	DEBUG_MSG(( "%x:Setup_Start done\n", GetSocketID()));

	return PacketLoginError::Success;
}

byte CClient::Setup_Play( uint iSlot ) // After hitting "Play Character" button
{
	ADDTOCALLSTACK("CClient::Setup_Play");
	// Mode == CLIMODE_SETUP_CHARLIST

	DEBUG_MSG(( "%x:Setup_Play slot %u\n", GetSocketID(), iSlot ));

	CAccount* pAccount = GetAccount();
	if ( !pAccount )
		return( PacketLoginError::Invalid );
	if ( iSlot >= CountOf(m_tmSetupCharList))
		return( PacketLoginError::BadCharacter );

	CChar * pChar = m_tmSetupCharList[ iSlot ].CharFind();
	if ( !pAccount->IsMyAccountChar( pChar ))
		return( PacketLoginError::BadCharacter );

	CChar * pCharLast = pAccount->m_uidLastChar.CharFind();
	if ( pCharLast && pAccount->IsMyAccountChar( pCharLast ) && pAccount->GetPrivLevel() <= PLEVEL_GM &&
		! pCharLast->IsDisconnected() && (pChar->GetUID() != pCharLast->GetUID()))
	{
		addIdleWarning(PacketWarningMessage::CharacterInWorld);
		return(PacketLoginError::CharIdle);
	}

	// LastLogged update
	CSTime datetime = CSTime::GetCurrentTime();
	pAccount->m_TagDefs.SetStr("LastLogged", false, pAccount->_dateConnectedLast.Format(nullptr));
	pAccount->_dateConnectedLast = datetime;

	return Setup_Start( pChar );
}

byte CClient::Setup_Delete( dword iSlot ) // Deletion of character
{
	ADDTOCALLSTACK("CClient::Setup_Delete");
	ASSERT( GetAccount() );
	DEBUG_MSG(( "%x:Setup_Delete slot=%u\n", GetSocketID(), iSlot ));
	if ( iSlot >= CountOf(m_tmSetupCharList))
		return PacketDeleteError::NotExist;

	CChar * pChar = m_tmSetupCharList[iSlot].CharFind();
	if ( ! GetAccount()->IsMyAccountChar( pChar ))
		return PacketDeleteError::BadPass;

	if ( ! pChar->IsDisconnected())
	{
		return PacketDeleteError::InUse;
	}

	// Make sure the char is at least x seconds old.
	if ( g_Cfg.m_iMinCharDeleteTime &&
		(CWorldGameTime::GetCurrentTime().GetTimeDiff(pChar->_iTimeCreate) < g_Cfg.m_iMinCharDeleteTime) )
	{
		if ( GetPrivLevel() < PLEVEL_Counsel )
		{
			return PacketDeleteError::NotOldEnough;
		}
	}

	//	Do the scripts allow to delete the char?
	enum TRIGRET_TYPE tr;
	CScriptTriggerArgs Args;
	Args.m_pO1 = this;
	pChar->r_Call("f_onchar_delete", pChar, &Args, nullptr, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		return PacketDeleteError::InvalidRequest;
	}

	g_Log.Event(LOGM_ACCOUNTS|LOGL_EVENT, "%x:Account '%s' deleted char '%s' [0%x] on client login screen.\n", GetSocketID(), GetAccount()->GetName(), pChar->GetName(), (dword)(pChar->GetUID()));
	pChar->Delete();

	// refill the list.
	new PacketCharacterListUpdate(this, GetAccount()->m_uidLastChar.CharFind());

	return PacketDeleteError::Success;
}

byte CClient::Setup_ListReq( const char * pszAccName, const char * pszPassword, bool fTest )
{
	ADDTOCALLSTACK("CClient::Setup_ListReq");
	// XCMD_CharListReq
	// Gameserver login and request character listing

	if ( GetConnectType() != CONNECT_GAME )	// Not a game connection ?
		return PacketLoginError::Other;

	switch ( GetTargMode())
	{
		case CLIMODE_SETUP_RELAY:
			ClearTargMode();
			break;

		default:
			break;
	}

	CSString sMsg;
	byte lErr = LogIn( pszAccName, pszPassword, sMsg );

	if ( lErr != PacketLoginError::Success )
	{
		if ( fTest && lErr != PacketLoginError::Other )
		{
			if ( ! sMsg.IsEmpty())
				SysMessage( sMsg );
		}
		return lErr;
	}


	/*
	CAccount * pAcc = GetAccount();
	ASSERT( pAcc );

	CChar *pCharLast = pAcc->m_uidLastChar.CharFind();
	if ( pCharLast && GetAccount()->IsMyAccountChar(pCharLast) && GetAccount()->GetPrivLevel() <= PLEVEL_GM && !pCharLast->IsDisconnected() )
	{
		// If the last char is lingering then log back into this char instantly.
		// m_iClientLingerTime
		if ( Setup_Start(pCharLast) )
			return PacketLoginError::Success;
		return PacketLoginError::Blocked;	//Setup_Start() returns false only when login blocked by Return 1 in @Login
	}*/

    dword dwFeatureFlags;
    dword dwCliVer = m_Crypt.GetClientVer();
    if ( dwCliVer && (dwCliVer < 1260000) )
    {
        dwFeatureFlags = 0x03;
    }
    else
    {
        uchar uiChars = (uchar)(maximum(m_pAccount->GetMaxChars(), m_pAccount->m_Chars.GetCharCount()));
        dwFeatureFlags = g_Cfg.GetPacketFlag( false, (RESDISPLAY_VERSION)(m_pAccount->GetResDisp()), uiChars );
    }
	new PacketEnableFeatures(this, dwFeatureFlags);
	new PacketCharacterList(this);

	m_Targ_Mode = CLIMODE_SETUP_CHARLIST;
	return PacketLoginError::Success;
}

byte CClient::LogIn( CAccount * pAccount, CSString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	if ( pAccount == nullptr )
		return( PacketLoginError::Invalid );

	if ( pAccount->IsPriv( PRIV_BLOCKED ))
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s' is blocked.\n", GetSocketID(), pAccount->GetName());
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_BLOCKED ), static_cast<lpctstr>(g_Serv.m_sEMail));
		return( PacketLoginError::Blocked );
	}

	// Look for this account already in use.
	CClient * pClientPrev = pAccount->FindClient( this );
	if ( pClientPrev != nullptr )
	{
		// Only if it's from a diff ip ?
		ASSERT( pClientPrev != this );

		bool fInUse = false;

		//	different ip - no reconnect
		if ( ! GetPeer().IsSameIP( pClientPrev->GetPeer() ))
			fInUse = true;
		else
		{
			//	from same ip - allow reconnect if the old char is lingering out
			CChar *pCharOld = pClientPrev->GetChar();
			if ( pCharOld )
			{
				CItem *pItem = pCharOld->LayerFind(LAYER_FLAG_ClientLinger);
				if ( !pItem )
					fInUse = true;
			}

			if ( !fInUse )
			{
				if ( IsConnectTypePacket() && pClientPrev->IsConnectTypePacket())
				{
					pClientPrev->CharDisconnect();
					pClientPrev->GetNetState()->markReadClosed();
				}
				else if ( GetConnectType() == pClientPrev->GetConnectType() )
					fInUse = true;
			}
		}

		if ( fInUse )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s' already in use.\n", GetSocketID(), pAccount->GetName());
			sMsg = "Account already in use.";
			return PacketLoginError::InUse;
		}
	}

	if ( g_Cfg.m_iClientsMax <= 0 )
	{
		// Allow no one but locals on.
		CSocketAddress SockName = GetPeer();
		if ( ! GetPeer().IsLocalAddr() && SockName.GetAddrIP() != GetPeer().GetAddrIP() )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s', maximum clients reached (only local connections allowed).\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_LD );
			return( PacketLoginError::MaxClients );
		}
	}
	if ( g_Cfg.m_iClientsMax <= 1 )
	{
		// Allow no one but Administrator on.
		if ( pAccount->GetPrivLevel() < PLEVEL_Admin )
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s', maximum clients reached (only administrators allowed).\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_AO );
			return( PacketLoginError::MaxClients );
		}
	}
	if ( (pAccount->GetPrivLevel() < PLEVEL_GM) &&
		((llong)g_Serv.StatGet(SERV_STAT_CLIENTS) > g_Cfg.m_iClientsMax) )
	{
		// Give them a polite goodbye.
		g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s', maximum clients reached.\n", GetSocketID(), pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_SERV_FULL );
		return( PacketLoginError::MaxClients );
	}

	//	Do the scripts allow to login this account?
	pAccount->m_Last_IP.SetAddrIP(GetPeer().GetAddrIP());
	CScriptTriggerArgs Args;
	Args.Init(pAccount->GetName());
	Args.m_iN1 = GetConnectType();
	Args.m_pO1 = this;
	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
	g_Serv.r_Call("f_onaccount_login", &g_Serv, &Args, nullptr, &tr);
	if ( tr == TRIGRET_RET_TRUE )
	{
		sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_DENIED );
		return (PacketLoginError::Blocked);
	}

	m_pAccount = pAccount;
	pAccount->OnLogin( this );

	return( PacketLoginError::Success );
}

byte CClient::LogIn( lpctstr ptcAccName, lpctstr ptcPassword, CSString & sMsg )
{
	ADDTOCALLSTACK("CClient::LogIn");
	// Try to validate this account.
	// Do not send output messages as this might be a console or web page or game client.
	// NOTE: addLoginErr() will get called after this.

	if ( GetAccount()) // already logged in.
		return( PacketLoginError::Success );

	char szTmp[ MAX_NAME_SIZE ];
	size_t iLen1 = strlen( ptcAccName );
	size_t iLen2 = strlen( ptcPassword );
	size_t iLen3 = Str_GetBare( szTmp, ptcAccName, MAX_NAME_SIZE );
	if ( (iLen1 == 0) || (iLen1 != iLen3) || (iLen1 > MAX_NAME_SIZE) )	// a corrupt message.
	{
		char pcVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_WCLI ), m_Crypt.WriteClientVer(pcVersion, sizeof(pcVersion)));
		return( PacketLoginError::BadAccount );
	}

	iLen3 = Str_GetBare( szTmp, ptcPassword, MAX_NAME_SIZE );
	if ( iLen2 != iLen3 )	// a corrupt message.
	{
		char pcVersion[ 256 ];
		sMsg.Format( g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_WCLI ), m_Crypt.WriteClientVer(pcVersion, sizeof(pcVersion)));
		return( PacketLoginError::BadPassword );
	}


	tchar ptcName[ MAX_ACCOUNT_NAME_SIZE ];
	if ( !CAccount::NameStrip(ptcName, ptcAccName) || Str_Check(ptcAccName) )
		return( PacketLoginError::BadAccount );
	else if ( Str_Check(ptcPassword) )
		return( PacketLoginError::BadPassword );

	const bool fGuestAccount = ! strnicmp( ptcAccName, "GUEST", 5 );
	if ( fGuestAccount )
	{
		// trying to log in as some sort of guest.
		// Find or create a new guest account.
		tchar *ptcTemp = Str_GetTemp();
		for ( int i = 0; ; ++i )
		{
			if ( i>=g_Cfg.m_iGuestsMax )
			{
				sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_GUSED );
				return( PacketLoginError::MaxGuests );
			}

			sprintf(ptcTemp, "GUEST%d", i);
			CAccount * pAccount = g_Accounts.Account_FindCreate(ptcTemp, true );
			ASSERT( pAccount );

			if ( pAccount->FindClient() == nullptr )
			{
				ptcAccName = pAccount->GetName();
				break;
			}
		}
	}
	else
	{
		if ( ptcPassword[0] == '\0' )
		{
			sMsg = g_Cfg.GetDefaultMsg( DEFMSG_MSG_ACC_NEEDPASS );
			return( PacketLoginError::BadPassword );
		}
	}

	bool fAutoCreate = ( g_Serv.m_eAccApp == ACCAPP_Free || g_Serv.m_eAccApp == ACCAPP_GuestAuto || g_Serv.m_eAccApp == ACCAPP_GuestTrial );
	CAccount * pAccount = g_Accounts.Account_FindCreate(ptcAccName, fAutoCreate);
	if ( ! pAccount )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%x: Account '%s' does not exist\n", GetSocketID(), ptcAccName);
		sMsg.Format(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_UNK), ptcAccName);
		return PacketLoginError::Invalid;
	}

	if ( g_Cfg.m_iClientLoginMaxTries && !pAccount->CheckPasswordTries(GetPeer()) )
	{
		g_Log.Event(LOGM_CLIENTS_LOG, "%x: '%s' exceeded password tries in time lapse\n", GetSocketID(), pAccount->GetName());
		sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_BADPASS);
		return PacketLoginError::MaxPassTries;
	}

	if ( ! fGuestAccount && ! pAccount->IsPriv(PRIV_BLOCKED) )
	{
		if ( ! pAccount->CheckPassword(ptcPassword))
		{
			g_Log.Event(LOGM_CLIENTS_LOG, "%x: '%s' bad password\n", GetSocketID(), pAccount->GetName());
			sMsg = g_Cfg.GetDefaultMsg(DEFMSG_MSG_ACC_BADPASS);
			return PacketLoginError::BadPass;
		}
	}

	return LogIn(pAccount, sMsg);
}


