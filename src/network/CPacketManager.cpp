#include "../sphere/threads.h"
#include "packet.h"
#include "receive.h"
#include "CPacketManager.h"
#include <cstring>


PacketManager::PacketManager(void)
{
    memset(m_handlers, 0, sizeof(m_handlers));
    memset(m_extended, 0, sizeof(m_extended));
    memset(m_encoded, 0, sizeof(m_encoded));
}

PacketManager::~PacketManager(void)
{
    // delete standard packet handlers
    for (size_t i = 0; i < ARRAY_COUNT(m_handlers); ++i)
        unregisterPacket((uint)i);

    // delete extended packet handlers
    for (size_t i = 0; i < ARRAY_COUNT(m_extended); ++i)
        unregisterExtended((uint)i);

    // delete encoded packet handlers
    for (size_t i = 0; i < ARRAY_COUNT(m_encoded); ++i)
        unregisterEncoded((uint)i);
}

void PacketManager::registerStandardPackets(void)
{
    ADDTOCALLSTACK("PacketManager::registerStandardPackets");

    // standard packets
    registerPacket(XCMD_Create, new PacketCreate());							// create character
    registerPacket(XCMD_WalkRequest, new PacketMovementReq());					// movement request
    registerPacket(XCMD_Talk, new PacketSpeakReq());							// speak
    registerPacket(XCMD_Attack, new PacketAttackReq());							// begin attack
    registerPacket(XCMD_DClick, new PacketDoubleClick());						// double click object
    registerPacket(XCMD_ItemPickupReq, new PacketItemPickupReq());				// pick up item
    registerPacket(XCMD_ItemDropReq, new PacketItemDropReq());					// drop item
    registerPacket(XCMD_Click, new PacketSingleClick());						// single click object
    registerPacket(XCMD_ExtCmd, new PacketTextCommand());						// extended text command
    registerPacket(XCMD_ItemEquipReq, new PacketItemEquipReq());				// equip item
    registerPacket(XCMD_WalkAck, new PacketResynchronize());					//
    registerPacket(XCMD_DeathMenu, new PacketDeathStatus());					//
    registerPacket(XCMD_CharStatReq, new PacketObjStatusReq());				// status request
    registerPacket(XCMD_Skill, new PacketSkillLockChange());					// change skill lock
    registerPacket(XCMD_VendorBuy, new PacketVendorBuyReq());					// buy items from vendor
    registerPacket(XCMD_StaticUpdate, new PacketStaticUpdate());				// UltimaLive Packet
    registerPacket(XCMD_MapEdit, new PacketMapEdit());							// edit map pins
    registerPacket(XCMD_CharPlay, new PacketCharPlay());						// select character
    registerPacket(XCMD_BookPage, new PacketBookPageEdit());					// edit book content
    registerPacket(XCMD_Options, new PacketUnknown());							// unused options packet
    registerPacket(XCMD_Target, new PacketTarget());							// target an object
    registerPacket(XCMD_SecureTrade, new PacketSecureTradeReq());				// secure trade action
    registerPacket(XCMD_BBoard, new PacketBulletinBoardReq());					// bulletin board action
    registerPacket(XCMD_War, new PacketWarModeReq());							// toggle war mode
    registerPacket(XCMD_Ping, new PacketPingReq());								// ping request
    registerPacket(XCMD_CharName, new PacketCharRename());						// change character name
    registerPacket(XCMD_MenuChoice, new PacketMenuChoice());					// select menu item
    registerPacket(XCMD_ServersReq, new PacketServersReq());					// request server list
    registerPacket(XCMD_CharDelete, new PacketCharDelete());					// delete character
    registerPacket(XCMD_CreateNew, new PacketCreateNew());						// create character (KR/SA)
    registerPacket(XCMD_CharListReq, new PacketCharListReq());					// request character list
    registerPacket(XCMD_BookOpen, new PacketBookHeaderEdit());					// edit book
    registerPacket(XCMD_DyeVat, new PacketDyeObject());							// colour selection dialog
    registerPacket(XCMD_AllNames3D, new PacketAllNamesReq());					// all names macro (ctrl+shift)
    registerPacket(XCMD_Prompt, new PacketPromptResponse());					// prompt response
    registerPacket(XCMD_HelpPage, new PacketHelpPageReq());						// GM help page request
    registerPacket(XCMD_VendorSell, new PacketVendorSellReq());					// sell items to vendor
    registerPacket(XCMD_ServerSelect, new PacketServerSelect());				// select server
    registerPacket(XCMD_Spy, new PacketSystemInfo());							//
    registerPacket(XCMD_Scroll, new PacketUnknown(5));							// scroll closed
    registerPacket(XCMD_TipReq, new PacketTipReq());							// request tip of the day
    registerPacket(XCMD_GumpInpValRet, new PacketGumpValueInputResponse());		// text input dialog
    registerPacket(XCMD_TalkUNICODE, new PacketSpeakReqUNICODE());				// speech (unicode)
    registerPacket(XCMD_GumpDialogRet, new PacketGumpDialogRet());				// dialog response (button press)
    registerPacket(XCMD_ChatText, new PacketChatCommand());						// chat command
    registerPacket(XCMD_Chat, new PacketChatButton());							// chat button
    registerPacket(XCMD_ToolTipReq, new PacketToolTipReq());					// popup help request
    registerPacket(XCMD_CharProfile, new PacketProfileReq());					// profile read/write request
    registerPacket(XCMD_MailMsg, new PacketMailMessage());						//
    registerPacket(XCMD_ClientVersion, new PacketClientVersion());				// client version
    registerPacket(XCMD_AssistVersion, new PacketAssistVersion());              // assist version
    registerPacket(XCMD_ExtData, new PacketExtendedCommand());					//
    registerPacket(XCMD_PromptUNICODE, new PacketPromptResponseUnicode());		// prompt response (unicode)
    registerPacket(XCMD_ViewRange, new PacketViewRange());						//
    registerPacket(XCMD_ConfigFile, new PacketUnknown());						//
    registerPacket(XCMD_LogoutStatus, new PacketLogout());						//
    registerPacket(XCMD_AOSBookPage, new PacketBookHeaderEditNew());			// edit book
    registerPacket(XCMD_AOSTooltip, new PacketAOSTooltipReq());					// request tooltip data
    registerPacket(XCMD_ExtAosData, new PacketEncodedCommand());				//
    registerPacket(XCMD_Spy2, new PacketHardwareInfo());						// client hardware info
    registerPacket(XCMD_BugReport, new PacketBugReport());						// bug report
    registerPacket(XCMD_KRClientType, new PacketClientType());					// report client type (2d/kr/sa)
    registerPacket(XCMD_HighlightUIRemove, new PacketRemoveUIHighlight());		//
    registerPacket(XCMD_UseHotbar, new PacketUseHotbar());						//
    registerPacket(XCMD_MacroEquipItem, new PacketEquipItemMacro());			// equip item(s) macro (KR)
    registerPacket(XCMD_MacroUnEquipItem, new PacketUnEquipItemMacro());		// unequip item(s) macro (KR)
    registerPacket(XCMD_WalkRequestNew, new PacketMovementReqNew());			// new movement request (KR/SA)
    registerPacket(XCMD_TimeSyncRequest, new PacketTimeSyncRequest());			// time sync request (KR/SA)
    registerPacket(XCMD_CrashReport, new PacketCrashReport());					//
    registerPacket(XCMD_CreateHS, new PacketCreateHS());						// create character (HS)
    registerPacket(XCMD_UltimaStoreButton, new PacketUltimaStoreButton());		// ultima store button (SA)
    registerPacket(XCMD_GlobalChat, new PacketGlobalChatReq());                 // global chat
    registerPacket(XCMD_PublicHouseContent, new PacketPublicHouseContent());    // show/hide public house content (SA)

    // extended packets (0xBF)
    registerExtended(EXTDATA_ScreenSize, new PacketScreenSize());				// client screen size
    registerExtended(EXTDATA_Party_Msg, new PacketPartyMessage());				// party command
    registerExtended(EXTDATA_Arrow_Click, new PacketArrowClick());				// click quest arrow
    registerExtended(EXTDATA_Wrestle_DisArm, new PacketWrestleDisarm());		// wrestling disarm macro
    registerExtended(EXTDATA_Wrestle_Stun, new PacketWrestleStun());			// wrestling stun macro
    registerExtended(EXTDATA_Lang, new PacketLanguage());						// client language
    registerExtended(EXTDATA_StatusClose, new PacketStatusClose());				// status window closed
    registerExtended(EXTDATA_Yawn, new PacketAnimationReq());					// play animation
    registerExtended(EXTDATA_Unk15, new PacketClientInfo());					// client information
    registerExtended(EXTDATA_OldAOSTooltipInfo, new PacketAosTooltipInfo());	//
    registerExtended(EXTDATA_Popup_Request, new PacketPopupReq());				// request popup menu
    registerExtended(EXTDATA_Popup_Select, new PacketPopupSelect());			// select popup option
    registerExtended(EXTDATA_Stats_Change, new PacketChangeStatLock());			// change stat lock
    registerExtended(EXTDATA_NewSpellSelect, new PacketSpellSelect());			//
    registerExtended(EXTDATA_HouseDesignDet, new PacketHouseDesignReq());		// house design request
    registerExtended(EXTDATA_AntiCheat, new PacketAntiCheat());					// anti-cheat / unknown
    registerExtended(EXTDATA_BandageMacro, new PacketBandageMacro());			//
    registerExtended(EXTDATA_TargetedSkill, new PacketTargetedSkill());		    // use targeted skill
    registerExtended(EXTDATA_GargoyleFly, new PacketGargoyleFly());				// gargoyle flying action
    registerExtended(EXTDATA_WheelBoatMove, new PacketWheelBoatMove());			// wheel boat movement

    // encoded packets (0xD7)
    registerEncoded(EXTAOS_HcBackup, new PacketHouseDesignBackup());			// house design - backup
    registerEncoded(EXTAOS_HcRestore, new PacketHouseDesignRestore());			// house design - restore
    registerEncoded(EXTAOS_HcCommit, new PacketHouseDesignCommit());			// house design - commit
    registerEncoded(EXTAOS_HcDestroyItem, new PacketHouseDesignDestroyItem());	// house design - remove item
    registerEncoded(EXTAOS_HcPlaceItem, new PacketHouseDesignPlaceItem());		// house design - place item
    registerEncoded(EXTAOS_HcExit, new PacketHouseDesignExit());				// house design - exit designer
    registerEncoded(EXTAOS_HcPlaceStair, new PacketHouseDesignPlaceStair());	// house design - place stairs
    registerEncoded(EXTAOS_HcSynch, new PacketHouseDesignSync());				// house design - synchronise
    registerEncoded(EXTAOS_HcClear, new PacketHouseDesignClear());				// house design - clear
    registerEncoded(EXTAOS_HcSwitch, new PacketHouseDesignSwitch());			// house design - change floor
    registerEncoded(EXTAOS_HcPlaceRoof, new PacketHouseDesignPlaceRoof());		// house design - place roof
    registerEncoded(EXTAOS_HcDestroyRoof, new PacketHouseDesignDestroyRoof());	// house design - remove roof
    registerEncoded(EXTAOS_SpecialMove, new PacketSpecialMove());				//
    registerEncoded(EXTAOS_HcRevert, new PacketHouseDesignRevert());			// house design - revert
    registerEncoded(EXTAOS_EquipLastWeapon, new PacketEquipLastWeapon());		//
    registerEncoded(EXTAOS_GuildButton, new PacketGuildButton());				// guild button press
    registerEncoded(EXTAOS_QuestButton, new PacketQuestButton());				// quest button press
}

void PacketManager::registerPacket(uint id, Packet* handler)
{
    // assign standard packet handler
    ADDTOCALLSTACK("PacketManager::registerPacket");
    ASSERT(id < ARRAY_COUNT(m_handlers));
    unregisterPacket(id);
    m_handlers[id] = handler;
}

void PacketManager::registerExtended(uint id, Packet* handler)
{
    // assign extended packet handler
    ADDTOCALLSTACK("PacketManager::registerExtended");
    ASSERT(id < ARRAY_COUNT(m_extended));
    unregisterExtended(id);
    m_extended[id] = handler;
}

void PacketManager::registerEncoded(uint id, Packet* handler)
{
    // assign encoded packet handler
    ADDTOCALLSTACK("PacketManager::registerEncoded");
    ASSERT(id < ARRAY_COUNT(m_encoded));
    unregisterEncoded(id);
    m_encoded[id] = handler;
}

void PacketManager::unregisterPacket(uint id)
{
    // delete standard packet handler
    ADDTOCALLSTACK("PacketManager::unregisterPacket");
    ASSERT(id < ARRAY_COUNT(m_handlers));
    if (m_handlers[id] == nullptr)
        return;

    delete m_handlers[id];
    m_handlers[id] = nullptr;
}

void PacketManager::unregisterExtended(uint id)
{
    // delete extended packet handler
    ADDTOCALLSTACK("PacketManager::unregisterExtended");
    ASSERT(id < ARRAY_COUNT(m_extended));
    if (m_extended[id] == nullptr)
        return;

    delete m_extended[id];
    m_extended[id] = nullptr;
}

void PacketManager::unregisterEncoded(uint id)
{
    // delete encoded packet handler
    ADDTOCALLSTACK("PacketManager::unregisterEncoded");
    ASSERT(id < ARRAY_COUNT(m_encoded));
    if (m_encoded[id] == nullptr)
        return;

    delete m_encoded[id];
    m_encoded[id] = nullptr;
}

Packet* PacketManager::getHandler(uint id) const
{
    // get standard packet handler
    if (id >= ARRAY_COUNT(m_handlers))
        return nullptr;

    return m_handlers[id];
}

Packet* PacketManager::getExtendedHandler(uint id) const
{
    // get extended packet handler
    if (id >= ARRAY_COUNT(m_extended))
        return nullptr;

    return m_extended[id];
}

Packet* PacketManager::getEncodedHandler(uint id) const
{
    // get encoded packet handler
    if (id >= ARRAY_COUNT(m_encoded))
        return nullptr;

    return m_encoded[id];
}
