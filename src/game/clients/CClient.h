/**
* @file CClient.h
*
*/

#ifndef _INC_CCLIENT_H
#define _INC_CCLIENT_H

#include "../../common/crypto/CCrypto.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../../common/CTextConsole.h"
#include "../../network/receive.h"
#include "../../network/send.h"
#include "../items/CItemBase.h"
#include "../items/CItemContainer.h"
#include "../CSectorEnviron.h"
#include "../game_enums.h"
#include "CAccount.h"
#include "CChat.h"
#include "CChatChanMember.h"
#include "CClientTooltip.h"
#include "CGMPage.h"


class CItemMap;
class CItemMultiCustom;

enum CV_TYPE
{
	#define ADD(a,b) CV_##a,
	#include "../../tables/CClient_functions.tbl"
	#undef ADD
	CV_QTY
};

enum CC_TYPE
{
	#define ADD(a,b) CC_##a,
	#include "../../tables/CClient_props.tbl"
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
		CSString const m_sText;

		TResponseString( word id, lpctstr pszText ) : m_ID( id ), m_sText( pszText )
		{
		}

	private:
		TResponseString( const TResponseString& copy );
		TResponseString& operator=( const TResponseString& other );
	};

	CSTypedArray<dword>				m_CheckArray;
	CSObjArray<TResponseString *>	m_TextArray;

public:
	void AddText( word id, lpctstr pszText );
	lpctstr GetName() const;
	bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false );

	CDialogResponseArgs() = default;

private:
	CDialogResponseArgs( const CDialogResponseArgs& copy );
	CDialogResponseArgs& operator=( const CDialogResponseArgs& other );
};

//////////////////

struct CMenuItem 	    // describe a menu item.
{
	word m_id;			// ITEMID_TYPE in base set.
	word m_color;
	CSString m_sText;

	bool ParseLine( tchar * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc );
};


class CClient : public CSObjListRec, public CScriptObj, public CChatChanMember, public CTextConsole
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
    CChar* m_pChar;		// What char are we playing ?
	CNetState* m_net;		// network state

	// Walk limiting code
	int m_iWalkStepCount;	// Count the actual steps. Turning does not count.
    llong m_iWalkTimeAvg;	// Average time betwwen 2 running step
	int64 m_timeWalkStep;	// the last walk step time.
	byte m_lastDir;

    // Client last know state stuff.
    CSectorEnviron m_Env;	// Last Environment Info Sent. so i don't have to keep resending if it's the same.
    uchar m_fUpdateStats;	// update our own status (weight change) when done with the cycle.

	// Screensize
	struct __screensize
	{
		ushort x;
        ushort y;
	} m_ScreenSize;

	// OxBF - 0x24 AntiCheat
	struct __bfanticheat
	{
		byte lastvalue;
		byte count;
	} m_BfAntiCheat;

	// Promptconsole
	CLIMODE_TYPE m_Prompt_Mode;		// type of prompt
	CUID m_Prompt_Uid;				// context uid
	CSString m_Prompt_Text;			// text (i.e. callback function)

public:
	CONNECT_TYPE m_iConnectType;	// what sort of a connection is this ?
	CAccount * m_pAccount;			// The account name. we logged in on

	int64 m_timeLogin;			    // World clock of login time. "LASTCONNECTTIME"
	int64 m_timeLastEvent;		    // Last time we got event from client.
	int64 m_timeLastEventWalk;	    // Last time we got a walk event from client
	int64 m_timeNextEventWalk;		// Fastwalk prevention: only allow more walk requests after this timer

    // Client-sent flags
    bool _fShowPublicHouseContent;

	// GM only stuff.
	CGMPage * m_pGMPage;	// Current GM page being handled by this client
	CUID m_Prop_UID;		// The object of /props (used for skills list as well!)

	// Gump stuff
    //  first uint: dialog's CResourceID.GetPrivateUID(), or special dialog code
    //  second int: count (how much of that dialogs are currently open)
	typedef std::map<uint,int> OpenedGumpsMap_t;
	OpenedGumpsMap_t m_mapOpenedGumps;

	// Throwing weapons stuff (this is used to play weapon returning anim after throw it)
	int64 m_timeLastSkillThrowing;          // Last time we throw the weapon
	CObjBase *m_pSkillThrowingTarg;			// Object from where the anim will return from
	ITEMID_TYPE m_SkillThrowingAnimID;		// Weapon anim ID (AMMOANIM)
	dword m_SkillThrowingAnimHue;			// Weapon anim hue (AMMOANIMHUE)
	dword m_SkillThrowingAnimRender;		// Weapon anim render (AMMOANIMRENDER)

	// Current operation context args for modal async operations..
private:
	CLIMODE_TYPE m_Targ_Mode;	// Type of async operation under way.
public:
	CUID m_Targ_Last;	// The last object targeted by the client
	CUID m_Targ_UID;			// The object of interest to apply to the target.
	CUID m_Targ_Prv_UID;		// The object of interest before this.
	CSString m_Targ_Text;		// Text transfered up from client.
	CPointMap  m_Targ_p;		// For script targeting,
	int64 m_Targ_Timeout;       // timeout time for targeting

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
		CUID m_tmSetupCharList[MAX_CHARS_PER_ACCT];

		// CLIMODE_INPVAL
		struct
		{
			CUID m_UID;
			CResourceID m_PrvGumpID;	// the gump that was up before this
		} m_tmInpVal;

		// CLIMODE_MENU_*
		// CLIMODE_MENU_SKILL
		struct
		{
			CUID m_UID;
			CResourceID m_ResourceID;		// What menu is this ?
			dword m_Item[MAX_MENU_ITEMS];	// It's a buffer to save the in-range tracking targets or other data
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
			CPointMap m_ptFirst; // Multi stage targetting.
			int m_Code;
			int m_id;
		} m_tmTile;

		// CLIMODE_TARG_ADDCHAR
		// CLIMODE_TARG_ADDITEM
		struct
		{
			int m_id;
			word m_vcAmount;
		} m_tmAdd;

		// CLIMODE_TARG_SKILL
		struct
		{
			SKILL_TYPE m_iSkill;			// targetting what spell ?
		} m_tmSkillTarg;

		// CLIMODE_TARG_SKILL_MAGERY
		struct
		{
			SPELL_TYPE m_iSpell;			// targetting what spell ?
			CREID_TYPE m_iSummonID;
		} m_tmSkillMagery;

		// CLIMODE_TARG_USE_ITEM
		struct
		{
			CSObjCont * m_pParent;	// the parent of the item being targetted .
		} m_tmUseItem;
	};

private:
	// encrypt/decrypt stuff.
	CCrypto m_Crypt;			// Client source communications are always encrypted.
	static CHuffman m_Comp;

private:
	bool OnRxConsoleLoginComplete();
	bool OnRxConsole( const byte * pData, uint len );
	bool OnRxAxis( const byte * pData, uint len );
	bool OnRxPing( const byte * pData, uint len );
	bool OnRxWebPageRequest( byte * pRequest, size_t len );

	byte LogIn( CAccount * pAccount, CSString & sMsg );
	byte LogIn( lpctstr pszName, lpctstr pPassword, CSString & sMsg );

	bool CanInstantLogOut() const;

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
	CItem* OnTarg_Use_Multi( const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed );

	int OnSkill_AnimalLore( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_Anatomy( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_Forensics( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_EvalInt( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_ArmsLore( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_ItemID( CUID uid, int iTestLevel, bool fTest );
	int OnSkill_TasteID( CUID uid, int iTestLevel, bool fTest );

	bool OnTarg_Skill_Magery( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Herd_Dest( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Skill_Poison( CObjBase * pObj );
	bool OnTarg_Skill_Provoke( CObjBase * pObj );
	bool OnTarg_Skill( CObjBase * pObj );

	bool OnTarg_Pet_Command( CObjBase * pObj, const CPointMap & pt );
	bool OnTarg_Pet_Stable( CChar * pCharPet );

	// Commands from client
	void Event_Skill_Use( SKILL_TYPE x ); // Skill is clicked on the skill list
	void Event_Talk_Common(lpctstr pszText ); // PC speech
	bool Event_Command( lpctstr pszCommand, TALKMODE_TYPE mode = TALKMODE_SAY ); // Client entered a '/' command like /ADD

public:
	void GetAdjustedCharID( const CChar * pChar, CREID_TYPE & id, HUE_TYPE &wHue ) const;
	void GetAdjustedItemID( const CChar * pChar, const CItem * pItem, ITEMID_TYPE & id, HUE_TYPE &wHue ) const;

	void Event_Attack(CUID uid);
	void Event_Book_Title( CUID uid, lpctstr pszTitle, lpctstr pszAuthor );
	void Event_BugReport( const tchar * pszText, int len, BUGREPORT_TYPE type, CLanguageID lang = 0 );
	void Event_ChatButton(const nchar * pszName); // Client's chat button was pressed
	void Event_ChatText( const nchar * pszText, int len, CLanguageID lang = 0 ); // Text from a client
    void Event_CombatAbilitySelect(dword dwAbility);
	void Event_CombatMode( bool fWar ); // Only for switching to combat mode
	bool Event_DoubleClick( CUID uid, bool fMacro, bool fTestTouch, bool fScript = false );
	void Event_ExtCmd( EXTCMD_TYPE type, tchar * pszName );
	void Event_Item_Drop( CUID uidItem, CPointMap pt, CUID uidOn, uchar gridIndex = 0 ); // Item is dropped on ground
	void Event_Item_Drop_Fail( CItem *pItem );
	void Event_Item_Dye( CUID uid, HUE_TYPE wHue );	// Rehue an item
	void Event_Item_Pickup( CUID uid, word amount ); // Client grabs an item
	void Event_MailMsg( CUID uid1, CUID uid2 );
	void Event_Profile( byte fWriteMode, CUID uid, lpctstr pszProfile, int iProfileLen );
	void Event_PromptResp( lpctstr pszText, size_t len, dword context1, dword context2, dword type, bool fNoStrip = false );
	void Event_PromptResp_GMPage( lpctstr pszReason );
	bool Event_SetName( CUID uid, const char * pszCharName );
	void Event_SingleClick( CUID uid );
	void Event_Talk( lpctstr pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, bool fNoStrip = false ); // PC speech
	void Event_TalkUNICODE( nword* wszText, int iTextLen, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, lpctstr pszLang );
	void Event_Target( dword context, CUID uid, CPointMap pt, byte flags = 0, ITEMID_TYPE id = ITEMID_NOTHING );
	void Event_Tips( word i ); // Tip of the day window
	void Event_ToolTip( CUID uid );
	void Event_UseToolbar(byte bType, dword dwArg);
	void Event_VendorBuy(CChar* pVendor, const VendorItem* items, uint uiItemCount);
	void Event_VendorBuy_Cheater( int iCode = 0 );
	void Event_VendorSell(CChar* pVendor, const VendorItem* items, uint uiItemCount);
	void Event_VendorSell_Cheater( int iCode = 0 );
    void Event_VirtueSelect(dword dwVirtue, CChar *pCharTarg);
	bool Event_Walk( byte rawdir, byte sequence = 0 ); // Player moves
	bool Event_CheckWalkBuffer(byte rawdir);
	bool Event_ExceededNetworkQuota(uchar uiType, int64 iBytes, int64 iQuota);

	TRIGRET_TYPE Menu_OnSelect( const CResourceID& rid, int iSelect, CObjBase * pObj );
	TRIGRET_TYPE Dialog_OnButton( const CResourceID& rid, dword dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs );

	bool Login_Relay( uint iServer ); // Relay player to a certain IP
	byte Login_ServerList( const char * pszAccount, const char * pszPassword ); // Initial login (Login on "loginserver", new format)

	byte Setup_Delete( dword iSlot ); // Deletion of character
	uint Setup_FillCharList(Packet* pPacket, const CChar * pCharFirst); // Write character list to packet
	byte Setup_ListReq( const char * pszAccount, const char * pszPassword, bool fTest ); // Gameserver login and character listing
	byte Setup_Play( uint iSlot ); // After hitting "Play Character" button
	byte Setup_Start( CChar * pChar ); // Send character startup stuff to player


    // translated commands.
private:
	int Cmd_Extract( CScript * pScript, const CRectMap &rect, int & zlowest );
	int Cmd_Skill_Menu_Build( const CResourceID& rid, int iSelect, CMenuItem* item, int iMaxSize, bool *fShowMenu, bool *fLimitReached );
public:
	bool Cmd_Skill_Menu( const CResourceID& rid, int iSelect = -1 );
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
	explicit CClient(CNetState* state);
	~CClient();

private:
	CClient(const CClient& copy);
	CClient& operator=(const CClient& other);

public:
	void CharDisconnect();

	CClient* GetNext() const
	{
		return ( static_cast <CClient*>( CSObjListRec::GetNext()));
	}

	virtual bool r_GetRef(lpctstr& ptcKey, CScriptObj*& pRef) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute script type command on me
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & s, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_LoadVal( CScript & s ) override;

	// Low level message traffic.
	static uint xCompress( byte * pOutput, const byte * pInput, uint outLen, uint inLen );

	bool xProcessClientSetup( CEvent * pEvent, uint uiLen );
	bool xPacketFilter(const byte * pEvent, uint uiLen = 0);
	bool xOutPacketFilter(const byte * pEvent, uint uiLen = 0);
	bool xCanEncLogin(bool bCheckCliver = false);	// Login crypt check
													// Low level push world data to the client.

	bool addRelay( const CServerDef * pServ );
	bool addLoginErr(byte code);
#define SF_UPDATE_HITS		0x01
#define SF_UPDATE_MANA		0x02
#define SF_UPDATE_STAM		0x04
#define SF_UPDATE_STATUS	0x08

	void addUpdateStatsFlag() {
		m_fUpdateStats |= SF_UPDATE_STATUS;
	}
	void addUpdateHitsFlag() {
		m_fUpdateStats |= SF_UPDATE_HITS;
	}
	void addUpdateManaFlag() {
		m_fUpdateStats |= SF_UPDATE_MANA;
	}
	void addUpdateStamFlag() {
		m_fUpdateStats |= SF_UPDATE_STAM;
	}
	void UpdateStats();
	bool addDeleteErr(byte code, dword iSlot) const;
	void addSeason(SEASON_TYPE season);
    void addWeather( WEATHER_TYPE weather = WEATHER_DEFAULT ); // Send new weather to player
    void addLight() const;
	void addTime( bool fCurrent = false ) const;
	void addObjectRemoveCantSee( const CUID& uid, lpctstr pszName = nullptr ) const;
	void closeContainer( const CObjBase * pObj ) const;
	void closeUIWindow( const CObjBase* pObj, PacketCloseUIWindow::UIWindow windowType ) const;
	void addObjectRemove( const CUID& uid ) const;
	void addObjectRemove( const CObjBase * pObj ) const;
	void addRemoveAll( bool fItems, bool fChars );

	void addItem_OnGround( CItem * pItem ); // Send items (on ground)
	void addItem_Equipped( const CItem * pItem );
	void addItem_InContainer( const CItem * pItem );
	void addItem( CItem * pItem );

	void addBuff( const BUFF_ICONS IconId, const dword ClilocOne, const dword ClilocTwo, const word durationSeconds = 0, lpctstr* pArgs = nullptr, uint uiArgCount = 0) const;
	void removeBuff(const BUFF_ICONS IconId) const;
	void resendBuffs() const;

	void addOpenGump( const CObjBase * pCont, GUMP_TYPE gump ) const;
	void addContainerContents( const CItemContainer * pCont, bool fCorpseEquip = false, bool fCorpseFilter = false, bool fShop = false ); // Send items
	bool addContainerSetup( const CItemContainer * pCont ); // Send Backpack (with items)

	void addPlayerStart( CChar * pChar );
	void addPlayerSee( const CPointMap & pt ); // Send objects the player can now see
	void addPlayerView( const CPointMap & pt, bool bFull = true );
	void addPlayerWarMode() const;

	void addCharMove( const CChar * pChar ) const;
	void addCharMove( const CChar * pChar, byte iCharDirFlag ) const;
	void addChar( CChar * pChar, bool fFull = true );
	void addCharName( const CChar * pChar ); // Singleclick text for a character
	void addItemName( CItem * pItem );

	bool addKick( CTextConsole * pSrc, bool fBlock = true );
	void addMusic( MIDI_TYPE id ) const;
	void addArrowQuest( int x, int y, int id ) const;
	void addEffect( EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate * pDst, const CObjBaseTemplate * pSrc,
        byte bSpeedMsecs = 5, byte bLoop = 1, bool fExplode = false, dword color = 0, dword render = 0, word effectid = 0,
        dword explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0 ) const;
	void addEffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *pptDest, const CPointMap *pptSrc,
        byte bSpeedMsecs = 5, byte bLoop = 1, bool fExplode = false, dword color = 0, dword render = 0, word effectid = 0,
        dword explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;

	void addSound( SOUND_TYPE id, const CObjBaseTemplate * pBase = nullptr, int iRepeat = 1 ) const;
	void addReSync();
	void addMap() const;
	void addMapDiff() const;
    void addMapWaypoint(CObjBase *pObj, MAPWAYPOINT_TYPE type) const;
	void addChangeServer() const;
	void addPlayerUpdate() const;

	void addBark( lpctstr pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD ) const;
	void addBarkUNICODE( const nchar * pText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang = 0 ) const;
	void addBarkLocalized( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, lpctstr pArgs = nullptr ) const;
	void addBarkLocalizedEx( int iClilocId, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_DEFAULT, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD, AFFIX_TYPE affix = AFFIX_APPEND, lpctstr pAffix = nullptr, lpctstr pArgs = nullptr ) const;
	void addBarkParse( lpctstr pszText, const CObjBaseTemplate * pSrc, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false, lpctstr name = "" ) const;
	void addSysMessage( lpctstr pMsg ); // System message (In lower left corner)
	void addObjMessage( lpctstr pMsg, const CObjBaseTemplate * pSrc, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY ); // The message when an item is clicked

    void addCodexOfWisdom(dword dwTopicID, bool fForceOpen = false);
	void addDyeOption( const CObjBase * pBase );
	void addWebLaunch( lpctstr pMsg ); // Direct client to a web page

	void addPromptConsole(CLIMODE_TYPE mode, lpctstr pMsg, CUID context1 = {}, CUID context2 = {}, bool fUnicode = false);
	void addTarget( CLIMODE_TYPE targmode, lpctstr pMsg, bool fAllowGround = false, bool fCheckCrime = false, int64 iTicksTimeout = 0, int iCliloc = 0 ); // Send targetting cursor to client
	void addTargetDeed( const CItem * pDeed );
	bool addTargetItems( CLIMODE_TYPE targmode, ITEMID_TYPE id, HUE_TYPE color = HUE_DEFAULT, bool fAllowGround = true );
	bool addTargetChars( CLIMODE_TYPE mode, CREID_TYPE id, bool fNoto, int64 iTicksTimeout = 0 );
	void addTargetVerb( lpctstr pCmd, lpctstr pArg );
	void addTargetFunctionMulti( lpctstr pszFunction, ITEMID_TYPE itemid, HUE_TYPE color = HUE_DEFAULT, bool fAllowGround = true );
	void addTargetFunction( lpctstr pszFunction, bool fAllowGround, bool fCheckCrime );
	void addTargetCancel();
	void addPromptConsoleFunction( lpctstr pszFunction, lpctstr pszSysmessage, bool bUnicode = false );

	void addScrollScript( CResourceLock &s, SCROLL_TYPE type, dword dwcontext = 0, lpctstr pszHeader = nullptr );
	void addScrollResource( lpctstr szResourceName, SCROLL_TYPE type, dword dwcontext = 0 );

	void addVendorClose( const CChar * pVendor );
	bool addShopMenuBuy( CChar * pVendor );
	bool addShopMenuSell( CChar * pVendor );
	void addBankOpen( CChar * pChar, LAYER_TYPE layer = LAYER_BANKBOX );

	void addSpellbookOpen( CItem * pBook );
	void addCustomSpellbookOpen( CItem * pBook, dword gumpID );
	bool addBookOpen( CItem * pBook ) const;
	void addBookPage( const CItem * pBook, word wPage, word wCount ) const;
	void addStatusWindow( CObjBase * pObj, bool fRequested = false ); // Opens the status window
	void addHitsUpdate( CChar * pChar );
	void addManaUpdate( CChar * pChar );
	void addStamUpdate( CChar * pChar );
	void addHealthBarUpdate( const CChar * pChar ) const;
	void addBondedStatus( const CChar * pChar, bool fIsDead ) const;
	void addSkillWindow(SKILL_TYPE skill, bool fFromInfo = false) const; // Opens the skills list
	void addBulletinBoard( const CItemContainer * pBoard );
	bool addBBoardMessage( const CItemContainer * pBoard, BBOARDF_TYPE flag, const CUID& uidMsg );

	void addToolTip( const CObjBase * pObj, lpctstr psztext ) const;
	void addDrawMap( CItemMap * pItem );
	void addMapMode( CItemMap * pItem, MAPCMD_TYPE iType, bool fEdit = false );

#define MAX_DIALOG_CONTROLTYPE_QTY  1000
	void addGumpTextDisp( const CObjBase * pObj, GUMP_TYPE gump, lpctstr pszName, lpctstr pszText );
	void addGumpInpVal( bool fcancel, INPVAL_STYLE style, dword dwmask, lpctstr ptext1, lpctstr ptext2, CObjBase * pObj );

	void addItemMenu( CLIMODE_TYPE mode, const CMenuItem * item, uint count, CObjBase * pObj = nullptr );
	void addGumpDialog( CLIMODE_TYPE mode, const CSString * sControls, uint iControls, const CSString * psText, uint iTexts, int x, int y, CObjBase * pObj = nullptr, dword dwRid = 0 );

	bool addGumpDialogProps( const CUID& uid );

	void addLoginComplete();
	void addChatSystemMessage(CHATMSG_TYPE iType, lpctstr pszName1 = nullptr, lpctstr pszName2 = nullptr, CLanguageID lang = 0 );

	void addCharPaperdoll( CChar * pChar );

	bool addAOSTooltip( CObjBase * pObj, bool fRequested = false, bool fShop = false );
private:
	void AOSTooltip_addName(CObjBase* pObj);
	void AOSTooltip_addDefaultCharData(CChar * pChar);
	void AOSTooltip_addDefaultItemData(CItem * pItem);

private:
// Number in comment are the old number working before we use the correct contextmenu ID
#define MAX_POPUPS 15
#define POPUPFLAG_LOCKED 0x01
#define POPUPFLAG_ARROW 0x02
#define POPUPFLAG_COLOR 0x20
#define POPUP_REQUEST 0
#define POPUP_PAPERDOLL 520		//11
#define POPUP_BACKPACK 302		//12
#define POPUP_PARTY_ADD 810		//13
#define POPUP_PARTY_REMOVE 811	//14
#define POPUP_TRADE_ALLOW 1013	//15
#define POPUP_TRADE_REFUSE 1014	//16
#define POPUP_TRADE_OPEN 819	//17
#define POPUP_BANKBOX 120		//21
#define POPUP_VENDORBUY 110		//31
#define POPUP_VENDORSELL 111	//32
#define POPUP_PETGUARD 130		//41
#define POPUP_PETFOLLOW 131		//42
#define POPUP_PETDROP 43
#define POPUP_PETKILL 134		//44
#define POPUP_PETSTOP 135		//45
#define POPUP_PETSTAY 137		//46
#define POPUP_PETFRIEND_ADD 133	//47
#define POPUP_PETFRIEND_REMOVE 140//48
#define POPUP_PETTRANSFER 136	//49
#define POPUP_PETRELEASE 138	//50
#define POPUP_STABLESTABLE 400	//51
#define POPUP_STABLERETRIEVE 401//52
#define POPUP_TAME 301			//53
#define POPUP_PETRENAME 919		//54
#define POPUP_TRAINSKILL 200	//100

/* This is a list of other context menu ID sphere do not use for now.
OpenMap = 10
TeleportVendor = 1015
OpenVendorContainer = 1018
NPCTalk = 303
DigForTreasure = 305
CancelProtection = 308
EnablePVPWarning = 320
ClaimBodRewards = 348
BodRequest = 403
ViewQuestLog = 404
CancelQuest = 405
QuestConversation = 406
InsuranceMenu = 416
ToggleItemInsurance = 418
Bribe = 419
OpenBackpackPet = 508
SetSecurity = 600
ReleaseCoOwnership = 602
LeaveHouse = 604
UnpackTransferCrate = 622
LoadShuriken = 701
QuestItem = 801
SiegeBless = 820
LoyaltyRating = 915
TitlesMenu = 918
EmergencyRepairs = 930
PermanentRepairs = 931
ShipSecurity = 934
ResetShipSecurity = 935
RenameShip = 936
DryDockShip = 937
MoveTillerman = 938
MannequinCompareSlotItem = 956
MannequinViewSuitStatsWithItems = 957
MannequinViewStats = 958
MannequinUseTransparentGump = 959
MannequinCustomizeBody = 1003
MannequinRotate = 1004
MannequinRedeed = 1006
VoidPool = 1010
Retrieve = 1012
SwitchMastery = 953
VendorSearch = 1016
AcceptFriendRequests = 1020
RefuseFriendRequests = 1021
RelocateContainer = 1022
*/

	PacketDisplayPopup* m_pPopupPacket;

public:
	void Event_AOSPopupMenuSelect( dword uid, word EntryTag );
	void Event_AOSPopupMenuRequest( dword uid );


	void addShowDamage( int damage, dword uid_damage );
	void addSpeedMode( byte speedMode = 0 );
	void addVisualRange( byte visualRange );
	void addIdleWarning( byte message );
	void addKRToolbar( bool bEnable );

	void SendPacket( tchar * pszPacket );
	void LogOpenedContainer(const CItemContainer* pContainer);

	// Test what I can do
	CAccount * GetAccount() const
	{
		return( m_pAccount );
	}
	bool IsPriv( word flag ) const
	{	// PRIV_GM
		if ( GetAccount() == nullptr )
			return false;
		return( GetAccount()->IsPriv( flag ));
	}
	void SetPrivFlags( word wPrivFlags )
	{
		if ( GetAccount() == nullptr )
			return;
		GetAccount()->SetPrivFlags( wPrivFlags );
	}
	void ClearPrivFlags( word wPrivFlags )
	{
		if ( GetAccount() == nullptr )
			return;
		GetAccount()->ClearPrivFlags( wPrivFlags );
	}

	// ------------------------------------------------
	bool IsResDisp( byte flag ) const
	{
		if ( GetAccount() == nullptr )
			return false;
		return( GetAccount()->IsResDisp( flag ) );
	}
	byte GetResDisp() const
	{
		if ( GetAccount() == nullptr )
			return( UINT8_MAX );
		return( GetAccount()->GetResDisp() );
	}
	bool SetResDisp( byte res )
	{
		if ( GetAccount() == nullptr )
			return false;
		return ( GetAccount()->SetResDisp( res ) );
	}
	bool SetGreaterResDisp( byte res )
	{
		if ( GetAccount() == nullptr )
			return false;
		return( GetAccount()->SetGreaterResDisp( res ) );
	}
	// ------------------------------------------------
	void SetScreenSize(ushort x, ushort y)
	{
		m_ScreenSize.x = x;
		m_ScreenSize.y = y;
	}

	PLEVEL_TYPE GetPrivLevel() const
	{
		// PLEVEL_Counsel
		if ( GetAccount() == nullptr )
			return( PLEVEL_Guest );
		return( GetAccount()->GetPrivLevel());
	}
	lpctstr GetName() const
	{
		if ( GetAccount() == nullptr )
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

	bool Dialog_Setup( CLIMODE_TYPE mode, const CResourceID& rid, int iPage, CObjBase * pObj, lpctstr Arguments = "" );
	bool Dialog_Close( CObjBase * pObj, dword dwRid, int buttonID );
	void Menu_Setup( CResourceID rid, CObjBase * pObj = nullptr );

	int OnSkill_Info( SKILL_TYPE skill, CUID uid, int iTestLevel, bool fTest );

	bool Cmd_Use_Item( CItem * pItem, bool fTestTouch, bool fScript = false );
	void Cmd_EditItem( CObjBase * pObj, int iSelect );

	bool IsConnectTypePacket() const
	{
		// This is a game or login server.
		// m_Crypt.IsInit()
		return ( (m_iConnectType == CONNECT_CRYPT) || (m_iConnectType == CONNECT_LOGIN) || (m_iConnectType == CONNECT_GAME) );
	}

	CONNECT_TYPE GetConnectType() const
	{
		return m_iConnectType;
	}
	lpctstr GetConnectTypeStr( CONNECT_TYPE iType );

	void SetConnectType( CONNECT_TYPE iType );

	// Stuff I am doing. Started by a command
	CLIMODE_TYPE GetTargMode() const
	{
		return m_Targ_Mode ;
	}
	void SetTargMode( CLIMODE_TYPE targmode = CLIMODE_NORMAL, lpctstr pszPrompt = nullptr, int64 iTickTimeout = 0, int iCliloc = 0 );
	void ClearTargMode()
	{
		// done with the last mode.
		m_Targ_Mode = CLIMODE_NORMAL;
		m_Targ_Timeout = 0;
	}

	bool IsConnecting() const;

	CNetState* GetNetState(void) const { return m_net; };

public:
	char		m_zLastMessage[SCRIPT_MAX_LINE_LEN];	// last sysmessage
	char		m_zLastObjMessage[SCRIPT_MAX_LINE_LEN];	// last message
	char		m_zLogin[64];
	int64       m_tNextPickup;
	CVarDefMap	m_TagDefs;
	CVarDefMap	m_BaseDefs;		// New Variable storage system
	typedef std::map<dword, std::pair<std::pair<dword,dword>, CPointMap> > OpenedContainerMap_t;
	OpenedContainerMap_t m_openedContainers;	// list of UIDs of all opened containers by the client

	CItemMultiCustom * m_pHouseDesign; // The building this client is designing

public:
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( ptcKey, fZero );
	}

	int64 GetDefNum( lpctstr ptcKey ) const
	{
		return m_BaseDefs.GetKeyNum( ptcKey );
	}

	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true)
	{
		m_BaseDefs.SetNum(ptcKey, iVal, fZero);
	}

	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true)
	{
		m_BaseDefs.SetStr(ptcKey, fQuoted, pszVal, fZero);
	}

	void DeleteDef(lpctstr ptcKey)
	{
		m_BaseDefs.DeleteKey(ptcKey);
	}

	friend class CNetworkInput;
	friend class CNetworkOutput;
	friend class PacketCreate;
	friend class PacketServerRelay;
};


#endif	// _INC_CCLIENT_H
