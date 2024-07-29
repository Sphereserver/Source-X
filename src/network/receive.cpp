
#include "../common/resource/sections/CDialogDef.h"
#include "../common/CLog.h"
#include "../common/CUOClientVersion.h"
#include "../game/chars/CChar.h"
#include "../game/clients/CClient.h"
#include "../game/items/CItem.h"
#include "../game/items/CItemMap.h"
#include "../game/items/CItemMessage.h"
#include "../game/items/CItemMultiCustom.h"
#include "../game/items/CItemShip.h"
#include "../game/items/CItemVendable.h"
#include "../game/CServer.h"
#include "../game/CWorldGameTime.h"
#include "../game/CWorldMap.h"
#include "../game/triggers.h"
#include "CClientIterator.h"
#include "CNetState.h"
#include "CNetworkManager.h"
#include "receive.h"
#include "send.h"


/***************************************************************************
 *
 *
 *	Packet ???? : PacketUnknown			unknown or unhandled packet
 *
 *
 ***************************************************************************/
PacketUnknown::PacketUnknown(uint size) : Packet(size)
{
}

bool PacketUnknown::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketUnknown::onReceive");
	UnreferencedParameter(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x00 : PacketCreate		create new character request
 *
 *
 ***************************************************************************/
PacketCreate::PacketCreate(uint size) : Packet(size)
{
}

bool PacketCreate::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCreate::onReceive");
	tchar charname[MAX_NAME_SIZE];
	SKILL_TYPE skill1 = SKILL_NONE, skill2 = SKILL_NONE, skill3 = SKILL_NONE, skill4 = SKILL_NONE;
	byte skillval1 = 0, skillval2 = 0, skillval3 = 0, skillval4 = 0;

	skip(9); // 4=pattern1, 4=pattern2, 1=kuoc
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(2); // 0x00
	dword flags = readInt32();
	skip(8); // unk
	PROFESSION_TYPE prof = static_cast<PROFESSION_TYPE>(readByte());
	skip(15); // 0x00
	byte race_sex_flag = readByte();
	byte strength = readByte();
	byte dexterity = readByte();
	byte intelligence = readByte();
	skill1 = (SKILL_TYPE)readByte();
	skillval1 = readByte();
	skill2 = (SKILL_TYPE)readByte();
	skillval2 = readByte();
	skill3 = (SKILL_TYPE)readByte();
	skillval3 = readByte();
	HUE_TYPE hue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE hairid = (ITEMID_TYPE)(readInt16());
	HUE_TYPE hairhue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE beardid = (ITEMID_TYPE)(readInt16());
	HUE_TYPE beardhue = (HUE_TYPE)(readInt16());
	skip(1); // shard index
	byte startloc = readByte();
	skip(8); // 4=slot, 4=ip
	HUE_TYPE shirthue = (HUE_TYPE)(readInt16());
	HUE_TYPE pantshue = (HUE_TYPE)(readInt16());

	bool isFemale = (race_sex_flag % 2) != 0; // Even=Male, Odd=Female (rule applies to all clients)
	RACE_TYPE rtRace = RACETYPE_HUMAN; // Human

	// determine which race the client has selected
	if (net->isClientVersionNumber(MINCLIVER_SA))
	{
		/*
			m_sex values from clients 7.0.0.0+
			0x2 = Human (male)
			0x3 = Human (female)
			0x4 = Elf (male)
			0x5 = Elf (female)
			0x6 = Gargoyle (male)
			0x7 = Gargoyle (female)
		*/
		switch (race_sex_flag)
		{
			case 0x2: case 0x3:
				rtRace = RACETYPE_HUMAN;
				break;
			case 0x4: case 0x5:
				rtRace = RACETYPE_ELF;
				break;
			case 0x6: case 0x7:
				rtRace = RACETYPE_GARGOYLE;
				break;
			default:
				g_Log.Event(LOGL_WARN | LOGM_NOCONTEXT, "PacketCreate: unknown race_sex_flag (%" PRIu8 "), defaulting to 2 (human male).\n", race_sex_flag);
		}
	}
	else
	{
		/*
			m_sex values from clients pre-7.0.0.0
			0x0 = Human (male)
			0x1 = Human (female)
			0x2 = Elf (male)
			0x3 = Elf (female)
		*/
		if ((race_sex_flag - 2) >= 0)
			rtRace = RACETYPE_ELF;
	}

	// validate race against resdisp
	byte resdisp = net->getClient()->GetAccount() != nullptr? net->getClient()->GetAccount()->GetResDisp() : (byte)RDS_T2A;
	if (resdisp < RDS_ML) // prior to ML, only human
	{
		if (rtRace >= RACETYPE_ELF)
			rtRace = RACETYPE_HUMAN;
	}
	else if (resdisp < RDS_SA) // prior to SA, only human and elf
	{
		if (rtRace >= RACETYPE_GARGOYLE)
			rtRace = RACETYPE_HUMAN;
	}

	return doCreate(net, charname, isFemale, rtRace,
		strength, dexterity, intelligence, prof,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4,
		hue, hairid, hairhue, beardid, beardhue, shirthue, pantshue, ITEMID_NOTHING, startloc, flags);
}

bool PacketCreate::doCreate(CNetState* net, lpctstr charname, bool fFemale, RACE_TYPE rtRace, ushort wStr, ushort wDex, ushort wInt, PROFESSION_TYPE prProf,
	SKILL_TYPE skSkill1, ushort uiSkillVal1, SKILL_TYPE skSkill2, ushort uiSkillVal2, SKILL_TYPE skSkill3, ushort uiSkillVal3, SKILL_TYPE skSkill4, ushort uiSkillVal4,
	HUE_TYPE wSkinHue, ITEMID_TYPE idHair, HUE_TYPE wHairHue, ITEMID_TYPE idBeard, HUE_TYPE wBeardHue, HUE_TYPE wShirtHue, HUE_TYPE wPantsHue, ITEMID_TYPE idFace, int iStartLoc, uint uiFlags)
{
	ADDTOCALLSTACK("PacketCreate::doCreate");

	CClient* client = net->getClient();
	ASSERT(client);
	const CAccount * account = client->GetAccount();
	ASSERT(account);

	if (client->GetChar() != nullptr)
	{
		// logging in as a new player whilst already online !
		client->addSysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_ALREADYONLINE));
		g_Log.Event(LOGM_CLIENTS_LOG|LOGM_NOCONTEXT, "%x:Account '%s' already in use\n", net->id(), account->GetName());
		return false;
	}

	// make sure they don't have an idling character
	const CChar* pCharLast = account->m_uidLastChar.CharFind();
	if (pCharLast != nullptr && account->IsMyAccountChar(pCharLast) && account->GetPrivLevel() <= PLEVEL_GM && !pCharLast->IsDisconnected() )
	{
		client->addIdleWarning(PacketWarningMessage::CharacterInWorld);
		client->addLoginErr(PacketLoginError::CharIdle);
		return false;
	}

	// make sure they don't already have too many characters
	byte iMaxChars = account->GetMaxChars();
	uint iQtyChars = (uint)account->m_Chars.GetCharCount();
	if (iQtyChars >= iMaxChars)
	{
		client->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_MAXCHARS), (int)(iQtyChars));
		if (client->GetPrivLevel() < PLEVEL_Seer)
		{
			client->addLoginErr(PacketLoginError::TooManyChars);
			return false;
		}
	}


	CChar* pChar = CChar::CreateBasic(CREID_MAN);
	ASSERT(pChar != nullptr);

	TRIGRET_TYPE tr = TRIGRET_RET_DEFAULT;
	CScriptTriggerArgs createArgs;
    // RW
	createArgs.m_iN1 = uiFlags;
	createArgs.m_iN2 = prProf;
	createArgs.m_iN3 = rtRace;
    // R
	createArgs.m_s1 = account->GetName();
	createArgs.m_pO1 = client;

    client->r_Call("f_onchar_create_init", &g_Serv, &createArgs, nullptr, &tr);
    if (tr == TRIGRET_RET_TRUE)
        goto block_creation;

    //uiFlags = (uint)createArgs.m_iN1;  // unused at this point
    prProf = (PROFESSION_TYPE)createArgs.m_iN2;
    rtRace = (RACE_TYPE)createArgs.m_iN3;

	// Creating the pChar
	pChar->InitPlayer(client, charname, fFemale, rtRace, wStr, wDex, wInt,
		prProf, skSkill1, uiSkillVal1, skSkill2, uiSkillVal2, skSkill3, uiSkillVal3, skSkill4, uiSkillVal4,
		wSkinHue, idHair, wHairHue, idBeard, wBeardHue, wShirtHue, wPantsHue, idFace, iStartLoc);

	// Calling the function after the char creation, it can't be done before or the function won't have SRC.
    // The createArgs are Read-Only for this function.
    tr = TRIGRET_RET_DEFAULT;
	client->r_Call("f_onchar_create", pChar, &createArgs, nullptr, &tr);

	if ( tr == TRIGRET_RET_TRUE )
	{
block_creation:
		client->addLoginErr(PacketLoginError::CreationBlocked);
		pChar->Delete();	//Delete it if function is returning 1 or the char will remain created
		return false;
	}

	g_Log.Event(LOGM_CLIENTS_LOG|LOGM_NOCONTEXT, "%x:Account '%s' created new char '%s' [0%" PRIx32 "]\n", net->id(), account->GetName(), pChar->GetName(), (dword)pChar->GetUID() );
	client->Setup_Start(pChar);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x02 : PacketMovementReq		movement request
 *
 *
 ***************************************************************************/
PacketMovementReq::PacketMovementReq(uint size) : Packet(size)
{
}

bool PacketMovementReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketMovementReq::onReceive");

	CClient *client = net->getClient();
	ASSERT(client);

	byte direction = readByte();
	byte sequence = readByte();
	//dword crypt = readInt32();	// client fastwalk crypt (not used anymore)

	if ( net->m_sequence == 0 && sequence != 0 )
		direction = DIR_QTY;	// setting invalid direction to intentionally reject the walk request

	if ( client->Event_Walk(direction, sequence) )
	{
		if ( sequence == UINT8_MAX )
			sequence = 0;
		net->m_sequence = ++sequence;
	}
	else
	{
		net->m_sequence = 0;
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x03 : PacketSpeakReq			character talking
 *
 *
 ***************************************************************************/
PacketSpeakReq::PacketSpeakReq() : Packet(0)
{
}

bool PacketSpeakReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSpeakReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == nullptr)
		return false;

	uint packetLength = readInt16();
	TALKMODE_TYPE mode = (TALKMODE_TYPE)(readByte());
	HUE_TYPE hue = (HUE_TYPE)(readInt16());
	skip(2); // font

	if (packetLength < getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER;

	tchar text[MAX_TALK_BUFFER];
	readStringASCII(text, minimum(ARRAY_COUNT(text), packetLength));

	client->Event_Talk(text, hue, mode, false);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x05 : PacketAttackReq		attack request
 *
 *
 ***************************************************************************/
PacketAttackReq::PacketAttackReq() : Packet(5)
{
}

bool PacketAttackReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAttackReq::onReceive");

	CUID target(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Attack(target);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x06 : PacketDoubleClick		double click object
 *
 *
 ***************************************************************************/
PacketDoubleClick::PacketDoubleClick() : Packet(5)
{
}

bool PacketDoubleClick::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketDoubleClick::onReceive");

	dword serial = readInt32();

	CUID target(serial &~ UID_F_RESOURCE);
	bool macro = (serial & UID_F_RESOURCE) == UID_F_RESOURCE;

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_DoubleClick(target, macro, true);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x07 : PacketItemPickupReq	pick up item request
 *
 *
 ***************************************************************************/
PacketItemPickupReq::PacketItemPickupReq() : Packet(7)
{
}

bool PacketItemPickupReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketItemPickupReq::onReceive");

	CUID serial(readInt32());
	word amount = readInt16();

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Item_Pickup(serial, amount);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x08 : PacketItemDropReq		drop item request
 *
 *
 ***************************************************************************/
PacketItemDropReq::PacketItemDropReq() : Packet(14)
{
}

uint PacketItemDropReq::getExpectedLength(CNetState* net, Packet* packet)
{
	ADDTOCALLSTACK("PacketItemDropReq::getExpectedLength");
	UnreferencedParameter(packet);

	// different size depending on client
	if (net != nullptr && (net->isClientVersionNumber(MINCLIVER_ITEMGRID) || net->isClientKR() || net->isClientEnhanced()))
		return 15;

	return 14;
}

bool PacketItemDropReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketItemDropReq::onReceive");

	CClient *client = net->getClient();
	ASSERT(client);
	const CChar *character = client->GetChar();
	if ( !character )
		return false;

	CUID serial(readInt32());
	word x = readInt16();
	word y = readInt16();
	byte z = readByte();

	byte grid = 0;
	if ( net->isClientVersionNumber(MINCLIVER_ITEMGRID) || net->isClientKR() || net->isClientEnhanced() )
	{
		grid = readByte();

		// Enhanced clients using containers on 'list view' mode always send grid=255 to server,
		// this means that the item must placed on first grid slot free, and not on slot 255.
		if ( grid == UINT8_MAX )
			grid = 0;
	}

	CUID container(readInt32());
	CPointMap pt(x, y, z, character->GetTopMap());

	client->Event_Item_Drop(serial, pt, container, grid);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x09 : PacketSingleClick		single click object
 *
 *
 ***************************************************************************/
PacketSingleClick::PacketSingleClick() : Packet(5)
{
}

bool PacketSingleClick::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSingleClick::onReceive");

	CUID serial(readInt32());

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_SingleClick(serial);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x12 : PacketTextCommand					text command
 *
 *
 ***************************************************************************/
PacketTextCommand::PacketTextCommand() : Packet(0)
{
}

bool PacketTextCommand::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketTextCommand::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	word packetLength = readInt16();
	if ((packetLength < 5) || (packetLength > MAX_EXTCMD_ARG_LEN + 4))
		return false;

	EXTCMD_TYPE type = EXTCMD_TYPE(readByte());
	tchar name[MAX_TALK_BUFFER];
	readStringNullASCII(name, MAX_TALK_BUFFER-1);

	client->Event_ExtCmd(type, name);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x13 : PacketItemEquipReq	item equip request
 *
 *
 ***************************************************************************/
PacketItemEquipReq::PacketItemEquipReq() : Packet(10)
{
}

bool PacketItemEquipReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketItemEquipReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CUID itemSerial(readInt32());
	LAYER_TYPE itemLayer = static_cast<LAYER_TYPE>(readByte());
	CUID targetSerial(readInt32());

	CChar* source = client->GetChar();
	if (source == nullptr)
		return false;

	CItem* item = source->LayerFind(LAYER_DRAGGING);
	if ( !item || (client->GetTargMode() != CLIMODE_DRAG) || (item->GetUID() != itemSerial) )
	{
		// I have no idea why i got here.
		new PacketDragCancel(client, PacketDragCancel::Other);
		return true;
	}

	client->ClearTargMode(); // done dragging.

	CChar* target = targetSerial.CharFind();
    bool fSuccess = false;
    if (target && (itemLayer < LAYER_HORSE) && source->CanDress(target) && source->CanTouch(item))
    {
       //if (target->CanCarry(item)) //Since Weight behavior rework, we want avoid don't be able to equip an item if overweight
            fSuccess = target->ItemEquip(item, source);
       // else
       //     client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_HEAVY));
	}

    if (!fSuccess)
        client->Event_Item_Drop_Fail(item);		//cannot equip

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketResynchronize	resend all request
 *
 *
 ***************************************************************************/
PacketResynchronize::PacketResynchronize() : Packet(3)
{
}

bool PacketResynchronize::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketResynchronize::onReceive");

	CClient * client = net->getClient();
	ASSERT(client);

	CChar * pChar = client->GetChar();
	if ( !pChar )
		return false;

	client->addChar(pChar);
	client->addPlayerUpdate();
	client->addPlayerSee(CPointMap());
	net->m_sequence = 0;
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x2c : PacketDeathStatus		death status
 *
 *
 ***************************************************************************/
PacketDeathStatus::PacketDeathStatus() : Packet(2)
{
}

bool PacketDeathStatus::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketDeathStatus::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* ghost = client->GetChar();
	if (ghost == nullptr)
		return false;

    Mode mode = (Mode)readByte();
	if (mode != Mode::Dead)
	{
		// Play as a ghost.
		client->SysMessage("You are a ghost");
		client->addSound(0x17f);
		client->addPlayerStart(ghost); // Do practically a full reset (to clear death menu)
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x34 : PacketObjStatusReq	request information on the object
 *
 *
 ***************************************************************************/
PacketObjStatusReq::PacketObjStatusReq() : Packet(10)
{
}

bool PacketObjStatusReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketObjStatusReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	if ( !client->GetChar() )
		return false;
	skip(4);	// 0xedededed
	byte requestType = readByte();
	CUID targetSerial(readInt32());

	if ( requestType == 4 )
		client->addStatusWindow(targetSerial.ObjFind(), true);
	else if ( requestType == 5 )
		client->addSkillWindow(SKILL_QTY);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkillLockChange	change skill locks
 *
 *
 ***************************************************************************/
PacketSkillLockChange::PacketSkillLockChange() : Packet(0)
{
}

bool PacketSkillLockChange::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSkillLockChange::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr || character->m_pPlayer == nullptr)
		return false;

	int len = readInt16();
	len -= 3;
	if ((len <= 0 )|| ((len % 3) != 0) || (len > 100*3))
		return false;

	while (len > 0)
	{
		// set next lock
		SKILL_TYPE index = (SKILL_TYPE)readInt16();
		SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());
		len -= 3;

		if (index <= SKILL_NONE || index >= SKILL_QTY ||
			state < SKILLLOCK_UP || state > SKILLLOCK_LOCK)
			continue;

		character->m_pPlayer->Skill_SetLock(index, state);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketVendorBuyReq	buy item from vendor
 *
 *
 ***************************************************************************/
PacketVendorBuyReq::PacketVendorBuyReq() : Packet(0)
{
}

bool PacketVendorBuyReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketVendorBuyReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* buyer = client->GetChar();
	if (buyer == nullptr)
		return false;

	word packetLength = readInt16();
	CUID vendorSerial(readInt32());
	byte flags = readByte();
	if (flags == 0)
		return true;

	CChar* vendor = vendorSerial.CharFind();
	if (!vendor || !vendor->m_pNPC || !vendor->NPC_IsVendor())
	{
		client->Event_VendorBuy_Cheater(0x1);
		return true;
	}

	if (buyer->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

    VendorItem items[MAX_ITEMS_CONT] = {};
    const uint uiCountFromPacket = (packetLength - 8u) / 7u;
	uint itemCount = minimum(uiCountFromPacket, g_Cfg.m_iContainerMaxItems);

	// check buying speed
	const CVarDefCont* vardef = g_Cfg.m_fAllowBuySellAgent ? nullptr : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != nullptr)
	{
		const int64 allowsell = vardef->GetValNum() + (itemCount * 3LL);
		if (CWorldGameTime::GetCurrentTime().GetTimeRaw() < allowsell)
		{
			client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_BUYFAST));
			return true;
		}
	}

	// combine goods into one list
    const int iConvertFactor = vendor->NPC_GetVendorMarkup();
	CItemVendable *item = nullptr;
	for (uint i = 0; i < itemCount; ++i)
	{
		skip(1); // layer
        const CUID serial(readInt32());
        const word amount = readInt16();

		item = dynamic_cast<CItemVendable*>(serial.ItemFind());
		if (item == nullptr || item->IsValidSaleItem(true) == false)
		{
			client->Event_VendorBuy_Cheater(0x2);
			return true;
		}

		// search for it in the list
		uint index;
		for (index = 0; index < itemCount; ++index)
		{
			if (serial == items[index].m_serial) //If the serials are the same, that means the items come from the same stack.
				break;
			else if (!items[index].m_serial.IsValidUID())
			{
				items[index].m_serial = serial;
				items[index].m_price = item->GetVendorPrice(iConvertFactor,0);
				break;
			}
		}

		items[index].m_vcAmount += amount;
		if (items[index].m_price <= 0)
		{
			vendor->Speak("Alas, I don't have these goods currently stocked. Let me know if there is something else thou wouldst buy.");
			client->Event_VendorBuy_Cheater(0x3);
			return true;
		}
	}

	client->Event_VendorBuy(vendor, items, itemCount);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketStaticUpdate		Ultima live and (God Client?)
 *
 *
 ***************************************************************************/
PacketStaticUpdate::PacketStaticUpdate() : Packet(0)
{
}

bool PacketStaticUpdate::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketStaticUpdate::onReceive");
	/*skip(12);
    byte UlCmd = readByte();*/
	TemporaryString tsDump;
	this->dump(tsDump);
	g_Log.EventDebug("%x:Parsing %s", net->id(), tsDump.buffer());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapEdit			edit map pins
 *
 *
 ***************************************************************************/
PacketMapEdit::PacketMapEdit() : Packet(11)
{
}

bool PacketMapEdit::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketMapEdit::onReceive");

	CUID mapSerial(readInt32());
	MAPCMD_TYPE action = static_cast<MAPCMD_TYPE>(readByte());
	byte pin = readByte();
	word x = readInt16();
	word y = readInt16();

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	CItemMap* map = dynamic_cast<CItemMap*>(mapSerial.ItemFind());
	if (map == nullptr || character->CanTouch(map) == false) // sanity check
	{
		client->SysMessage("You can't reach it");
		return true;
	}

	if (map->m_itMap.m_fPinsGlued)
	{
		client->SysMessage("The pins seem to be glued in place");
		if (client->IsPriv(PRIV_GM) == false)
			return true;
	}

	// NOTE: while in edit mode, right click canceling of the
	// dialog sends the same msg as
	// request to edit in either mode...strange huh?
	switch (action)
	{
		case MAP_ADD: // add pin
		{
			if (map->m_Pins.size() > CItemMap::MAX_PINS)
				return true;

			CMapPinRec mapPin(x, y);
			map->m_Pins.push_back(mapPin);
		} break;

		case MAP_INSERT: // insert between 2 pins
		{
			if (map->m_Pins.size() > CItemMap::MAX_PINS)
				return true;

			CMapPinRec mapPin(x, y);
			map->m_Pins.insert(pin, mapPin);
		} break;

		case MAP_MOVE: // move pin
			if (pin >= map->m_Pins.size())
			{
				client->SysMessage("That's strange... (bad pin)");
				return true;
			}
			map->m_Pins[pin].m_x = x;
			map->m_Pins[pin].m_y = y;
			break;

		case MAP_DELETE: // delete pin
			if (pin >= map->m_Pins.size())
			{
				client->SysMessage("That's strange... (bad pin)");
				return true;
			}
			map->m_Pins.erase_at(pin);
			break;

		case MAP_CLEAR: // clear all pins
			map->m_Pins.clear();
			break;

		case MAP_TOGGLE: // edit req/cancel
			client->addMapMode(map, MAP_SENT, !map->m_fPlotMode);
			break;

		default:
			break;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x5D : PacketCharPlay		character select
 *
 *
 ***************************************************************************/
PacketCharPlay::PacketCharPlay() : Packet(73)
{
}

bool PacketCharPlay::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCharPlay::onReceive");

	skip(4); // 0xedededed
	skip(MAX_NAME_SIZE); // char name
	skip(MAX_NAME_SIZE); // char pass
	uint slot = readInt32();
	skip(4); // ip

	CClient* client = net->getClient();
	if (!client)	//Sometimes seems to happen? returning here to avoid console errors because of assert
		return false;
	//ASSERT(client);

	byte err = client->Setup_Play(slot);

	client->addLoginErr(err);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageEdit	edit book page
 *
 *
 ***************************************************************************/
PacketBookPageEdit::PacketBookPageEdit() : Packet(0)
{
}

bool PacketBookPageEdit::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBookPageEdit::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // packet length
	CUID bookSerial(readInt32());
	word pageCount = readInt16();

	CItem* book = bookSerial.ItemFind();
	if (character->CanSee(book) == false)
	{
		client->addObjectRemoveCantSee(bookSerial, "the book");
		return true;
	}

	word page = readInt16();
	word lineCount = readInt16();
	if (lineCount == 0xFFFF || getLength() <= 0x0D)
	{
		client->addBookPage(book, page, 1); // just a request for a page
		return true;
	}

	// trying to write to the book
	CItemMessage* text = dynamic_cast<CItemMessage*>( book );
	if (text == nullptr || book->IsBookWritable() == false)
		return true;

	skip(-4);

	uint len = 0;
	tchar* content = Str_GetTemp();

	for (ushort i = 0; i < pageCount; ++i)
	{
		// read next page to change with line count
		page = readInt16();
		lineCount = readInt16();

		if (lineCount > 100)	// hard and arbitrary limit, to limit the effectmalicious packets
			break;

		if (page < 1 || page > MAX_BOOK_PAGES || lineCount == 0u)
			continue;

		-- page;
		len = 0;

		// read each line of the page
		while (lineCount > 0)
		{
			len += readStringNullASCII(content + len, SCRIPT_MAX_LINE_LEN-1 - len);
			if (len >= SCRIPT_MAX_LINE_LEN)
			{
				len = SCRIPT_MAX_LINE_LEN - 1;
				break;
			}

			content[len++] = '\t';
			--lineCount;
		}

		ASSERT(len > 0);
		content[--len] = '\0';
		if (Str_Check(content))
			break;

		// set page content
		text->SetPageText(page, content);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketTarget			target object
 *
 *
 ***************************************************************************/
PacketTarget::PacketTarget() : Packet(19)
{
}

bool PacketTarget::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketTarget::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(1); // target type
	dword context = readInt32();
	byte flags = readByte();
	CUID targetSerial(readInt32());
	word x = readInt16();
	word y = readInt16();
	skip(1);
	byte z = readByte();
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt16());

	client->Event_Target(context, targetSerial, CPointMap(x, y, z, character->GetTopMap()), flags, id);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketSecureTradeReq	trade with another character
 *
 *
 ***************************************************************************/
PacketSecureTradeReq::PacketSecureTradeReq() : Packet(0)
{
}

bool PacketSecureTradeReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSecureTradeReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // length
	SECURE_TRADE_TYPE action = static_cast<SECURE_TRADE_TYPE>(readByte());
	CUID containerSerial(readInt32());
	bool fCheck = readInt32();

	CItemContainer* container = dynamic_cast<CItemContainer*>( containerSerial.ItemFind() );
	if (container == nullptr)
		return true;
	else if (character != container->GetParent())
		return true;

	switch ( action )
	{
		case SECURE_TRADE_CLOSE:		// cancel trade. send each person cancel messages, move items
			container->Delete();
			return true;

		case SECURE_TRADE_CHANGE:		// change check marks. possible conclude trade
		{
			if ( character->GetDist(container) > character->GetVisualRange() )
			{
				client->SysMessageDefault(DEFMSG_MSG_TRADE_TOOFAR);
				return true;
			}

			container->Trade_Status(fCheck);
			return true;
		}

		case SECURE_TRADE_UPDATEGOLD:	// update trade window virtual gold
			container->Trade_UpdateGold(readInt32(), readInt32());
			return true;
		default:
			return true;
	}

	//return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoardReq	request bulletin board
 *
 *
 ***************************************************************************/
PacketBulletinBoardReq::PacketBulletinBoardReq() : Packet(0)
{
}

bool PacketBulletinBoardReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBulletinBoardReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	word packetlen = readInt16();
    if (packetlen > 300)
        return false;
	BBOARDF_TYPE action = static_cast<BBOARDF_TYPE>(readByte());
	CUID boardSerial(readInt32());
	CUID messageSerial(readInt32());

	CItemContainer* board = dynamic_cast<CItemContainer*>( boardSerial.ItemFind() );
	if (character->CanSee(board) == false)
	{
		client->addObjectRemoveCantSee(boardSerial, "the board");
		return true;
	}

	if (board->IsType(IT_BBOARD) == false)
		return true;

	switch (action)
	{
		case BBOARDF_REQ_FULL:
		case BBOARDF_REQ_HEAD:
			// request for message header and/or body
			if (getLength() != 0x0c)
			{
				DEBUG_ERR(( "%x:BBoard feed back message bad length %u\n", net->id(), getLength()));
				return true;
			}
			if (client->addBBoardMessage(board, action, messageSerial) == false)
			{
				// sanity check fails
				client->addObjectRemoveCantSee(messageSerial, "the message");
				return true;
			}
			break;

		case BBOARDF_NEW_MSG:
		{
			// submit a message
			if (getLength() < 0x0c)
				return true;

			if (character->CanTouch(board) == false)
			{
				character->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_REACH);
				return true;
			}

			size_t uiContCount = board->GetContentCount();
			if (uiContCount > 32)
			{
				// roll a message off
				CItem *pMsg = static_cast<CItem*>(board->GetContentIndex(uiContCount - 1));
				ASSERT(pMsg);
				pMsg->Delete();
			}

			uint lenstr = readByte();
			tchar* str = Str_GetTemp();
			readStringASCII(str, lenstr, false);
			if (Str_Check(str))
				return true;

			// if
			CItemMessage* newMessage = dynamic_cast<CItemMessage*>( CItem::CreateBase(ITEMID_BBOARD_MSG) );
			if (newMessage == nullptr)
			{
				DEBUG_ERR(("%x:BBoard can't create message item\n", net->id()));
				return true;
			}
			CSTime datetime = CSTime::GetCurrentTime();
			newMessage->SetAttr(ATTR_MOVE_NEVER);
			newMessage->SetName(str);
			newMessage->SetTimeStampS(datetime.GetTime());
			newMessage->m_sAuthor = character->GetName();
			newMessage->m_uidLink = character->GetUID();

			int lines = readByte();
			if (lines > 32)
                lines = 32;

			while (lines--)
			{
				lenstr = readByte();
				readStringASCII(str, lenstr, false);
				if (Str_Check(str) == false)
					newMessage->AddPageText(str);
			}

			board->ContentAdd(newMessage);
			break;
		}

		case BBOARDF_DELETE:
		{
			// remove the message
			CItemMessage* message = dynamic_cast<CItemMessage*>( messageSerial.ItemFind() );
			if (board->IsItemInside(message) == false)
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_COR);
				return true;
			}

			if (client->IsPriv(PRIV_GM) == false && message->m_uidLink != character->GetUID())
			{
				client->SysMessageDefault(DEFMSG_ITEMUSE_BBOARD_DEL);
				return true;
			}

			message->Delete();
			break;
		}

		default:
			DEBUG_ERR(( "%x:BBoard unknown flag %d\n", net->id(), action));
			return true;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarModeReq		toggle war mode
 *
 *
 ***************************************************************************/
PacketWarModeReq::PacketWarModeReq() : Packet(5)
{
}

bool PacketWarModeReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketWarModeReq::onReceive");

	bool war = readBool();
	skip(3); // unknown
	net->getClient()->Event_CombatMode(war);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingReq			ping requests
 *
 *
 ***************************************************************************/
PacketPingReq::PacketPingReq() : Packet(2)
{
}

bool PacketPingReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPingReq::onReceive");

	byte value = readByte();
	new PacketPingAck(net->getClient(), value);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x75 : PacketCharRename		rename character/pet
 *
 *
 ***************************************************************************/
PacketCharRename::PacketCharRename() : Packet(35)
{
}

bool PacketCharRename::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCharRename::onReceive");

	CUID serial(readInt32());
	tchar* name = Str_GetTemp();
	readStringASCII(name, MAX_NAME_SIZE);

	net->getClient()->Event_SetName(serial, name);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x7D : PacketMenuChoice		select menu option
 *
 *
 ***************************************************************************/
PacketMenuChoice::PacketMenuChoice() : Packet(13)
{
}

bool PacketMenuChoice::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketMenuChoice::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	CUID serial(readInt32());
	word context = readInt16();
	word select = readInt16();

	if (context != client->GetTargMode() || serial != client->m_tmMenu.m_UID)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MENU_UNEXPECTED));
		return true;
	}

	client->ClearTargMode();

	switch (context)
	{
		case CLIMODE_MENU:
			// a generic menu from script
			client->Menu_OnSelect(client->m_tmMenu.m_ResourceID, select, serial.ObjFind());
			return true;

		case CLIMODE_MENU_SKILL:
			// some skill menu got us here
			if (select >= ARRAY_COUNT(client->m_tmMenu.m_Item))
				return true;

			client->Cmd_Skill_Menu(client->m_tmMenu.m_ResourceID, (select > 0) ? client->m_tmMenu.m_Item[select] : 0 );
			return true;

		case CLIMODE_MENU_SKILL_TRACK_SETUP:
			// pretracking menu got us here
			client->Cmd_Skill_Tracking(select, false);
			return true;

		case CLIMODE_MENU_SKILL_TRACK:
			// tracking menu got us here. start tracking the selected creature
			client->Cmd_Skill_Tracking(select, true);
			return true;

		case CLIMODE_MENU_EDIT:
			// m_Targ_Text = what are we doing to it
			client->Cmd_EditItem(serial.ObjFind(), select);
			return true;

		default:
			DEBUG_ERR(("%x:Unknown Targetting mode for menu %d\n", net->id(), context));
			return true;
	}
}


/***************************************************************************
 *
 *
 *	Packet 0x80 : PacketServersReq		request server list
 *
 *
 ***************************************************************************/
PacketServersReq::PacketServersReq() : Packet(62)
{
}

bool PacketServersReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketServersReq::onReceive");

	tchar acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, ARRAY_COUNT(acctname));
	tchar acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, ARRAY_COUNT(acctpass));
	skip(1);

	CClient* client = net->getClient();
	ASSERT(client);

	byte lErr = client->Login_ServerList(acctname, acctpass);
	client->addLoginErr(lErr);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0x83 : PacketCharDelete		delete character
 *
 *
 ***************************************************************************/
PacketCharDelete::PacketCharDelete() : Packet(39)
{
}

bool PacketCharDelete::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCharDelete::onReceive");

	skip(MAX_NAME_SIZE); // charpass
	dword slot = readInt32();
	skip(4); // client ip

	CClient* client = net->getClient();
	ASSERT(client);

	byte err = client->Setup_Delete(slot);
	client->addDeleteErr(err,slot);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x8D : PacketCreateNew		create new character request (KR/SA Enhanced Client)
 *
 *
 ***************************************************************************/
PacketCreateNew::PacketCreateNew() : PacketCreate(146)
{
}

bool PacketCreateNew::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCreateNew::onReceive");

	skip(10); // 2=length, 4=pattern1, 4=pattern2
	tchar charname[MAX_NAME_SIZE];
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(30);
	PROFESSION_TYPE profession = static_cast<PROFESSION_TYPE>(readByte());
	byte startloc = readByte();
	byte sex = readByte();
	byte race_raw = readByte();
	if ( net->isClientKR() && (race_raw > 0) )	// SA client sends race packet one higher than KR
		race_raw -= 1;
	RACE_TYPE race = static_cast<RACE_TYPE>(race_raw);
	byte strength = readByte();
	byte dexterity = readByte();
	byte intelligence = readByte();
	HUE_TYPE hue = (HUE_TYPE)(readInt16());
	skip(8);
	SKILL_TYPE skill1 = (SKILL_TYPE)readByte();
	byte skillval1 = readByte();
	SKILL_TYPE skill2 = (SKILL_TYPE)readByte();
	byte skillval2 = readByte();
	SKILL_TYPE skill3 = (SKILL_TYPE)readByte();
	byte skillval3 = readByte();
	SKILL_TYPE skill4 = (SKILL_TYPE)readByte();
	byte skillval4 = readByte();
	skip(26);
	HUE_TYPE hairhue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE hairid = (ITEMID_TYPE)(readInt16());
	skip(6);
	HUE_TYPE shirthue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE shirtid = (ITEMID_TYPE)(readInt16());
	skip(1);
	HUE_TYPE facehue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE faceid = (ITEMID_TYPE)(readInt16());
	skip(1);
	HUE_TYPE beardhue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE beardid = (ITEMID_TYPE)(readInt16());

	// This creation packet does not contain skills and values if
	// a profession is selected, so here we must translate the selected profession -> skills
	switch (profession)
	{
		case PROFESSION_WARRIOR:
			strength = 45;
			dexterity = 35;
			intelligence = 10;
			skill1 = SKILL_SWORDSMANSHIP;
			skillval1 = 30;
			skill2 = SKILL_TACTICS;
			skillval2 = 30;
			skill3 = SKILL_HEALING;
			skillval3 = 30;
			skill4 = SKILL_ANATOMY;
			skillval4 = 30;
			break;
		case PROFESSION_MAGE:
			strength = 25;
			dexterity = 20;
			intelligence = 45;
			skill1 = SKILL_MAGERY;
			skillval1 = 30;
			skill2 = SKILL_EVALINT;
			skillval2 = 30;
			skill3 = SKILL_MEDITATION;
			skillval3 = 30;
			skill4 = SKILL_WRESTLING;
			skillval4 = 30;
			break;
		case PROFESSION_BLACKSMITH:
			strength = 60;
			dexterity = 10;
			intelligence = 10;
			skill1 = SKILL_BLACKSMITHING;
			skillval1 = 30;
			skill2 = SKILL_MINING;
			skillval2 = 30;
			skill3 = SKILL_TINKERING;
			skillval3 = 30;
			skill4 = SKILL_TAILORING;
			skillval4 = 30;
			break;
		case PROFESSION_NECROMANCER:
			strength = 25;
			dexterity = 20;
			intelligence = 45;
			skill1 = SKILL_NECROMANCY;
			skillval1 = 30;
			skill2 = SKILL_SPIRITSPEAK;
			skillval2 = 30;
			skill3 = SKILL_FENCING;
			skillval3 = 30;
			skill4 = SKILL_MEDITATION;
			skillval4 = 30;
			break;
		case PROFESSION_PALADIN:
			strength = 45;
			dexterity = 20;
			intelligence = 25;
			skill1 = SKILL_CHIVALRY;
			skillval1 = 30;
			skill2 = SKILL_SWORDSMANSHIP;
			skillval2 = 30;
			skill3 = SKILL_TACTICS;
			skillval3 = 30;
			skill4 = SKILL_FOCUS;
			skillval4 = 30;
			break;
		case PROFESSION_SAMURAI:
			strength = 40;
			dexterity = 30;
			intelligence = 10;
			skill1 = SKILL_BUSHIDO;
			skillval1 = 30;
			skill2 = SKILL_SWORDSMANSHIP;
			skillval2 = 30;
			skill3 = SKILL_FOCUS;
			skillval3 = 30;
			skill4 = SKILL_PARRYING;
			skillval4 = 30;
			break;
		case PROFESSION_NINJA:
			strength = 40;
			dexterity = 30;
			intelligence = 10;
			skill1 = SKILL_NINJITSU;
			skillval1 = 30;
			skill2 = SKILL_FENCING;
			skillval2 = 30;
			skill3 = SKILL_HIDING;
			skillval3 = 30;
			skill4 = SKILL_STEALTH;
			skillval4 = 30;
			break;
		case PROFESSION_ADVANCED:
			break;
		default:
			DEBUG_WARN(("Unknown profession '%d' selected.\n", profession));
			break;
	}

	bool success = doCreate(net, charname, sex > 0, race,
		strength, dexterity, intelligence, profession,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4,
		hue, hairid, hairhue, beardid, beardhue, shirthue, shirthue, faceid, startloc, UINT32_MAX);
	if (!success)
		return false;

	CChar* pChar = net->getClient()->GetAccount()->m_uidLastChar.CharFind();
	ASSERT(pChar);

	//CItemBase * pItemDef = CItemBase::FindItemBase( shirtid );
	CItem* pItem = CItem::CreateScript( shirtid );
	pItem->SetHue(shirthue);
	pChar->LayerAdd(pItem);

	pItem = CItem::CreateScript( faceid );
	pItem->SetHue(facehue);
	pChar->LayerAdd(pItem);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x91 : PacketCharListReq		request character list
 *
 *
 ***************************************************************************/
PacketCharListReq::PacketCharListReq() : Packet(65)
{
}

bool PacketCharListReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCharListReq::onReceive");

	skip(4);
	tchar acctname[MAX_ACCOUNT_NAME_SIZE];
	readStringASCII(acctname, ARRAY_COUNT(acctname));
	tchar acctpass[MAX_NAME_SIZE];
	readStringASCII(acctpass, ARRAY_COUNT(acctpass));

	net->getClient()->Setup_ListReq(acctname, acctpass, false);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketBookHeaderEdit	edit book header (title/author)
 *
 *
 ***************************************************************************/
PacketBookHeaderEdit::PacketBookHeaderEdit() : Packet(99)
{
}

bool PacketBookHeaderEdit::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBookHeaderEdit::onReceive");

	CUID bookSerial(readInt32());
	skip(1); // writable
	skip(1); // unknown
	skip(2); // pages

	tchar title[2 * MAX_NAME_SIZE];
	readStringASCII(title, ARRAY_COUNT(title));

	tchar author[MAX_NAME_SIZE];
	readStringASCII(author, ARRAY_COUNT(author));

	net->getClient()->Event_Book_Title(bookSerial, title, author);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketDyeObject		colour selection dialog
 *
 *
 ***************************************************************************/
PacketDyeObject::PacketDyeObject() : Packet(9)
{
}

bool PacketDyeObject::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketDyeObject::onReceive");

	CUID serial(readInt32());
	skip(2); // item id
	HUE_TYPE hue = (HUE_TYPE)(readInt16());

	net->getClient()->Event_Item_Dye(serial, hue);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesReq					all names command (ctrl+shift)
 *
 *
 ***************************************************************************/
PacketAllNamesReq::PacketAllNamesReq() : Packet(0)
{
}

bool PacketAllNamesReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAllNamesReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	for (int length = readInt16(); length > (int)sizeof(dword); length -= sizeof(dword))
	{
		const CObjBase* object = CUID::ObjFindFromUID(readInt32());
		if (object == nullptr)
			continue;
		else if (character->CanSee(object) == false)
			continue;

		new PacketAllNamesResponse(client, object);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketPromptResponse	prompt response (ascii)
 *
 *
 ***************************************************************************/
PacketPromptResponse::PacketPromptResponse() : Packet(0)
{
}

bool PacketPromptResponse::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPromptResponse::onReceive");

	uint packetLength = readInt16();
	dword context1 = readInt32();
	dword context2 = readInt32();
	dword type = readInt32();

	if (packetLength < getPosition())
		return false;

	packetLength -= getPosition();
	if (packetLength > MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER;

	tchar* text = Str_GetTemp();
	readStringASCII(text, packetLength, false);

	net->getClient()->Event_PromptResp(text, packetLength, context1, context2, type);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9B : PacketHelpPageReq		GM help page request
 *
 *
 ***************************************************************************/
PacketHelpPageReq::PacketHelpPageReq() : Packet(258)
{
}

bool PacketHelpPageReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHelpPageReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	CScript script("HelpPage");
	character->r_Verb(script, client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0x9F : PacketVendorSellReq	sell item to vendor
 *
 *
 ***************************************************************************/
PacketVendorSellReq::PacketVendorSellReq() : Packet(0)
{
}

bool PacketVendorSellReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketVendorSellReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* seller = client->GetChar();
	if (seller == nullptr)
		return false;

	skip(2); // length
	const CUID vendorSerial(readInt32());
	const uint itemCount = readInt16();

	CChar* vendor = vendorSerial.CharFind();
	if (vendor == nullptr || vendor->m_pNPC == nullptr || !vendor->NPC_IsVendor())
	{
		client->Event_VendorSell_Cheater(0x1); //Both gives same message, but we have to use correct event while we already have.
		return true;
	}

	if (seller->CanTouch(vendor) == false)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_CANTREACH));
		return true;
	}

	if (itemCount < 1)
	{
		client->addVendorClose(vendor);
		return true;
	}
	else if (itemCount >= g_Cfg.m_iContainerMaxItems)
	{
		client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELLMUCH));
		return true;
	}

	// check selling speed
	const CVarDefCont* vardef = g_Cfg.m_fAllowBuySellAgent ? nullptr : client->m_TagDefs.GetKey("BUYSELLTIME");
	if (vardef != nullptr)
	{
		int64 allowsell = vardef->GetValNum() + ((itemCount * 3LL) * MSECS_PER_TENTH);
		if (CWorldGameTime::GetCurrentTime() < allowsell)
		{
			client->SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_NPC_VENDOR_SELLFAST));
			return true;
		}
	}

    VendorItem items[MAX_ITEMS_CONT] = {};
	for (uint i = 0; i < itemCount; ++i)
	{
		items[i].m_serial = CUID(readInt32());
		items[i].m_vcAmount = readInt16();
	}

	client->Event_VendorSell(vendor, items, itemCount);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA0 : PacketServerSelect	play server
 *
 *
 ***************************************************************************/
PacketServerSelect::PacketServerSelect() : Packet(3)
{
}

bool PacketServerSelect::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketServerSelect::onReceive");

	uint server = readInt16();

	net->getClient()->Login_Relay(server);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA4 : PacketSystemInfo		system info from client
 *
 *
 ***************************************************************************/
PacketSystemInfo::PacketSystemInfo() : Packet(149)
{
}

bool PacketSystemInfo::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSystemInfo::onReceive");
	UnreferencedParameter(net);

	skip(148);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xA7 : PacketTipReq			tip request
 *
 *
 ***************************************************************************/
PacketTipReq::PacketTipReq() : Packet(4)
{
}

bool PacketTipReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketTipReq::onReceive");

	word index = readInt16();	// current tip shown to the client
	bool forward = readBool();	// 0=previous, 1=next

	if (forward)
		index++;
	else
		index--;

	CClient* client = net->getClient();
	ASSERT(client);
	client->Event_Tips(index - 1);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xAC : PacketGumpValueInputResponse	gump text input
 *
 *
 ***************************************************************************/
PacketGumpValueInputResponse::PacketGumpValueInputResponse() : Packet(0)
{
}

bool PacketGumpValueInputResponse::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketGumpValueInputResponse::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	skip(2); // length
	CUID uid(readInt32());
	readInt16(); // context
	byte action = readByte();
	word textLength = readInt16();
	tchar text[MAX_NAME_SIZE];
	readStringASCII(text, minimum(MAX_NAME_SIZE, textLength));

	tchar* fix;
	if ((fix = strchr(text, '\n')) != nullptr)
		*fix = '\0';
	if ((fix = strchr(text, '\r')) != nullptr)
		*fix = '\0';
	if ((fix = strchr(text, '\t')) != nullptr)
		*fix = ' ';

	if (client->GetTargMode() != CLIMODE_INPVAL || uid != client->m_Targ_UID)
	{
		client->SysMessage("Unexpected text input");
		return true;
	}

	client->ClearTargMode();

	CObjBase* object = uid.ObjFind();
	if (object == nullptr)
		return true;

	// take action based on the parent context
	if (action == 1) // ok
	{
		// Properties Dialog, page x
		// m_Targ_Text = the verb we are dealing with
		// m_Prop_UID = object we are after

		CScript script(client->m_Targ_Text, text);
		bool ret = object->r_Verb(script, client->GetChar());
		if (ret == false)
		{
			client->SysMessagef("Invalid set: %s = %s", static_cast<lpctstr>(client->m_Targ_Text), static_cast<lpctstr>(text));
		}
		else
		{
			if (client->IsPriv(PRIV_DETAIL))
			{
				client->SysMessagef("Set: %s = %s", static_cast<lpctstr>(client->m_Targ_Text), static_cast<lpctstr>(text));
			}

			object->RemoveFromView(); // weird client thing
			object->Update();
		}

		g_Log.Event(LOGM_GM_CMDS|LOGM_NOCONTEXT, "%x:'%s' tweak uid=0%x (%s) to '%s %s'=%d\n", net->id(), client->GetName(), (dword)(object->GetUID()), object->GetName(), static_cast<lpctstr>(client->m_Targ_Text), static_cast<lpctstr>(text), ret);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xAD : PacketSpeakReqUNICODE				character talking (unicode)
 *
 *
 ***************************************************************************/
PacketSpeakReqUNICODE::PacketSpeakReqUNICODE() : Packet(0)
{
}

bool PacketSpeakReqUNICODE::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSpeakReqUNICODE::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == nullptr)
		return false;

	uint packetLength = readInt16();
	TALKMODE_TYPE mode = static_cast<TALKMODE_TYPE>(readByte());
	HUE_TYPE hue = (HUE_TYPE)(readInt16());
	FONT_TYPE font = (FONT_TYPE)(readInt16());
	tchar language[4];
	readStringASCII(language, ARRAY_COUNT(language));

	if (packetLength < getPosition())
		return false;

	packetLength = (packetLength - getPosition()) / 2;
	if (packetLength >= MAX_TALK_BUFFER)
		packetLength = MAX_TALK_BUFFER - 1;

	if (mode & 0xc0) // text contains keywords
	{
		mode = static_cast<TALKMODE_TYPE>(mode & ~0xc0);

		uint count = (readInt16() & 0xFFF0) >> 4;
		if (count > 50) // malformed check
			return true;

		skip(-2);
		count = (count + 1) * 12;
		uint toskip = count / 8;
		if ((count % 8) > 0)
			toskip++;

		if (toskip > (packetLength * 2))
			return true;

		skip((int)(toskip));
		tchar text[MAX_TALK_BUFFER];
		readStringNullASCII(text, ARRAY_COUNT(text));
		client->Event_Talk(text, hue, mode, true);
	}
	else
	{
		nachar text[MAX_TALK_BUFFER];
		readStringUTF16(reinterpret_cast<wchar *>(text), packetLength, false);
		client->Event_TalkUNICODE(text, (int)(packetLength), hue, mode, font, language);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB1 : PacketGumpDialogRet				dialog button pressed
 *
 *
 ***************************************************************************/
PacketGumpDialogRet::PacketGumpDialogRet() : Packet(0)
{
}

bool PacketGumpDialogRet::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketGumpDialogRet::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // length
	CUID serial(readInt32());
	dword context = readInt32();
	dword button = readInt32();
	dword checkCount = readInt32();
    if (checkCount > MAX_DIALOG_CONTROLTYPE_QTY)
    {
        g_Log.EventError("%x:PacketGumpDialogRet check count too high.\n", net->id());
        return false;
    }

	// relying on the context given by the gump might be a security problem, much like
	// relying on the uid returned.
	// maybe keep a memory for each gump?
	CObjBase* object = serial.ObjFind();

	// Check client internal dialogs first (they must be handled separately because packets can be a bit different on these dialogs)
	if (character == object)
	{
		if (context == CLIMODE_DIALOG_VIRTUE)
		{
			CChar *viewed = character;
			if ((button == 1) && (checkCount > 0))
			{
				viewed = CUID::CharFindFromUID(readInt32());
				if (!viewed)
					viewed = character;
			}
            client->Event_VirtueSelect(button, viewed);
			return true;
		}
		else if (context == CLIMODE_DIALOG_FACESELECTION)
		{
			dword maxID = (g_Cfg.m_iFeatureExtra & FEATURE_EXTRA_ROLEPLAYFACES) ? ITEMID_FACE_VAMPIRE : ITEMID_FACE_10;
			if ((button >= ITEMID_FACE_1) && (button <= maxID))
			{
				CItem *pFace = character->LayerFind(LAYER_FACE);
				if (pFace)
					pFace->Delete();

				pFace = CItem::CreateBase((ITEMID_TYPE)button);
				if (pFace)
				{
                    if (pFace->GetEquipLayer() != LAYER_FACE)
                    {
                        pFace->Delete();
                        return false;
                    }
					pFace->SetHue(character->GetHue());
					character->LayerAdd(pFace, LAYER_FACE);
				}
			}
			return true;
		}
	}

#ifdef _DEBUG
    if (g_Cfg.m_iDebugFlags & DEBUGF_SCRIPTS)
	{
		const CResourceDef* resource = g_Cfg.ResourceGetDef(CResourceID(RES_DIALOG, ResGetIndex(context)));
		if (resource == nullptr)
			g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGM_NOCONTEXT, "[DEBUG_SCRIPTS] Gump context: %x (%s), UID: 0x%x, Button: %u.\n", context, "undefined resource", (dword)serial, button);
		else
		{
			const CDialogDef* dialog = dynamic_cast<const CDialogDef*>(resource);
			if (dialog == nullptr)
				g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGM_NOCONTEXT, "[DEBUG_SCRIPTS] Gump context: %x (%s), UID: 0x%x, Button: %u.\n", context, "undefined dialog", (dword)serial, button);
			else
				g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGM_NOCONTEXT, "[DEBUG_SCRIPTS] Gump context: %x (%s), UID: 0x%x, Button: %u.\n", context, dialog->GetName(), (dword)serial, button);
		}
	}
#endif

	// sanity check
	CClient::OpenedGumpsMap_t::iterator itGumpFound = client->m_mapOpenedGumps.find(context);
	if ((itGumpFound == client->m_mapOpenedGumps.end()) || (itGumpFound->second <= 0))
		return true;

	// Decrement, if <= 0, delete entry.
	-- itGumpFound->second;
	if (itGumpFound->second <= 0)
		client->m_mapOpenedGumps.erase(itGumpFound);

	// package up the gump response info.
	CDialogResponseArgs resp;

	// store the returned checked boxes' ids for possible later use
	for (uint i = 0; i < checkCount; ++i)
		resp.m_CheckArray.push_back(readInt32());


	dword textCount = readInt32();
	textCount = minimum(textCount, THREAD_STRING_LENGTH);
	tchar* text = Str_GetTemp();
	for (uint i = 0; i < textCount; ++i)
	{
		word id = readInt16();
		word length = readInt16();
		length = minimum(length, THREAD_STRING_LENGTH);
		readStringNETUTF16(text, THREAD_STRING_LENGTH, length, false);

		tchar* fix;
		if ((fix = strpbrk(text, "\n\r")) != nullptr)
			*fix = '\0';
		if ((fix = strchr(text, '\t')) != nullptr)
			*fix = ' ';

		resp.AddText(id, text);
	}

	if (net->isClientKR())
		context = g_Cfg.GetKRDialogMap(context);

	CResourceID	ridContext;
    if ((RES_TYPE)ResGetType(context) == RES_DIALOG)
    {
        ridContext = CResourceID(context, 0);
    }
    else
    {
        ridContext = CResourceID(RES_DIALOG, context);
        DEBUG_MSG(("Gump: Received dialog context (%x) without restype from UID=0%x.\n", (uint)context, (uint)character->GetUID().GetObjUID()));
    }
    if (!ridContext.IsValidUID())
    {
        g_Log.EventWarn("Gump: Received wrong dialog context (%x) from UID=0%x.\n", (uint)context, (uint)character->GetUID().GetObjUID());
        return false;
    }
	//
	// Call the scripted response. Lose all the checks and text.
	//
	client->Dialog_OnButton( ridContext, button, object, &resp );
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB3 : PacketChatCommand					chat command
 *
 *
 ***************************************************************************/
PacketChatCommand::PacketChatCommand() : Packet(0)
{
}

bool PacketChatCommand::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketChatCommand::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	uint packetLength = readInt16();
	tchar language[4];
	readStringASCII(language, ARRAY_COUNT(language));

	if (packetLength < getPosition())
		return false;

	uint textLength = (packetLength - getPosition()) / 2;
	if (textLength >= MAX_TALK_BUFFER)
		textLength = MAX_TALK_BUFFER - 1;

	nachar text[MAX_TALK_BUFFER];
	readStringUTF16(reinterpret_cast<wchar *>(text), textLength, false);

	client->Event_ChatText(text, (int)(textLength), CLanguageID(language));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB5 : PacketChatButton					chat button pressed
 *
 *
 ***************************************************************************/
PacketChatButton::PacketChatButton() : Packet(64)
{
}

bool PacketChatButton::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketChatButton::onReceive");
	CClient* client = net->getClient();
	ASSERT(client);

	if (!(g_Cfg.m_iFeatureT2A & FEATURE_T2A_CHAT))
		return true;

	if (client->m_fUseNewChatSystem)
	{
		client->Event_ChatButton();
	}
	else
	{
		// On old chat system, client will always send this packet when click on chat button
		skip(1);	// 0x0
		nachar name[MAX_NAME_SIZE * 2 + 2];
		readStringUTF16(reinterpret_cast<wchar *>(name), ARRAY_COUNT(name));
		client->Event_ChatButton(name);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB6 : PacketToolTipReq					tooltip requested
 *
 *
 ***************************************************************************/
PacketToolTipReq::PacketToolTipReq() : Packet(9)
{
}

bool PacketToolTipReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketToolTipReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CUID serial(readInt32());
	client->Event_ToolTip(serial);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfileReq					character profile requested
 *
 *
 ***************************************************************************/
PacketProfileReq::PacketProfileReq() : Packet(0)
{
}

bool PacketProfileReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketProfileReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	word packetLength = readInt16();
	bool write = readBool();
	CUID serial(readInt32());
	word textLength(0);
	tchar* text(nullptr);

	if (write == true && packetLength > 12)
	{
		skip(1); // unknown
		skip(1); // unknown-return code?

		textLength = readInt16();
		text = Str_GetTemp();
		readStringNETUTF16(text, (uint)Str_TempLength(), textLength+1, false);
	}

	client->Event_Profile(write, serial, text, textLength);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBB : PacketMailMessage					send mail message
 *
 *
 ***************************************************************************/
PacketMailMessage::PacketMailMessage() : Packet(9)
{
}

bool PacketMailMessage::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketMailMessage::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CUID serial1(readInt32());
	CUID serial2(readInt32());

	client->Event_MailMsg(serial1, serial2);
	return true;
}


/***************************************************************************
*
*
*	Packet 0xBD : PacketClientVersion				client version
*
*
***************************************************************************/
PacketClientVersion::PacketClientVersion() : Packet(0)
{
}

bool PacketClientVersion::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketClientVersion::onReceive");
	if (net->getReportedVersion())
	{
		return true;
	}

	word length = readInt16();
	if (length < getPosition())
		return false;

	length -= (word)getPosition();
	if (length > 20)
		length = 20;

	tchar* versionStr = Str_GetTemp();
	readStringASCII(versionStr, length, false);

	if (Str_Check(versionStr))
		return true;

	if (strstr(versionStr, "UO:3D") != nullptr)
		net->m_clientType = CLIENTTYPE_3D;

	length = (word)Str_GetBare(versionStr, versionStr, (int)length, " '`-+!\"#$%&()*,/:;<=>?@[\\]^{|}~");
	if (length > 0)
	{
		CClient* client = net->getClient();
		ASSERT(client);

		dword version = CUOClientVersion(versionStr).GetLegacyVersionNumber();
		net->m_reportedVersionNumber = version;
		net->detectAsyncMode();

		DEBUG_MSG(("Getting CliVersionReported %u\n", version));
		if ((g_Serv.m_ClientVersion.GetClientVerNumber() != 0) && (g_Serv.m_ClientVersion.GetClientVerNumber() != version))
			client->addLoginErr(PacketLoginError::BadVersion);
        //we have asked client version in serverlist to configure character list and game feature.
        if ( client->m_pAccount )
                    client->m_pAccount->m_TagDefs.SetNum("ReportedCliVer", version);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtendedCommand				extended command
 *
 *
 ***************************************************************************/
PacketExtendedCommand::PacketExtendedCommand() : Packet(0)
{
}

bool PacketExtendedCommand::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketExtendedCommand::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	if (client->GetChar() == nullptr)
		return false;

	word packetLength = readInt16();
    if (packetLength > 1000)
        return false;

	EXTDATA_TYPE type = static_cast<EXTDATA_TYPE>(readInt16());
	seek();

	Packet* handler = g_NetworkManager.getPacketManager().getExtendedHandler(type);
	if (handler == nullptr)
		return false;

	handler->seek();
	for (int i = 0; i < packetLength; ++i)
	{
		byte next = readByte();
		handler->writeByte(next);
	}

	handler->resize(packetLength);
	handler->seek(5);
	return handler->onReceive(net);
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x05 : PacketScreenSize				screen size report
 *
 *
 ***************************************************************************/
PacketScreenSize::PacketScreenSize() : Packet(0)
{
}

bool PacketScreenSize::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketScreenSize::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

    skip(2);
    ushort x = readInt16();
    ushort y = readInt16();
    skip(2);

	//DEBUG_MSG(("PacketScreenSize::onReceive 0x%hx - 0x%hx (%hu-%hu)\n", x, y, x, y));
	if (net->isClientVersionNumber(MINCLIVER_NEWBOOK))
	{
		switch (x)
		{
		case 800:
			y = 600;
			break;
		case 1024:
			y = 768;
			break;
		case 1152:
			y = 864;
			break;
		case 1280:
			y = 720;
			break;
		default:
			y = 480;
			break;
		}
	}
	client->SetScreenSize(x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketPartyMessage			party message
 *
 *
 ***************************************************************************/
PacketPartyMessage::PacketPartyMessage() : Packet(0)
{
}

bool PacketPartyMessage::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPartyMessage::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	PARTYMSG_TYPE code = static_cast<PARTYMSG_TYPE>(readByte());
	switch (code)
	{
		case PARTYMSG_Add:
			// request to add a new member
			client->addTarget( CLIMODE_TARG_PARTY_ADD, g_Cfg.GetDefaultMsg(DEFMSG_PARTY_TARG_WHO), false, false);
			break;

		case PARTYMSG_Disband:
			if (character->m_pParty == nullptr)
				return false;

			character->m_pParty->Disband(character->GetUID());
			break;

		case PARTYMSG_Remove:
		{
			// request to remove this member of the party
			if (character->m_pParty == nullptr)
				return false;

			CUID serial(readInt32());
			character->m_pParty->RemoveMember(serial, character->GetUID());
		} break;

		case PARTYMSG_MsgMember:
		{
			// message a specific member of my party
			if (character->m_pParty == nullptr)
				return false;

			CUID serial(readInt32());
			nachar * text = reinterpret_cast<nachar *>(Str_GetTemp());
			int length = (int)readStringNullUTF16(reinterpret_cast<wchar *>(text), MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(serial, character->GetUID(), text, length);
		} break;

		case PARTYMSG_Msg:
		{
			// send message to the whole party
			if (character->m_pParty == nullptr)
				return false;

			nachar * text = reinterpret_cast<nachar *>(Str_GetTemp());
			int length = (int)readStringNullUTF16(reinterpret_cast<wchar *>(text), MAX_TALK_BUFFER);
			character->m_pParty->MessageEvent(CUID(0), character->GetUID(), text, length);
		} break;

		case PARTYMSG_Option:
		{
			// set the loot flag
			if (character->m_pParty == nullptr)
				return false;

			character->m_pParty->SetLootFlag(character, readBool());
			character->NotoSave_Update();
		} break;

		case PARTYMSG_Accept:
		{
			// we accept or decline the offer of an invite
			CUID serial(readInt32());
			CPartyDef::AcceptEvent(character, serial);
		} break;

		case PARTYMSG_Decline:
		{
			CUID serial(readInt32());
			CPartyDef::DeclineEvent(character, serial);
		} break;

		default:
			client->SysMessagef("Unknown party system msg %d", code);
			break;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x07 : PacketArrowClick				click quest arrow
 *
 *
 ***************************************************************************/
PacketArrowClick::PacketArrowClick() : Packet(0)
{
}

bool PacketArrowClick::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketArrowClick::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	bool rightClick = readBool();

	if ( IsTrigUsed(TRIGGER_USERQUESTARROWCLICK) )
	{
		CScriptTriggerArgs Args;
		Args.m_iN1 = (rightClick == true? 1 : 0);
#ifdef _ALPHASPHERE
		Args.m_iN2 = character->GetKeyNum("ARROWQUEST_X", true);
		Args.m_iN3 = character->GetKeyNum("ARROWQUEST_Y", true);
#endif

		if (character->OnTrigger(CTRIG_UserQuestArrowClick, character, &Args) == TRIGRET_RET_TRUE)
			return true;
	}

	client->SysMessageDefault(DEFMSG_MSG_FOLLOW_ARROW);
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x09 : PacketWrestleDisarm			wrestle disarm macro
 *
 *
 ***************************************************************************/
PacketWrestleDisarm::PacketWrestleDisarm() : Packet(0)
{
}

bool PacketWrestleDisarm::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketWrestleDisarm::onReceive");

	net->getClient()->SysMessageDefault(DEFMSG_MSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0A : PacketWrestleStun			wrestle stun macro
 *
 *
 ***************************************************************************/
PacketWrestleStun::PacketWrestleStun() : Packet(0)
{
}

bool PacketWrestleStun::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketWrestleStun::onReceive");

	net->getClient()->SysMessageDefault(DEFMSG_MSG_WRESTLING_NOGO);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0B : PacketLanguage				language report
 *
 *
 ***************************************************************************/
PacketLanguage::PacketLanguage() : Packet(0)
{
}

bool PacketLanguage::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketLanguage::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	tchar language[4];
	readStringNullASCII(language, ARRAY_COUNT(language));

	client->GetAccount()->m_lang.Set(language);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0C : PacketStatusClose			status window closed
 *
 *
 ***************************************************************************/
PacketStatusClose::PacketStatusClose() : Packet(0)
{
}

bool PacketStatusClose::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketStatusClose::onReceive");
	UnreferencedParameter(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0E : PacketAnimationReq			play an animation
 *
 *
 ***************************************************************************/
PacketAnimationReq::PacketAnimationReq() : Packet(0)
{
}

bool PacketAnimationReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAnimationReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	static int validAnimations[] =
	{
		6, 21, 32, 33,
		100, 101, 102,
		103, 104, 105,
		106, 107, 108,
		109, 110, 111,
		112, 113, 114,
		115, 116, 117,
		118, 119, 120,
		121, 123, 124,
		125, 126, 127,
		128
	};

	ANIM_TYPE anim = static_cast<ANIM_TYPE>(readInt32());
	bool ok = false;
	for (uint i = 0; ok == false && i < ARRAY_COUNT(validAnimations); i++)
		ok = (anim == validAnimations[i]);

	if (ok == false)
		return false;

	character->UpdateAnimate(anim);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x0F : PacketClientInfo				client information
 *
 *
 ***************************************************************************/
PacketClientInfo::PacketClientInfo() : Packet(0)
{
}

bool PacketClientInfo::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketClientInfo::onReceive");
	UnreferencedParameter(net);

	skip(1); // 0x0A
	skip(4); // flags
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketAosTooltipInfo			tooltip request (old)
 *
 *
 ***************************************************************************/
PacketAosTooltipInfo::PacketAosTooltipInfo() : Packet(0)
{
}

bool PacketAosTooltipInfo::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAosTooltipInfo::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	if (net->isClientVersionNumber(MINCLIVER_TOOLTIP) == false)
		return true;
	else if (client->GetResDisp() < RDS_AOS || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
		return true;

	CObjBase* object = CUID(readInt32()).ObjFind();
	if (object != nullptr && character->CanSee(object))
		client->addAOSTooltip(object, true);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x13 : PacketPopupReq				request popup menu
 *
 *
 ***************************************************************************/
PacketPopupReq::PacketPopupReq() : Packet(0)
{
}

bool PacketPopupReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPopupReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	if (IsAosFlagEnabled( FEATURE_AOS_POPUP ) && client->GetResDisp() >= RDS_AOS)
	{
		dword serial(readInt32());
		client->Event_AOSPopupMenuRequest(serial);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x15 : PacketPopupSelect			popup menu option selected
 *
 *
 ***************************************************************************/
PacketPopupSelect::PacketPopupSelect() : Packet(0)
{
}

bool PacketPopupSelect::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPopupSelect::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	if (IsAosFlagEnabled( FEATURE_AOS_POPUP ) && client->GetResDisp() >= RDS_AOS)
	{
		dword serial = readInt32();
		word tag = readInt16();

		client->Event_AOSPopupMenuSelect(serial, tag);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1A : PacketChangeStatLock			set stat locks
 *
 *
 ***************************************************************************/
PacketChangeStatLock::PacketChangeStatLock() : Packet(0)
{
}

bool PacketChangeStatLock::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketChangeStatLock::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr || character->m_pPlayer == nullptr)
		return false;

	byte code = readByte();
	SKILLLOCK_TYPE state = static_cast<SKILLLOCK_TYPE>(readByte());

	if (code >= STAT_BASE_QTY)
		return false;
	else if (state < SKILLLOCK_UP || state > SKILLLOCK_LOCK)
		return false;

	// translate UO stat to Sphere stat
	STAT_TYPE stat(STAT_NONE);
	switch (code)
	{
		case 0:
			stat = STAT_STR;
			break;
		case 1:
			stat = STAT_DEX;
			break;
		case 2:
			stat = STAT_INT;
			break;
		default:
			stat = STAT_NONE;
			break;
	}

	if (stat != STAT_NONE)
		character->m_pPlayer->Stat_SetLock(stat, state);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1C : PacketSpellSelect			select/cast spell
 *
 *
 ***************************************************************************/
PacketSpellSelect::PacketSpellSelect() : Packet(0)
{
}

bool PacketSpellSelect::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSpellSelect::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // unknown
	SPELL_TYPE spell = (SPELL_TYPE)(readInt16());
	if (!spell)
		 return false;

	const CSpellDef* spellDef = g_Cfg.GetSpellDef(spell);
	if (spellDef == nullptr)
		return true;

	int skill;
	if (spellDef->GetPrimarySkill(&skill, nullptr) == false)
		return true;
	if ( !character->Skill_CanUse((SKILL_TYPE)skill) )
		return true;

	if (IsSetMagicFlags(MAGICF_PRECAST))
	{
		if (spellDef->IsSpellType(SPELLFLAG_NOPRECAST) == false)
		{
			client->m_tmSkillMagery.m_iSpell = spell;
			character->m_atMagery.m_iSpell = spell;
			client->m_Targ_UID = character->GetUID();
			client->m_Targ_Prv_UID = character->GetUID();
			character->Skill_Start((SKILL_TYPE)skill);
			return true;
		}
	}

	client->Cmd_Skill_Magery(spell, character);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1E : PacketHouseDesignReq			house design request
 *
 *
 ***************************************************************************/
PacketHouseDesignReq::PacketHouseDesignReq() : Packet(0)
{
}

bool PacketHouseDesignReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItem* house = CUID(readInt32()).ItemFind();
	if (house == nullptr)
		return true;

	CItemMultiCustom* multi = dynamic_cast<CItemMultiCustom*>(house);
	if (multi == nullptr)
		return true;

	multi->SendStructureTo(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x24 : PacketAntiCheat				anti-cheat (unknown)
 *
 *
 ***************************************************************************/
PacketAntiCheat::PacketAntiCheat() : Packet(0)
{
}

bool PacketAntiCheat::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAntiCheat::onReceive");
	UnreferencedParameter(net);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x2C : PacketBandageMacro			bandage macro
 *
 *
 ***************************************************************************/
PacketBandageMacro::PacketBandageMacro() : Packet(0)
{
}

bool PacketBandageMacro::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBandageMacro::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
	{
		return false;
	}

    CUID uidBandage(readInt32());
    CUID uidTarget(readInt32());
	CItem* bandage = uidBandage.ItemFind();
	CObjBase* target = uidTarget.ObjFind();
	if (bandage == nullptr || target == nullptr)
	{
		return true;
	}

	// check the client can see the bandage they're trying to use
	if (character->CanSee(bandage) == false)
	{
		client->addObjectRemoveCantSee(uidBandage, "the target");
		return true;
	}

	// check the client is capable of using the bandage
	if (character->CanUse(bandage, false) == false)
	{
		return true;
	}

	// check the bandage is in the possession of the client
	if (bandage->GetTopLevelObj() != character)
	{
		return true;
	}

	// make sure the macro isn't used for other types of items
	if (bandage->IsType(IT_BANDAGE) == false)
	{
		return true;
	}

	// clear previous target
	client->SetTargMode();

	//Should we simulate the dclick?
	client->m_Targ_UID = bandage->GetUID();
	CScriptTriggerArgs extArgs(1); // Signal we're from the macro
	if (bandage->OnTrigger( ITRIG_DCLICK, character, &extArgs ) == TRIGRET_RET_TRUE)
	{
		return true;
	}

	client->SetTargMode();

	// prepare targeting information
	client->m_Targ_UID = uidBandage;
	client->m_tmUseItem.m_pParent = bandage->GetParent();
	client->SetTargMode(CLIMODE_TARG_USE_ITEM);

	client->Event_Target(CLIMODE_TARG_USE_ITEM, uidTarget, target->GetUnkPoint());
	return true;
}

/***************************************************************************
*
*
*	Packet 0xBF.0x2E : PacketTargetedSkill		    use targeted skill
*
*
***************************************************************************/
PacketTargetedSkill::PacketTargetedSkill() : Packet(0)
{
}

bool PacketTargetedSkill::onReceive(CNetState* net)
{
    ADDTOCALLSTACK("PacketTargetedSkill::onReceive");

    word  wSkillID    = readInt16();    // if SkillID = 0, it means that is lastskill
    dword dwTargetUID = readInt32();

    if ((wSkillID != 0) && !CChar::IsSkillBase((SKILL_TYPE)wSkillID))
    {
        DEBUG_MSG(("PacketTargetedSkill::onReceive invalid SkillID=%d.\n", (int)wSkillID));
        return true;
    }

    const CUID uidTarget(dwTargetUID);
    CObjBase * pTarget = uidTarget.ObjFind();
    if (pTarget)
    {
        CClient* pClient = net->getClient();
        ASSERT(pClient);
        CChar* pChar = pClient->GetChar();
        if (pChar == nullptr)
            return true;

        pChar->m_Act_UID = uidTarget;
        pChar->Skill_Start((SKILL_TYPE)wSkillID);
    }
    else
    {
        DEBUG_MSG(("PacketTargetedSkill::onReceive invalid uidTarget=0%x.\n", (uint)dwTargetUID));
    }

    return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x32 : PacketGargoyleFly			gargoyle toggle flying
 *
 *
 ***************************************************************************/
PacketGargoyleFly::PacketGargoyleFly() : Packet(0)
{
}

bool PacketGargoyleFly::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketGargoyleFly::onReceive");

	if ( !(g_Cfg.m_iRacialFlags & RACIALF_GARG_FLY) )
		return false;

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if ( character == nullptr || !character->IsGargoyle() || character->IsStatFlag(STATF_DEAD) )
		return false;

	// The client always send these 2 values to server, but they're not really used
	//word one = readInt16();
	//dword zero = readInt32();

	if ( IsTrigUsed(TRIGGER_TOGGLEFLYING) )
	{
		if ( character->OnTrigger(CTRIG_ToggleFlying,character,0) == TRIGRET_RET_TRUE )
			return false;
	}

	if ( character->IsStatFlag(STATF_HOVERING) )
	{
		// stop hovering
		character->StatFlag_Clear(STATF_HOVERING);
		client->removeBuff(BI_GARGOYLEFLY);
	}
	else
	{
		// begin hovering
		character->StatFlag_Set(STATF_HOVERING);
		client->addBuff(BI_GARGOYLEFLY, 1112193, 1112567);

		// float player up to the hover Z
		CPointMap ptHover = CWorldMap::FindItemTypeNearby(character->GetTopPoint(), IT_HOVEROVER, 0);
		if ( ptHover.IsValidPoint() )
			character->MoveTo(ptHover);
	}

	// Sending this packet here instead of calling UpdateAnimate because of conversions, NANIM_TAKEOFF = 9 and the function
	// is reading 9 from old ANIM_TYPE to know when the character is attacking and modifying its animation accordingly
	PacketActionBasic *cmd = new PacketActionBasic(character, character->IsStatFlag(STATF_HOVERING) ? NANIM_TAKEOFF : NANIM_LANDING, static_cast<ANIM_TYPE_NEW>(0), (byte)(0));
	ClientIterator it;
	for ( CClient *pClient = it.next(); pClient != nullptr; pClient = it.next() )
	{
		if ( !pClient->GetNetState()->isClientVersionNumber(MINCLIVER_NEWMOBILEANIM) )
			continue;
		if ( !pClient->CanSee(character) )
			continue;
		pClient->addCharMove(character);
		cmd->send(pClient);
	}
	delete cmd;
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xBF.0x33 : PacketWheelBoatMove			Move the ship through the mouse wheel
 *
 *
 ***************************************************************************/
PacketWheelBoatMove::PacketWheelBoatMove() : Packet(0)
{
}

bool PacketWheelBoatMove::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketWheelBoatMove::onReceive");

	//UO:HS clients >= 7.0.8f
	//base code below, cleaning needed

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (!character)
		return false;

	skip(4);
	//dword serial = readInt32(); //player serial
	//CUID from(serial &~ UID_F_RESOURCE); //do we need this? CNetState provides the player character

	DIR_TYPE facing = static_cast<DIR_TYPE>(readByte()); //new boat facing, yes client send it
	DIR_TYPE moving = static_cast<DIR_TYPE>(readByte()); //the boat movement
	//skip(1);
	byte bMovementType = readByte(); //(0 = Stop Movement, 1 = One Tile Movement, 2 = Normal Movement) ***These speeds are NOT the same as 0xF6 packet

	CRegionWorld *area = character->m_pArea;
	if (area && area->IsFlag(REGION_FLAG_SHIP))
	{
		CItemShip *pShipItem = dynamic_cast<CItemShip *>(area->GetResourceID().ItemFindFromResource());
		if (pShipItem && (pShipItem->m_itShip.m_Pilot == character->GetUID()))
		{
			//direction of movement = moving - ship_face
			//	moving = read from packet
			//	ship_face = pShipItem->Ship_Face()

			//Ship_* need to be private? there is another way to ask the ship to move?
			//pShipItem->Ship_Move(static_cast<DIR_TYPE>((moving - pShipItem->m_itShip.m_DirFace)), pShipItem->_shipSpeed.tiles);

			if ((facing == DIR_N || facing == DIR_E || facing == DIR_S || facing == DIR_W) && pShipItem->m_itShip.m_DirFace != facing) //boat cannot face intermediate directions
				pShipItem->Face(moving);

			if (pShipItem->SetMoveDir(facing, (ShipMovementType)bMovementType, true)) //pShipItem->m_itShip.m_DirMove = (byte)(facing);
				pShipItem->Move(moving, bMovementType);
		}
		else
			return false;
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xC2 : PacketPromptResponseUnicode		prompt response (unicode)
 *
 *
 ***************************************************************************/
PacketPromptResponseUnicode::PacketPromptResponseUnicode() : Packet(0)
{
}

bool PacketPromptResponseUnicode::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketPromptResponseUnicode::onReceive");

	uint length = readInt16();
	dword context1 = readInt32();
	dword context2 = readInt32();
	dword type = readInt32();
	char language[4];
	readStringASCII(language, ARRAY_COUNT(language));

	if (length < getPosition())
		return false;

	length = (length - getPosition()) / 2;
	tchar* text = Str_GetTemp();
	readStringUTF16(text, THREAD_STRING_LENGTH, length+1);

	net->getClient()->Event_PromptResp(text, length, context1, context2, type);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xC8 : PacketViewRange					change view range
 *
 *
 ***************************************************************************/
PacketViewRange::PacketViewRange() : Packet(2)
{
}

bool PacketViewRange::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketViewRange::onReceive");

	CChar *character = net->getClient()->GetChar();
	if ( !character )
		return false;

	byte iVal = readByte();
	character->SetVisualRange(iVal);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogout			client logout notification
 *
 *
 ***************************************************************************/
PacketLogout::PacketLogout() : Packet(1)
{
}

bool PacketLogout::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketLogout::onReceive");

	new PacketLogoutAck(net->getClient());
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketBookHeaderEditNew	edit book header (title/author)
 *
 *
 ***************************************************************************/
PacketBookHeaderEditNew::PacketBookHeaderEditNew() : Packet(0)
{
}

bool PacketBookHeaderEditNew::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBookHeaderEditNew::onReceive");

	skip(2); // length
	CUID bookSerial(readInt32());
	skip(1); // unknown
	skip(1); // writable
	skip(2); // pages

	tchar title[2 * MAX_NAME_SIZE];
	tchar author[MAX_NAME_SIZE];

	uint titleLength = readInt16();
	readStringASCII(title, minimum(titleLength, ARRAY_COUNT(title)));

	uint authorLength = readInt16();
	readStringASCII(author, minimum(authorLength, ARRAY_COUNT(author)));

	net->getClient()->Event_Book_Title(bookSerial, title, author);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketAOSTooltipReq				aos tooltip request
 *
 *
 ***************************************************************************/
PacketAOSTooltipReq::PacketAOSTooltipReq() : Packet(0)
{
}

bool PacketAOSTooltipReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketAOSTooltipReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	if (net->isClientVersionNumber(MINCLIVER_TOOLTIP) == false)
		return true;
	else if (client->GetResDisp() < RDS_AOS || !IsAosFlagEnabled(FEATURE_AOS_UPDATE_B))
		return true;

    word length = readInt16();
    if (length > 500 * sizeof(dword))
        return false;
	for (; length > sizeof(dword); length -= sizeof(dword))
	{
		CObjBase* object = CUID(readInt32()).ObjFind();
		if (object == nullptr)
			continue;

        bool bShop = false;
        const CItem* pSearchObjItem = dynamic_cast<const CItem*>(object);
        if (pSearchObjItem)
        {
            // Check if this item is shown from a shop gump: for shop items we need to always send the tooltip!
            const CObjBase* pSearchObj;
            while ( (pSearchObj = pSearchObjItem->GetContainer()) != nullptr )
            {
                // Get the top container
                pSearchObjItem = dynamic_cast<const CItem*>(pSearchObj);
                if (!pSearchObjItem)
                    break;

                LAYER_TYPE objContItemLayer = pSearchObjItem->GetEquipLayer();
                if (objContItemLayer >= 26 && objContItemLayer <= 28)
                {
                    // If this container is equipped in the shop layers, it's a shop item
                    bShop = true;
                    break;
                }
            }
        }

		if (bShop)	// shop item
			client->addAOSTooltip(object, true, true);
		else		// char or regular items
		{
			if (character->CanSee(object) == false)
				continue;
			client->addAOSTooltip(object, true, false);
		}
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7 : PacketEncodedCommand				encoded command
 *
 *
 ***************************************************************************/
PacketEncodedCommand::PacketEncodedCommand() : Packet(0)
{
}

bool PacketEncodedCommand::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketEncodedCommand::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	const CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	word packetLength = readInt16();
	if (packetLength > 1000)
		return false;
	CUID serial(readInt32());
	if (character->GetUID() != serial)
		return false;

	EXTAOS_TYPE type = static_cast<EXTAOS_TYPE>(readInt16());
	seek();


	Packet* handler = g_NetworkManager.getPacketManager().getEncodedHandler(type);
	if (handler == nullptr)
		return false;

	handler->seek();
	for (int i = 0; i < packetLength; ++i)
	{
		byte next = readByte();
		handler->writeByte(next);
	}

	handler->resize(packetLength);
	handler->seek(9);
	return handler->onReceive(net);
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x02 : PacketHouseDesignBackup		backup house design
 *
 *
 ***************************************************************************/
PacketHouseDesignBackup::PacketHouseDesignBackup() : Packet(0)
{
}

bool PacketHouseDesignBackup::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignBackup::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->BackupStructure();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x03 : PacketHouseDesignRestore		restore house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRestore::PacketHouseDesignRestore() : Packet(0)
{
}

bool PacketHouseDesignRestore::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignRestore::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->RestoreStructure(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x04 : PacketHouseDesignCommit		commit house design
 *
 *
 ***************************************************************************/
PacketHouseDesignCommit::PacketHouseDesignCommit() : Packet(0)
{
}

bool PacketHouseDesignCommit::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignCommit::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->CommitChanges(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x05 : PacketHouseDesignDestroyItem	    destroy house design item
 *
 *
 ***************************************************************************/
PacketHouseDesignDestroyItem::PacketHouseDesignDestroyItem() : Packet(0)
{
}

bool PacketHouseDesignDestroyItem::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignDestroyItem::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32());
	skip(1); // 0x00
	word x = (word)(readInt32());
	skip(1); // 0x00
	word y = (word)(readInt32());
	skip(1); // 0x00
	word z = (word)(readInt32());

	house->RemoveItem(client, id, x, y, (char)z);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x06 : PacketHouseDesignPlaceItem	place house design item
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceItem::PacketHouseDesignPlaceItem() : Packet(0)
{
}

bool PacketHouseDesignPlaceItem::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceItem::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32());
	skip(1); // 0x00
	word x = (word)(readInt32());
	skip(1); // 0x00
	word y = (word)(readInt32());

	house->AddItem(client, id, x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0C : PacketHouseDesignExit		exit house designer
 *
 *
 ***************************************************************************/
PacketHouseDesignExit::PacketHouseDesignExit() : Packet(0)
{
}

bool PacketHouseDesignExit::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignExit::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->EndCustomize();
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0D : PacketHouseDesignPlaceStair	place house design stairs
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceStair::PacketHouseDesignPlaceStair() : Packet(0)
{
}

bool PacketHouseDesignPlaceStair::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceStair::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32() + ITEMID_MULTI);
	skip(1); // 0x00
	word x = (word)(readInt32());
	skip(1); // 0x00
	word y = (word)(readInt32());

	house->AddStairs(client, id, x, y);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x0E : PacketHouseDesignSync		synchronise house design
 *
 *
 ***************************************************************************/
PacketHouseDesignSync::PacketHouseDesignSync() : Packet(0)
{
}

bool PacketHouseDesignSync::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignSync::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->SendStructureTo(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x10 : PacketHouseDesignClear		clear house design
 *
 *
 ***************************************************************************/
PacketHouseDesignClear::PacketHouseDesignClear() : Packet(0)
{
}

bool PacketHouseDesignClear::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignClear::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->ResetStructure(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x12 : PacketHouseDesignSwitch		switch house design floor
 *
 *
 ***************************************************************************/
PacketHouseDesignSwitch::PacketHouseDesignSwitch() : Packet(0)
{
}

bool PacketHouseDesignSwitch::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignSwitch::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	dword level = readInt32();

	house->SwitchToLevel(client, (uchar)(level));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x13 : PacketHouseDesignPlaceRoof	place house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignPlaceRoof::PacketHouseDesignPlaceRoof() : Packet(0)
{
}

bool PacketHouseDesignPlaceRoof::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignPlaceRoof::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32());
	skip(1); // 0x00
	word x = (word)(readInt32());
	skip(1); // 0x00
	word y = (word)(readInt32());
	skip(1); // 0x00
	word z = (word)(readInt32());

	house->AddRoof(client, id, x, y, (char)(z));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x14 : PacketHouseDesignDestroyRoof	destroy house design roof
 *
 *
 ***************************************************************************/
PacketHouseDesignDestroyRoof::PacketHouseDesignDestroyRoof() : Packet(0)
{
}

bool PacketHouseDesignDestroyRoof::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignDestroyRoof::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	skip(1); // 0x00
	ITEMID_TYPE id = (ITEMID_TYPE)(readInt32());
	skip(1); // 0x00
	word x = (word)(readInt32());
	skip(1); // 0x00
	word y = (word)(readInt32());
	skip(1); // 0x00
	word z = (word)(readInt32());

	house->RemoveRoof(client, id, x, y, (char)(z));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x19 : PacketSpecialMove			perform special move
 *
 *
 ***************************************************************************/
PacketSpecialMove::PacketSpecialMove() : Packet(0)
{
}

bool PacketSpecialMove::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketSpecialMove::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(1);
	dword ability = readInt32();

	if ( IsTrigUsed(TRIGGER_USERSPECIALMOVE) )
	{
		CScriptTriggerArgs args;
		args.m_iN1 = ability;
		character->OnTrigger(CTRIG_UserSpecialMove, character, &args);
	}
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1A : PacketHouseDesignRevert		revert house design
 *
 *
 ***************************************************************************/
PacketHouseDesignRevert::PacketHouseDesignRevert() : Packet(0)
{
}

bool PacketHouseDesignRevert::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHouseDesignRevert::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	CItemMultiCustom* house = client->m_pHouseDesign;
	if (house == nullptr)
		return true;

	house->RevertChanges(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x1E : PacketEquipLastWeapon		equip last weapon macro
 *
 *
 ***************************************************************************/
PacketEquipLastWeapon::PacketEquipLastWeapon() : Packet(0)
{
}

bool PacketEquipLastWeapon::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketEquipLastWeapon::onReceive");

    CClient *pClient = net->getClient();
    ASSERT(pClient);
    CChar *pChar = pClient->GetChar();
    if ( !pChar )
        return false;
    CCharPlayer* pCharPlayer = pChar->m_pPlayer;
    if ( !pCharPlayer )
        return false;

    if (!pCharPlayer->m_uidWeaponLast.IsValidUID())
        return false;
    if ( pCharPlayer->m_uidWeaponLast == pChar->m_uidWeapon )
        return true;

    CItem *pWeapon = pCharPlayer->m_uidWeaponLast.ItemFind();
    if (!pWeapon)
        return false;
    if (pChar->ItemPickup(pWeapon, 1) == -1)
        return true;

    pChar->ItemEquip(pWeapon);

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x28 : PacketGuildButton			guild button pressed
 *
 *
 ***************************************************************************/
PacketGuildButton::PacketGuildButton() : Packet(0)
{
}

bool PacketGuildButton::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketGuildButton::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	if ( IsTrigUsed(TRIGGER_USERGUILDBUTTON) )
		character->OnTrigger(CTRIG_UserGuildButton, character, nullptr);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD7.0x32 : PacketQuestButton			quest button pressed
 *
 *
 ***************************************************************************/
PacketQuestButton::PacketQuestButton() : Packet(0)
{
}

bool PacketQuestButton::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketQuestButton::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	if ( IsTrigUsed(TRIGGER_USERQUESTBUTTON) )
		character->OnTrigger(CTRIG_UserQuestButton, character, nullptr);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xD9 : PacketHardwareInfo	hardware info from client
 *
 *
 ***************************************************************************/
PacketHardwareInfo::PacketHardwareInfo() : Packet(268)
{
}

bool PacketHardwareInfo::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketHardwareInfo::onReceive");
	UnreferencedParameter(net);

	skip(1); // client type
	skip(4); // instance id
	skip(4); // os major
	skip(4); // os minor
	skip(4); // os revision
	skip(1); // cpu manufacturer
	skip(4); // cpu family
	skip(4); // cpu model
	skip(4); // cpu clock speed
	skip(1); // cpu quantity
	skip(4); // physical memory
	skip(4); // screen width
	skip(4); // screen height
	skip(4); // screen depth
	skip(2); // directx major
	skip(2); // directx minor
	skip(64 * sizeof(wchar)); // video card description
	skip(4); // video card vendor id
	skip(4); // video card device id
	skip(4); // video card memory
	skip(1); // distribution
	skip(1); // clients running
	skip(1); // clients installed
	skip(1); // clients partial installed
	skip(4 * sizeof(wchar)); // language
	skip(32 * sizeof(wchar)); // unknown
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE0 : PacketBugReport					bug report
 *
 *
 ***************************************************************************/
PacketBugReport::PacketBugReport() : Packet(0)
{
}

bool PacketBugReport::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketBugReport::onReceive");

	word packetLength = readInt16(); // packet length
	if (packetLength < 10)
		return false;

	tchar language[4];
	readStringASCII(language, ARRAY_COUNT(language));

	BUGREPORT_TYPE type = static_cast<BUGREPORT_TYPE>(readInt16());

	tchar text[MAX_TALK_BUFFER];
	int textLength = (int)readStringNullNETUTF16(text, MAX_TALK_BUFFER, MAX_TALK_BUFFER-1);

	net->getClient()->Event_BugReport(text, textLength, type, CLanguageID(language));
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE1 : PacketClientType		client type (KR/SA)
 *
 *
 ***************************************************************************/
PacketClientType::PacketClientType() : Packet(0)
{
}

bool PacketClientType::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketClientType::onReceive");

	word packetLength = readInt16(); // packet length
	if (packetLength < 9)
		return false;

	skip(2); // ..count?
	GAMECLIENT_TYPE type = static_cast<GAMECLIENT_TYPE>(readInt32());

	net->m_clientType = type;
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xE8 : PacketRemoveUIHighlight			remove ui highlight
 *
 *
 ***************************************************************************/
PacketRemoveUIHighlight::PacketRemoveUIHighlight() : Packet(13)
{
}

bool PacketRemoveUIHighlight::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketRemoveUIHighlight::onReceive");
	UnreferencedParameter(net);

	skip(4); // serial
	skip(2); // ui id
	skip(4); // destination serial
	skip(1); // 1
	skip(1); // 1
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xEB : PacketUseHotbar					use hotbar
 *
 *
 ***************************************************************************/
PacketUseHotbar::PacketUseHotbar() : Packet(11)
{
}

bool PacketUseHotbar::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketUseHotbar::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // 1
	skip(2); // 6
	byte type = readByte();
	skip(1); // zero
	dword parameter = readInt32();

	client->Event_UseToolbar(type, parameter);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xEC : PacketEquipItemMacro				equip item(s) macro (KR)
 *
 *
 ***************************************************************************/
PacketEquipItemMacro::PacketEquipItemMacro() : Packet(0)
{
}

bool PacketEquipItemMacro::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketEquipItemMacro::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // packet length
	byte itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	CItem* item;
	for (byte i = 0; i < itemCount; i++)
	{
		item = CUID(readInt32()).ItemFind();
		if (item == nullptr)
			continue;

		if (item->GetTopLevelObj() != character || item->IsItemEquipped())
			continue;

		if (character->ItemPickup(item, item->GetAmount()) < 1)
			continue;

		character->ItemEquip(item);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xED : PacketUnEquipItemMacro			unequip item(s) macro (KR)
 *
 *
 ***************************************************************************/
PacketUnEquipItemMacro::PacketUnEquipItemMacro() : Packet(0)
{
}

bool PacketUnEquipItemMacro::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketUnEquipItemMacro::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);
	CChar* character = client->GetChar();
	if (character == nullptr)
		return false;

	skip(2); // packet length
	byte itemCount = readByte();
	if ( itemCount > 3 )	// prevent packet exploit sending fake values just to create heavy loops and overload server CPU
		itemCount = 3;

	LAYER_TYPE layer;
	CItem* item;
	for (byte i = 0; i < itemCount; i++)
	{
		layer = static_cast<LAYER_TYPE>(readInt16());

		item = character->LayerFind(layer);
		if (item == nullptr)
			continue;

		if (character->ItemPickup(item, item->GetAmount()) < 1)
			continue;

		character->ItemBounce(item);
	}

	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xF0 : PacketMovementReqNew			new movement request (KR/SA)
 *
 *
 ***************************************************************************/
PacketMovementReqNew::PacketMovementReqNew() : Packet(0)
{
}

bool PacketMovementReqNew::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketMovementReqNew::onReceive");
	// New walk packet used on newest clients (still incomplete)
	// It must be enabled using login flags on packet 0xA9, otherwise the client will
	// stay using the old walk packet 0x02.

	// The 'time' values here are used by fastwalk prevention, and linked somehow to
	// time sync packets 0xF1 (client request) / 0xF2 (server response) but I have no
	// idea how it works. The client request an time resync at every 60 seconds.
	// Anyway, these values are not in use because Sphere already have another fastwalk
	// detection engine from the old packet 0x02 (PacketMovementReq).

    // This packet (0xF0) is used by the Krrios client as a special login packet.
	// Also classic clients using Injection 2014 will strangely send this packet to server when the player press the 'Chat' button.

	if ( !(g_Cfg.m_iFeatureSA & FEATURE_SA_MOVEMENT) )
		return false;

	CClient *client = net->getClient();
	ASSERT(client);

	word packetlen = readInt16();
    if (getLength() != packetlen)
        return false;   // It's not a valid PacketMovementReqNew, maybe it's a Krrios or Injection packet?

	byte steps = readByte();
	while ( steps )
	{
		skip(8);	//int64 iTime1 = readInt64();
		skip(8);	//int64 iTime2 = readInt64();
		byte sequence = readByte();
		byte direction = readByte();
		dword mode = readInt32();	// 1 = walk, 2 = run
		if ( mode == 2 )
			direction |= DIR_MASK_RUNNING;

		// The client send these values, but they're not really needed
		//dword x = readInt32();
		//dword y = readInt32();
		//dword z = readInt32();
		skip(12);

		if ( !client->Event_Walk(direction, sequence) )
		{
			net->m_sequence = 0;
			break;
		}

		--steps;
	}
	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xF1 : PacketTimeSyncRequest				time sync request (KR/SA)
 *
 *
 ***************************************************************************/
PacketTimeSyncRequest::PacketTimeSyncRequest() : Packet(9)
{
}

bool PacketTimeSyncRequest::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketTimeSyncRequest::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	//int64 iTime = readInt64();	// what we must do with this value?
	new PacketTimeSyncResponse(client);
	return true;
}


/***************************************************************************
 *
 *
 *	Packet 0xF4 : PacketCrashReport					crash report
 *
 *
 ***************************************************************************/
PacketCrashReport::PacketCrashReport() : Packet(0)
{
}

bool PacketCrashReport::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCrashReport::onReceive");

	skip(2); // packet length
	byte versionMaj = readByte();
	byte versionMin = readByte();
	byte versionRev = readByte();
	byte versionPat = readByte();
	word x = readInt16();
	word y = readInt16();
	byte z = readByte();
	byte map = readByte();
	skip(32); // account name
	skip(32); // character name
	skip(15); // ip address
	skip(4); // unknown
	dword errorCode = readInt32();
	tchar executable[100];
	readStringASCII(executable, ARRAY_COUNT(executable));
	tchar description[100];
	readStringASCII(description, ARRAY_COUNT(description));
	skip(1); // zero
	dword errorOffset = readInt32();

    lpctstr ptcAcctName = "none";
    const CClient *pClient = net->getClient();
    if (pClient)
    {
        if (const CAccount * pAccount = pClient->GetAccount())
            ptcAcctName = pAccount->GetName();
    }
	g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN|LOGM_NOCONTEXT, "%x:Client crashed. Account: '%s'. Data from Crash Report packet:\n", net->id(), ptcAcctName);
    g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN|LOGM_NOCONTEXT, "P=%d,%d,%d,%d, ErrorCode=0x%08X, Description='%s', Offset=0x%08X, ClientExe='%s', ClientVer=%d.%d.%d.%d\n",
        x, y, z, map, errorCode, description, errorOffset, executable, versionMaj, versionMin, versionRev, versionPat);
    if (pClient)
    {
        if (const CChar *pChar = pClient->GetChar())
        {
            CPointMap const& ptChar = pChar->GetTopPoint();
            g_Log.Event(LOGM_CLIENTS_LOG|LOGL_WARN|LOGM_NOCONTEXT, "Char attached. Last server P=%d,%d,%d,%d\n", ptChar.m_x, ptChar.m_y, ptChar.m_z, ptChar.m_map);
        }
    }

	return true;
}

/***************************************************************************
 *
 *
 *	Packet 0xF8 : PacketCreateHS					create new character request (only by CC 7.0.16+)
 *
 *
 ***************************************************************************/
PacketCreateHS::PacketCreateHS() : PacketCreate(106)
{
}

bool PacketCreateHS::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketCreateHS::onReceive");
	// standard character creation packet, but with 4 skills and different handling of race and sex.

	tchar charname[MAX_NAME_SIZE];
	SKILL_TYPE skill1 = SKILL_NONE, skill2 = SKILL_NONE, skill3 = SKILL_NONE, skill4 = SKILL_NONE;
	byte skillval1 = 0, skillval2 = 0, skillval3 = 0, skillval4 = 0;

	skip(9); // 4=pattern1, 4=pattern2, 1=kuoc
	readStringASCII(charname, MAX_NAME_SIZE);
	skip(2); // 0x00
	dword flags = readInt32();
	skip(8); // unk
	PROFESSION_TYPE prof = static_cast<PROFESSION_TYPE>(readByte());
	skip(15); // 0x00
	byte race_sex_flag = readByte();
	byte strength = readByte();
	byte dexterity = readByte();
	byte intelligence = readByte();
	skill1 = (SKILL_TYPE)readByte();
	skillval1 = readByte();
	skill2 = (SKILL_TYPE)readByte();
	skillval2 = readByte();
	skill3 = (SKILL_TYPE)readByte();
	skillval3 = readByte();
	skill4 = (SKILL_TYPE)readByte();
	skillval4 = readByte();
	HUE_TYPE hue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE hairid = (ITEMID_TYPE)(readInt16());
	HUE_TYPE hairhue = (HUE_TYPE)(readInt16());
	ITEMID_TYPE beardid = (ITEMID_TYPE)(readInt16());
	HUE_TYPE beardhue = (HUE_TYPE)(readInt16());
	skip(1); // shard index
	byte startloc = readByte();
	skip(8); // 4=slot, 4=ip
	HUE_TYPE shirthue = (HUE_TYPE)(readInt16());
	HUE_TYPE pantshue = (HUE_TYPE)(readInt16());

	// convert race_sex_flag: determine which race and sex the client has selected
	bool isFemale = (race_sex_flag % 2) != 0;	// Even=Male, Odd=Female (rule applies to all clients)
	RACE_TYPE rtRace = RACETYPE_HUMAN;			// Human
	/*
	race_sex_flag values since Classic Client 7.0.16.0
	0x2 = Human (male)
	0x3 = Human (female)
	0x4 = Elf (male)
	0x5 = Elf (female)
	0x6 = Gargoyle (male)
	0x7 = Gargoyle (female)
	*/
	switch (race_sex_flag)
	{
	case 0x2: case 0x3:
		rtRace = RACETYPE_HUMAN;
		break;
	case 0x4: case 0x5:
		rtRace = RACETYPE_ELF;
		break;
	case 0x6: case 0x7:
		rtRace = RACETYPE_GARGOYLE;
		break;
	default:
		g_Log.Event(LOGL_WARN | LOGM_NOCONTEXT, "Creating new character (client > 7.0.16.0 packet) with unknown race_sex_flag (%" PRIu8 "): defaulting to 2 (human male).\n", race_sex_flag);
	}

	return doCreate(net, charname, isFemale, rtRace,
		strength, dexterity, intelligence, prof,
		skill1, skillval1, skill2, skillval2, skill3, skillval3, skill4, skillval4,
		hue, hairid, hairhue, beardid, beardhue, shirthue, pantshue, ITEMID_NOTHING, startloc, flags);
}

/***************************************************************************
 *
 *
 *	Packet 0xF9 : PacketGlobalChatReq				global chat (INCOMPLETE)
 *
 *
 ***************************************************************************/
PacketGlobalChatReq::PacketGlobalChatReq() : Packet(0)
{
}

bool PacketGlobalChatReq::onReceive(CNetState* net)
{
	ADDTOCALLSTACK("PacketGlobalChatReq::onReceive");

	CClient* client = net->getClient();
	ASSERT(client);

	if (!(g_Cfg.m_iChatFlags & CHATF_GLOBALCHAT))
	{
		client->SysMessage("Global Chat is currently unavailable.");
		return true;
	}

	readByte();
	byte action = readByte();
	skip(1);

	tchar xml[MAX_TALK_BUFFER * 2];
	readStringASCII(xml, ARRAY_COUNT(xml));
	//DEBUG_ERR(("GlobalChat XML received: %s\n", xml));

	switch (action)
	{
	case PacketGlobalChat::MessageSend:
		// TO-DO
		return true;
	case PacketGlobalChat::FriendRemove:
		// TO-DO
		return true;
	case PacketGlobalChat::FriendAddTarg:
		client->addTarget(CLIMODE_TARG_GLOBALCHAT_ADD, "Target player to request as Global Chat friend.");
		return true;
	case PacketGlobalChat::StatusToggle:
		client->addGlobalChatStatusToggle();
		return true;
	default:
		DEBUG_ERR(("%x:Unknown global chat action 0%d\n", net->id(), action));
		return true;
	}
}

/***************************************************************************
*
*
*	Packet 0xFA : PacketUltimaStoreButton			ultima store button pressed (SA)
*
*
***************************************************************************/
PacketUltimaStoreButton::PacketUltimaStoreButton() : Packet(1)
{
}

bool PacketUltimaStoreButton::onReceive(CNetState *net)
{
    ADDTOCALLSTACK("PacketUltimaStoreButton::onReceive");

    CClient *client = net->getClient();
    ASSERT(client);
    CChar *character = client->GetChar();
    if (!character)
        return false;

    if (IsTrigUsed(TRIGGER_USERULTIMASTOREBUTTON))
        character->OnTrigger(CTRIG_UserUltimaStoreButton, character, nullptr);
    return true;
}



/***************************************************************************
*
*
*	Packet 0xFB : PacketPublicHouseContent			show/hide public house content (SA)
*
*
***************************************************************************/
PacketPublicHouseContent::PacketPublicHouseContent() : Packet(2)
{
}

bool PacketPublicHouseContent::onReceive(CNetState* net)
{
    ADDTOCALLSTACK("PacketPublicHouseContent::onReceive");

    CClient* client = net->getClient();
    ASSERT(client);

    client->_fShowPublicHouseContent = readBool();

    return true;
}
