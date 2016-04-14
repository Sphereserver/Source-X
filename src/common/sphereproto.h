/**
* @file CAcount.h
* @brief Protocol formats. Define all the data passed from Client to Server.
*/

#pragma once
#ifndef _INC_SPHEREPROTO_H
#define _INC_SPHEREPROTO_H

#include "./sphere_library/CString.h"
#include "spherecom.h"
#include "../sphere/threads.h"


//---------------------------PROTOCOL DEFS---------------------------

// All these structures must be byte packed.
#if defined _WIN32 && (!__MINGW32__)
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif

#ifdef _WIN32
	//#include "os_windows.h"
#else
	#include <netinet/in.h>
#endif

// Pack/unpack in network order.

struct nword
{
	// Deal with the stupid Little Endian / Big Endian crap just once.
	// On little endian machines this code would do nothing.
	word m_val;
	operator word () const
	{
		return( ntohs( m_val ));
	}
	nword & operator = ( word val )
	{
		m_val = htons( val );
		return( * this );
	}

#define PACKWORD(p,w)	(p)[0]=HIBYTE(w);(p)[1]=LOBYTE(w)
#define UNPACKWORD(p)	MAKEWORD((p)[1],(p)[0])	// low,high

} PACK_NEEDED;
struct ndword
{
	dword m_val;
	operator dword () const
	{
		return( ntohl( m_val ));
	}
	ndword & operator = ( dword val )
	{
		m_val = htonl( val );
		return( * this );
	}

#define PACKDWORD(p,d)	(p)[0]=((d)>>24)&0xFF;(p)[1]=((d)>>16)&0xFF;(p)[2]=HIBYTE(d);(p)[3]=LOBYTE(d)
#define UNPACKDWORD(p)	MAKEDWORD( MAKEWORD((p)[3],(p)[2]), MAKEWORD((p)[1],(p)[0]))

} PACK_NEEDED;

#define	NCHAR	nword			// a UNICODE text char on the network.

extern int CvtSystemToNUNICODE( NCHAR * pOut, int iSizeOutChars, lpctstr pInp, int iSizeInBytes );
extern int CvtNUNICODEToSystem( tchar * pOut, int iSizeOutBytes, const NCHAR * pInp, int iSizeInChars );

class CLanguageID
{
private:
	// 3 letter code for language.
	// ENU,FRA,DEU,etc. (see langcode.iff)
	// terminate with a 0
	// 0 = english default.
	char m_codes[4]; // UNICODE language pref. ('ENU'=english)
public:
	CLanguageID()
	{
		m_codes[0] = 0;
	}
	CLanguageID( const char * pszInit )
	{
		Set( pszInit );
	}
	CLanguageID( int iDefault )
	{
		UNREFERENCED_PARAMETER(iDefault);
		ASSERT(iDefault==0);
		m_codes[0] = 0;
	}
	bool IsDef() const
	{
		return( m_codes[0] != 0 );
	}
	void GetStrDef( tchar * pszLang )
	{
		if ( ! IsDef())
		{
			strcpy( pszLang, "enu" );
		}
		else
		{
			memcpy( pszLang, m_codes, 3 );
			pszLang[3] = '\0';
		}
	}
	void GetStr( tchar * pszLang ) const
	{
		memcpy( pszLang, m_codes, 3 );
		pszLang[3] = '\0';
	}
	lpctstr GetStr() const
	{
		tchar * pszTmp = Str_GetTemp();
		GetStr( pszTmp );
		return( pszTmp );
	}
	bool Set( lpctstr pszLang )
	{
		// needs not be terminated!
		if ( pszLang != NULL )
		{
			memcpy( m_codes, pszLang, 3 );
			m_codes[3] = 0;
			if ( isalnum(m_codes[0]))
				return true;
			// not valid !
		}
		m_codes[0] = 0;
		return( false );
	}
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
	EXTDATA_Stats_Enable	= 0x19,	// extended stats
	EXTDATA_Stats_Change	= 0x1a,	// extended stats
	EXTDATA_NewSpellbook	= 0x1b,
	EXTDATA_NewSpellSelect	= 0x1c,
	EXTDATA_HouseDesignVer	= 0x1d,	// server
	EXTDATA_HouseDesignDet	= 0x1e, // client
	//
	EXTDATA_HouseCustom		= 0x20,
	EXTDATA_Ability_Confirm	= 0x21,	// server (empty packet only id is required)
	EXTDATA_DamagePacketOld	= 0x22,	//  server
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
	//
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

#define MAX_TALK_BUFFER		256	// how many chars can anyone speak all at once? (client speech is limited to 128 chars and journal is limited to 256 chars)

union CExtData
{
	byte Default[1];			// Unknown stuff.
	ndword WalkCode[16];	// EXTDATA_WalkCode_Prime or EXTDATA_WalkCode_Add

	struct
	{
		byte m_code;	// PARTYMSG_TYPE
		byte m_data[32];
	} Party_Msg_Opt;	// from client.

	struct
	{
		byte m_code;	// PARTYMSG_TYPE
		ndword m_UID;		// source/dest uid.
		NCHAR m_msg[MAX_TALK_BUFFER];	// text of the msg. var len
	} Party_Msg_Rsp;	// from server.

	struct
	{
		byte m_code;	// PARTYMSG_TYPE
		byte m_Qty;
		ndword m_uids[3];
	} Party_Msg_Data;	// from client.

	struct // EXTDATA_Map_Change = 0x08
	{
		byte m_state;	// 1 = on
	} MapChange;

	struct	// EXTDATA_GumpChange = 0x04
	{
		ndword	dialogID;
		ndword	buttonID;
	} GumpChange;	// from server

	struct	// EXTDATA_ScreenSize = 0x05
	{
		ndword m_x;
		ndword m_y;
	} ScreenSize;	// from client

	struct	// EXTDATA_Arrow_Click = 0x07
	{
		byte m_rightClick;	// arrow was right clicked
	} QuestArrow;

	struct	// EXTDATA_Lang = 0x0b
	{
		char m_code[3];	// CLanguageID
	} Lang;	// from client

	struct // EXTDATA_OldAOSTooltipInfo	= 0x10 (server version)
	{
		ndword m_uid;
		ndword m_ListID;
	} OldAOSTooltipInfo;

	struct // EXTDATA_OldAOSTooltipInfo = 0x10 (client version)
	{
		ndword m_uid;
	} OldAOSTooltipRequest;

	struct  // EXTDATA_Popup_Request = 0x13
	{
		ndword m_UID;
	} Popup_Request; // from client

	struct  // EXTDATA_Popup_Display = 0x14
	{
		nword m_unk1; // 2
		ndword m_UID;
		byte m_NumPopups;
		struct
		{
			ndword m_TextID;
			nword m_EntryTag;
			nword m_Flags;
			char memreserve[112];
		} m_List;
	} KRPopup_Display;

	struct  // EXTDATA_Popup_Display = 0x14
	{
		nword m_unk1; // 1
		ndword m_UID;
		byte m_NumPopups;
		struct
		{
			nword m_EntryTag;
			nword m_TextID;
			nword m_Flags;
			nword m_Color;
			char memreserve[112];
		} m_List;
	} Popup_Display;

	struct  // EXTDATA_Popup_Select = 0x15
	{
		ndword m_UID;
		nword m_EntryTag;
	} Popup_Select;
	
	struct // EXTDATA_Codex_Wisdom = 0x17
	{
		byte m_unk_1; // 0x01 always
		ndword m_messageid;
		byte m_presentation; // 0 = Flash, 1 = Direct
	} Codex_Wisdom;
	
	struct // EXTDATA_Map_Diff = 0x18
	{
		ndword m_maps_number;
		struct
		{
			ndword m_map_patch;
			ndword m_static_patch;
		} m_patch[1];
	} Map_Diff;

	struct // EXTDATA_Stats_Enable = 0x19 type 0
	{
		byte m_type; // 0x00
		ndword m_UID; // (of Bonded Pet?)
		byte m_unk; // 0x01
	} Bonded_Status;

	struct // EXTDATA_Stats_Enable = 0x19 type 2
	{
		byte m_type; // 0x02
		ndword m_UID;
		byte m_unk; // 0x00
		byte m_lockbits; // Bits: XXSSDDII (s=strength, d=dex, i=int), 0 = up, 1 = down, 2 = locked
	} Stats_Enable;
	
	struct // EXTDATA_Stats_Change = 0x1A
	{
		byte m_stat;
		byte m_status;
	} Stats_Change;
	
	struct  // EXTDATA_NewSpellbook = 0x1b
	{
		nword m_Unk1;
		ndword m_UID;
		nword m_ItemId;
		nword m_Offset;
#ifdef _WIN32
		dword m_Content0;
		dword m_Content1;
#else
		ndword m_Content0;
		ndword m_Content1;
#endif
	} NewSpellbook;

	struct  // EXTDATA_NewSpellSelect = 0x1C
	{
		nword m_Unk1;
		nword m_SpellId;
	} NewSpellSelect;

	struct  // EXTDATA_NewSpellSelectWyatt = 0x1C (version as in wyatt docs)
	{
		nword m_HasSpell; // 2=no spell, 1=has spellbook, 0=no spellbook but has spell
		ndword m_SpellBook; // if 1 spellbook uid
		nword m_Expansionflag; // 0x0 = LBR&AOS; 0x1 = SE; 0x2 = ML;
		nword m_SpellId; // 0x00 if last spell
	} NewSpellSelectWyatt;

	struct // EXTDATA_HouseDesignVer = 0x1d
	{
		ndword m_HouseUID;
		ndword m_Version;
	} HouseDesignVersion;

	struct // EXTDATA_HouseDesignDet = 0x1e
	{
		ndword m_HouseUID;
	} HouseDesignDetail;

	struct // EXTDATA_HouseCustom = 0x20
	{
		ndword m_HouseSerial;
		byte m_type; // 0x04 = start; 0x05 = end;
		nword m_unk1; // 0x0000
		nword m_unk2; // 0xFFFF
		nword m_unk3; // 0xFFFF
		byte  m_unk4; // 0xFF
	} HouseCustom;

	struct  // EXTDATA_DamagePacketOld = 0x22 (LBR/AOS damage counter)
	{
		byte m_unk1;
		ndword m_UID;
		byte m_dmg;
	} DamagePacketOld;  // from server

	struct	// EXTDATA_Unk24
	{
		byte m_unk; // variable
	} AntiCheat; // from client

	struct // EXTDATA_SpellSE = 0x25
	{
		byte m_unk; // 0x01
		byte m_SpellID;
		byte m_Enable; // 0x00/0x01
	} SpellSE;

	struct // EXTDATA_SpeedMode	= 0x26
	{
		byte m_speed; // 0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement
	} SpeedMode;

	struct // EXTDATA_BandageMacro = 0x2c
	{
		ndword m_bandageSerial;
		ndword m_targetSerial;
	} BandageMacro;

	struct // EXTDATA_GargoyleFly = 0x32
	{
		nword m_one;	// always 1
		dword m_zero;	// always 0
	} GargoyleFly;

}PACK_NEEDED;

union CExtAosData
{
	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseBackup;
	
	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseRestore;

	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseCommit;
	
	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Dispid;
		byte m_Unk_00_2; // 0x00
		ndword m_PosX;
		byte m_Unk_00_3; // 0x00
		ndword m_PosY;
		byte m_Unk_00_4; // 0x00
		ndword m_PosZ;
		byte m_Unk; 	 // 0x0A
		
	} HouseDestroyItem;
	
	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Dispid;
		byte m_Unk_00_2; // 0x00
		ndword m_PosX;
		byte m_Unk_00_3; // 0x00
		ndword m_PosY;
		byte m_Unk; 	 // 0x0A
		
	} HousePlaceItem;

	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseExit;

	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Dispid;
		byte m_Unk_00_2; // 0x00
		ndword m_PosX;
		byte m_Unk_00_3; // 0x00
		ndword m_PosY;
		byte m_Unk; 	 // 0x0A
		
	} HousePlaceStair;

	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseSynch;
	
	struct
	{
		byte m_Unk; // 0x0A
		
	} HouseClear;

	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Floor;
		byte m_Unk; 	 // 0x0A
		
	} HouseSwitchFloor;

	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Roof;
		byte m_Unk_00_2; // 0x00
		ndword m_PosX;
		byte m_Unk_00_3; // 0x00
		ndword m_PosY;
		byte m_Unk_00_4; // 0x00
		ndword m_PosZ;
		byte m_Unk;		 // 0x0A
	} HousePlaceRoof;

	struct
	{
		byte m_Unk_00_1; // 0x00
		ndword m_Roof;
		byte m_Unk_00_2; // 0x00
		ndword m_PosX;
		byte m_Unk_00_3; // 0x00
		ndword m_PosY;
		byte m_Unk_00_4; // 0x00
		ndword m_PosZ;
		byte m_Unk;		 // 0x0A
	} HouseDestroyRoof;

	struct
	{
		byte m_Unk_0;	// 0x00
		ndword m_Ability;
		byte m_Unk; 	// 0x0A
		
	} SpecialMove;
	
	struct
	{
		byte m_Unk; 	 // 0x0A
		
	} HouseRevert;

	struct
	{
		byte m_Unk;		// 0x0A
	} EquipLastWeapon;
	
	struct
	{
		byte m_Unk; // 0x0A
	} GuildButton;
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

enum SEASON_TYPE
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
	CHATMSG_NoError = -1,				// -1 = Our return code for no error, but no message either
	CHATMSG_Error = 0,					// 0 - Error
	CHATMSG_AlreadyIgnoringMax,			// 1 - You are already ignoring the maximum amount of people
	CHATMSG_AlreadyIgnoringPlayer,		// 2 - You are already ignoring <name>
	CHATMSG_NowIgnoring,				// 3 - You are now ignoring <name>
	CHATMSG_NoLongerIgnoring,			// 4 - You no longer ignoring <name>
	CHATMSG_NotIgnoring,				// 5 - You are not ignoring <name>
	CHATMSG_NoLongerIgnoringAnyone,		// 6 - You are no longer ignoring anyone
	CHATMSG_InvalidConferenceName,		// 7 - That is not a valid conference name
	CHATMSG_AlreadyAConference,			// 8 - There is already a conference of that name
	CHATMSG_MustHaveOps,				// 9 - You must have operator status to do this
	CHATMSG_ConferenceRenamed,			// a - Conference <name> renamed to .
	CHATMSG_MustBeInAConference,		// b - You must be in a conference to do this. To join a conference, select one from the Conference menu
	CHATMSG_NoPlayer,					// c - There is no player named <name>
	CHATMSG_NoConference,				// d - There is no conference named <name>
	CHATMSG_IncorrectPassword,			// e - That is not the correct password
	CHATMSG_PlayerIsIgnoring,			// f - <name> has chosen to ignore you. None of your messages to them will get through
	CHATMSG_RevokedSpeaking,			// 10 - The moderator of this conference has not given you speaking priveledges.
	CHATMSG_ReceivingPrivate,			// 11 - You can now receive private messages
	CHATMSG_NoLongerReceivingPrivate,	// 12 - You will no longer receive private messages. Those who send you a message will be notified that you are blocking incoming messages
	CHATMSG_ShowingName,				// 13 - You are now showing your character name to any players who inquire with the whois command
	CHATMSG_NotShowingName,				// 14 - You are no long showing your character name to any players who inquire with the whois command
	CHATMSG_PlayerIsAnonymous,			// 15 - <name> is remaining anonymous
	CHATMSG_PlayerNotReceivingPrivate,	// 16 - <name> has chosen to not receive private messages at the moment
	CHATMSG_PlayerKnownAs,				// 17 - <name> is known in the lands of britania as .
	CHATMSG_PlayerIsKicked,				// 18 - <name> has been kicked out of the conference
	CHATMSG_ModeratorHasKicked,			// 19 - <name>, a conference moderator, has kicked you out of the conference
	CHATMSG_AlreadyInConference,		// 1a - You are already in the conference <name>
	CHATMSG_PlayerNoLongerModerator,	// 1b - <name> is no longer a conference moderator
	CHATMSG_PlayerIsAModerator,			// 1c - <name> is now a conference moderator
	CHATMSG_RemovedListModerators,		// 1d - <name> has removed you from the list of conference moderators.
	CHATMSG_YouAreAModerator,			// 1e - <name> has made you a conference moderator
	CHATMSG_PlayerNoSpeaking,			// 1f - <name> no longer has speaking priveledges in this conference.
	CHATMSG_PlayerNowSpeaking,			// 20 - <name> now has speaking priveledges in this conference
	CHATMSG_ModeratorRemovedSpeaking,	// 21 - <name>, a channel moderator, has removed your speaking priveledges for this conference.
	CHATMSG_ModeratorGrantSpeaking,		// 22 - <name>, a channel moderator, has granted you speaking priveledges for this conference.
	CHATMSG_SpeakingByDefault,			// 23 - From now on, everyone in the conference will have speaking priviledges by default.
	CHATMSG_ModeratorsSpeakDefault,		// 24 - From now on, only moderators in this conference will have speaking priviledges by default.
	CHATMSG_PlayerTalk,					// 25 - <name>:
	CHATMSG_PlayerEmote,				// 26 - *<name>*
	CHATMSG_PlayerPrivate,				// 27 - [<name>]:
	CHATMSG_PasswordChanged,			// 28 - The password to the conference has been changed
	CHATMSG_NoMorePlayersAllowed,		// 29 - Sorry--the conference named <name> is full and no more players are allowed in.

	CHATMSG_SendChannelName =	0x03e8, // Message to send a channel name and mode to client's channel list
	CHATMSG_RemoveChannelName =	0x03e9, // Message to remove a channel name from client's channel list
	CHATMSG_GetChatName =		0x03eb,	// Ask the client for a permanent chat name
	CHATMSG_CloseChatWindow =	0x03ec,	// Close the chat system dialogs on the client???
	CHATMSG_OpenChatWindow =	0x03ed,	// Open the chat system dialogs on the client
	CHATMSG_SendPlayerName =	0x03ee,	// Message to add a player out to client (prefixed with a "1" if they are the moderator or a "0" if not
	CHATMSG_RemoveMember =		0x03ef,	// Message to remove a player from clients channel member list
	CHATMSG_ClearMemberList =	0x03f0,	// This message clears the list of channel participants (for when leaving a channel)
	CHATMSG_UpdateChannelBar =	0x03f1,	// This message changes the name in the channel name bar
	CHATMSG_QTY							// Error (but does 0x03f1 anyways)
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

enum WEATHER_TYPE
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
	EFFECT_OBJ			// effect at single Object.
};

enum NOTO_TYPE
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

// client versions (expansions)
#define MINCLIVER_LBR				3000702	// minimum client to activate LBR packets (3.0.7b)
#define MINCLIVER_AOS				4000000	// minimum client to activate AOS packets (4.0.0a)
#define MINCLIVER_SE				4000500	// minimum client to activate SE packets (4.0.5a)
#define MINCLIVER_ML				5000000	// minimum client to activate ML packets (5.0.0a)
#define MINCLIVER_SA				7000000	// minimum client to activate SA packets (7.0.0.0)
#define MINCLIVER_HS				7000900	// minimum client to activate HS packets (7.0.9.0)
#define MINCLIVER_TOL				7004565	// minimum client to activate TOL packets (7.0.45.65)

// client versions (extended status gump info)
#define MINCLIVER_STATUS_V2			3000804	// minimum client to receive v2 of 0x11 packet (3.0.8d)
#define MINCLIVER_STATUS_V3			3000810	// minimum client to receive v3 of 0x11 packet (3.0.8j)
#define MINCLIVER_STATUS_V4			4000000	// minimum client to receive v4 of 0x11 packet (4.0.0a)
#define MINCLIVER_STATUS_V5			5000000	// minimum client to receive v5 of 0x11 packet (5.0.0a)
#define MINCLIVER_STATUS_V6			7003000	// minimum client to receive v6 of 0x11 packet (7.0.30.0)

// client versions (behaviours)
#define MINCLIVER_CHECKWALKCODE		1260000	// minimum client to use walk crypt codes for fastwalk prevention (obsolete)
#define MINCLIVER_PADCHARLIST		3000010	// minimum client to pad character list to at least 5 characters
#define MINCLIVER_AUTOASYNC			4000000	// minimum client to auto-enable async networking
#define MINCLIVER_NOTOINVUL			4000000	// minimum client required to view noto_invul health bar
#define MINCLIVER_SKILLCAPS			4000000	// minimum client to send skill caps in 0x3A packet
#define MINCLIVER_CLOSEDIALOG		4000400	// minimum client where close dialog does not trigger a client response
#define MINCLIVER_COMPRESSDIALOG	5000000	// minimum client to receive zlib compressed dialogs (5.0.0a)
#define MINCLIVER_NEWVERSIONING		5000605	// minimum client to use the new versioning format (after 5.0.6e it change to 5.0.6.5)
#define MINCLIVER_ITEMGRID			6000107	// minimum client to use grid index (6.0.1.7)
#define MINCLIVER_NEWSECURETRADE	7004565	// minimum client to use virtual gold/platinum on trade window (7.0.45.65)

// client versions (packets)
#define MAXCLIVER_REVERSEIP			4000000	// maximum client to reverse IP in 0xA8 packet
#define MINCLIVER_CUSTOMMULTI		4000000	// minimum client to receive custom multi packets
#define MINCLIVER_SPELLBOOK			4000000	// minimum client to receive 0xBF.0x1B packet (4.0.0a)
#define MINCLIVER_DAMAGE			4000000	// minimum client to receive 0xBF.0x22 packet
#define MINCLIVER_TOOLTIP			4000000	// minimum client to receive tooltip packets
#define MINCLIVER_STATLOCKS			4000100	// minimum client to receive 0xBF.0x19.0x02 packet
#define MINCLIVER_TOOLTIPHASH		4000500	// minimum client to receive 0xDC packet
#define MINCLIVER_NEWDAMAGE			4000700	// minimum client to receive 0x0B packet (4.0.7a)
#define MINCLIVER_NEWBOOK			5000000	// minimum client to receive 0xD4 packet
#define MINCLIVER_BUFFS				5000202	// minimum client to receive buff packets (5.0.2b)
#define MINCLIVER_NEWCONTEXTMENU	6000000	// minimun client to receive 0xBF.0x14.0x02 packet instead of 0xBF.0x14.0x01 (6.0.0.0)
#define MINCLIVER_EXTRAFEATURES		6001402	// minimum client to receive 4-byte feature mask (6.0.14.2)
#define MINCLIVER_NEWMOBILEANIM		7000000	// minimum client to receive 0xE2 packet (7.0.0.0)
#define MINCLIVER_SMOOTHSHIP		7000900	// minimum client to receive 0xF6 packet (7.0.9.0)
#define MINCLIVER_NEWMAPDISPLAY		7001300	// minimum client to receive 0xF5 packet (7.0.13.0)
#define MINCLIVER_EXTRASTARTINFO 	7001300	// minimum client to receive extra start info (7.0.13.0)
#define MINCLIVER_NEWMOBINCOMING	7003301	// minimun client to receive 0x78 packet (7.0.33.1)

enum TALKMODE_TYPE	// Modes we can talk/bark in.
{
	TALKMODE_SYSTEM = 0,	// 0 = Normal system message
	TALKMODE_PROMPT,		// 1 = Display as system prompt
	TALKMODE_EMOTE,			// 2 = *smiles* at object (client shortcut: :+space)
	TALKMODE_SAY,			// 3 = A chacter speaking.
	TALKMODE_OBJ,			// 4 = At object
	TALKMODE_NOTHING,		// 5 = Does not display
	TALKMODE_ITEM,			// 6 = text labeling an item. Preceeded by "You see"
	TALKMODE_NOSCROLL,		// 7 = As a status msg. Does not scroll
	TALKMODE_WHISPER,		// 8 = Only those close can here. (client shortcut: ;+space)
	TALKMODE_YELL,			// 9 = Can be heard 2 screens away. (client shortcut: !+space)
	TALKMODE_SPELL,			// 10 = Used by spells
	TALKMODE_GUILD = 0xd,	// 13 = Used by guild chat (client shortcut: \)
	TALKMODE_ALLIANCE,		// 14 = Used by alliance chat (client shortcut: shift+\)
	TALKMODE_BROADCAST = 0xFF
};

enum SKILLLOCK_TYPE
{
	SKILLLOCK_UP = 0,
	SKILLLOCK_DOWN,
	SKILLLOCK_LOCK
};

enum DEATH_MODE_TYPE	// DeathMenu
{
	DEATH_MODE_MANIFEST = 0,
	DEATH_MODE_RES_IMMEDIATE,
	DEATH_MODE_PLAY_GHOST
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

struct CEventCharDef
{
#define MAX_ITEM_NAME_SIZE	256		// imposed by client for protocol
#define MAX_NAME_SIZE 30
#define ACCOUNT_NAME_VALID_CHAR " !\"#$%&()*,/:;<=>?@[\\]^{|}~"
#define MAX_ACCOUNT_NAME_SIZE MAX_NAME_SIZE
#define MAX_ACCOUNT_PASSWORD_ENTER 16	// client only allows n chars.
	char m_charname[MAX_ACCOUNT_NAME_SIZE];
	char m_charpass[MAX_NAME_SIZE];	// but the size of the structure is 30 (go figure)
};

struct CEvent	// event buffer from client to server..
{
#define MAX_BUFFER			15360	// Buffer Size (For socket operations)
#define MAX_ITEMS_CONT		255		// Max items in a container. (arbitrary)
#define MAX_MENU_ITEMS		64		// number of items in a menu. (arbitrary)
#define MAX_CHARS_PER_ACCT	7

	// Some messages are bydirectional.

	union
	{
		byte m_Raw[ MAX_BUFFER ];

		dword m_CryptHeader;	// This may just be a crypt header from the client.

		struct
		{
			byte m_Cmd;		// XCMD_TYPE, 0 = ?
			byte m_Arg[1];	// unknown size.
		} Default;	// default unknown type.

		struct	// XCMD_Create, size = 100	// create a new char
		{
			byte m_Cmd;		// 0=0
			ndword m_pattern1; // 0xedededed
			ndword m_pattern2; // 0xffffffff
			byte m_kuoc; // KUOC Signal (0x00 normally. 0xff for Krrios' client)

			char m_charname[MAX_NAME_SIZE];		// 10
			//char m_charpass[MAX_NAME_SIZE];		// Not used anymore
			
			byte m_unk8[2];
			ndword m_flags;
			byte m_unk10[8];
			byte m_prof;
			byte m_unk11[15];

			byte m_sex;		// 70, 0 = male
			byte m_str;		// 71
			byte m_dex;		// 72
			byte m_int;		// 73

			byte m_skill1;
			byte m_val1;
			byte m_skill2;
			byte m_val2;
			byte m_skill3;
			byte m_val3;

			nword m_wSkinHue;	// 0x50 // HUE_TYPE
			nword m_hairid;
			nword m_hairHue;
			nword m_beardid;
			nword m_beardHue;	// 0x58
			byte m_unk2;
			byte m_startloc;
			byte m_unk3;
			byte m_unk4;
			byte m_unk5;
			byte m_slot;
			byte m_clientip[4];
			nword m_shirtHue;
			nword m_pantsHue;
		} Create;

		struct	// size = 7
		{
			byte m_Cmd;		// 0 = 0x02
			byte m_dir;		// 1 = DIR_TYPE (| 0x80 = running)
			byte m_count; 	// 2 = just a count that goes up as we walk. (handshake)
			ndword m_cryptcode;
		} Walk;

		struct // size = 38
		{
			byte m_Cmd;		// 0     = 0xF0  XCMD_WalkRequestNew
			nword m_len;	// 1-2   = length of packet
			byte m_unk1;	// 3     = 01
			ndword m_unk2;	// 4-7   = 00 00 01 22
			ndword m_unk3;	// 8-11  = AE . DF 64
			ndword m_unk4;	// 12-15 = 00 00 01 22
			ndword m_unk5;	// 16-19 = AE 8A BC 1A
			byte m_count;	// 20    = increments every packet
			byte m_dir;		// 21    = new direction
			ndword m_mode;	// 22-25 = movement mode (1 = walk, 2 = run)
			ndword m_x;		// 26-29 = new x position
			ndword m_y;		// 30-33 = new y position
			ndword m_z;		// 34-37 = new z position
		} WalkNew;

		struct // size = 9
		{
			byte m_Cmd;		// 0   = 0xF1  XCMD_TSyncRequest
			ndword m_unk1;	// 1-4 = 00 00 01 22
			ndword m_unk2;	// 5-8 = AE .. E1 0E
		} TSyncRequest;

		struct	// size = >3	// user typed in text.
		{
			byte m_Cmd;		// 0 = 0x03
			nword m_len;	// 1,2=length of packet
			byte m_mode;	// 3=mode(9=yell) TALKMODE_TYPE
			nword m_wHue;
			nword m_font;	// 6,7 = FONT_TYPE
			char m_text[1];	// 8=var size MAX_TALK_BUFFER
		} Talk;

		struct	// size = 5
		{
			byte m_Cmd;	// 0 = 0x05, 0x06 or 0x09
			ndword m_UID;
		} Click;	// Attack, DClick, Click

		struct // size = 7 = pick up an item
		{
			byte m_Cmd;			// 0 = 0x07
			ndword m_UID;		// 1-4
			nword m_amount;
		} ItemPickupReq;

		struct	// size = 14 = drop item on ground or in container.
		{
			byte m_Cmd;			// 0 = 0x08
			ndword m_UID;		// 1-4 = object being dropped.
			nword m_x;		// 5,6 = 255 = invalid
			nword m_y;		// 7,8
			byte m_z;			// 9
			ndword m_UIDCont;	// 10 = dropped on this object. 0xFFFFFFFF = no object
		} ItemDropReq;

		struct	// size = 15 = drop item on ground or in container.
		{
			byte m_Cmd;			// 0 = 0x08
			ndword m_UID;		// 1-4 = object being dropped.
			nword m_x;		// 5,6 = 255 = invalid
			nword m_y;		// 7,8
			byte m_z;			// 9
			byte m_grid;		// 10	(Client 6.0.1.7+)
			ndword m_UIDCont;	// 11 = dropped on this object. 0xFFFFFFFF = no object
		} ItemDropReqNew;

		struct	// size > 3
		{
			byte m_Cmd;		// 0 = 0x12
			nword m_len;	// 1-2 = len
			byte m_type;	// 3 = 0xc7=anim, 0x24=skill...
			char m_name[1];	// 4=var size string
		} ExtCmd;

		struct	// size = 10 = item dropped on paper doll.
		{
			byte m_Cmd;		// 0 = 0x13
			ndword m_UID;	// 1-4	item UID.
			byte m_layer;	// LAYER_TYPE
			ndword m_UIDChar;
		} ItemEquipReq;

		struct // size = ? reserved GM tool message.
		{
			byte m_Cmd;		// 0 = 0x11
			nword m_len;	// 1-2 = len
			byte m_type;	// 3 = sub type

		} GMToolMsg;

		struct // size = 3	// WalkAck gone bad = request a resync
		{
			byte m_Cmd;		// 0 = 0x22
			byte m_unk1[2];
		} WalkAck;

		struct // size = 7(m_mode==0) or 2(m_mode!=0)  // Manifest ghost (War Mode) or auto res ?
		{
			byte m_Cmd;		// 0 = 0x2c
			byte m_mode;	// 1 = DEATH_MODE_TYPE 0=manifest or unmanifest, 1=res w/penalties, 2=play as a ghost 
			byte m_unk2;	// 2 = 72 or 73
			byte m_manifest;// 3 = manifest or not. = war mode.
			byte m_unk4[3]; // 4 = unk = 00 32 00
		} DeathMenu;

		struct	// size = 10	// Client requests stats.
		{
			byte m_Cmd;		// 0 =0x34
			ndword m_edededed;	// 1-4 = 0xedededed
			byte m_type;	// 5 = 4 = Basic Stats (Packet 0x11 Response)  5 = Request Skills (Packet 0x3A Response)
			ndword m_UID;	// 6 = character UID for status
		} CharStatReq;

		struct // size = ??? = Get a skill lock (or multiple)
		{
			byte m_Cmd;		// 0= 0x3A
			nword m_len;	// 1= varies
			struct
			{
				nword m_index;	// skill index
				byte m_lock;	// SKILLLOCK_TYPE current lock mode (0 = none (up), 1 = down, 2 = locked)
			} skills[1];
		} Skill;

		struct // size = variable // Buy item from vendor.
		{
			byte m_Cmd;			// 0 =0x3b
			nword m_len;		// 1-2=
			ndword m_UIDVendor;	// 3-6=
			byte m_flag;	// 0x00 = no items following, 0x02 - items following
			struct
			{
				byte m_layer; // (0x1A)
				ndword m_UID; // itemID (from 3C packet)
				nword m_amount; //  # bought
			} m_item[1];
		} VendorBuy;

		struct	// size = 11	// plot course on map.
		{
			byte m_Cmd;		// 0 = 0x56
			ndword m_UID;  // uid of the map
			byte m_action;	// 1, MAPCMD_TYPE 1: add pin, 5: delete pin, 6: toggle edit/noedit or cancel while in edit
			byte m_pin;
			nword m_pin_x;
			nword m_pin_y;
		} MapEdit;

		struct	// size = 0x49 = 73	// play this slot char
		{
			byte m_Cmd;			// 0 = 0x5D
			ndword m_edededed;	// 1-4 = ed ed ed ed
			char m_charname[MAX_NAME_SIZE];
			char m_charpass[MAX_NAME_SIZE];
			ndword m_slot;		// 0x44 = slot
			byte m_clientip[4];	// = 18 80 ea 15
		} CharPlay;

#define MAX_BOOK_PAGES 64	// arbitrary max number of pages.
#define MAX_BOOK_LINES 8	// max lines per page.

		struct	// size > 3 // user is changing a book page.
		{
			byte m_Cmd;		// 0 = 0x66
			nword m_len;	// 1-2
			ndword m_UID;	// 3-6 = the book
			nword m_pages;	// 7-8 = 0x0001 = # of pages here
			// repeat these fields for multiple pages.
			struct
			{
				nword m_pagenum;	// 9-10=page number  (1 based)
				nword m_lines;	// 11
				char m_text[1];
			} m_page[1];
		} BookPage;

		struct	// size = var // Client text Hue changed but it does not tell us to what !!
		{
			byte m_Cmd;		// 0=0x69
			nword m_len;
			nword m_index;
		} Options;

		struct	// size = 19
		{
			byte m_Cmd;		// 0 = 0x6C
			byte m_TargType;	// 1 = 0=select object, 1=x,y,z=allow ground
			ndword m_context;	// 2-5 = we sent this at target setup.
			byte m_fCheckCrime;		// 6= 0

			ndword m_UID;	// 7-10	= serial number of the target.
			nword m_x;		// 11,12
			nword m_y;		// 13,14
			byte m_unk2;	// 15 = fd ?
			byte m_z;		// 16
			nword m_id;		// 17-18 = static id of tile
		} Target;

		struct // size = var // Secure trading
		{
			byte m_Cmd;		// 0=0x6F
			nword m_len;	// 1-2 = len
			byte m_action;	// 3 = trade action
			ndword m_UID;	// 4-7 = uid
			ndword m_UID1;	// 8-11
		} SecureTrade;

		struct // size = var (0x0c)	// Bulletin request
		{
			byte m_Cmd;		// 0=0x71
			nword m_len;	// 1-2 = len = 0x0c
			byte m_flag;	// 3= BBOARDF_TYPE 0=board name, 5=new message, 4=associate message, 1=message header, 2=message body
			ndword m_UID;	// 4-7 = UID for the bboard.
			ndword m_UIDMsg; // 8- = name or links data.
			byte m_data[1];	// submit new message data here.
		} BBoard;

		struct // size = 5	// Client wants to go into war mode.
		{
			byte m_Cmd;		// 0 = 0x72
			byte m_warmode;	// 1 = 0 or 1
			byte m_unk2[3];	// 2 = 00 32 00
		} War;

		struct // size = 2	// ping goes both ways.
		{
			byte m_Cmd;		// 0 = 0x73
			byte m_bCode;	// 1 = seems to make no diff
		} Ping;

		struct // size = 35	// set the name for this NPC (pet) char
		{
			byte m_Cmd;		// 0=0x75
			ndword m_UID;
			char m_charname[ MAX_NAME_SIZE ];
		} CharName;

		struct	// size = 13	// choice off a menu
		{
			byte m_Cmd;		// 0 = 0x7d
			ndword m_UID;	// 1 = char id from 0x7c message
			nword m_context;	// 5,6 = context info passed from server
			nword m_select;	// 7,8	= what item selected from menu. 1 based ?
			ndword m_unk9;
		} MenuChoice;

		struct // size = 62		// first login to req listing the servers.
		{
			byte m_Cmd;	// 0 = 0x80 = XCMD_ServersReq
			char m_acctname[ MAX_ACCOUNT_NAME_SIZE ];
			char m_acctpass[ MAX_NAME_SIZE ];
			byte m_loginKey;	// 61 = NextLoginKey from uo.cfg
		} ServersReq;

		struct	// size = 39  // delete the char in this slot.
		{
			byte m_Cmd;		// 0 = 0x83
			byte m_charpass[MAX_NAME_SIZE];
			ndword m_slot;	// 0x22
			byte m_clientip[4];
		} CharDelete;

		struct	// size = 65	// request to list the chars I can play.
		{
			byte m_Cmd;		  // 0 = 0x91
			ndword m_Account; // 1-4 = account id from XCMD_Relay message to log server.
			char m_acctname[MAX_ACCOUNT_NAME_SIZE];	// This is corrupted or encrypted seperatly ?
			char m_acctpass[MAX_NAME_SIZE];
		} CharListReq;

		struct // size = 98 // Get a change to the book title.
		{
			byte m_Cmd;			// 0 = 0x93
			ndword m_UID;		// 1-4 = book
			byte m_writable;	// 5 = 0 = non writable, 1 = writable. (NOT USED HERE)
			byte m_NEWunk1;
			nword m_pages;		// 6-7 = number of pages. (NOT USED HERE)
			char m_title[ 2 * MAX_NAME_SIZE ];
			char m_author[ MAX_NAME_SIZE ];
		} BookOpen;

		struct	// size = 9		// select Hue from dye chart.
		{
			byte m_Cmd;		// 0x95
			ndword m_UID;	// 1-4
			nword m_unk5;	// = 0
			nword m_wHue;	// 7,8	HUE_TYPE
		} DyeVat;

		struct	// size = 9		// New AllNames3D.
		{
			byte m_Cmd;		// 0x98
			nword m_len;	// Always 7
			ndword m_UID;
		} AllNames3D;

		struct // size = 16+var = console Prompt response.
		{
			byte m_Cmd;		// 0x9a
			nword m_len;	// 1-2
			ndword m_serial;	// 3-6 = serial
			ndword m_prompt;	// 7-10 = prompt id
			ndword m_type;	// 11-14 = type (0=request/esc, 1=reply)
			char m_text[1];	// 15+ = null terminated text.
		} Prompt;

		struct // size = 0x102	// GM page = request for help.
		{
			byte m_Cmd;		// 0 = 0x9b
			byte m_unk1[0x101];
		} HelpPage;

		struct // size = var // Sell stuff to vendor
		{
			byte m_Cmd;		// 0=0x9F
			nword m_len;	// 1-2
			ndword m_UIDVendor;	// 3-6
			nword m_count;		// 7-8
			struct
			{
				ndword m_UID;
				nword m_amount;
			} m_item[1];
		} VendorSell;

		struct	// size = 3;	// relay me to the server i choose.
		{
			byte m_Cmd;		// 0 = 0xa0
			nword m_select;	// 2 = selection this server number
		} ServerSelect;

		struct	// sizeof = 149 // not sure what this is for.
		{
			byte m_Cmd;		// 0=0xA4
			byte  m_ProcessorType;		// @0 = 03
			word m_ProcessorClock;		// @1 = 01 8E
			byte  m_nProcessors;		// @3 = 01
			wchar m_wDirectory[0x10];	// @4 = 00 00
			byte m_szVideoCardDescrip2[0x20]; // @24 = AA 7E 3F 04 00
			byte m_szModemManufacturer[0x10];	// @44 = 40 F8
			byte m_Unk54[0x30];
			byte m_bUnk84;				// @84 = 0
			word m_totalRAMinMB;		// @85 = ff 1e
			word m_largestPartitionInMB; // @87 = 2e 00 
			word m_Unk8a;					// @89 = 00 01
			dword m_timeZoneBias;		// @8b = 2c 77 f8 37
			byte m_bUnk8f;				// @8f = 6a
			dword m_dwUnk90;			// @90 = 20 00 00 00

			// ? m_szModemDescrip
			// ? m_szVideoCardDescrip1

		} Spy;

		struct // size = 5	// scroll close
		{
			byte m_Cmd;			// 0=0xA6
			ndword m_context;	// 1= context info passed from server
		} Scroll;

		struct	// size = 4 // request for tip
		{
			byte m_Cmd;		// 0=0xA7
			nword m_index;
			byte m_type;	// 0=tips, 1=notice
		} TipReq;

		struct // size = var // Get text from gump input
		{
			byte m_Cmd;		// 0=0xAC
			nword m_len;
			ndword m_UID;	// just another context really.
			nword m_context; // = context info passed from server
			byte m_retcode; // 0=canceled, 1=okayed
			nword m_textlen; // length of text entered
			char m_text[MAX_NAME_SIZE];
		} GumpInpValRet;

		struct // size = var // Unicode Speech Request
		{
			byte m_Cmd;		// 0=0xAD
			nword m_len;
			byte m_mode;	// (0=say, 2=emote, 8=whisper, 9=yell) TALKMODE_TYPE
			nword m_wHue;	// HUE_TYPE
			nword m_font;	// FONT_TYPE
			char m_lang[4];	// lang (null terminated, "ENU" for US english.) CLanguageID
			NCHAR m_utext[1];	// NCHAR[?] text (null terminated, ?=(msgsize-12)/2) MAX_TALK_BUFFER
		} TalkUNICODE;

		struct // size = var // Get gump button change
		{
			byte m_Cmd;				//  0 =  0xB1
			nword m_len;			//  1 -  2
			ndword m_UID;			//  1 -  4
			ndword m_context;	//  7 - 10 = context info passed from server
			ndword m_buttonID;		// 11 - 14 = what button was pressed ?
			ndword m_checkQty;	// 15 - 18
			ndword m_checkIds[1];	// 19 - ?? id of a checkbox or radio button that is checked
			ndword m_textQty;	// textentry boxes have returned data
			struct
			{
				nword m_id;		// id specified in the gump definition file
				nword m_len;		// length of the string (in unicode so multiply by 2!)
				NCHAR m_utext[1];	// character data (in unicode!):
			} m_texts[1];
		} GumpDialogRet;

		struct // size = 13?	// ?Request user name
		{
			byte m_Cmd;		// 0 = 0xb2
			ndword m_uid;	// 1 = targ uid ?
			ndword m_unk2;
			ndword m_unk3;
		} ChatReq;

		struct // size = various
		{
			byte m_Cmd;		//	0   = 0xb3
			nword m_len;	//  1-2 = packet length
			char m_lang[4];	//	3-6 = unicode language code CLanguageID
			NCHAR m_utext[1];	//	7-? = text from client
		} ChatText;

		struct // size = 64	// response to pressing the chat button
		{
			byte m_Cmd;		// 0 = 0xb5
			byte m_unk1;	// 1 = ???
			NCHAR m_uname[MAX_NAME_SIZE+1];	// 2-63 - unicode name
		} Chat;

		struct // size = 9	// request for tooltip
		{
			byte m_Cmd;			// 0 = 0xb6
			ndword m_UID;		// 1-4
			byte m_langID;	// 5 = 1 = english CLanguageID
			byte m_lang[3];	// 6-8 = ENU = english
		} ToolTipReq;

		struct	// size = 8 // Get Character Profile.
		{
			byte m_Cmd;		// 0=0xB8
			nword m_len;	// 1 - 2
			byte m_WriteMode;	// 3; 0 = Get profile, 1 = Set profile
			ndword m_UID;	// 4=uid
			byte m_unk1;	// 8
			byte m_retcode; // ????? 0=canceled, 1=okayed or something similar???
			nword m_textlen; // length of text entered
			NCHAR m_utext[1]; // Text, unicode!
		} CharProfile;

		struct	// size = 9 // mail msg. get primed with itself at start.
		{
			byte m_Cmd;		// 0=0xBB
			ndword m_uid1;	// 1-4 = uid
			ndword m_uid2;	// 5-8 = uid
		} MailMsg;

		struct	// size = ??
		{
			byte m_Cmd;		// 0=0xBD
			nword m_len;	// 1-2 = len of packet
			char m_text[1024];
		} ClientVersion;

		struct	// size = 2 // mail msg. get primed with itself at start.
		{
			byte m_Cmd;	// 0=0xC8
			byte m_Range;	// 5-18 = range
		} ViewRange;
		
		struct	// we get walk codes from the > 1.26.0 server to be sent back in the Walk messages.
		{
			// walk codes from the > 1.26.0 server to be sent back in the Walk messages
			byte m_Cmd;	// 0=0xbf
			nword m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			nword m_type;	// 3 - 4 EXTDATA_TYPE 1=prime LIFO stack, 2=add to beginning of LIFO stack, 6=add to party
			CExtData m_u;
		} ExtData;

		struct // size = 21+var = console Prompt response.
		{
			byte m_Cmd;			// 0xc2
			nword m_len;		// 1-2
			ndword m_serial;	// 3-6 = serial
			ndword m_prompt;	// 7-10 = prompt id
			ndword m_type;		// 11-14 = type (0=request/esc, 1=reply)
			char m_lang[4];		// 15-18 = language (3 chars + NULL)
			wchar m_utext[1];	// 19+ = null terminated unicode text.
		} PromptUNICODE;
		
		struct	// size = ?? // Config File (IGR)
		{
			byte m_Cmd;		// 0=0xD0
			nword m_len;	// 1 - 2
			byte m_type;	// file type
			byte m_config_data[1]; // Data m_len-4
		} ConfigFile;
		
		struct	// size = 2 // Logout Confirm
		{
			byte m_Cmd;		// 0=0xD1
			byte m_unk;	// always 1
		} LogoutStatus;

		struct // size = 17+var // Get a change to the book title.
		{
			byte m_Cmd;			// 0 = 0xD4
			nword m_len;		// 1-2 = length
			ndword m_UID;		// 3-6 = book
			byte m_unk1;		// 7 = 0x00
			byte m_writable;	// 8 = 0 = non writable, 1 = writable
			nword m_pages;		// 9-10 = number of pages
			nword m_titlelen;	// 11-12 = title length
			char m_title[1];	// 13 - = title
			nword m_authorlen;	// = author length
			char m_author[1];	// = author
		} BookOpenNew;
		
		struct	// sizeof = 268 // Spy2.
		{
			byte m_Cmd;		// 0=0xD9
			nword m_len;
			byte m_client; // 1 = client version < 4.0.1a; 0 = client version >= 4.0.1a
			ndword m_IstanceId;
			ndword m_OSMajor;
			ndword m_OSMinor;
			ndword m_OSRevision;
			byte m_CPUManufacter;
			ndword m_CPUFamily;
			ndword m_CPUModel;
			ndword m_CPUClockSpeed;
			byte m_PhysicalMemory;
			ndword m_ScreenWidth;
			ndword m_ScreenHeight;
			ndword m_ScreenDepth;
			nword m_DXMajor;
			nword m_DXMinor;
			char m_VCDesc[64];
			ndword m_VCVendor;
			ndword m_VCDevice;
			ndword m_VCMemory;
			byte m_Distribution;
			byte m_ClientRunning;
			byte m_ClientInstalled;
			byte m_PartialInstalled;
			char m_Language[4];
			char m_Unknown[64];
		} Spy2;

		struct
		{
			byte m_Cmd;	// 0 = 0xd6
			nword m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			ndword m_uid[1];
		} AosItemInfoRequest;

		struct
		{
			byte m_Cmd;	// 0=0xd7
			nword m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			ndword m_uid; 
			nword m_type;	// 3 - 4 EXTDATA_TYPE
			CExtAosData m_u;
		} ExtAosData;

		struct // XCMD_CreateNew, size = ? // create a new char (KR/SA)
		{
			byte m_Cmd;						// 0 = 0x8D
			nword m_len;					// 1 - 2 = length
			ndword m_pattern1;				// 3 - 6 = 0xedededed
			ndword m_pattern2;				// 7 - 10 = 0xffffffff
			char m_charname[MAX_NAME_SIZE];	// 11 - 40 = ascii name
			char m_unknown[30];				// 41 - 70 = "Unknown"
			byte m_profession;				// 71 = profession (0=custom)
			byte m_unk1;	// 72;
			byte m_sex;						// 73 = sex (0=male, 1=female)
			byte m_race;					// 74 = race (1=human, 2=elf, 3=gargoyle) (RACE_TYPE)
			byte m_str;						// 75 = str
			byte m_dex;						// 76 = dex
			byte m_int;						// 77 = int
			nword m_wSkinHue;				// 78 - 79 = skin hue
			byte m_unk2[8];	// 80 - 87

			// skill values only used if m_prof=0
			byte m_skill1;					// 88 = skill #1
			byte m_val1;					// 89 = skill #1 val
			byte m_skill2;					// 90 = skill #2
			byte m_val2;					// 91 = skill #2 val
			byte m_skill4;					// 92 = skill #4
			byte m_val4;					// 93 = skill #4 val
			byte m_skill3;					// 94 = skill #3
			byte m_val3;					// 94 = skill #3 val

			byte m_unk3[26];	// 95 - 121
			nword m_hairHue;				// 122 - 123 = hair hue
			nword m_hairId;					// 124 - 125 = hair id
			byte m_unk4[11];	// 126 - 136
			nword m_wSkinHue2;				// 137 - 138 = skin hue (included twice?)
			byte m_unk5;	// 139
			byte m_portrait;				// 140 = portrait id (could be nword)
			byte m_unk6;	// 141
			nword m_beardHue;				// 142 - 143 = beard hue
			nword m_beardId;				// 144 - 145 = beard id
		} CreateKR;

		struct // XCMD_BugReport
		{
			byte m_Cmd;			// 0=0xe0
			nword m_len;		// 1 - 2 (len = )
			char m_Language[4];	// 3 - 6 (lang, ENU)
			nword m_type;		// 7 - 8 (bug type)
			NCHAR m_utext[1];	// 9 - ? (NCHAR[?] text)
		} BugReport;

		struct // XCMD_KRClientType
		{
			byte m_Cmd;			// 0=0xe1
			nword m_len;		// 1 - 2 (len = )
			nword m_one;		// 3 - 4 (0x01)
			ndword m_clientType;		// 5 - 8 (0x02 = KR, 0x03 = SA)
		} KRClientType;

		struct // XCMD_EncryptionReply
		{
			byte m_Cmd;			// 0 = 0xE4
			nword m_len;		// 1 - 2 = length
			ndword m_lenUnk1;				// 3 - 6 = length of m_unk1
			byte m_unk1[1];					// 7 - ? = ?
		} EncryptionReply;

		struct // XCMD_HighlightUIRemove
		{
			byte m_Cmd;				// 0 = 0xE8
			ndword m_Uid;			// 1 - 4 = serial uid
			nword m_IdUi;			// 5 - 6 = id ui
			ndword m_destUid;		// 7 - 11 = destination serial uid
			byte m_One;				// 12 = 0x01
			byte m_One2;			// 13 = 0x01
		} HighlightUIRemove;

		struct // XCMD_UseHotbar
		{
			byte m_Cmd;				// 0 = 0xEB
			nword m_One;			// 1 - 2 = 0x01
			nword m_Six;			// 3 - 4 = 0x06
			byte m_Type;			// 5 = 0x1 � spell, 0x2 � weapon ability, 0x3 � skill, 0x4 � item, 0x5 � scroll
			byte m_Zero;			// 6 = 0x00
			ndword m_ObjectUID;		// 7 - 11 = serial uid
		} UseHotbar;

		struct // XCMD_MacroEquipItem
		{
			byte m_Cmd;			// 0=0xec
			nword m_len;		// 1 - 2 (len)
			byte m_count;		// 3	 (number of layers)
			ndword m_items[1];	// 4 - ? (items to equip)
		} MacroEquipItems;

		struct // XCMD_MacroUnEquipItem
		{
			byte m_Cmd;			// 0=0xed
			nword m_len;		// 1 - 2 (len)
			byte m_count;		// 3	 (number of layers)
			nword m_layers[1];	// 4 - ? (layers to unequip)
		} MacroUnEquipItems;

		struct // XCMD_NewSeed		// 21 bytes
		{
			byte m_Cmd;				// 0 = 0xEF
			ndword m_Seed;			// 1 - 4   = seed
			ndword m_Version_Maj;	// 5 - 8   = Ver Major
			ndword m_Version_Min;	// 9 - 12  = Ver Minor
			ndword m_Version_Rev;	// 13 - 16 = Ver Revision
			ndword m_Version_Pat;	// 17 - 20 = Ver Patch
		} NewSeed;

		struct // XCMD_PutNew	// size = 24	// draw the item at a location
		{
			byte m_Cmd;		// 0 = 0xF3
			nword m_unk;	// 1-2 = 0x001
			byte m_type;	// 3 = type (0x0=TileData, 0x2=Multi)
			ndword m_UID;	// 4-7 = UID | UID_O_ITEM | UID_F_RESOURCE
			nword m_id;		// 8-9
			byte m_dir;		// 10 = direction
			nword m_amount;	// 11-12 = amount
			nword m_unkAmount; // 13-14 = amount, no noticable effect on client
			nword m_x;		// 15-18 = x
			nword m_y;		// 19-22 = y
			byte m_z;		// 23 = char
			byte m_layer;	// 24 = item layer
			nword m_wHue;	// 25-26 = HUE_TYPE
			byte m_flags;	// 27 = 0x20 = is it movable, 0x80 = hidden
		} PutNew;

		struct // XCMD_CrashReport	// 310+ bytes
		{
			byte m_Cmd;					// 0 = 0xF4
			nword m_len;				// 1-2 = length
			byte m_versionMaj;			// 3 = exe version (major)
			byte m_versionMin;			// 4 = exe version (minor)
			byte m_versionRev;			// 5 = exe version (revision)
			byte m_versionPat;			// 6 = exe version (patch)
			nword m_x;					// 7-8 = location x
			nword m_y;					// 9-10 = location y
			byte m_z;					// 11= location z
			byte m_map;					// 12 = location map
			char m_account[32];			// 13-44 = account name
			char m_charname[32];		// 45-76 = character name
			char m_ipAddress[15];		// 77-91
			ndword m_unk;				// 92-95
			ndword m_errorCode;			// 96-99 = error code
			char m_executable[100];		// 100-199 = executable name
			char m_description[100];	// 200-299 = error description
			byte m_zero;				// 300
			ndword m_offset;			// 301-304 = exception offset
			byte m_addressCount;		// 305 = address count
			ndword m_address[1];		// 306-.. = address
		} CrashReport;

		/*
			byte m_Cmd;		// 0=0
			ndword m_pattern1; // 0xedededed
			ndword m_pattern2; // 0xffffffff
			byte m_kuoc; // KUOC Signal (0x00 normally. 0xff for Krrios' client)

			char m_charname[MAX_NAME_SIZE];		// 10
			//char m_charpass[MAX_NAME_SIZE];		// Not used anymore
			
			byte m_unk8[2];
			ndword m_flags;
			byte m_unk10[8];
			byte m_prof;
			byte m_unk11[15];

			byte m_sex;		// 70, 0 = male
			byte m_str;		// 71
			byte m_dex;		// 72
			byte m_int;		// 73

			byte m_skill1;
			byte m_val1;
			byte m_skill2;
			byte m_val2;
			byte m_skill3;
			byte m_val3;

			nword m_wSkinHue;	// 0x50 // HUE_TYPE
			nword m_hairid;
			nword m_hairHue;
			nword m_beardid;
			nword m_beardHue;	// 0x58
			byte m_unk2;
			byte m_startloc;
			byte m_unk3;
			byte m_unk4;
			byte m_unk5;
			byte m_slot;
			byte m_clientip[4];
			nword m_shirtHue;
			nword m_pantsHue;*/
		struct // XCMD_CreateHS, size = 106 // create a new char (uohs)
		{
			byte m_Cmd;						// 0 = 0xF8
			ndword m_pattern1;				// 1 - 4 = 0xedededed
			ndword m_pattern2;				// 5 - 8 = 0xffffffff
			byte m_kuoc;					// 9 = 0x0 (0xFF = Krrios)
			char m_charname[MAX_NAME_SIZE];	// 10 - 39 = ascii name
			byte m_unk1[2];					// 40 - 41 = 0x00
			ndword m_flags;					// 42 - 45 = flags
			byte m_unk2[8];					// 46 - 53 = unknown
			byte m_prof;					// 54 = profession
			byte m_unk3[15];				// 55 - 69 = unknown
			byte m_sex;						// 70 = race+sex
			byte m_str;						// 71 = str
			byte m_dex;						// 72 = dex
			byte m_int;						// 73 = int
			byte m_skill1;					// 74 = skill id 1
			byte m_val1;					// 75 = skill value 1
			byte m_skill2;					// 76 = skill id 2
			byte m_val2;					// 77 = skill value 2
			byte m_skill3;					// 78 = skill id 3
			byte m_val3;					// 79 = skill value 3
			byte m_skill4;					// 80 = skill id 4
			byte m_val4;					// 81 = skill value 4
			nword m_wSkinHue;				// 82 - 83 = skin hue
			nword m_hairid;					// 84 - 85 = hair id
			nword m_hairHue;				// 86 - 87 = hair hue
			nword m_beardid;				// 88 - 89 = beard id
			nword m_beardHue;				// 90 - 91 = beard hue
			byte m_server;					// 92 = server index
			byte m_startloc;				// 93 = start location
			ndword m_slot;					// 94 - 97 = character slot
			ndword m_clientip;				// 98 - 101 = client ip
			nword m_shirtHue;				// 102 - 103 = shirt hue
			nword m_pantsHue;				// 104 - 105 = pants hue
		} CreateHS;
	};
} PACK_NEEDED;

struct CCommand	// command buffer from server to client.
{
	union
	{
		byte m_Raw[ MAX_BUFFER ];

		struct
		{
			byte m_Cmd;		// 0 = ?
			byte m_Arg[1];	// unknown size.
		} Default;	// default unknown type.

		struct  // Damage (SE/AOS damage counter)
		{
			byte m_Cmd;
			ndword m_UID;
			nword m_dmg;
		} DamagePacket;  // from server

		struct // size = 36 + 30 = 66	// Get full status on char.
		{
			byte m_Cmd;			// 0 = 0x11
			nword  m_len;		// 1-2 = size of packet (2 bytes)
			ndword m_UID;		// 3-6 = (first byte is suspected to be an id byte - 0x00 = player)
			char m_charname[ MAX_NAME_SIZE ];	// 7-36= character name (30 bytes) ( 00 padded at end)
			nword m_health;		// 37-38 = current health
			nword m_maxhealth;	// 39-40 = max health
			byte m_perm;		// 41 = permission to change name ? 0=stats invalid, 1=stats valid., 0xff = i can change the name
			byte m_ValidStats;	// 42 = 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end, 5 = include maxweight+race.
			// only send the rest here if player.
			byte m_sex;			// 43 = sex (0 = male)
			nword m_str;		// 44-45 = strength
			nword m_dex;		// 46-47 = dexterity
			nword m_int;		// 48-49 = intelligence
			nword m_stam;		// 50-51 = current stamina
			nword m_maxstam;	// 52-53 = max stamina
			nword m_mana;		// 54-55 = current mana
			nword m_maxmana;	// max mana (2 bytes)
			ndword m_gold;		// 58-61 = gold
			nword m_armor;		// 62-63 = armor class
			nword m_weight;		// 64-65 = weight in stones
			// new {5}
			// nword m_maxweight;
			// byte m_race;
			// new (2)
			nword m_statcap;
			// new (3)
			byte m_curFollower;
			byte m_maxFollower;
			// new (4)
			nword m_ResFire;
			nword m_ResCold;
			nword m_ResPoison;
			nword m_ResEnergy;
			nword m_Luck;
			nword m_minDamage;
			nword m_maxDamage;
			ndword m_Tithing;
		} Status;

		struct // size = 36 + 30 = 66	// Get full status on char.
		{
			byte m_Cmd;			// 0 = 0x11
			nword  m_len;		// 1-2 = size of packet (2 bytes)
			ndword m_UID;		// 3-6 = (first byte is suspected to be an id byte - 0x00 = player)
			char m_charname[ MAX_NAME_SIZE ];	// 7-36= character name (30 bytes) ( 00 padded at end)
			nword m_health;		// 37-38 = current health
			nword m_maxhealth;	// 39-40 = max health
			byte m_perm;		// 41 = permission to change name ? 0=stats invalid, 1=stats valid., 0xff = i can change the name
			byte m_ValidStats;	// 42 = 0 not valid, 1 = till weight, 2 = till statcap, 3 till follower, 4 = till the end.
			// only send the rest here if player.
			byte m_sex;			// 43 = sex (0 = male)
			nword m_str;		// 44-45 = strength
			nword m_dex;		// 46-47 = dexterity
			nword m_int;		// 48-49 = intelligence
			nword m_stam;		// 50-51 = current stamina
			nword m_maxstam;	// 52-53 = max stamina
			nword m_mana;		// 54-55 = current mana
			nword m_maxmana;	// max mana (2 bytes)
			ndword m_gold;		// 58-61 = gold
			nword m_armor;		// 62-63 = armor class
			nword m_weight;		// 64-65 = weight in stones
			nword m_maxWeight;	// 66-68 = max weight in stones
			byte m_race;		// 69 = race (1=human, 2=elf, 3=gargoyle) (RACE_TYPE)
			nword m_statcap;
			byte m_curFollower;
			byte m_maxFollower;
			nword m_ResFire;
			nword m_ResCold;
			nword m_ResPoison;
			nword m_ResEnergy;
			nword m_Luck;
			nword m_minDamage;
			nword m_maxDamage;
			ndword m_Tithing;
			// new {6}
			nword m_ResPhysicalMax;
			nword m_ResFireMax;
			nword m_ResColdMax;
			nword m_ResPoisonMax;
			nword m_ResEnergyMax;
			nword m_IncreaseDefChance;
			nword m_IncreaceDefChanceMax;
			nword m_IncreaseHitChance;
			nword m_IncreaseSwingSpeed;
			nword m_IncreaseDam;
			nword m_LowerReagentCost;
			nword m_IncreaseSpellDam;
			nword m_FasterCastRecovery;
			nword m_FasterCasting;
			nword m_LowerManaCost;
		} StatusNew;
 
		struct // size = ? change colour of hp bar, SA
		{
#define HEALTHCOLOR_GREEN	1
#define HEALTHCOLOR_YELLOW	2
			byte m_Cmd;		// 0 = 0x17 / XCMD_HealthBarColor
			nword m_len;	// 1-2 = len
			ndword m_UID;	// 3-6 = character serial
			nword m_count;	// 7-8 = number of states
			struct
			{
				nword m_color;	// 9-10 = status color (1=green, 2=yellow)
				byte m_state;	// 11 = color state
			} m_states[1];
		} HealthBarColor;

		struct // size = 19	or var len // draw the item at a location
		{
			byte m_Cmd;		// 0 = 0x1a = XCMD_Put
			nword m_len;	// 1-2 = var len = 0x0013 or 0x000e or 0x000f
			ndword m_UID;	// 3-6 = UID | UID_O_ITEM | UID_F_RESOURCE
			nword m_id;		// 7-8
			nword m_amount;	// 9-10 - only present if m_UID | UID_F_RESOURCE = pile (optional)
			nword m_x;		// 11-12 - | 0x8000 = m_dir arg.
			nword m_y;		// 13-14 = y | 0xC000 = m_wHue and m_flags fields.
			byte m_dir;		// (optional)
			byte m_z;		// 15 = char
			nword m_wHue;	// 16-17 = HUE_TYPE (optional)
			byte m_flags;	// 18 = 0x20 = is it movable ? (direction?) (optional)
		} Put;

		// Item flags
#define ITEMF_MOVABLE	0x20
#define ITEMF_INVIS		0x80

		struct // size = 37	// start up player
		{
			byte	m_Cmd;			// 0 = 0x1B
			ndword	m_UID;			// 1-4
			ndword	m_unk_5_8;		// 5-8 = 00 00 00 00
			nword	m_id;			// 9-10
			nword	m_x;			// 11-12
			nword	m_y;			// 13-14
			nword	m_z;			// 15-16
			byte	m_dir;			// 17
			byte	m_unk_18;		// 18 = 00
			ndword	m_unk_19_22;	// 19-22 = FF FF FF FF
			nword	m_boundX;	// 23-14
			nword	m_boundY;	// 23-14
			nword	m_mode;			// 27-28 = CHARMODE_WAR
			nword	m_boundH;	// 29-30 = 0960
			nword	m_zero_31;
			ndword	m_zero_33;
		} Start;

		// Char mode flags
#define CHARMODE_FREEZE		0x01
#define CHARMODE_FEMALE		0x02
#define CHARMODE_POISON		0x04	// green status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_FLYING		0x04	// flying (gargoyles, SA)
#define CHARMODE_YELLOW		0x08	// yellow status bar. (note: see XCMD_HealthBarColor for SA)
#define CHARMODE_IGNOREMOBS	0x10
#define CHARMODE_WAR		0x40
#define CHARMODE_INVIS		0x80

		struct // size = 14 + 30 + var
		{
			byte m_Cmd;			// 0 = 0x1C
			nword m_len;		// 1-2 = var len size.
			ndword m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			nword m_id;			// 7-8 = CREID_TYPE of speaker.
			byte m_mode;		// 9 = TALKMODE_TYPE
			nword m_wHue;		// 10-11 = HUE_TYPE.
			nword m_font;		// 12-13 = FONT_TYPE
			char m_charname[MAX_NAME_SIZE];	// 14
			char m_text[1];		// var size. MAX_TALK_BUFFER
		} Speak;

		struct // sizeo = 5	// remove an object (item or char)
		{
			byte m_Cmd;			// 0 = 0x1D
			ndword m_UID;		// 1-4 = object UID.
		} Remove;

		struct // size = 19 // set client player view x,y,z.
		{
			byte m_Cmd;			// 0 = 0x20
			ndword m_UID;		// 1-4 = my UID.
			nword m_id;			// 5-6
			byte m_zero7;		// 7 = 0
			nword m_wHue;		// 8-9 = HUE_TYPE
			byte m_mode;		// 10 = 0=normal, 4=poison, 0x40=attack, 0x80=hidden CHARMODE_WAR
			nword m_x;			// 11-12
			nword m_y;			// 13-14
			nword m_zero15;		// 15-16 = noto ?
			byte m_dir;			// 17 = high bit set = run
			byte m_z;			// 18
		} View;

		struct // size = 8
		{
			byte m_Cmd;			// 0 = 0x21
			byte m_count;		// sequence #
			nword m_x;
			nword m_y;
			byte m_dir;			// DIR_TYPE
			byte m_z;
		} WalkCancel;

		struct	// size=3
		{
			byte m_Cmd;		// 0 = 0x22
			byte m_count; 	// 1 = goes up as we walk. (handshake)
			byte m_noto;	// 2 = notoriety flag
		} WalkAck;

		struct // size = 26
		{
			byte m_Cmd;		// 0=0x23
			nword m_id;
			nword m_unk3;	// 3-4 = 0
			nword m_unk5;	// 5-9 = ?
			byte  m_unk7;	// 7 = 0, 72, 99, c1, 73
			ndword m_srcUID; // NULL if from ground.
			nword m_src_x;
			nword m_src_y;
			byte m_src_z;
			ndword m_dstUID;
			nword m_dst_x;
			nword m_dst_y;
			byte m_dst_z;
		} DragAnim;

		struct // size = 7	// Open a container gump
		{
			byte m_Cmd;		// 0 = 0x24
			ndword m_UID;	// 1-4
			nword m_gump;	// 5 = gump graphic id. GUMP_TYPE
		} ContOpen;

		struct // size = 20	// Add Single Item To Container.
		{
			byte m_Cmd;		// 0 = 0x25
			ndword m_UID;	// 1-4
			nword m_id;
			byte m_zero7;
			nword m_amount;
			nword m_x;
			nword m_y;
			ndword m_UIDCont;	// the container.
			nword m_wHue;		// HUE_TYPE
		} ContAdd;

		struct // size = 21	// Add Single Item To Container.
		{
			byte m_Cmd;		// 0 = 0x25
			ndword m_UID;	// 1-4
			nword m_id;
			byte m_zero7;
			nword m_amount;
			nword m_x;
			nword m_y;
			byte m_grid;
			ndword m_UIDCont;	// the container.
			nword m_wHue;		// HUE_TYPE
		} ContAddNew;

		struct // size = 5	// Kick the player off.
		{
			byte m_Cmd;		// 0 = 0x26
			ndword m_unk1;	// 1-4 = 0 = GM who issued kick ?
		} Kick;

		struct // size = 2	// kill a drag. (Bounce)
		{
			byte m_Cmd;		// 0 = 0x27
			byte m_type;	// 1 = bounce type ? = 5 = drag
		} DragCancel;

		struct // size = 2 Death Menu
		{
			byte m_Cmd;		// 0 = 0x2c
			byte m_shift;	// 1 = 0
		} DeathMenu;

		struct // size = 15 = equip this item
		{
			byte m_Cmd;			// 0 = 0x2e
			ndword m_UID;		// 1-4 = object UID.
			nword m_id;			// 5-6
			byte m_zero7;
			byte m_layer;		// 8=LAYER_TYPE
			ndword m_UIDchar;	// 9-12=UID num of owner.
			nword m_wHue;		// HUE_TYPE
		} ItemEquip;

		struct // size = 10	// There is a fight some place.
		{
			byte m_Cmd;		// 0 = 0x2f
			byte m_dir;		// 1 = 0 = DIR_TYPE
			ndword m_AttackerUID;
			ndword m_AttackedUID;
		} Fight;

		struct // size = ?? = Fill in the skills list.
		{
			byte m_Cmd;		// 0= 0x3A
			nword m_len;	// 1=
			byte m_single;	// 3=0 = all w/ 0 term, ff=single and no terminator
			struct
			{
				nword m_index;	// 1 based, 0 = terminator. (no val)
				nword m_val;	// Skill * 10 (stat modified!)
				nword m_valraw; // Skill * 10 (stat unmodified!)
				byte m_lock;	// current lock mode (0 = none (up), 1 = down, 2 = locked)
			} skills[1];
		} Skill;

		struct // size = ??? = Get a skill lock (or multiple)
		{
			byte m_Cmd;		// 0= 0x3A
			nword m_len;	// 1= varies
			byte m_single;	// 3=0 = all w/ 0 term, ff=single and no terminator
			struct
			{
				nword m_index;	// skill index
				nword m_val;	// Skill * 10 (stat modified!)
				nword m_valraw; // Skill * 10 (stat unmodified!)
				byte m_lock;	// SKILLLOCK_TYPE current lock mode (0 = none (up), 1 = down, 2 = locked)
				nword m_cap;	// cap of the current skill
			} skills[1];
		} Skill_New;

		struct // size = variable	// close the vendor window.
		{
			byte m_Cmd;		// 0 =0x3b
			nword m_len;
			ndword m_UIDVendor;
			byte m_flag;	// 0x00 = no items following, 0x02 - items following
		} VendorBuy;

		struct // size = 5 + ( x * 19 ) // set up the content of some container.
		{
#define MAX_ITEMS_CONTENT (MAX_BUFFER - 5) / 19
			byte m_Cmd;		// 0 = 0x3C
			nword m_len;
			nword m_count;

			struct // size = 19
			{
				ndword m_UID;	// 0-3
				nword m_id;	// 4-5
				byte m_zero6;
				nword m_amount;	// 7-8
				nword m_x;	// 9-10
				nword m_y;	// 11-12
				ndword m_UIDCont;	// 13-16 = What container is it in.
				nword m_wHue;	// 17-18 = HUE_TYPE
			} m_item[ MAX_ITEMS_CONTENT ];
		} Content;

		struct // size = 5 + ( x * 20 ) // set up the content of some container.
		{
#define MAX_ITEMS_CONTENTNEW (MAX_BUFFER - 5) / 20
			byte m_Cmd;		// 0 = 0x3C
			nword m_len;
			nword m_count;

			struct // size = 19
			{
				ndword m_UID;	// 0-3
				nword m_id;	// 4-5
				byte m_zero6;
				nword m_amount;	// 7-8
				nword m_x;	// 9-10
				nword m_y;	// 11-12
				byte m_grid;	// 13
				ndword m_UIDCont;	// 14-17 = What container is it in.
				nword m_wHue;	// 18-19 = HUE_TYPE
			} m_item[ MAX_ITEMS_CONTENTNEW ];
		} ContentNew;

		struct // size = 6	// personal light level.
		{
			byte m_Cmd;		// 0 = 0x4e
			ndword m_UID;	// 1 = creature uid
			byte m_level;	// 5 = 0 = shape of the light. (LIGHT_PATTERN?)
		} LightPoint;

		struct // size = 2
		{
			byte m_Cmd;			// 0 = 0x4f
			byte m_level;		// 1=0-19, 19=dark, On t2a 30=dark, LIGHT_BRIGHT
		} Light;

		struct	// size = 2	// Sent for idle/wrong char
		{
			byte m_Cmd;		// 0 =0x53
			byte m_Value;	// 0x05 = Another char is online / 0x07 = Idle for too much
		} IdleWarning;

		struct // size = 12
		{
			byte m_Cmd;		// 0 = 0x54
			byte m_flags;	// 1 = quiet, infinite repeat or single shot (0/1)
			nword m_id;		// 2-3=sound id (SOUND_TYPE)
			nword m_volume;	// 4-5=0 = (speed/volume modifier?)
			nword m_x;		// 6-7
			nword m_y;		// 8-9
			nword m_z;		// 10-11
		} Sound;

		struct	// size = 1	// Sent at start up. Cause Client to do full reload and redraw of config.
		{
			byte m_Cmd;		// 0 =0x55
		} ReDrawAll;

		struct	// size = 11
		{
			byte m_Cmd; // 0 = 0x56
			ndword m_UID;
			byte m_Mode; // 1: Add a pin location, 5: Display map, pins to follow 7: Editable, no pins follow 8: read only
			byte m_Req;	// 0: req for read mode, 1: request for edit mode
			nword m_pin_x;
			nword m_pin_y;
		} MapEdit;

		struct	// size = 4	// Set Game Time. not sure why the client cares about this.
		{
			byte m_Cmd;		// 0 =0x5b
			byte m_hours;
			byte m_min;
			byte m_sec;
		} Time;

		struct // size = 4
		{
			byte m_Cmd;			// 0 = 0x65
			byte m_type;		// 1 = WEATHER_TYPE. 0xff=dry, 0=rain, 1=storm, 2=snow
			byte m_ext1;		// 2 = other weather info (severity?) 0x40 = active, 0=dry
			byte m_unk010;		// 3 = =0x10 always
		} Weather;

		struct	// size = 13 + var // send a book page.
		{
			byte m_Cmd;		// 0 = 0x66
			nword m_len;	// 1-2
			ndword m_UID;	// 3-6 = the book
			nword m_pages;	// 7-8 = 0x0001 = # of pages here
			// repeat these fields for multiple pages.
			struct
			{
				nword m_pagenum;	// 9-10=page number  (1 based)
				nword m_lines;	// 11
				char m_text[1];
			} m_page[1];
		} BookPage;

		struct	// size = var	// seems to do nothing here.
		{
			byte m_Cmd;		// 0=0x69
			nword m_len;
			nword m_index;
		} Options;

		struct	// size = 19
		{
			byte m_Cmd;		// 0 = 0x6C
			byte m_TargType;	// 1 = 0=select object, 1=x,y,z
			ndword m_context;	// 2-5 = we sent this at target setup.
			byte m_fCheckCrime;		// 6= 0

			// Unused junk.
			ndword m_UID;	// 7-10	= serial number of the target.
			nword m_x;		// 11,12
			nword m_y;		// 13,14
			byte m_unk2;	// 15 = fd ?
			byte m_z;		// 16
			nword m_id;		// 17-18 = static id of tile
		} Target;

		struct // size = 3 // play music
		{
			byte m_Cmd;		// 0 = 0x6d
			nword m_musicid;// 1= music id number
		} PlayMusic;

		struct // size = 14 // Combat type animation.
		{
			byte m_Cmd;			// 0 = 0x6e
			ndword m_UID;		// 1-4=uid
			nword m_action;		// 5-6 = ANIM_TYPE
			byte m_zero7;		// 7 = 0 or 5 ?
			byte m_dir;			// DIR_TYPE
			nword m_repeat;		// 9-10 = repeat count. 0=forever.
			byte m_backward;	// 11 = backwards (0/1)
			byte m_repflag;		// 12 = 0=dont repeat. 1=repeat
			byte m_framedelay;	// 13 = 0=fastest. (number of seconds)
		} CharAction;

		struct // size = var < 47 // Secure trading
		{
			byte m_Cmd;		// 0=0x6F
			nword m_len;	// 1-2 = len
			byte m_action;	// 3 = trade action. SECURE_TRADE_TYPE
			ndword m_UID;	// 4-7 = uid = other character
			ndword m_UID1;	// 8-11 = container 1
			ndword m_UID2;	// 12-15 = container 2
			byte m_fname;	// 16 = 0=none, 1=name
			char m_charname[ MAX_NAME_SIZE ];
		} SecureTrade;

		struct // size = 28 // Graphical effect.
		{
			byte m_Cmd;			// 0 = 0x70
			byte m_motion;		// 1= the motion type. (0=point to point,3=static) EFFECT_TYPE
			ndword m_UID;		// 2-5 = The target item. or source item if point to point.
			ndword m_targUID;	// 6-9 = 0
			nword m_id;			// 10-11 = base display item 0x36d4 = fireball
			nword m_srcx;		// 12
			nword m_srcy;		// 14
			byte  m_srcz;		// 16
			nword m_dstx;		// 17
			nword m_dsty;		// 19
			byte  m_dstz;		// 21
			byte  m_speed;		// 22= 0=very fast, 7=slow.
			byte  m_loop;		// 23= 0 is really long.  1 is the shortest.
			word 	m_unk;		// 24=0 HUE_TYPE ?
			byte 	m_OneDir;		// 26=1=point in single dir else turn.
			byte 	m_explode;	// 27=effects that explode on impact.
			ndword	m_hue;
			ndword	m_render;
		} Effect;

		struct // size = var (38)	// Bulletin Board stuff
		{
			byte m_Cmd;		// 0=0x71
			nword m_len;	// 1-2 = len
			byte m_flag;	// 3= 0=board name, 4=associate message, 1=message header, 2=message body
			ndword m_UID;	// 4-7 = UID for the bboard.
			byte m_data[1];	// 8- = name or links data.
		} BBoard;

		struct // size = 5	// put client to war mode.
		{
			byte m_Cmd;		// 0 = 0x72
			byte m_warmode;	// 1 = 0 or 1
			byte m_unk2[3];	// 2 = 00 32 00
		} War;

		struct // size = 2	// ping goes both ways.
		{
			byte m_Cmd;		// 0 = 0x73
			byte m_bCode;	// 1 = ?
		} Ping;

		struct // size = var // Open buy window
		{
			byte m_Cmd;		// 0 = 0x74
			nword m_len;
			ndword m_VendorUID;
			byte m_count;
			struct
			{
				ndword m_price;
				byte m_len;
				char m_text[1];
			} m_item[1];
		} VendOpenBuy;

		struct // size = 16	// Move to a new server. Not sure why the client cares about this.
		{
			byte m_Cmd;		// 0 = 0x76
			nword m_x;
			nword m_y;
			nword m_z;
			byte m_unk7_zero;	// is a toggle of some sort
			word m_serv_boundX;	// distances ?
			word m_serv_boundY;
			nword m_serv_boundW;	// word ??
			nword m_serv_boundH;	// describes the new server some how.
		} ZoneChange;

		// 00 14 00 00 00 04 00 09 00

		struct // size = 17	// simple move of a char already on screen.
		{
			byte m_Cmd;		// 0 = 0x77
			ndword m_UID;	// 1-4
			nword m_id;		// 5-6 = id
			nword m_x;		// 7-8 = x
			nword m_y;		// 9-10
			byte m_z;		// 11
			byte m_dir;		// 12 = DIR_TYPE (| 0x80 = running ?)
			nword m_wHue;	// 13-14 = HUE_TYPE
			byte m_mode;	// 15 = CHARMODE_WAR
			byte m_noto;	// 16 = NOTO_TYPE
		} CharMove;

		struct // size = 23 or var len // draw char
		{
			byte m_Cmd;		// 0 = 0x78
			nword m_len;	// 1-2 = 0x0017 or var len?
			ndword m_UID;	// 3-6=
			nword m_id;	// 7-8
			nword m_x;	// 9-10
			nword m_y;	// 11-12
			byte m_z;		// 13
			byte m_dir;		// 14 = DIR_TYPE
			nword m_wHue;	// 15-16 = HUE_TYPE
			byte m_mode;	// 17 = CHARMODE_WAR
			byte m_noto;	// 18 = NOTO_TYPE

			struct	// This packet is extendable to show equip.
			{
				ndword m_UID;	// 0-3 = 0 = end of the list.
				nword m_id;		// 4-5 = | 0x8000 = include m_wHue.
				byte m_layer;	// LAYER_TYPE
				nword m_wHue;	// only if m_id | 0x8000
			} equip[1];

		} Char;

		struct // size = var // put up a menu of items.
		{
			byte m_Cmd;			// 0=0x7C
			nword m_len;
			ndword m_UID;		// if player then gray menu choice bar.
			nword m_context;		// = context info passed from server
			byte m_lenname;
			char m_name[1];		// var len. (not null term)
			byte m_count;
			struct		// size = var
			{
				nword m_id;		// image next to menu.
				nword m_color;	// color of the image
				byte m_lentext;
				char m_name[1];	// var len. (not null term)
			} m_item[1];
		} MenuItems;

		struct // size = var // Switch to this player? (in game)
		{
			// have no clue what it does to the crypt keys (probably nothing)
			// all I know is it puts up the list and sends back a 0x5d (Play) message...
			// if it goes further than that, it'll be awesome...what else can we do with this?

			byte m_Cmd; // 0 = 0x81
			nword m_len;
			byte m_count;
			byte m_unk;	// ? not sure.
			CEventCharDef m_char[1];
		} CharList3;

		struct // size = 2
		{
			byte m_Cmd;		// 0 = 0x82
			byte m_code;	// 1 = PacketLoginError::Reason
		} LogBad;

		struct // size = 2
		{
			byte m_Cmd;		// 0 = 0x85
			byte m_code;	// 1 = DELETE_ERR_TYPE
		} DeleteBad;

		struct // size = var	// refill char list after delete.
		{
			byte m_Cmd;		// 0 = 0x86
			nword m_len;
			byte m_count;
			CEventCharDef m_char[1];
		} CharList2;

		struct // size = 66
		{
			byte m_Cmd;			// 0 = 0x88
			ndword m_UID;		// 1-4 =
			char m_text[60];	// 5-
			byte m_mode;		// CHARMODE_WAR 0=normal, 4=poison, 0x40=attack, 0x80=hidden
		} PaperDoll;

		struct // size = 7 + count * 5 // Equip stuff on corpse.
		{
			byte m_Cmd;		// 0 = 0x89
			nword m_len;
			ndword m_UID;

			struct // size = 5
			{
				byte m_layer;	// 0 = LAYER_TYPE, list must be null terminated ! LAYER_NONE.
				ndword m_UID;	// 1-4
			} m_item[ MAX_ITEMS_CONT ];
		} CorpEquip;

		struct	// display a gump with some text on it.
		{
			byte m_Cmd; // 0x8b
			nword m_len; // 13 + len(m_unktext) + len(m_text)
			ndword m_UID; // Some uid which doesn't clash with any others that the client might have
			nword m_gump; // Signs and scrolls work well with this. GUMP_TYPE
			nword m_len_unktext; // Evidently m_unktext is either an array of bytes, or a non-nulled char array
			nword m_unk11; // who knows??
			char m_unktext[1]; // same
			char m_text[1]; // text to display
		} GumpTextDisp;

		struct	// size = 11
		{
			byte m_Cmd;			// 0 = 0x8C
			byte m_ip[4];		// 1 = struct in_addr
			nword m_port;		// 5 = Port server is on
			ndword m_Account;	// 7-10 = customer id (sent to game server)
		} Relay;

		struct	// size = 19
		{
			byte m_Cmd; // 0 = 0x90
			ndword m_UID; // uid of the map item
			nword m_Gump_Corner; // GUMP_TYPE always 0x139d....compass tile id in the corner.,
			nword m_x_ul; // upper left x coord.
			nword m_y_ul; // upper left y coord.
			nword m_x_lr; // lower right x coord.
			nword m_y_lr; // lower right y coord.
			nword m_xsize; // client width
			nword m_ysize; // client height
		} MapDisplay;	// Map1

		struct // size = 98 // Get a change to the book title.
		{
			byte m_Cmd;			// 0 = 0x93
			ndword m_UID;		// 1-4 = book
			byte m_writable;	// 5 = 0 = non writable, 1 = writable. (NOT USED HERE)
			byte m_NEWunk1;
			nword m_pages;		// 6-7 = number of pages. (NOT USED HERE)
			char m_title[ 2 * MAX_NAME_SIZE ];
			char m_author[ MAX_NAME_SIZE ];
		} BookOpen;

		struct // size = 9
		{
			byte m_Cmd;			// 0 = 0x95
			ndword m_UID;		// 1-4
			nword m_zero5;		// 5-6
			nword m_id;		// 7-8
		} DyeVat;

		struct	// size = 9		// New AllNames3D.
		{
			byte m_Cmd;		// 0x98
			nword m_len;	// Always 37
			ndword m_UID;
			char m_name[ MAX_NAME_SIZE ];
		} AllNames3D;

		struct // size = 26	// preview a house/multi
		{
			byte m_Cmd;				// 0 = 0x99
			byte m_fAllowGround;	// 1 = 1=ground only, 0=dynamic object
			ndword m_context;		// 2-5 = we sent this at target setup.
			byte m_zero6[12];		// 6-17
			nword m_id;				// 18-19 = The multi id to preview. (id-0x4000)
			nword m_x;				// 20-21 = x offset
			nword m_y;				// 22-23 = y offset
			nword m_z;				// 24-25 = z offset
		} TargetMulti;

		struct // size = 16 // console prompt request.
		{
			byte m_Cmd;		// 0 = 0x9a
			nword m_len;	// 1-2 = length = 16
			ndword m_serial;	// 3-6 = serial
			ndword m_prompt;	// 7-10 = prompt id
			ndword m_type;	// 11-14 = type (0=request/esc, 1=reply)
			char m_text[1];	// 15+ = null terminated text? doesn't seem to work.
		} Prompt;

		struct // size = var	// vendor sell dialog
		{
			byte m_Cmd;		// 0 = 0x9e
			nword m_len;	// 1-2
			ndword m_UIDVendor;	// 3-6
			nword m_count;	// 7-8
			struct
			{
				ndword m_UID;
				nword m_id;
				nword m_wHue;	// HUE_TYPE
				nword m_amount;
				nword m_price;	// Hmm what if price is > 65K?
				nword m_len;
				char m_text[1];
			} m_item[1];
		} VendOpenSell;

		struct	// size = 9	// update some change in stats.
		{
			byte m_Cmd;	// 0=0xa1 (str), 0xa2 (int), or 0xa3 (dex)
			ndword m_UID;	// 1-4
			nword m_max;	// 5-6
			nword m_val;	// 7-8
		} StatChng;

		struct // size = 3+var
		{
			byte m_Cmd;		// 0 = 0xA5
			nword m_len;
			char m_page[ 1 ];	// var size
		} Web;

		struct // size = 10 + var	// Read scroll
		{
			byte m_Cmd;			// 0=0xA6
			nword m_len;		// 1-2
			byte  m_type;		// 3 = form or gump = 0=tips,1=notice or 2 (SCROLL_TYPE)
			ndword m_context;	// 4-7 = context info passed from server
			nword m_lentext;	// 8
			char m_text[1];		// 10
		} Scroll;

		struct // size = 6+servers*40
		{
			byte m_Cmd;				// 0 = 0xA8
			nword m_len;			// 1-2
			byte m_nextLoginKey;	// 3=next login key the client should use in ServersReq (0x80) packet
			nword m_count;			// 4-5=num servers.

#define MAX_SERVERS 32
#define MAX_SERVER_NAME_SIZE 32
			struct	// size=40
			{
				nword m_count;							// 0=0 based enum
				char m_servname[32];	// 2
				byte m_percentfull;						// 34 = 25 or 2e
				char m_timezone;					// 35 = 0x05=east coast or 0x08=west coast
				byte m_ip[4];							// 36-39 = ip to ping
			} m_serv[ MAX_SERVERS ];
		} ServerList;

		struct // size = 4+(5*2*MAX_NAME_SIZE)+1+(starts*63) // list available chars for your account.
		{
			byte m_Cmd;		// 0 = 0xa9
			nword m_len;	// 1-2 = var len
			byte m_count;	// 3=5 needs to be 5 for some reason.
			CEventCharDef m_char[1];

			byte m_startcount;
			struct	// size = 63
			{
				byte m_id;
				char m_area[MAX_NAME_SIZE+1];
				char m_name[MAX_NAME_SIZE+1];
			} m_start[1];
			ndword	m_flags;
		} CharList;

		struct // size = 5 = response to attack.
		{
			byte m_Cmd;		// 0= 0xaa
			ndword m_UID;	// 1= char attacked. 0=attack refused.
		} AttackOK;

		struct // size = 13 + var // Send a gump text entry dialog.
		{
			byte m_Cmd;			// 0 = 0xab
			nword m_len;		// 1-2
			ndword m_UID;		// 3-6
			nword m_context;	// 7-8= context info passed from server
			nword m_textlen1;	// length of string #1
			char m_text1[256];	// line 1 variable text length
			byte m_cancel;		// 0 = disable, 1=enable
			byte m_style;		// 0 = disable, 1=normal, 2=numerical
			ndword m_mask;		// format[1] = max length of textbox input, format[2] = max value of textbox input
			nword m_textlen2;	// length of string #2
			char m_text2[256];	// line 2 text
		} GumpInpVal;

		struct // size = 14 + 30 + var // Unicode speech
		{
			byte m_Cmd;			// 0 = 0xAE
			nword m_len;		// 1-2 = var len size.
			ndword m_UID;		// 3-6 = UID num of speaker., 0xFFFFFFFF = noone.
			nword m_id;			// 7-8 = CREID_TYPE of speaker. 0xFFFF = none
			byte m_mode;		// 9 = TALKMODE_TYPE
			nword m_wHue;		// 10-11 = HUE_TYPE.
			nword m_font;		// 12-13 = FONT_TYPE
			char m_lang[4];		// language (same as before) CLanguageID
			char m_charname[MAX_NAME_SIZE];	//
			NCHAR m_utext[1];		// text (null terminated, ?=(msgsize-48)/2 MAX_TALK_BUFFER
		} SpeakUNICODE;

		struct // size = 13	// Death anim of a creature
		{
			byte m_Cmd;		// 0 = 0xaf
			ndword m_UID;	// 1-4
			ndword m_UIDCorpse; // 5-8
			nword m_DeathFlight;		// 9= in flight ?
			nword m_Death2Anim;	// 11= forward/backward,dir ?
		} CharDeath;

		struct // size = 21 + var // Send a gump menu dialog.
		{
			byte m_Cmd;		// 0 = 0xb0
			nword m_len;	// 1
			ndword m_UID;	// 3
			ndword m_context;	// 7-10= context info passed from server
			ndword m_x;
			ndword m_y;
			nword m_lenCmds; // 19-20 = command section length.
			char m_commands[1];	// Format commands (var len) { %s }
			byte m_zeroterm;
			nword m_textlines;	// How many lines of text to follow.
			struct
			{
				nword m_len;	// len of this line.
				NCHAR m_utext[1];	// UNICODE text. (not null term)
			} m_texts[1];
		} GumpDialog;

		struct
		{
			byte m_Cmd;			// 0 = 0xb2
			nword m_len;		// 1-2 = length of packet
			nword m_subcmd;		// 3-4 = 0x001a - already in this channel
			char m_lang[4];// 5-8 = unicode language code (null term....default = 'enu' (english?) CLanguageID
			NCHAR m_uname[1];		// 9-? = name in unicode (zero terminated) (may have  prefix, moderator, etc)
			NCHAR m_uname2[1];	// ?-? = name in unicode (also used for other things (passworded, etc)
		} ChatReq;

		struct
		{
#define MAX_ITEMS_PREVIEW (MAX_BUFFER - 16) / 10
			// MAX_ITEMS_PREVIEW = 1534
			byte m_Cmd; // 0 = 0xb4
			nword m_len; // 1-2 = 16 + (10 * m_count)
			byte m_fAllowGround; // 3 =
			ndword m_code; // 4-7 = target id
			nword m_xOffset; // 8-9 = x targ offset
			nword m_yOffset; // 10-11 = y targ offset
			nword m_zOffset; // 12-13 = z targ offset
			nword m_count;	// 14-15 = how many items in this preview image
			struct
			{
				nword m_id;	// 0-1 = ITEMID_TYPE
				nword m_dx;	// 2-3 = delta x
				nword m_dy;	// 4-5 = delta y
				nword m_dz;	// 6-7 = delta z
				nword m_unk;	// 6-7 = ???
			} m_item[MAX_ITEMS_PREVIEW];
		} TargetItems;

		struct // size = 64	// response to pressing the chat button
		{
			byte m_Cmd;		// 0 = 0xb5
			byte unk1;		// 1 = 0 (???)
			NCHAR m_uname[MAX_NAME_SIZE+1];// 2-63 - unicode name
		} Chat;

		struct // size = variable	// Put up a tooltip
		{
			byte m_Cmd;				// 0 = 0xb7
			nword m_len;			// 1-2
			ndword m_UID;			// 3-6
			NCHAR m_utext[1];		// zero terminated UNICODE string
		} ToolTip;

		struct	// size = 8 // Show Character Profile.
		{
			byte m_Cmd;		// 0=0xB8
			nword m_len;	// 1 - 2
			ndword m_UID;	// 3-6=uid
			char m_title[1];	// Description 1 (not unicode!, name, zero terminated)
			NCHAR m_utext1[1];	// Static Description 2 (unicode!, zero terminated)
			NCHAR m_utext2[1];	// Player Description 3 (unicode!, zero terminated)
		} CharProfile;

		struct // size = 3	// enable features on client
		{
#define CLI_FEAT_T2A_CHAT	0x0001
#define CLI_FEAT_LBR_SOUND	0x0002
#define CLI_FEAT_T2A_FULL	0x0004
#define CLI_FEAT_LBR_FULL	0x0008
#define CLI_FEAT_T2A		(CLI_FEAT_T2A_CHAT|CLI_FEAT_T2A_FULL)
#define CLI_FEAT_LBR		(CLI_FEAT_LBR_SOUND|CLI_FEAT_LBR_FULL)
#define CLI_FEAT_T2A_LBR	(CLI_FEAT_T2A|CLI_FEAT_LBR)
			byte m_Cmd;		// 0 = 0xb9
			nword m_enable;
		} FeaturesEnable;

		struct // size = 5	// enable features on client (new)
		{
			byte m_Cmd;		// 0 = 0xb9
			ndword m_enable;
		} FeaturesEnableNew;

		struct
		{
			byte m_Cmd;	// 0xba
			byte m_Active;	// 1/0
			nword m_x;
			nword m_y;
		} Arrow;

		struct	// Season, len = 3
		{
			byte m_Cmd; // 0=0xbc
			byte m_season;	// 0-5 = SEASON_TYPE, 5 = undead.
			byte m_cursor;  // 0/1 (0=gold / 1=normal cursor? Haven't checked really)
		} Season;

		struct	// we get walk codes from the > 1.26.0 server to be sent back in the Walk messages.
		{
			// walk codes from the > 1.26.0 server to be sent back in the Walk messages
			byte m_Cmd;	// 0=0xbf
			nword m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			nword m_type;	// 3 - 4 EXTDATA_TYPE = 1=prime LIFO stack, 2=add to beginning of LIFO stack
			CExtData m_u;
		} ExtData;

		struct
		{
			byte m_Cmd;			// 0 = 0xc1
			nword m_len;		// 1-2 = var len size.
			ndword m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			nword m_id;			// 7-8 = CREID_TYPE of speaker.
			byte m_mode;		// 9 = TALKMODE_TYPE
			nword m_wHue;		// 10-11 = HUE_TYPE.
			nword m_font;		// 12-13 = FONT_TYPE
			ndword m_clilocId;	// 14-17 = Cliloc ID to display
			char m_charname[MAX_NAME_SIZE];	// 18-47
			tchar m_args[1];		// 48+ = arguments
		} SpeakLocalized;

		struct // size = 21 // console prompt request. (unicode version)
		{
			byte m_Cmd;			// 0 = 0xc2
			nword m_len;		// 1-2 = length = 16
			ndword m_serial;	// 3-6 = serial
			ndword m_prompt;	// 7-10 = prompt id
			ndword m_type;		// 11-14 = type (0=request/esc, 1=reply)
			char m_lang[4];		// 15-18 = lang (3 chars + NULL)
			wchar m_utext[1];	// 19+ = response
		} PromptUNICODE;
		
		struct
		{
			byte m_Cmd; // 0=0xc4
			ndword m_UID;
			byte m_Intensity;
		} Semivisible;
		
		struct	// size = 2	// Sent for altering visual range
		{
			byte m_Cmd;		// 0 =0xC8
			byte m_Value;	// 0x01 to UO_MAP_VIEW_SIZE
		} VisualRange;

		struct
		{
			byte m_Cmd;			// 0 = 0xCC
			nword m_len;		// 1-2 = var len size.
			ndword m_UID;		// 3-6 = UID num of speaker. 01010101 = system
			nword m_id;			// 7-8 = CREID_TYPE of speaker.
			byte m_mode;		// 9 = TALKMODE_TYPE
			nword m_wHue;		// 10-11 = HUE_TYPE.
			nword m_font;		// 12-13 = FONT_TYPE
			ndword m_clilocId;	// 14-17 = Cliloc ID to display
			byte m_affixType;	// 18 = Affix type (0=append, 1=prepend, 2=system)
			char m_charname[MAX_NAME_SIZE];	// 19-48
			char m_affix[1];	// 49+ = affix (ASCII only)
			tchar m_args[1];	// 50+len(affix)+ = arguments (UNICODE only)
		} SpeakLocalizedEx;

		struct	// size = 2 // Logout Confirm Answer
		{
			byte m_Cmd;		// 0=0xD1
			byte m_unk;	// always 1
		} LogoutStatus;

		struct // size = 17+var // Get a change to the book title.
		{
			byte m_Cmd;			// 0 = 0xD4
			nword m_len;		// 1-2 = length
			ndword m_UID;		// 3-6 = book
			byte m_unk1;		// 7 = 0x01
			byte m_writable;	// 8 = 0 = non writable, 1 = writable
			nword m_pages;		// 9-10 = number of pages
			nword m_titlelen;	// 11-12 = title length
			char m_title[1];	// 13 - = title
			nword m_authorlen;	// = author length
			char m_author[1];	// = author
		} BookOpenNew;
		
		struct
		{
			byte m_Cmd; // 0xD6
			nword m_len;
			nword m_Unk1; // 0x0000
			ndword m_UID;
			nword m_Unk2; // 0x0003
			ndword m_ListID;
			struct
			{
				ndword m_LocID;
				nword m_textlen;
				tchar m_utext[1]; // this is NCHAR !!!
			} m_list[1];
		} AOSTooltip;
		
		struct
		{
			byte m_Cmd;	// 0=0xd7
			nword m_len;	// 1 - 2 (len = 5 + (# of codes * 4)
			ndword m_uid; 
			nword m_type;	// 3 - 4 EXTDATA_TYPE
			CExtAosData m_u;
		} ExtAosData;
		
		struct 
		{
			byte m_Cmd; // 0xd8
			nword m_len;
			byte m_compression; // 0x00 not compressed
			byte m_unk1;	// 0x00
			ndword m_UID;
			ndword m_revision;
			nword m_itemcount;
			nword m_datasize; // itemcount * 5
			byte m_planeCount;
			struct
			{
				byte m_index;	// index | 0x20
				byte m_size;
				byte m_length;
				byte m_flags;
				nword m_data[1];
			} m_planeList[1];
			struct
			{
				byte m_index;	// index + 9
				byte m_size;
				byte m_length;
				byte m_flags;
				struct
				{
					nword m_id;
					byte m_x;
					byte m_y;
					byte m_z;
				} m_data[1];
			} m_stairsList[1];
		} AOSCustomHouse;

		struct
		{
			byte m_Cmd;	// 0 = 0xdc
			ndword m_uid;
			ndword m_ListID;
		} AOSTooltipInfo;

		struct // Send a gump menu dialog.
		{
			byte m_Cmd;		// 0 = 0xDD
			nword m_len;	// 1
			ndword m_UID;	// 3
			ndword m_context;	// 7-10= context info passed from server
			ndword m_x;
			ndword m_y;
			ndword m_compressed_lenCmds;
			ndword m_uncompressed_lenCmds;
			byte m_commands[1];	// Format commands (Lenght: m_compressed_lenCmds)
			ndword m_lineTxts;
			ndword m_compressed_lenTxts;
			ndword m_uncompressed_lenTxs;
			byte m_textlines[1]; // Text lines (Lenght: m_compressed_lenTxts)
		} CompressedGumpDialog;

		struct
		{
			byte m_Cmd;
			nword m_Length;
			ndword m_Uid;
			nword m_IconId_1;
			nword m_Show_1;
			ndword m_unk_1;
			nword m_IconId_2;
			nword m_Show_2;
			ndword m_unk_2;
			nword m_Time;
			nword m_unk_3;
			byte m_unk_4;
			ndword m_ClilocOne;
			ndword m_ClilocTwo;
			ndword m_unk_5;
			nword m_unk_6;
			nword m_unk_7;
			byte m_MBUTab_1[2];
			byte m_MBUText_1[6];
			byte m_MBUTab_2[2];
			byte m_MBUText_2[6];
			byte m_MBUTab_3[2];
			byte m_MBUText_3[6];
			nword m_unk_8;
		} AddBuff;

		struct
		{
			byte m_Cmd;
			nword m_Length;
			ndword m_Uid;
			nword m_IconId;
			nword m_Show;
			ndword m_unk_1;
		} RemoveBuff;

		struct // XCMD_NewAnimUpdate
		{
			byte m_Cmd;			// 0=0xe2
			ndword m_uid;		// 1 - 4 (len)
			nword m_action;		// 5 - 6
			byte m_zero;		// 7	 (0x00)
			nword m_count;		// 8 - 10 (layers to unequip)
		} NewAnimUpdate;

		struct // XCMD_WaypointShow
		{
			byte m_Cmd;				// 0 = 0xE5
			nword m_len;			// 1 - 2 = length
			ndword m_Wpnt_uid;		// 3 - 7 = serial uid
			nword m_Wpnt_px;		// 8 - 9 = X
			nword m_Wpnt_py;		// 10 - 11 = Y
			byte m_Wpnt_pz;			// 12 = Y
			byte m_Wpnt_pmap;		// 13 = MAP
			nword m_Wpnt_type;		// 14 - 15 = object type (??)
			nword m_ignoreUid;		// 16 - 17 = don't use the uid (0/1)
			ndword m_ClilocId;		// 18 - 22 = clilocId
			NCHAR m_ClilocArgs[1];	// 23 - ? = clilocArgs
			nword m_zero;			// ? - ?+1 = zero
		} WaypointShow;

		struct // XCMD_WaypointHide
		{
			byte m_Cmd;				// 0 = 0xE6
			ndword m_Wpnt_uid;		// 1 - 4 = serial uid
		} WaypointHide;

		struct // XCMD_HighlightUIContinue
		{
			byte m_Cmd;				// 0 = 0xE7
			ndword m_Uid;			// 1 - 4 = serial uid
			nword m_IdUi;			// 5 - 6 = id ui
			ndword m_destUid;		// 7 - 11 = destination serial uid
			byte m_One;				// 12 = 0x01
		} HighlightUIContinue;

		struct // XCMD_HighlightUIToggle
		{
			byte m_Cmd;				// 0 = 0xE9
			ndword m_Uid;			// 1 - 4 = serial uid
			nword m_IdUi;			// 5 - 6 = id ui
			char m_Desc[64];		// 7 - 71 = dexcription: �ToggleInventory�, �TogglePaperdoll�, �ToggleMap�, ��
			ndword m_commandId;		// 72 - 76 = command Id
		} HighlightUIToggle;

		struct // XCMD_ToggleHotbar
		{
			byte m_Cmd;				// 0 = 0xEA
			nword m_Enable;			// 1 - 2 = enable
		} ToggleHotbar;

		struct // XCMD_TSyncReply
		{
			byte m_Cmd;				// 0 = 0xF2
			ndword m_unk1;			// 1 - 4
			ndword m_unk2;			// 5 - 8
			ndword m_unk3;			// 9 - 12
			ndword m_unk4;			// 13 - 16
			ndword m_unk5;			// 17 - 20
			ndword m_unk6;			// 21 - 24
		} TSyncReply;

		struct	// XCMD_MapDisplayNew, size = 21
		{
			byte m_Cmd; // 0 = 0xF5
			ndword m_UID; // uid of the map item
			nword m_Gump_Corner; // GUMP_TYPE always 0x139d....compass tile id in the corner.,
			nword m_x_ul; // upper left x coord.
			nword m_y_ul; // upper left y coord.
			nword m_x_lr; // lower right x coord.
			nword m_y_lr; // lower right y coord.
			nword m_xsize; // client width
			nword m_ysize; // client height
			nword m_map; // map id
		} MapDisplayNew;	// MapX

		struct	// XCMD_MoveShip, size = 18+var
		{
			byte m_Cmd; // 0 = 0xF6
			nword m_len;
			ndword m_UID; // uid of the ship
			byte m_unk1; // 0x04
			byte m_unk2; // 0x04
			byte m_unk3; // 0x04
			nword m_shipX; // ship x
			nword m_shipY; // ship y
			nword m_shipZ; // ship z
			nword m_objectCount; // number of objects on deck
			struct
			{
				ndword m_UID; // object uid
				nword m_x; // object x
				nword m_y; // object y
				nword m_z; // object z
			} m_objects[1];
		} MoveShip;
	};
} PACK_NEEDED;

// Turn off structure packing.
#if defined _WIN32 && (!__MINGW32__)
#pragma pack()
#endif

#endif // _INC_SPHEREPROTO_H
