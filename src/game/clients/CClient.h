//
// CClient.h
//

#ifndef _INC_CCLIENT_H
#define _INC_CCLIENT_H
#pragma once

#include "../common/CArray.h"
#include "../common/CEncrypt.h"
#include "../common/CScriptObj.h"
#include "../common/CTextConsole.h"
#include "../network/network.h"
#include "../network/receive.h"
#include "../network/send.h"
#include "../items/CItemBase.h"
#include "../items/CItemContainer.h"
#include "../items/CItemMultiCustom.h"
#include "../CSectorEnviron.h"
#include "../enums.h"
#include "CAccount.h"
#include "CChat.h"
#include "CChatChanMember.h"
#include "CClientTooltip.h"
#include "CGMPage.h"


class CItemMap;

enum CV_TYPE
{
	#define ADD(a,b) CV_##a,
	#include "../tables/CClient_functions.tbl"
	#undef ADD
	CV_QTY
};

enum CC_TYPE
{
	#define ADD(a,b) CC_##a,
	#include "../tables/CClient_props.tbl"
	#undef ADD
	CC_QTY
};




class CDialogResponseArgs : public CScriptTriggerArgs
{
	// The scriptable return from a gump dialog.
	// "ARG" = dialog args script block. ex. ARGTXT(id), ARGCHK(i)
public:
	static const char *m_sClassName;
	struct TResponseString
	{
	public:
		const word m_ID;
		CGString const m_sText;

		TResponseString( word id, lpctstr pszText ) : m_ID( id ), m_sText( pszText )
		{
		}

	private:
		TResponseString( const TResponseString& copy );
		TResponseString& operator=( const TResponseString& other );
	};

	CGTypedArray<dword, dword>		m_CheckArray;
	CGObArray<TResponseString *>	m_TextArray;
public:
	void AddText( word id, lpctstr pszText );
	lpctstr GetName() const;
	bool r_WriteVal( lpctstr pszKey, CGString &sVal, CTextConsole * pSrc );

public:
	CDialogResponseArgs()
	{
	};

private:
	CDialogResponseArgs( const CDialogResponseArgs& copy );
	CDialogResponseArgs& operator=( const CDialogResponseArgs& other );
};

//////////////////

class CMenuItem 	// describe a menu item.
{
public:
	word m_id;			// ITEMID_TYPE in base set.
	word m_color;
	CGString m_sText;
public:
	bool ParseLine( tchar * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc );
};


class CClient : public CGObListRec, public CScriptObj, public CChatChanMember, public CTextConsole
{
	// TCP/IP connection to the player or console.
private:
	static lpctstr const sm_szCmd_Redirect[];		// default to redirect these.

public:
	static const char *m_sClassName;
	static lpctstr const sm_szRefKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];
private:
	CChar * m_pChar;			// What char are we playing ?
	NetState* m_net; // network state

					 // Client last know state stuff.
	CSectorEnviron m_Env;		// Last Environment Info Sent. so i don't have to keep resending if it's the same.

	uchar m_fUpdateStats;	// update our own status (weight change) when done with the cycle.

									// Walk limiting code
	int	m_iWalkTimeAvg;
	int m_iWalkStepCount;		// Count the actual steps . Turning does not count.
	llong m_timeWalkStep;	// the last %8 walk step time.

								// Screensize
	struct __screensize
	{
		dword x;
		dword y;
	} m_ScreenSize;

	// OxBF - 0x24 AntiCheat
	struct __bfanticheat
	{
		byte lastvalue;
		byte count;
	} m_BfAntiCheat;

	// Promptconsole
	CLIMODE_TYPE m_Prompt_Mode;	// type of prompt
	CGrayUID m_Prompt_Uid;		// context uid
	CGString m_Prompt_Text;		// text (i.e. callback function)

public:
	CONNECT_TYPE	m_iConnectType;	// what sort of a connection is this ?
	CAccount * m_pAccount;		// The account name. we logged in on

	CServTime m_timeLogin;			// World clock of login time. "LASTCONNECTTIME"
	CServTime m_timeLastEvent;		// Last time we got event from client.
	CServTime m_timeLastEventWalk;	// Last time we got a walk event from client (only used to handle STATF_Fly char flag)
	int64 m_timeNextEventWalk;		// Fastwalk prevention: only allow more walk requests after this timer

									// GM only stuff.
	CGMPage * m_pGMPage;		// Current GM page we are connected to.
	CGrayUID m_Prop_UID;		// The object of /props (used for skills list as well!)

								// Gump stuff
	typedef std::map<int,int> OpenedGumpsMap_t;
	OpenedGumpsMap_t m_mapOpenedGumps;

	// Current operation context args for modal async operations..
private:
	CLIMODE_TYPE m_Targ_Mode;	// Type of async operation under way.
public:
	CGrayUID m_Targ_Last;	// The last object targeted by the client
	CGrayUID m_Targ_UID;			// The object of interest to apply to the target.
	CGrayUID m_Targ_PrvUID;		// The object of interest before this.
	CGString m_Targ_Text;		// Text transfered up from client.
	CPointMap  m_Targ_p;			// For script targeting,
	CServTime m_Targ_Timeout;	// timeout time for targeting

								// Context of the targetting setup. depends on CLIMODE_TYPE m_Targ_Mode
	union
	{
		// CLIMODE_SETUP_CONNECTING
		struct
		{
			dword m_dwIP;
			int m_iConnect;	// used for debug only.
			bool m_bNewSeed;
		} m_tmSetup;

		// CLIMODE_SETUP_CHARLIST
		CGrayUIDBase m_tmSetupCharList[MAX_CHARS_PER_ACCT];

		// CLIMODE_INPVAL
		struct
		{
			CGrayUIDBase m_UID;
			RESOURCE_ID_BASE m_PrvGumpID;	// the gump that was up before this
		} m_tmInpVal;

		// CLIMODE_MENU_*
		// CLIMODE_MENU_SKILL
		// CLIMODE_MENU_GM_PAGES
		struct
		{
			CGrayUIDBase m_UID;
			RESOURCE_ID_BASE m_ResourceID;		// What menu is this ?
			dword m_Item[MAX_MENU_ITEMS];	// This saves the inrange tracking targets or other context
		} m_tmMenu;	// the menu that is up.

					// CLIMODE_TARG_PET_CMD
		struct
		{
			int	m_iCmd;
			bool m_fAllPets;
		} m_tmPetCmd;	// which pet command am i targetting ?

						// CLIMODE_TARG_CHAR_BANK
		struct
		{
			LAYER_TYPE m_Layer;	// gm command targetting what layer ?
		} m_tmCharBank;

		// CLIMODE_TARG_TILE
		// CLIMODE_TARG_UNEXTRACT
		struct
		{
			CPointBase m_ptFirst; // Multi stage targetting.
			int m_Code;
			int m_id;
		} m_tmTile;

		// CLIMODE_TARG_ADDCHAR
		// CLIMODE_TARG_ADDITEM
		struct
		{
			dword m_junk0;
			int m_id;
		} m_tmAdd;

		// CLIMODE_TARG_SKILL
		struct
		{
			SKILL_TYPE m_Skill;			// targetting what spell ?
		} m_tmSkillTarg;

		// CLIMODE_TARG_SKILL_MAGERY
		struct
		{
			SPELL_TYPE m_Spell;			// targetting what spell ?
			CREID_TYPE m_SummonID;
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CGObList * m_pParent;	// the parent of the item being targetted .
		} m_tmUseItem;
	};

private:
	// encrypt/decrypt stuff.
	CCrypt m_Crypt;			// Client source communications are always encrypted.
	static CHuffman m_Comp;

private:
	bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );

	bool OnRxConsoleLoginComplete();
	bool OnRxConsole( const byte * pData, size_t len );
	bool OnRxAxis( const byte * pData, size_t len );
	bool OnRxPing( const byte * pData, size_t len );
	bool OnRxWebPageRequest( byte * pRequest, size_t len );

	byte LogIn( CAccountRef pAccount, CGString & sMsg );
	byte LogIn( lpctstr pszName, lpctstr pPassword, CGString & sMsg );

	bool CanInstantLogOut() const;
	void Cmd_GM_PageClear();

	void Announce( bool fArrive ) const;

	// GM stuff.
	bool OnTarg_Obj_Set( CObjBase * pObj );
	bool OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id );

	bool OnTarg_UnExtract( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Stone_Recruit( CChar * pChar, bool bFull = false );
	bool OnTarg_Char_Add( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Item_Add( CObjBase * pObj, CPointMap & pt );
	bool OnTarg_Item_Link( CObjBase * pObj );
	bool OnTarg_Tile( CObjBase * pObj, const CPointMap & pt );

	// Normal user stuff.
	bool OnTarg_Use_Deed( CItem * pDeed, CPointMap &pt );
	bool OnTarg_Use_Item( CObjBase * pObj, CPointMap & pt, ITEMID_TYPE id );
	bool OnTarg_Party_Add( CChar * pChar );
	CItem* OnTarg_Use_Multi( const CItemBase * pItemDef, CPointMap & pt, dword dwAttr, HUE_TYPE wHue );

	int OnSkill_AnimalLore( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_Anatomy( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_Forensics( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_EvalInt( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_ArmsLore( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_ItemID( CGrayUID uid, int iTestLevel, bool fTest );
	int OnSkill_TasteID( CGrayUID uid, int iTestLevel, bool fTest );

	bool OnTarg_Skill_Magery( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Herd_Dest( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Poison( CObjBase * pObj );
	bool OnTarg_Skill_Provoke( CObjBase * pObj );
	bool OnTarg_Skill( CObjBase * pObj );

	bool OnTarg_Pet_Command( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Pet_Stable( CChar * pCharPet );

	// Commands from client
	void Event_Skill_Use( SKILL_TYPE x ); // Skill is clicked on the skill list
	void Event_Talk_Common(tchar * szText ); // PC speech
	bool Event_Command( lpctstr pszCommand, TALKMODE_TYPE mode = TALKMODE_SYSTEM ); // Client entered a '/' command like /ADD

public:
	void GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE &wHue ) const;
	void GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE &wHue ) const;

	void Event_Attack(CGrayUID uid);
	void Event_Book_Title( CGrayUID uid, lpctstr pszTitle, lpctstr pszAuthor );
	void Event_BugReport( const tchar * pszText, int len, BUGREPORT_TYPE type, CLanguageID lang = 0 );
	void Event_ChatButton(const NCHAR * pszName); // Client's chat button was pressed
	void Event_ChatText( const NCHAR * pszText, int len, CLanguageID lang = 0 ); // Text from a client
	void Event_CombatMode( bool fWar ); // Only for switching to combat mode
	bool Event_DoubleClick( CGrayUID uid, bool fMacro, bool fTestTouch, bool fScript = false );
	void Event_ExtCmd( EXTCMD_TYPE type, tchar * pszName );
	void Event_Item_Drop( CGrayUID uidItem, CPointMap pt, CGrayUID uidOn, uchar gridIndex = 0 ); // Item is dropped on ground
	void Event_Item_Drop_Fail( CItem *pItem );
	void Event_Item_Dye( CGrayUID uid, HUE_TYPE wHue );	// Rehue an item
	void Event_Item_Pickup( CGrayUID uid, int amount ); // Client grabs an item
	void Event_MailMsg( CGrayUID uid1, CGrayUID uid2 );
	void Event_Profile( byte fWriteMode, CGrayUID uid, lpctstr pszProfile, int iProfileLen );
	void Event_PromptResp( lpctstr pszText, size_t len, dword context1, dword context2, dword type, bool bNoStrip = false );
	void Event_SetName( CGrayUID uid, const char * pszCharName );
	void Event_SingleClick( CGrayUID uid );
	void Event_Talk( lpctstr pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool bNoStrip = false ); // PC speech
	void Event_TalkUNICODE( nword* wszText, int iTextLen, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, lpctstr pszLang );
	void Event_Target( dword context, CGrayUID uid, CPointMap pt, byte flags = 0, ITEMID_TYPE id = ITEMID_NOTHING );
	void Event_Tips( word i ); // Tip of the day window
	void Event_ToolTip( CGrayUID uid );
	void Event_UseToolbar(byte bType, dword dwArg);
	void Event_VendorBuy(CChar* pVendor, const VendorItem* items, size_t itemCount);
	void Event_VendorBuy_Cheater( int iCode = 0 );
	void Event_VendorSell(CChar* pVendor, const VendorItem* items, size_t itemCount);
	void Event_VendorSell_Cheater( int iCode = 0 );
	bool Event_Walk( byte rawdir, byte sequence = 0 ); // Player moves
	bool Event_CheckWalkBuffer();

	TRIGRET_TYPE Menu_OnSelect( RESOURCE_ID_BASE rid, int iSelect, CObjBase * pObj );
	TRIGRET_TYPE Dialog_OnButton( RESOURCE_ID_BASE rid, dword dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs );

	bool Login_Relay( uint iServer ); // Relay player to a certain IP
	byte Login_ServerList( const char * pszAccount, const char * pszPassword ); // Initial login (Login on "loginserver", new format)

	byte Setup_Delete( dword iSlot ); // Deletion of character
	int Setup_FillCharList(Packet* pPacket, const CChar * pCharFirst); // Write character list to packet
	byte Setup_ListReq( const char * pszAccount, const char * pszPassword, bool fTest ); // Gameserver login and character listing
	byte Setup_Play( uint iSlot ); // After hitting "Play Character" button
	byte Setup_Start( CChar * pChar ); // Send character startup stuff to player


									   // translated commands.
private:
	void Cmd_GM_PageInfo();
	int Cmd_Extract( CScript * pScript, CRectMap &rect, int & zlowest );
	size_t Cmd_Skill_Menu_Build( RESOURCE_ID_BASE rid, int iSelect, CMenuItem* item, size_t iMaxSize, bool &fShowMenu, bool &fLimitReached );
public:
	bool Cmd_CreateItem( ITEMID_TYPE id );
	bool Cmd_CreateChar( CREID_TYPE id );

	void Cmd_GM_PageMenu( uint iEntryStart = 0 );
	void Cmd_GM_PageCmd( lpctstr pCmd );
	void Cmd_GM_PageSelect( size_t iSelect );
	void Cmd_GM_Page( lpctstr pszreason); // Help button (Calls GM Call Menus up)

	bool Cmd_Skill_Menu( RESOURCE_ID_BASE rid, int iSelect = -1 );
	bool Cmd_Skill_Smith( CItem * pIngots );
	bool Cmd_Skill_Magery( SPELL_TYPE iSpell, CObjBase * pSrc );
	bool Cmd_Skill_Tracking( uint track_type = UINT32_MAX, bool fExec = false ); // Fill menu with specified creature types
	bool Cmd_Skill_Inscription();
	bool Cmd_SecureTrade( CChar * pChar, CItem * pItem );
	bool Cmd_Control( CChar * pChar );

public:
	CSocketAddress &GetPeer();								// get peer address
	lpctstr GetPeerStr() const;								// get string representation of the peer address
	int GetSocketID() const;								// get socket id

public:
	explicit CClient(NetState* state);
	~CClient();

private:
	CClient(const CClient& copy);
	CClient& operator=(const CClient& other);

public:
	void CharDisconnect();

	CClient* GetNext() const
	{
		return( STATIC_CAST <CClient*>( CGObListRec::GetNext()));
	}

	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute script type command on me
	virtual bool r_WriteVal( lpctstr pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s );

	// Low level message traffic.
	static size_t xCompress( byte * pOutput, const byte * pInput, size_t inplen );

	bool xProcessClientSetup( CEvent * pEvent, size_t iLen );
	bool xPacketFilter(const byte * pEvent, size_t iLen = 0);
	bool xOutPacketFilter(const byte * pEvent, size_t iLen = 0);
	bool xCanEncLogin(bool bCheckCliver = false);	// Login crypt check
													// Low level push world data to the client.

	bool addRelay( const CServerDef * pServ );
	bool addLoginErr(byte code);
#define SF_UPDATE_HITS		0x01
#define SF_UPDATE_MANA		0x02
#define SF_UPDATE_STAM		0x04
#define SF_UPDATE_STATUS	0x08

	void addUpdateStatsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STATUS;
	}
	void addUpdateHitsFlag()
	{
		m_fUpdateStats |= SF_UPDATE_HITS;
	}
	void addUpdateManaFlag()
	{
		m_fUpdateStats |= SF_UPDATE_MANA;
	}
	void addUpdateStamFlag()
	{
		m_fUpdateStats |= SF_UPDATE_STAM;
	}
	void UpdateStats();
	bool addDeleteErr(byte code, dword iSlot);
	void addSeason(SEASON_TYPE season);
	void addTime( bool bCurrent = false );
	void addObjectRemoveCantSee( CGrayUID uid, lpctstr pszName = NULL );
	void closeContainer( const CObjBase * pObj );
	void closeUIWindow( const CChar* character, dword command );
	void addObjectRemove( CGrayUID uid );
	void addObjectRemove( const CObjBase * pObj );
	void addRemoveAll( bool fItems, bool fChars );

	void addItem_OnGround( CItem * pItem ); // Send items (on ground)
	void addItem_Equipped( const CItem * pItem );
	void addItem_InContainer( const CItem * pItem );
	void addItem( CItem * pItem );

	void addBuff( const BUFF_ICONS IconId, const dword ClilocOne, const dword ClilocTwo, const word Time = 0, lpctstr* pArgs = 0, size_t iArgCount = 0);
	void removeBuff(const BUFF_ICONS IconId);
	void resendBuffs();

	void addOpenGump( const CObjBase * pCont, GUMP_TYPE gump );
	void addContents( const CItemContainer * pCont, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false ); // Send items
	bool addContainerSetup( const CItemContainer * pCont ); // Send Backpack (with items)

	void addPlayerStart( CChar * pChar );
	void addAOSPlayerSeeNoCrypt(); //sends tooltips for items in LOS
	void addPlayerSee( const CPointMap & pt ); // Send objects the player can now see
	void addPlayerSeeShip( const CPointMap & pt ); // Send objects the player can now see from a ship
	void addPlayerView( const CPointMap & pt, bool bFull = true );
	void addPlayerWarMode();

	void addCharMove( const CChar * pChar );
	void addCharMove( const CChar * pChar, byte bCharDir );
	void addChar( const CChar * pChar );
	void addCharName( const CChar * pChar ); // Singleclick text for a character
	void addItemName( const CItem * pItem );

	bool addKick( CTextConsole * pSrc, bool fBlock = true );
	void addWeather( WEATHER_TYPE weather = WEATHER_DEFAULT ); // Send new weather to player
	void addLight();
	void addMusic( MIDI_TYPE id );
	void addArrowQuest( int x, int y, int id );
	void addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc, byte speed = 5, byte loop = 1, bool explode = false, dword color = 0, dword render = 0, word effectid = 0, dword explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0 );
	void addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase = NULL, int iRepeat = 1 );
	void addReSync();
	void addMap();
	void addMapDiff();
	void addChangeServer();
	void addPlayerUpdate();

	void addBark( lpctstr pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void addBarkUNICODE( const NCHAR * pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0 );
	void addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, lpctstr pArgs = NULL );
	void addBarkLocalizedEx( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, AFFIX_TYPE affix = AFFIX_APPEND, lpctstr pAffix = NULL, lpctstr pArgs = NULL );
	void addBarkParse( lpctstr pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false, lpctstr name = "" );
	void addSysMessage( lpctstr pMsg ); // System message (In lower left corner)
	void addObjMessage( lpctstr pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_OBJ ); // The message when an item is clicked

	void addDyeOption( const CObjBase * pBase );
	void addWebLaunch( lpctstr pMsg ); // Direct client to a web page

	void addPromptConsole( CLIMODE_TYPE mode, lpctstr pMsg, CGrayUID context1 = 0, CGrayUID context2 = 0, bool bUnicode = false );
	void addTarget( CLIMODE_TYPE targmode, lpctstr pMsg, bool fAllowGround = false, bool fCheckCrime = false, int iTimeout = 0 ); // Send targetting cursor to client
	void addTargetDeed( const CItem * pDeed );
	bool addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, bool fGround = true );
	bool addTargetChars( CLIMODE_TYPE mode, CREID_TYPE id, bool fNoto, int iTimeout = 0 );
	void addTargetVerb( lpctstr pCmd, lpctstr pArg );
	void addTargetFunctionMulti( lpctstr pszFunction, ITEMID_TYPE itemid, bool fGround );
	void addTargetFunction( lpctstr pszFunction, bool fAllowGround, bool fCheckCrime );
	void addTargetCancel();
	void addPromptConsoleFunction( lpctstr pszFunction, lpctstr pszSysmessage, bool bUnicode = false );

	void addScrollScript( CResourceLock &s, SCROLL_TYPE type, dword dwcontext = 0, lpctstr pszHeader = NULL );
	void addScrollResource( lpctstr szResourceName, SCROLL_TYPE type, dword dwcontext = 0 );

	void addVendorClose( const CChar * pVendor );
	bool addShopMenuBuy( CChar * pVendor );
	bool addShopMenuSell( CChar * pVendor );
	void addBankOpen( CChar * pChar, LAYER_TYPE layer = LAYER_BANKBOX );

	void addSpellbookOpen( CItem * pBook, word offset = 1 );
	void addCustomSpellbookOpen( CItem * pBook, dword gumpID );
	bool addBookOpen( CItem * pBook );
	void addBookPage( const CItem * pBook, size_t iPage, size_t iCount );
	void addCharStatWindow( CChar * pChar, bool fRequested = false ); // Opens the status window
	void addHitsUpdate( CChar * pChar );
	void addManaUpdate( CChar * pChar );
	void addStamUpdate( CChar * pChar );
	void addHealthBarUpdate( const CChar * pChar );
	void addBondedStatus( const CChar * pChar, bool bIsDead );
	void addSkillWindow(SKILL_TYPE skill, bool bFromInfo = false); // Opens the skills list
	void addBulletinBoard( const CItemContainer * pBoard );
	bool addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, CGrayUID uidMsg );

	void addToolTip( const CObjBase * pObj, lpctstr psztext );
	void addDrawMap( CItemMap * pItem );
	void addMapMode( CItemMap * pItem, MAPCMD_TYPE iType, bool fEdit = false );

	void addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, lpctstr pszName, lpctstr pszText );
	void addGumpInpVal( bool fcancel, INPVAL_STYLE style, dword dwmask, lpctstr ptext1, lpctstr ptext2, CObjBase * pObj );

	void addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, size_t count, CObjBase * pObj = NULL );
	void addGumpDialog( CLIMODE_TYPE mode, const CGString * sControls, size_t iControls, const CGString * psText, size_t iTexts, int x, int y, CObjBase * pObj = NULL, dword rid = 0 );

	bool addGumpDialogProps( CGrayUID uid );

	void addLoginComplete();
	void addChatSystemMessage(CHATMSG_TYPE iType, lpctstr pszName1 = NULL, lpctstr pszName2 = NULL, CLanguageID lang = 0 );

	void addCharPaperdoll( CChar * pChar );

	void addAOSTooltip( const CObjBase * pObj, bool bRequested = false, bool bShop = false );

private:
#define MAX_POPUPS 15
#define POPUPFLAG_LOCKED 0x01
#define POPUPFLAG_ARROW 0x02
#define POPUPFLAG_COLOR 0x20
#define POPUP_REQUEST 0
#define POPUP_PAPERDOLL 11
#define POPUP_BACKPACK 12
#define POPUP_PARTY_ADD 13
#define POPUP_PARTY_REMOVE 14
#define POPUP_TRADE_ALLOW 15
#define POPUP_TRADE_REFUSE 16
#define POPUP_TRADE_OPEN 17
#define POPUP_BANKBOX 21
#define POPUP_VENDORBUY 31
#define POPUP_VENDORSELL 32
#define POPUP_PETGUARD 41
#define POPUP_PETFOLLOW 42
#define POPUP_PETDROP 43
#define POPUP_PETKILL 44
#define POPUP_PETSTOP 45
#define POPUP_PETSTAY 46
#define POPUP_PETFRIEND_ADD 47
#define POPUP_PETFRIEND_REMOVE 48
#define POPUP_PETTRANSFER 49
#define POPUP_PETRELEASE 50
#define POPUP_STABLESTABLE 51
#define POPUP_STABLERETRIEVE 52
#define POPUP_TRAINSKILL 100

	PacketDisplayPopup* m_pPopupPacket;

public:
	void Event_AOSPopupMenuSelect( dword uid, word EntryTag );
	void Event_AOSPopupMenuRequest( dword uid );


	void addShowDamage( int damage, dword uid_damage );
	void addSpeedMode( byte speedMode = 0 );
	void addVisualRange( byte visualRange = UO_MAP_VIEW_SIZE );
	void addIdleWarning( byte message );
	void addKRToolbar( bool bEnable );

	void SendPacket( tchar * pszPacket );
	void LogOpenedContainer(const CItemContainer* pContainer);

	// Test what I can do
	CAccountRef GetAccount() const
	{
		return( m_pAccount );
	}
	bool IsPriv( word flag ) const
	{	// PRIV_GM
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->IsPriv( flag ));
	}
	void SetPrivFlags( word wPrivFlags )
	{
		if ( GetAccount() == NULL )
			return;
		GetAccount()->SetPrivFlags( wPrivFlags );
	}
	void ClearPrivFlags( word wPrivFlags )
	{
		if ( GetAccount() == NULL )
			return;
		GetAccount()->ClearPrivFlags( wPrivFlags );
	}

	// ------------------------------------------------
	bool IsResDisp( byte flag ) const
	{
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->IsResDisp( flag ) );
	}
	byte GetResDisp() const
	{
		if ( GetAccount() == NULL )
			return( UINT8_MAX );
		return( GetAccount()->GetResDisp() );
	}
	bool SetResDisp( byte res )
	{
		if ( GetAccount() == NULL )
			return( false );
		return ( GetAccount()->SetResDisp( res ) );
	}
	bool SetGreaterResDisp( byte res )
	{
		if ( GetAccount() == NULL )
			return( false );
		return( GetAccount()->SetGreaterResDisp( res ) );
	}
	// ------------------------------------------------
	void SetScreenSize(dword x, dword y)
	{
		m_ScreenSize.x = x;
		m_ScreenSize.y = y;
	}

	PLEVEL_TYPE GetPrivLevel() const
	{
		// PLEVEL_Counsel
		if ( GetAccount() == NULL )
			return( PLEVEL_Guest );
		return( GetAccount()->GetPrivLevel());
	}
	lpctstr GetName() const
	{
		if ( GetAccount() == NULL )
			return( "NA" );
		return( GetAccount()->GetName());
	}
	CChar * GetChar() const
	{
		return( m_pChar );
	}

	void SysMessage( lpctstr pMsg ) const; // System message (In lower left corner)
	bool CanSee( const CObjBaseTemplate * pObj ) const;
	bool CanHear( const CObjBaseTemplate * pSrc, TALKMODE_TYPE mode ) const;

	bool Dialog_Setup( CLIMODE_TYPE mode, RESOURCE_ID_BASE rid, int iPage, CObjBase * pObj, lpctstr Arguments = "" );
	bool Dialog_Close( CObjBase * pObj, dword rid, int buttonID );
	void Menu_Setup( RESOURCE_ID_BASE rid, CObjBase * pObj = NULL );

	int OnSkill_Info( SKILL_TYPE skill, CGrayUID uid, int iTestLevel, bool fTest );

	bool Cmd_Use_Item( CItem * pItem, bool fTestTouch, bool fScript = false );
	void Cmd_EditItem( CObjBase * pObj, int iSelect );

	bool IsConnectTypePacket() const
	{
		// This is a game or login server.
		// m_Crypt.IsInit()
		return( m_iConnectType == CONNECT_CRYPT || m_iConnectType == CONNECT_LOGIN || m_iConnectType == CONNECT_GAME );
	}

	CONNECT_TYPE	GetConnectType() const
	{
		return m_iConnectType;
	}

	void SetConnectType( CONNECT_TYPE iType );

	// Stuff I am doing. Started by a command
	CLIMODE_TYPE GetTargMode() const
	{
		return( m_Targ_Mode );
	}
	void SetTargMode( CLIMODE_TYPE targmode = CLIMODE_NORMAL, lpctstr pszPrompt = NULL, int iTimeout = 0 );
	void ClearTargMode()
	{
		// done with the last mode.
		m_Targ_Mode = CLIMODE_NORMAL;
		m_Targ_Timeout.Init();
	}

	bool IsConnecting() const;

	NetState* GetNetState(void) const { return m_net; };

private:
	CGString	m_BarkBuffer;

public:
	char		m_zLastMessage[SCRIPT_MAX_LINE_LEN];	// last sysmessage
	char		m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];	// last message
	char		m_zLogin[64];
	CServTime	m_tNextPickup;
	CVarDefMap	m_TagDefs;
	CVarDefMap	m_BaseDefs;		// New Variable storage system
	typedef std::map<dword, std::pair<std::pair<dword,dword>, CPointMap> > OpenedContainerMap_t;
	OpenedContainerMap_t m_openedContainers;	// list of UIDs of all opened containers by the client

	CGObArray<CClientTooltip *> m_TooltipData; // Storage for tooltip data while in trigger

	CItemMultiCustom * m_pHouseDesign; // The building this client is designing

public:
	lpctstr GetDefStr( lpctstr pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( pszKey, fZero );
	}

	int64 GetDefNum( lpctstr pszKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyNum( pszKey, fZero );
	}

	void SetDefNum(lpctstr pszKey, int64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(pszKey, iVal, fZero);
	}

	void SetDefStr(lpctstr pszKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(pszKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(lpctstr pszKey)
	{
		m_BaseDefs.DeleteKey(pszKey);
	}

#ifndef _MTNETWORK
	friend class NetworkIn;
	friend class NetworkOut;
#else
	friend class NetworkInput;
	friend class NetworkOutput;
#endif
	friend class PacketCreate;
	friend class PacketServerRelay;
};

#endif	// _INC_CCLIENT_H
