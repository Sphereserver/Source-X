/**
* @file sphereproto.h
* @brief Protocol formats. Define all the data passed from Client to Server.
*/

#ifndef _INC_SPHEREPROTO_H
#define _INC_SPHEREPROTO_H

#include "../network/net_datatypes.h"
#include "../sphere/threads.h"
#include "sphere_library/CSString.h"


//---------------------------PROTOCOL DEFS---------------------------


class CLanguageID
{
private:
	// 3 letter code for language.
	// ENU,FRA,DEU,etc. (see langcode.iff)
	// terminate with a 0
	// 0 = english default.
	char m_codes[4]; // UNICODE language pref. ('ENU'=english)
public:
	CLanguageID() :
		m_codes{}
	{}
	CLanguageID( const char * pszInit )
	{
		Set( pszInit );
	}
	CLanguageID(int iDefault);
	bool IsDef() const
	{
		return ( m_codes[0] != 0 );
	}
    void GetStrDef(tchar* pszLang);
    void GetStr(tchar* pszLang) const;
    lpctstr GetStr() const;
    bool Set(lpctstr pszLang);
};

enum XCMD_TYPE	// XCMD_* messages are unique in both directions.
{
	//	0x00
	XCMD_Create			= 0x00,
	XCMD_WalkRequest	= 0x02,
	XCMD_Talk			= 0x03,
	XCMD_Attack			= 0x05,
	XCMD_DClick			= 0x06,
	XCMD_ItemPickupReq	= 0x07,
	XCMD_ItemDropReq	= 0x08,
	XCMD_Click			= 0x09,
	XCMD_DamagePacket	= 0x0b,
	//	0x10
	XCMD_Status			= 0x11,
	XCMD_ExtCmd			= 0x12,
	XCMD_ItemEquipReq	= 0x13,
    XCMD_HealthBarColorNew	= 0x16,
	XCMD_HealthBarColor	= 0x17,
	XCMD_Put			= 0x1a,
	XCMD_Start			= 0x1b,
	XCMD_Speak			= 0x1c,
	XCMD_Remove			= 0x1d,
	//	0x20
	XCMD_PlayerUpdate	= 0x20,
	XCMD_WalkReject		= 0x21,
	XCMD_WalkAck		= 0x22,
	XCMD_DragAnim		= 0x23,
	XCMD_ContOpen		= 0x24,
	XCMD_ContAdd		= 0x25,
	XCMD_Kick			= 0x26,
	XCMD_DragCancel		= 0x27,
	XCMD_DropRejected	= 0x28,
	XCMD_DropAccepted	= 0x29,
	XCMD_DeathMenu		= 0x2c,
	XCMD_ItemEquip		= 0x2e,
	XCMD_Fight			= 0x2f,
	//	0x30
	XCMD_CharStatReq	= 0x34,
	XCMD_Skill			= 0x3a,
	XCMD_VendorBuy		= 0x3b,
	XCMD_Content		= 0x3c,
	XCMD_StaticUpdate	= 0x3f,
	//	0x40
	XCMD_LightPoint		= 0x4e,
	XCMD_Light			= 0x4f,
	//	0x50
	XCMD_IdleWarning	= 0x53,
	XCMD_Sound			= 0x54,
	XCMD_LoginComplete	= 0x55,
	XCMD_MapEdit		= 0x56,
	XCMD_Time			= 0x5b,
	XCMD_CharPlay		= 0x5d,
	//	0x60
	XCMD_Weather		= 0x65,
	XCMD_BookPage		= 0x66,
	XCMD_Options		= 0x69,
	XCMD_Target			= 0x6c,
	XCMD_PlayMusic		= 0x6d,
	XCMD_CharAction		= 0x6e,
	XCMD_SecureTrade	= 0x6f,
	//	0x70
	XCMD_Effect			= 0x70,
	XCMD_BBoard			= 0x71,
	XCMD_War			= 0x72,
	XCMD_Ping			= 0x73,
	XCMD_VendOpenBuy	= 0x74,
	XCMD_CharName		= 0x75,
	XCMD_ZoneChange		= 0x76,
	XCMD_CharMove		= 0x77,
	XCMD_Char			= 0x78,
	XCMD_MenuItems		= 0x7c,
	XCMD_MenuChoice		= 0x7d,
	XCMD_UOGRequest		= 0x7F,
	//	0x80
	XCMD_ServersReq		= 0x80,
	XCMD_CharList3		= 0x81,
	XCMD_LogBad			= 0x82,
	XCMD_CharDelete		= 0x83,
	XCMD_DeleteBad		= 0x85,
	XCMD_CharList2		= 0x86,
	XCMD_PaperDoll		= 0x88,
	XCMD_CorpEquip		= 0x89,
	XCMD_GumpTextDisp	= 0x8b,
	XCMD_Relay			= 0x8c,
	XCMD_CreateNew		= 0x8d,
	//	0x90
	XCMD_MapDisplay		= 0x90,
	XCMD_CharListReq	= 0x91,
	XCMD_BookOpen		= 0x93,
	XCMD_DyeVat			= 0x95,
	XCMD_AllNames3D		= 0x98,
	XCMD_TargetMulti	= 0x99,
	XCMD_Prompt			= 0x9a,
	XCMD_HelpPage		= 0x9b,
	XCMD_VendOpenSell	= 0x9e,
	XCMD_VendorSell		= 0x9f,
	//	0xA0
	XCMD_ServerSelect	= 0xa0,
	XCMD_StatChngStr	= 0xa1,
	XCMD_StatChngInt	= 0xa2,
	XCMD_StatChngDex	= 0xa3,
	XCMD_Spy			= 0xa4,
	XCMD_Web			= 0xa5,
	XCMD_Scroll			= 0xa6,
	XCMD_TipReq			= 0xa7,
	XCMD_ServerList		= 0xa8,
	XCMD_CharList		= 0xa9,
	XCMD_AttackOK		= 0xaa,
	XCMD_GumpInpVal		= 0xab,
	XCMD_GumpInpValRet	= 0xac,
	XCMD_TalkUNICODE	= 0xad,
	XCMD_SpeakUNICODE	= 0xae,
	XCMD_CharDeath		= 0xaf,
	//	0xB0
	XCMD_GumpDialog		= 0xb0,
	XCMD_GumpDialogRet	= 0xb1,
	XCMD_ChatReq		= 0xb2,
	XCMD_ChatText		= 0xb3,
	XCMD_Chat			= 0xb5,
	XCMD_ToolTipReq		= 0xb6,
	XCMD_ToolTip		= 0xb7,
	XCMD_CharProfile	= 0xb8,
	XCMD_Features		= 0xb9,
	XCMD_Arrow			= 0xba,
	XCMD_MailMsg		= 0xbb,
	XCMD_Season			= 0xbc,
	XCMD_ClientVersion	= 0xbd,
	XCMD_ExtData		= 0xbf,
	//	0xC0
	XCMD_EffectEx		= 0xc0,
	XCMD_SpeakLocalized	= 0xc1,
	XCMD_PromptUNICODE	= 0xc2,
	XCMD_Semivisible	= 0xc4,
	XCMD_EffectParticle	= 0xc7,
	XCMD_ViewRange		= 0xc8,
	XCMD_GQCount		= 0xcb,
	XCMD_SpeakLocalizedEx	= 0xcc,
	//	0xD0
	XCMD_ConfigFile		= 0xd0,
	XCMD_LogoutStatus	= 0xd1,
	XCMD_CharMove2		= 0xd2,
	XCMD_DrawChar2		= 0xd3,
	XCMD_AOSBookPage	= 0xd4,
	XCMD_AOSTooltip		= 0xd6,
	XCMD_ExtAosData		= 0xd7,
	XCMD_AOSCustomHouse	= 0xd8,
	XCMD_Spy2			= 0xd9,
	XCMD_Majong			= 0xda,
	XCMD_CharTransferLog	= 0xdb,
	XCMD_AOSTooltipInfo	= 0xdc,
	XCMD_CompressedGumpDialog	= 0xdd,
	XCMD_BuffPacket		= 0xdf,
	//	0xE0
	XCMD_BugReport				= 0xe0,
	XCMD_KRClientType			= 0xe1,
	XCMD_NewAnimUpdate			= 0xe2,
	XCMD_EncryptionReq			= 0xe3,
	XCMD_EncryptionReply		= 0xe4,
	XCMD_WaypointShow			= 0xe5,
	XCMD_WaypointHide			= 0xe6,
	XCMD_HighlightUIContinue	= 0xe7,
	XCMD_HighlightUIRemove		= 0xe8,
	XCMD_HighlightUIToggle		= 0xe9,
	XCMD_ToggleHotbar			= 0xea,
	XCMD_UseHotbar				= 0xeb,
	XCMD_MacroEquipItem			= 0xec,
	XCMD_MacroUnEquipItem		= 0xed,
	XCMD_NewSeed				= 0xef,
	//	0xF0
	XCMD_WalkRequestNew		= 0xf0,
	XCMD_TimeSyncRequest	= 0xf1,
	XCMD_TimeSyncResponse	= 0xf2,
	XCMD_PutNew				= 0xf3,
	XCMD_CrashReport		= 0xf4,
	XCMD_MapDisplayNew		= 0xf5,
	XCMD_MoveShip			= 0xf6,
	XCMD_PacketCont			= 0xf7,
	XCMD_CreateHS			= 0xf8,
	XCMD_GlobalChat			= 0xf9,
    XCMD_UltimaStoreButton  = 0xfa,
    XCMD_PublicHouseContent = 0xfb,
	XCMD_QTY				= 0xf9
};

#define SEEDLENGTH_OLD (sizeof( dword ))
#define SEEDLENGTH_NEW (1 + sizeof( dword )*5)

enum PARTYMSG_TYPE
{
	PARTYMSG_Add = 1,	// (from client) Invite someone to join my party. or (from server) list who is in the party.
	PARTYMSG_Remove = 2,	// (to/from client) request to kick someone or notify that someone has been kicked.
	PARTYMSG_MsgMember = 3,	// (from client ) send a msg to a single member.
	PARTYMSG_Msg = 4,		// (to/from client) echos
	PARTYMSG_Disband,
	PARTYMSG_Option = 6,	// (from client) loot flag.
	PARTYMSG_NotoInvited = 7,	// (to client) I've been invited to join another party.
	PARTYMSG_Accept = 8,	// (from client) first
	PARTYMSG_Decline,
	PARTYMSG_QTY
};

enum EXTDATA_TYPE
{
	EXTDATA_Fastwalk_Init	= 0x01,	// send to client
	EXTDATA_Fastwalk_Add	= 0x02,	// send to client
	EXTDATA_Unk3,
	EXTDATA_GumpChange		= 0x04,	// len=8 "00 00 00 67 00 00 00 00"
	EXTDATA_ScreenSize		= 0x05,	// len=8 "00 00 02 80 00 00 00 0a"
	EXTDATA_Party_Msg		= 0x06,	// len=5 data for total of 10. Client wants to add to the party.
	EXTDATA_Arrow_Click		= 0x07,	// Client clicked on the quest arrow.
	EXTDATA_Map_Change		= 0x08, // send to the client (? not sure)
	EXTDATA_Wrestle_DisArm  = 0x09,	// From Client: Wrestling disarm
	EXTDATA_Wrestle_Stun    = 0x0a,	// From Client: Wrestling stun
	EXTDATA_Lang			= 0x0b,	// len=3 = my language. ENU CLanguageID
	EXTDATA_StatusClose		= 0x0c, // 12= closing a status window.
	EXTDATA_Unk13,
	EXTDATA_Yawn			= 0x0e, // Yawn animation
	EXTDATA_Unk15			= 0x0f,	// Unknown, sent at login
	EXTDATA_OldAOSTooltipInfo	= 0x10, // Equip info
	//
	//
	EXTDATA_Popup_Request	= 0x13, // client message
	EXTDATA_Popup_Display	= 0x14,	// server message
	EXTDATA_Popup_Select	= 0x15,	// client message
	EXTDATA_CloseUI_Window	= 0x16,	// client message
	EXTDATA_Codex_Wisdom	= 0x17,	// server message
	EXTDATA_Map_Diff		= 0x18,	// enable mapdiff files
    EXTDATA_BondedStatus    = 0x19, // 0x19.0x0: update bonded status
	EXTDATA_Stats_Enable	= 0x19,	// 0x19.0x2: extended stats
    //EXTDATA_NewBondedStatus    = 0x19, // 0x19.0x5.0xff specialization: update bonded status (does it actually exist?)
    EXTDATA_StatueAnimation = 0x19, // 0x19.0x5.0xff: update character current animation frame
	EXTDATA_Stats_Change	= 0x1a,	// extended stats
	EXTDATA_NewSpellbook	= 0x1b,
	EXTDATA_NewSpellSelect	= 0x1c,
	EXTDATA_HouseDesignVer	= 0x1d,	// server
	EXTDATA_HouseDesignDet	= 0x1e, // client
	//
	EXTDATA_HouseCustom		= 0x20,
	EXTDATA_Ability_Confirm	= 0x21,	// server (empty packet only id is required)
	EXTDATA_DamagePacketOld	= 0x22,	// server
	//
	EXTDATA_Unk23			= 0x23,
	EXTDATA_AntiCheat		= 0x24, // Sent by SE clients, every second or so.
	EXTDATA_SpellSE			= 0x25,
	EXTDATA_SpeedMode		= 0x26, // server message
	//
	//
	//
	//
	//
	EXTDATA_BandageMacro	= 0x2c, // client message
	//
	EXTDATA_TargetedSkill   = 0x2e, // Use targeted skill (from client)
	//
	//
	//
	EXTDATA_GargoyleFly		= 0x32, // client message
	EXTDATA_WheelBoatMove = 0x33, // client message
	EXTDATA_QTY
};

enum EXTAOS_TYPE
{
	//
	//
	EXTAOS_HcBackup = 0x02, // House Customization :: Backup
    EXTAOS_HcRestore = 0x03, // House Customization :: Restore
	EXTAOS_HcCommit = 0x04, // House Customization :: Commit
	EXTAOS_HcDestroyItem = 0x05, // House Customization :: Destroy Item
	EXTAOS_HcPlaceItem = 0x06, // House Customization :: Place Item
	//
	//
	//
	//
	//
	EXTAOS_HcExit = 0x0C, // House Customization :: Exit
	EXTAOS_HcPlaceStair = 0x0D, // House Customization :: Place Multi (Stairs)
	EXTAOS_HcSynch = 0x0E, // House Customization :: Synch
	//
	EXTAOS_HcClear = 0x10, // House Customization :: Clear
	//
	EXTAOS_HcSwitch = 0x12, // House Customization :: Switch Floors
	EXTAOS_HcPlaceRoof = 0x13, // House Customization :: Place Roof Tile
	EXTAOS_HcDestroyRoof = 0x14, // House Customization :: Destroy Roof Tile
	//
	//
	//
	//
	EXTAOS_SpecialMove = 0x19, // Special Moves :: Activate/Deactivate
	EXTAOS_HcRevert = 0x1A, // House Customization :: Revert
	//
	//
	//
	EXTAOS_EquipLastWeapon = 0x1E, // Equip Last Weapon
	//
	// 0x20
	//
	//
	//
	//
	//
	//
	//
	EXTAOS_GuildButton = 0x28,	// Guild Button
	//
	//
	//
	//
	//
	//
	// 0x30
	//
	EXTAOS_QuestButton = 0x32,	// Quest Button

	EXTAOS_QTY
};


enum SECURE_TRADE_TYPE
{
	// SecureTrade Action types.
	SECURE_TRADE_OPEN = 0,
	SECURE_TRADE_CLOSE = 1,
	SECURE_TRADE_CHANGE = 2,
	SECURE_TRADE_UPDATEGOLD = 3,
	SECURE_TRADE_UPDATELEDGER = 4
};

enum SEASON_TYPE : uchar
{
	// The seasons can be:
	SEASON_Spring = 0,
	SEASON_Summer,		// 1
	SEASON_Fall,		// 2
	SEASON_Winter,		// 3
	SEASON_Desolate,	// 4 = (Felucca) undead
	SEASON_Nice,		// 5 = (Trammal) summer ?
	SEASON_QTY
};

enum BBOARDF_TYPE	// Bulletin Board Flags. m_flag
{
	BBOARDF_NAME = 0,	// board name
	BBOARDF_MSG_HEAD,	// 1=message header, 
	BBOARDF_MSG_BODY,	// 2=message body
	BBOARDF_REQ_FULL,	// 3=request for full msg.
	BBOARDF_REQ_HEAD,	// 4=request for just head.
	BBOARDF_NEW_MSG,	// 5=new message, 
	BBOARDF_DELETE		// 6=Delete
};

enum EXTCMD_TYPE
{
	EXTCMD_SKILL			= 0x24,	// skill start. "skill number"
	EXTCMD_CAST_BOOK		= 0x27,	// cast spell from book. "spell number"
	EXTCMD_AUTOTARG			= 0x2f,	// bizarre new autotarget mode. "target x y z"
	EXTCMD_OPEN_SPELLBOOK	= 0x43,	// open spell book if we have one. "book type"
	EXTCMD_CAST_MACRO		= 0x56,	// macro spell. "spell number"
	EXTCMD_DOOR_AUTO		= 0x58,	// open door macro
	EXTCMD_ANIMATE			= 0xc7,	// "bow" or "salute"
	EXTCMD_INVOKE_VIRTUE	= 0xf4	// invoke virtue
};

enum CHATMSG_TYPE	// Chat system messages.
{
	CHATMSG_AlreadyIgnoringMax			= 0x01,		// 1 - You are already ignoring the maximum amount of people
	CHATMSG_AlreadyIgnoringPlayer,					// 2 - You are already ignoring <name>
	CHATMSG_NowIgnoring,							// 3 - You are now ignoring <name>
	CHATMSG_NoLongerIgnoring,						// 4 - You no longer ignoring <name>
	CHATMSG_NotIgnoring,							// 5 - You are not ignoring <name>
	CHATMSG_NoLongerIgnoringAnyone,					// 6 - You are no longer ignoring anyone
	CHATMSG_InvalidConferenceName,					// 7 - That is not a valid conference name
	CHATMSG_AlreadyAConference,						// 8 - There is already a conference of that name
	CHATMSG_MustHaveOps,							// 9 - You must have operator status to do this
	CHATMSG_ConferenceRenamed,						// a - Conference <name> renamed to .
	CHATMSG_MustBeInAConference,					// b - You must be in a conference to do this. To join a conference, select one from the Conference menu
	CHATMSG_NoPlayer,								// c - There is no player named <name>
	CHATMSG_NoConference,							// d - There is no conference named <name>
	CHATMSG_IncorrectPassword,						// e - That is not the correct password
	CHATMSG_PlayerIsIgnoring,						// f - <name> has chosen to ignore you. None of your messages to them will get through
	CHATMSG_RevokedSpeaking,						// 10 - The moderator of this conference has not given you speaking priveledges.
	CHATMSG_ReceivingPrivate,						// 11 - You can now receive private messages
	CHATMSG_NoLongerReceivingPrivate,				// 12 - You will no longer receive private messages. Those who send you a message will be notified that you are blocking incoming messages
	CHATMSG_ShowingName,							// 13 - You are now showing your character name to any players who inquire with the whois command
	CHATMSG_NotShowingName,							// 14 - You are no long showing your character name to any players who inquire with the whois command
	CHATMSG_PlayerIsAnonymous,						// 15 - <name> is remaining anonymous
	CHATMSG_PlayerNotReceivingPrivate,				// 16 - <name> has chosen to not receive private messages at the moment
	CHATMSG_PlayerKnownAs,							// 17 - <name> is known in the lands of britania as .
	CHATMSG_PlayerIsKicked,							// 18 - <name> has been kicked out of the conference
	CHATMSG_ModeratorHasKicked,						// 19 - <name>, a conference moderator, has kicked you out of the conference
	CHATMSG_AlreadyInConference,					// 1a - You are already in the conference <name>
	CHATMSG_PlayerNoLongerModerator,				// 1b - <name> is no longer a conference moderator
	CHATMSG_PlayerIsAModerator,						// 1c - <name> is now a conference moderator
	CHATMSG_RemovedListModerators,					// 1d - <name> has removed you from the list of conference moderators.
	CHATMSG_YouAreAModerator,						// 1e - <name> has made you a conference moderator
	CHATMSG_PlayerNoSpeaking,						// 1f - <name> no longer has speaking priveledges in this conference.
	CHATMSG_PlayerNowSpeaking,						// 20 - <name> now has speaking priveledges in this conference
	CHATMSG_ModeratorRemovedSpeaking,				// 21 - <name>, a channel moderator, has removed your speaking priveledges for this conference.
	CHATMSG_ModeratorGrantSpeaking,					// 22 - <name>, a channel moderator, has granted you speaking priveledges for this conference.
	CHATMSG_SpeakingByDefault,						// 23 - From now on, everyone in the conference will have speaking priviledges by default.
	CHATMSG_ModeratorsSpeakDefault,					// 24 - From now on, only moderators in this conference will have speaking priviledges by default.
	CHATMSG_PlayerTalk,								// 25 - <name>:
	CHATMSG_PlayerEmote,							// 26 - *<name>*
	CHATMSG_PlayerPrivate,							// 27 - [<name>]:
	CHATMSG_PasswordChanged,						// 28 - The password to the conference has been changed
	CHATMSG_NoMorePlayersAllowed,					// 29 - Sorry--the conference named <name> is full and no more players are allowed in.
	CHATMSG_Banning,								// 2a - You are banning <name> from this conference.
	CHATMSG_ModeratorHasBanned,						// 2b - <name>, a conference moderator, has banned you from the conference.
	CHATMSG_Banned,									// 2c - You have been banned from this conference.

	CHATMSG_TrialAccOnlyHelpChannel		= 0x03f5,	// 3f5 - Trial accounts may only join the Help channel.
	CHATMSG_TrialAccNoCustomChannel,				// 3f6 - Trial accounts may not participate in custom channels.


	// Actions (client -> server)					// OLD CHAT		NEW CHAT
	CHATACT_ChangeChannelPassword		= 0x41,		// x
	CHATACT_LeaveChannel				= 0x43,		//				x
	CHATACT_LeaveChat					= 0x58,		// x
	CHATACT_ChannelMessage				= 0x61,		// x			x
	CHATACT_JoinChannel					= 0x62,		// x			x
	CHATACT_CreateChannel				= 0x63,		// x			x
	CHATACT_RenameChannel				= 0x64,		// x
	CHATACT_PrivateMessage				= 0x65,		// x
	CHATACT_AddIgnore					= 0x66,		// x
	CHATACT_RemoveIgnore				= 0x67,		// x
	CHATACT_ToggleIgnore				= 0x68,		// x
	CHATACT_AddVoice					= 0x69,		// x
	CHATACT_RemoveVoice					= 0x6A,		// x
	CHATACT_ToggleVoice					= 0x6B,		// x
	CHATACT_AddModerator				= 0x6C,		// x
	CHATACT_RemoveModerator				= 0x6D,		// x
	CHATACT_ToggleModerator				= 0x6E,		// x
	CHATACT_EnablePrivateMessages		= 0x6F,		// x
	CHATACT_DisablePrivateMessages		= 0x70,		// x
	CHATACT_TogglePrivateMessages		= 0x71,		// x
	CHATACT_ShowCharacterName			= 0x72,		// x
	CHATACT_HideCharacterName			= 0x73,		// x
	CHATACT_ToggleCharacterName			= 0x74,		// x
	CHATACT_WhoIs						= 0x75,		// x
	CHATACT_Kick						= 0x76,		// x
	CHATACT_EnableDefaultVoice			= 0x77,		// x
	CHATACT_DisableDefaultVoice			= 0x78,		// x
	CHATACT_ToggleDefaultVoice			= 0x79,		// x
	CHATACT_EmoteMessage				= 0x7A,		// x

	// Commands (client <- server)					// OLD CHAT		NEW CHAT
	CHATCMD_AddChannel					= 0x3E8,	// x			x
	CHATCMD_RemoveChannel,							// x			x
	CHATCMD_SetChatName					= 0x3EB,	// x
	CHATCMD_CloseChatWindow,						// x
	CHATCMD_OpenChatWindow,							// x
	CHATCMD_AddMemberToChannel,						// x
	CHATCMD_RemoveMemberFromChannel,				// x
	CHATCMD_ClearMembers,							// x
	CHATCMD_JoinedChannel,							// x			x
	CHATCMD_LeftChannel					= 0x3F4		//				x
};

enum INPVAL_STYLE	// for the various styles for InpVal box.
{
	INPVAL_STYLE_NOEDIT		= 0,	// No textbox, just a message
	INPVAL_STYLE_TEXTEDIT	= 1,	// Alphanumeric
	INPVAL_STYLE_NUMEDIT	= 2		// Numeric
};

enum MAPCMD_TYPE
{
	MAP_ADD = 1,
	MAP_PIN = 1,
	MAP_INSERT = 2,
	MAP_MOVE = 3,
	MAP_DELETE = 4,
	MAP_UNSENT = 5,
	MAP_CLEAR = 5,
	MAP_TOGGLE = 6,
	MAP_SENT = 7
};

enum MAPWAYPOINT_TYPE
{
    MAPWAYPOINT_Remove = 0x0,
	MAPWAYPOINT_Corpse = 0x1,
	MAPWAYPOINT_PartyMember = 0x2,
	MAPWAYPOINT_Unk1 = 0x3,
	MAPWAYPOINT_QuestGiver = 0x4,
	MAPWAYPOINT_NewPlayerQuest = 0x5,
	MAPWAYPOINT_Healer = 0x6,
	MAPWAYPOINT_Unk2 = 0x7,
	MAPWAYPOINT_Unk3 = 0x8,
	MAPWAYPOINT_Unk4 = 0x9,
	MAPWAYPOINT_Unk5 = 0xA,
	MAPWAYPOINT_Shrine = 0xB,
	MAPWAYPOINT_Moongate = 0xC,
	MAPWAYPOINT_Unk6 = 0xD,
	MAPWAYPOINT_GreenDot = 0xE,
	MAPWAYPOINT_GreenDotFlashing = 0xF
};

enum WEATHER_TYPE : uchar
{
	WEATHER_DEFAULT = 0xFE,
	WEATHER_DRY = 0xFF,
	WEATHER_RAIN = 0,
	WEATHER_STORM,
	WEATHER_SNOW,
	WEATHER_CLOUDY	// not client supported ? (Storm brewing)
};

enum SCROLL_TYPE	// Client messages for scrolls types.
{
	SCROLL_TYPE_TIPS = 0,	// type = 0 = TIPS
	SCROLL_TYPE_NOTICE = 1,
	SCROLL_TYPE_UPDATES = 2	// type = 2 = UPDATES
};

enum EFFECT_TYPE
{
	EFFECT_BOLT = 0,	// a targetted bolt
	EFFECT_LIGHTNING,	// lightning bolt.
	EFFECT_XYZ,			// Stay at current xyz ??? not sure about this.
	EFFECT_OBJ,			// effect at single Object.
    EFFECT_FADE_SCREEN  // Fade client screen (only available on clients >= 6.0.0.0)
};

enum NOTO_TYPE : byte
{
	NOTO_INVALID = 0,	// 0= not a valid color!!
	NOTO_GOOD,			// 1= good(blue),
	NOTO_GUILD_SAME,	// 2= same guild,
	NOTO_NEUTRAL,		// 3= Neutral,
	NOTO_CRIMINAL,		// 4= criminal
	NOTO_GUILD_WAR,		// 5= Waring guilds,
	NOTO_EVIL,			// 6= evil(red),
	NOTO_INVUL			// 7= invulnerable
};

enum TALKMODE_TYPE	// Modes we can talk/bark in.
{
	TALKMODE_SAY        = 0,    // 0 = A character speaking.
	TALKMODE_SYSTEM     = 1,	// 1 = Display as system prompt
	TALKMODE_EMOTE      = 2,	// 2 = *smiles* at object (client shortcut: :+space)
	TALKMODE_ITEM       = 6,	// 6 = text labeling an item. Preceeded by "You see"
	TALKMODE_NOSCROLL   = 7,	// 7 = As a status msg. Does not scroll (as reported by the packet guides)
	TALKMODE_WHISPER    = 8,	// 8 = Only those close can here. (client shortcut: ;+space)
	TALKMODE_YELL       = 9,	// 9 = Can be heard 2 screens away. (client shortcut: !+space)
	TALKMODE_SPELL      = 10,	// 10 = Used by spells
	TALKMODE_GUILD      = 0xD,	// 13 = Used by guild chat (client shortcut: \)
	TALKMODE_ALLIANCE   = 0xE,	// 14 = Used by alliance chat (client shortcut: shift+\)
    TALKMODE_COMMAND    = 0xF,  // 15 = GM command prompt
    // Special talkmodes, used internally by Sphere
    TALKMODE_SOUND      = 0xFE, // Used to check if a char can hear a sound.
	TALKMODE_BROADCAST  = 0xFF  // It will be converted to something else.
};

enum SKILLLOCK_TYPE
{
	SKILLLOCK_UP = 0,
	SKILLLOCK_DOWN,
	SKILLLOCK_LOCK
};

enum DELETE_ERR_TYPE
{
	DELETE_ERR_BAD_PASS = 0, // 0 That character password is invalid.
	DELETE_ERR_NOT_EXIST,	// 1 That character does not exist.
	DELETE_ERR_IN_USE,	// 2 That character is being played right now.
	DELETE_ERR_NOT_OLD_ENOUGH, // 3 That character is not old enough to delete. The character must be 7 days old before it can be deleted.
	DELETE_SUCCESS = 255
};
/*
enum LOGIN_ERR_TYPE	// error codes sent to client.
{
	LOGIN_ERR_NONE = 0,			// no account
	LOGIN_ERR_USED = 1,			// already in use.
	LOGIN_ERR_BLOCKED = 2,		// client blocked
	LOGIN_ERR_BAD_PASS = 3,		// incorrect password
	LOGIN_ERR_OTHER = 4,		// like timeout.

	// the error codes below are not sent to or understood by the client,
	// and should be translated into one of the codes above
	LOGIN_ERR_BAD_CLIVER,		// cliver not permitted
	LOGIN_ERR_BAD_CHAR,			// bad character selected
	LOGIN_ERR_BAD_AUTHID,		// bad auth id
	LOGIN_ERR_BAD_ACCTNAME,		// bad account name (length, characters)
	LOGIN_ERR_BAD_PASSWORD,		// bad password (length, characters)
	LOGIN_ERR_ENC_BADLENGTH,	// bad message length
	LOGIN_ERR_ENC_UNKNOWN,		// unknown encryption
	LOGIN_ERR_ENC_CRYPT,		// crypted client not allowed
	LOGIN_ERR_ENC_NOCRYPT,		// non-crypted client not allowed
	LOGIN_ERR_CHARIDLE,			// character is already ingame
	LOGIN_ERR_TOOMANYCHARS,		// account has too many characters
	LOGIN_ERR_BLOCKED_IP,		// ip is blocked
	LOGIN_ERR_BLOCKED_MAXCLIENTS,	// max clients reached
	LOGIN_ERR_BLOCKED_MAXGUESTS,	// max guests reached
	LOGIN_ERR_MAXPASSTRIES,		// max password tries reached

	// this 'error' code indicates there was no error
	LOGIN_SUCCESS	= 255
};
*/
enum BUGREPORT_TYPE	// bug report codes
{
	BUGREPORT_ENVIRONMENT	= 0x01,
	BUGREPORT_WEARABLES		= 0x02,
	BUGREPORT_COMBAT		= 0x03,
	BUGREPORT_UI			= 0x04,
	BUGREPORT_CRASH			= 0x05,
	BUGREPORT_STUCK			= 0x06,
	BUGREPORT_ANIMATIONS	= 0x07,
	BUGREPORT_PERFORMANCE	= 0x08,
	BUGREPORT_NPCS			= 0x09,
	BUGREPORT_CREATURES		= 0x0A,
	BUGREPORT_PETS			= 0x0B,
	BUGREPORT_HOUSING		= 0x0C,
	BUGREPORT_LOST_ITEM		= 0x0D,
	BUGREPORT_EXPLOIT		= 0x0E,
	BUGREPORT_OTHER			= 0x0F
};

enum PROFESSION_TYPE	// profession ids
{
	PROFESSION_ADVANCED		= 0x00,
	PROFESSION_WARRIOR		= 0x01,
	PROFESSION_MAGE			= 0x02,
	PROFESSION_BLACKSMITH	= 0x03,
	PROFESSION_NECROMANCER	= 0x04,
	PROFESSION_PALADIN		= 0x05,
	PROFESSION_SAMURAI		= 0x06,
	PROFESSION_NINJA		= 0x07
};

enum GAMECLIENT_TYPE	// game client type, KR and Enhanced are from the 0xE1 packet, other values are for convenience
{
	CLIENTTYPE_2D = 0x00,	// Standard client
	CLIENTTYPE_3D = 0x01,	// 3D client
	CLIENTTYPE_KR = 0x02,	// KR client
	CLIENTTYPE_EC = 0x03	// Enhanced Client
};

enum RACE_TYPE		// character race, used in new character creation (0x8D) and status (0x11) packets
{
	RACETYPE_UNDEFINED = 0x00,	// none of the below
	RACETYPE_HUMAN = 0x01,		// human
	RACETYPE_ELF = 0x02,		// elf
	RACETYPE_GARGOYLE = 0x03	// gargoyle
};


#define HEALTHCOLOR_GREEN	1
#define HEALTHCOLOR_YELLOW	2

// Item flags
#define ITEMF_MOVABLE	0x20
#define ITEMF_INVIS		0x80

// Char mode flags
#define CHARMODE_FREEZE		0x01
#define CHARMODE_FEMALE		0x02
#define CHARMODE_POISON		0x04	// green status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_FLYING		0x04	// flying (gargoyles, SA)
#define CHARMODE_YELLOW		0x08	// yellow status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_IGNOREMOBS	0x10
#define CHARMODE_WAR		0x40
#define CHARMODE_INVIS		0x80

#define CLI_FEAT_T2A_CHAT	0x0001
#define CLI_FEAT_LBR_SOUND	0x0002
#define CLI_FEAT_T2A_FULL	0x0004
#define CLI_FEAT_LBR_FULL	0x0008
#define CLI_FEAT_T2A		(CLI_FEAT_T2A_CHAT|CLI_FEAT_T2A_FULL)
#define CLI_FEAT_LBR		(CLI_FEAT_LBR_SOUND|CLI_FEAT_LBR_FULL)
#define CLI_FEAT_T2A_LBR	(CLI_FEAT_T2A|CLI_FEAT_LBR)


/*
    Client Versions numeric constants.
    WARNING: DEPRECATED! Use CUOClientVersion (or at least wrap those numbers in that class).
*/

// client versions (expansions)
#define MINCLIVER_LBR				 3000702	// minimum client to activate LBR packets (3.0.7b)
#define MINCLIVER_AOS				 4000000	// minimum client to activate AOS packets (4.0.0a)
#define MINCLIVER_SE				 4000500	// minimum client to activate SE packets (4.0.5a)
#define MINCLIVER_ML				 5000000	// minimum client to activate ML packets (5.0.0a)
#define MINCLIVER_SA				 7000000	// minimum client to activate SA packets (7.0.0.0)
#define MINCLIVER_HS				 7000900	// minimum client to activate HS packets (7.0.9.0)
#define MINCLIVER_TOL				 7004565	// minimum client to activate TOL packets (7.0.45.65)

// client versions (extended status gump info)
#define MINCLIVER_STATUS_V2			 3000804	// minimum client to receive v2 of 0x11 packet (3.0.8d)
#define MINCLIVER_STATUS_V3			 3000810	// minimum client to receive v3 of 0x11 packet (3.0.8j)
#define MINCLIVER_STATUS_V4			 4000000	// minimum client to receive v4 of 0x11 packet (4.0.0a)
#define MINCLIVER_STATUS_V5			 5000000	// minimum client to receive v5 of 0x11 packet (5.0.0a)
#define MINCLIVER_STATUS_V6			 7003000	// minimum client to receive v6 of 0x11 packet (7.0.30.0)

// client versions (behaviours)
#define MINCLIVER_CHECKWALKCODE		 1260000	// minimum client to use walk crypt codes for fastwalk prevention (obsolete)
#define MINCLIVER_PADCHARLIST		 3000010	// minimum client to pad character list to at least 5 characters
#define MINCLIVER_AUTOASYNC			 4000000	// minimum client to auto-enable async networking
#define MINCLIVER_NOTOINVUL			 4000000	// minimum client required to view noto_invul health bar
#define MINCLIVER_SKILLCAPS			 4000000	// minimum client to send skill caps in 0x3A packet
#define MINCLIVER_CLOSEDIALOG		 4000400	// minimum client where close dialog does not trigger a client response
#define MINCLIVER_NEWCHATSYSTEM_EC	67000400	// minimum client to use the new chat system (4.0.4.0) - enhanced client
#define MINCLIVER_COMPRESSDIALOG	 5000000	// minimum client to receive zlib compressed dialogs (5.0.0a)
#define MINCLIVER_NEWVERSIONING		 5000605	// minimum client to use the new versioning format (after 5.0.6e it change to 5.0.6.5)
#define MINCLIVER_ITEMGRID			 6000107	// minimum client to use grid index (6.0.1.7)
#define MINCLIVER_NEWCHATSYSTEM		 7000401	// minimum client to use the new chat system (7.0.4.1) - classic client
#define MINCLIVER_GLOBALCHAT		 7006202    // minimum client to use global chat system (7.0.62.2)
#define MINCLIVER_NEWSECURETRADE	 7004565	// minimum client to use virtual gold/platinum on trade window (7.0.45.65)
#define MINCLIVER_MAPWAYPOINT		 7008400    // minimum client to use map waypoints on classic client (7.0.84.0)

// client versions (packets)
#define MAXCLIVER_REVERSEIP			 4000000	// maximum client to reverse IP in 0xA8 packet
#define MINCLIVER_CUSTOMMULTI		 4000000	// minimum client to receive custom multi packets
#define MINCLIVER_SPELLBOOK			 4000000	// minimum client to receive 0xBF.0x1B packet (4.0.0a)
#define MINCLIVER_DAMAGE			 4000000	// minimum client to receive 0xBF.0x22 packet
#define MINCLIVER_TOOLTIP			 4000000	// minimum client to receive tooltip packets
#define MINCLIVER_STATLOCKS			 4000100	// minimum client to receive 0xBF.0x19.0x02 packet
#define MINCLIVER_TOOLTIPHASH		 4000500	// minimum client to receive 0xDC packet
#define MINCLIVER_NEWDAMAGE			 4000700	// minimum client to receive 0x0B packet (4.0.7a)
#define MINCLIVER_NEWBOOK			 5000000	// minimum client to receive 0xD4 packet
#define MINCLIVER_BUFFS				 5000202	// minimum client to receive buff packets (5.0.2b)
#define MINCLIVER_NEWCONTEXTMENU	 6000000	// minimun client to receive 0xBF.0x14.0x02 packet instead of 0xBF.0x14.0x01 (6.0.0.0)
#define MINCLIVER_EXTRAFEATURES		 6001402	// minimum client to receive 4-byte feature mask (6.0.14.2)
#define MINCLIVER_NEWMOBILEANIM		 7000000	// minimum client to receive 0xE2 packet (7.0.0.0)
#define MINCLIVER_SMOOTHSHIP		 7000900	// minimum client to receive 0xF6 packet (7.0.9.0)
#define MINCLIVER_NEWMAPDISPLAY		 7001300	// minimum client to receive 0xF5 packet (7.0.13.0)
#define MINCLIVER_EXTRASTARTINFO 	 7001300	// minimum client to receive extra start info (7.0.13.0)
#define MINCLIVER_NEWMOBINCOMING	 7003301	// minimun client to receive 0x78 packet (7.0.33.1)


/* Network data buffers and hardcoded limits */

#define MAX_BUFFER			18000	// Buffer Size (For socket operations)

#define MAX_TALK_BUFFER		256	// how many chars can anyone speak all at once? (client speech is limited to 128 chars and journal is limited to 256 chars)
#define MAX_ITEM_NAME_SIZE	256	// imposed by client for protocol
#define MAX_NAME_SIZE       30

#define MAX_ACCOUNT_NAME_SIZE       MAX_NAME_SIZE
#define MAX_ACCOUNT_PASSWORD_ENTER  16	// client only allows n chars.
#define ACCOUNT_NAME_VALID_CHAR     " !\"#$%&()*,/:;<=>?@[\\]^{|}~"
#define MAX_CHARS_PER_ACCT	   (byte) 7

#define MAX_SERVERS             32
#define MAX_SERVER_NAME_SIZE    32

#define MAX_ITEMS_CONT  255 // Max items in a container. (arbitrary)
#define MAX_MENU_ITEMS  64  // number of items in a menu. (arbitrary)

#define MAX_BOOK_PAGES  64  // arbitrary max number of pages.
#define MAX_BOOK_LINES  8   // max lines per page.


struct CEvent	// event buffer from client to server..
{
#define MAX_EXTCMD_ARG_LEN  300  // Arbitrary, used to prevent exploits (smaller values will break some old 3rd party client tools)

    union
    {
        byte m_Raw[ MAX_BUFFER ];

        struct
        {
            byte m_Cmd;		// XCMD_TYPE, 0 = ?
            byte m_Arg[1];	// unknown size.
        } Default;	// default unknown type.

        struct // size = 62		// first login to req listing the servers.
        {
            byte m_Cmd;	// 0 = 0x80 = XCMD_ServersReq
            char m_acctname[ MAX_ACCOUNT_NAME_SIZE ];
            char m_acctpass[ MAX_NAME_SIZE ];
            byte m_loginKey;	// 61 = NextLoginKey from uo.cfg
        } ServersReq;

        struct	// size = 65	// request to list the chars I can play.
        {
            byte m_Cmd;		  // 0 = 0x91
            ndword m_Account; // 1-4 = account id from XCMD_Relay message to log server.
            char m_acctname[MAX_ACCOUNT_NAME_SIZE];	// This is corrupted or encrypted seperatly ?
            char m_acctpass[MAX_NAME_SIZE];
        } CharListReq;

        struct	// size = 3;	// relay me to the server i choose.
        {
            byte m_Cmd;		// 0 = 0xa0
            nword m_select;	// 2 = selection this server number
        } ServerSelect;

        struct // XCMD_EncryptionReply
        {
            byte m_Cmd;		    // 0 = 0xE4
            nword m_len;	    // 1 - 2 = length
            ndword m_lenUnk1;	// 3 - 6 = length of m_unk1
            byte m_unk1[1];		// 7 - ? = ?
        } EncryptionReply;
    };
};

struct CCommand	// command buffer from server to client.
{
	byte m_Raw[ MAX_BUFFER ];
};


#endif // _INC_SPHEREPROTO_H
