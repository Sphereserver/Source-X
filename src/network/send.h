/**
* @file send.h
* @brief Outward packets.
*/

#ifndef _INC_SEND_H
#define _INC_SEND_H


#include "../common/CUID.h"
#include "../common/CLanguageID.h"
#include "../common/sphereproto.h"
#include "../common/CRect.h"
#include "../game/game_enums.h"
#include "../game/CServerConfig.h"
#include "CNetState.h"
#include "packet.h"
#include <memory>


struct CMenuItem;
class CItemMap;
class CObjBaseTemplate;
class CSpellDef;
class CItemContainer;
class CItemMessage;
class CItemCorpse;
class CCharRefArray;
class CItemMultiCustom;
class CItemShip;
class CClientTooltip;


// TODO: define the virtual destructor of each class with virtual methods in the .cpp file.
//  Not doing that makes the compiler emit the virtual table (vtable) in every translation unit, instead
//  of having that only in one .cpp file (translation unit) -> the one file where we define the destructor or at least one virtual method.
#ifdef __clang__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wweak-vtables"
#endif

/***************************************************************************
 *
 *
 *	Packet **** : PacketGeneric				Temporary packet till all will be redone! (NORMAL)
 *
 *
 ***************************************************************************/
/*
class PacketGeneric : public PacketSend
{
public:
	PacketGeneric(const CClient* target, byte *data, uint length);
};
*/

/***************************************************************************
 *
 *
 *	Packet **** : PacketTelnet				send message to telnet client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTelnet : public PacketSend
{
public:

	PacketTelnet(const CClient* target, lpctstr message, bool fNullTerminated = false);
};

/***************************************************************************
 *
 *
 *	Packet **** : PacketWeb					send message to web client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketWeb : public PacketSend
{
public:
	PacketWeb(const CClient * target = nullptr, const byte * data = nullptr, uint length = 0);
	void setData(const byte * data, uint length);
};

/***************************************************************************
 *
 *
 *	Packet 0x0B : PacketCombatDamage		sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCombatDamage : public PacketSend
{
public:
	PacketCombatDamage(const CClient* target, word damage, CUID defender);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_NEWDAMAGE);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x11 : PacketObjectStatus		sends status window data (LOW)
 *
 *
 ***************************************************************************/
class PacketObjectStatus : public PacketSend
{
public:
	PacketObjectStatus(const CClient* target, CObjBase* object);

private:
	void WriteVersionSpecific(const CClient* target, CChar* other, byte version);
};

/***************************************************************************
*
*
*	Packet 0x16 : PacketHealthBarUpdateNew		update health bar colour (LOW)
*
*
***************************************************************************/
class PacketHealthBarUpdateNew : public PacketSend
{
private:
    CUID m_character;

public:
    enum Color
    {
        GreenBar	= 0x1,
        YellowBar	= 0x2
    };

    PacketHealthBarUpdateNew(const CClient* target, const CChar* character);

    virtual bool onSend(const CClient* client);

    virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
    static bool CanSendTo(const CNetState* state)
    {
        return state->isClientEnhanced();
    }
};

/***************************************************************************
 *
 *
 *	Packet 0x17 : PacketHealthBarUpdate		update health bar colour (LOW)
 *
 *
 ***************************************************************************/
class PacketHealthBarUpdate : public PacketSend
{
private:
	CUID m_character;

public:
	enum Color
	{
		GreenBar = 1,
		YellowBar = 2
	};

	PacketHealthBarUpdate(const CClient* target, const CChar* character);

	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
	    return state->isClientVersionNumber(MINCLIVER_SA) || state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x1A : PacketItemWorld			sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemWorld : public PacketSend
{
private:
	CUID m_item;

protected:
	PacketItemWorld(byte id, uint size, const CUID& uid);

public:
	PacketItemWorld(const CClient* target, const CItem* item);

	static void adjustItemData(const CClient* target, const CItem* item, ITEMID_TYPE &id, HUE_TYPE &hue, word &amount, DIR_TYPE &dir, byte &flags, byte &light);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x1B : PacketPlayerStart			allow client to start playing (HIGH)
 *
 *
 ***************************************************************************/
class PacketPlayerStart : public PacketSend
{
public:
	PacketPlayerStart(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x1C: PacketMessageASCII			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageASCII : public PacketSend
{
public:
	PacketMessageASCII(const CClient* target, lpctstr pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font);
};

/***************************************************************************
 *
 *
 *	Packet 0x1D : PacketRemoveObject		removes object from view (NORMAL)
 *
 *
 ***************************************************************************/
class PacketRemoveObject : public PacketSend
{
public:
	PacketRemoveObject(const CClient* target, CUID uid);
};

/***************************************************************************
 *
 *
 *	Packet 0x20 : PacketPlayerUpdate		update player character on screen (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPlayerUpdate : public PacketSend
{
public:
	PacketPlayerUpdate(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x21 : PacketMovementRej			rejects movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementRej : public PacketSend
{
public:
	PacketMovementRej(const CClient* target, byte sequence);
};

/***************************************************************************
 *
 *
 *	Packet 0x22 : PacketMovementAck			accepts movement (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketMovementAck : public PacketSend
{
public:
	PacketMovementAck(const CClient* target, byte sequence);
};

/***************************************************************************
 *
 *
 *	Packet 0x23 : PacketDragAnimation		drag animation (LOW)
 *
 *
 ***************************************************************************/
class PacketDragAnimation : public PacketSend
{
public:
	PacketDragAnimation(const CChar* source, const CItem* item, const CObjBase* container, const CPointMap* pt);

	virtual bool canSendTo(const CNetState* state) const;
};

/***************************************************************************
 *
 *
 *	Packet 0x24 : PacketContainerOpen		open container gump (LOW)
 *
 *
 ***************************************************************************/
class PacketContainerOpen : public PacketSend
{
private:
	CUID m_container;

public:
	PacketContainerOpen(const CClient* target, const CObjBase* container, GUMP_TYPE gump);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x25 : PacketItemContainer		sends item in a container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemContainer : public PacketSend
{
private:
	CUID m_item;

public:
	PacketItemContainer(const CClient* target, const CItem* item);
	PacketItemContainer(const CItem* spellbook, const CSpellDef* spell);

	void completeForTarget(const CClient* target, const CItem* spellbook);
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x26 : PacketKick				notifies client they have been kicked (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketKick : public PacketSend
{
public:
	PacketKick(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x27 : PacketDragCancel			cancel item drag (HIGH)
 *
 *
 ***************************************************************************/
class PacketDragCancel : public PacketSend
{
public:
	enum Reason
	{
		CannotLift = 0x00,
		OutOfRange = 0x01,
		OutOfSight = 0x02,
		TryToSteal = 0x03,
		AreHolding = 0x04,
		Other = 0x05
	};

	PacketDragCancel(const CClient* target, Reason code);
};

/***************************************************************************
 *
 *
 *	Packet 0x29 : PacketDropAccepted		notify drop accepted (kr) (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDropAccepted : public PacketSend
{
public:
	PacketDropAccepted(const CClient* target);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x2C : PacketDeathMenu			display death menu/effect (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDeathMenu : public PacketSend
{
public:
	enum Mode
	{
		Dead = 0x00,        // Old "server sent"
        //Resurrect = 0x1,    // Sent by the client
		//Alive = 0x02        // Sent by the client
	};

	PacketDeathMenu(const CClient* target, Mode mode);
};

/***************************************************************************
 *
 *
 *	Packet 0x2E : PacketItemEquipped		sends equipped item (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemEquipped : public PacketSend
{
public:
	PacketItemEquipped(const CClient* target, const CItem* item);
};

/***************************************************************************
 *
 *
 *	Packet 0x2F : PacketSwing				fight swing (LOW)
 *
 *
 ***************************************************************************/
class PacketSwing : public PacketSend
{
public:
	PacketSwing(const CClient* target, const CChar* defender);
};

/***************************************************************************
 *
 *
 *	Packet 0x3A : PacketSkills				character skills (LOW)
 *
 *
 ***************************************************************************/
class PacketSkills : public PacketSend
{
public:
	PacketSkills(const CClient* target, const CChar* character, SKILL_TYPE skill);
};

/***************************************************************************
 *
 *
 *	Packet 0x3B : PacketCloseVendor			close vendor menu (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseVendor : public PacketSend
{
public:
	PacketCloseVendor(const CClient* target, const CChar* vendor);
};

/***************************************************************************
 *
 *
 *	Packet 0x3C : PacketItemContents		contents of an item (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemContents : public PacketSend
{
private:
	CUID m_container;
	word m_count;

public:
	PacketItemContents(CClient* target, const CItemContainer* container, bool fIsShop, bool fFilterLayers); // standard content
	PacketItemContents(const CClient* target, const CItem* spellbook);			// spellbook spells
	PacketItemContents(const CClient* target, const CItemContainer* spellbook); // custom spellbook spells
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x3F : PacketQueryClient			Query Client for block info (NORMAL)
 *
 *
 ***************************************************************************/
class PacketQueryClient : public PacketSend
{
public:
	PacketQueryClient(CClient* target, byte bCmd = 0xFF);

};

/***************************************************************************
 *
 *
 *	Packet 0x4F : PacketGlobalLight			sets global light level (NORMAL)
 *
 *
 ***************************************************************************/
class PacketGlobalLight : public PacketSend
{
public:
	PacketGlobalLight(const CClient* target, byte light);
};

/***************************************************************************
 *
 *
 *	Packet 0x53 : PacketWarningMessage		show popup warning message (NORMAL)
 *
 *
 ***************************************************************************/
class PacketWarningMessage : public PacketSend
{
public:
	enum Message
	{
		BadPassword = 0x00,
		NoCharacter = 0x01,
		CharacterExists = 0x02,
		Other = 0x03,
		Other2 = 0x04,
		CharacterInWorld = 0x05,
		SyncError = 0x06,
		Idle = 0x07,
		CouldntAttachServer = 0x08,
		CharacterTransfer = 0x09,
		InvalidName = 0x0A
	};

	PacketWarningMessage(const CClient* target, Message code);
};

/***************************************************************************
 *
 *
 *	Packet 0x54 : PacketPlaySound			play a sound (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPlaySound : public PacketSend
{
public:
	PacketPlaySound(const CClient* target, SOUND_TYPE sound, int flags, int volume, const CPointMap& pos);
};

/***************************************************************************
 *
 *
 *	Packet 0x55 : PacketLoginComplete		redraw all (NORMAL)
 *
 *
 ***************************************************************************/
class PacketLoginComplete : public PacketSend
{
public:
	PacketLoginComplete(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0x56 : PacketMapPlot				show/edit map plots (LOW)
 *
 *
 ***************************************************************************/
class PacketMapPlot : public PacketSend
{
public:
	PacketMapPlot(const CClient* target, const CItem* map, MAPCMD_TYPE mode, bool edit);
	PacketMapPlot(const CItem* map, MAPCMD_TYPE mode, bool edit);

	void setPin(short x, short y);
};

/***************************************************************************
 *
 *
 *	Packet 0x5B : PacketGameTime			current game time (IDLE)
 *
 *
 ***************************************************************************/
class PacketGameTime : public PacketSend
{
public:
	PacketGameTime(const CClient* target, int hours = 0, int minutes = 0, int seconds = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0x65 : PacketWeather				set current weather (IDLE)
 *
 *
 ***************************************************************************/
class PacketWeather : public PacketSend
{
public:
	PacketWeather(const CClient* target, WEATHER_TYPE weather, int severity, int temperature);
};

/***************************************************************************
 *
 *
 *	Packet 0x66 : PacketBookPageContent		send book page content (LOW)
 *
 *
 ***************************************************************************/
class PacketBookPageContent : public PacketSend
{
protected:
	uint m_pages;

public:
	PacketBookPageContent(const CClient* target, const CItem* book, word startpage, word pagecount = 1);
	void addPage(const CItem* book, word page);
};

/***************************************************************************
 *
 *
 *	Packet 0x6C : PacketAddTarget				adds target cursor to client (LOW)
 *	Packet 0x99 : PacketAddTarget				adds target cursor to client with multi (LOW)
 *
 *
 ***************************************************************************/
class PacketAddTarget : public PacketSend
{
public:
	enum TargetType
	{
		Object = 0x00, // items/chars only
		Ground = 0x01  // also allow ground
	};

	enum Flags
	{
		None = 0x00,
		Harmful = 0x01,
		Beneficial = 0x02,
		Cancel = 0x03
	};

	PacketAddTarget(const CClient* target, TargetType type, dword context, Flags flags);
	PacketAddTarget(const CClient* target, TargetType type, dword context, Flags flags, ITEMID_TYPE id, HUE_TYPE color);
};

/***************************************************************************
 *
 *
 *	Packet 0x6D : PacketPlayMusic			adds music to the client (IDLE)
 *
 *
 ***************************************************************************/
class PacketPlayMusic : public PacketSend
{
public:
	PacketPlayMusic(const CClient* target, word musicID);
};

/***************************************************************************
 *
 *
 *	Packet 0x6E : PacketAction				plays an animation (LOW)
 *	Packet 0xE2 : PacketActionBasic			plays an animation (client > 7.0.0.0) (LOW)
 *
 *
 ***************************************************************************/
class PacketAction : public PacketSend
{
public:
	PacketAction(const CChar* character, ANIM_TYPE action, word repeat, bool backward, byte delay, byte len);
    virtual ~PacketAction();
};

class PacketActionBasic : public PacketSend
{
public:
	PacketActionBasic(const CChar* character, ANIM_TYPE_NEW action, ANIM_TYPE_NEW subaction, byte variation);
    virtual ~PacketActionBasic();
};

/***************************************************************************
 *
 *
 *	Packet 0x6F : PacketTradeAction			perform a trade action (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTradeAction : public PacketSend
{
public:
	PacketTradeAction(SECURE_TRADE_TYPE action);
    ~PacketTradeAction();
	void prepareContainerOpen(const CChar *character, const CItem *container1, const CItem *container2);
	void prepareReadyChange(const CItemContainer *container1, const CItemContainer *container2);
	void prepareClose(const CItemContainer *container);
	void prepareUpdateGold(const CItemContainer *container, dword gold, dword platinum);
	void prepareUpdateLedger(const CItemContainer *container, dword gold, dword platinum);
};

/***************************************************************************
 *
 *
 *	Packet 0x70 : PacketEffect				displays a visual effect (NORMAL)
 *	Packet 0xC0 : PacketEffect				displays a hued visual effect (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEffect : public PacketSend
{
public:
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, byte speed, byte loop, bool explode);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, byte speed, byte loop, bool explode, dword hue, dword render);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, byte speed, byte loop, bool explode, dword hue, dword render, word effectid, dword explodeid, word explodesound, dword effectuid, byte type);
	void writeBasicEffect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBaseTemplate* dst, const CObjBaseTemplate* src, byte speed, byte loop, bool explode);

	/*Effect at a Map Point instead of an Object*/
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptDest, const CPointMap *ptSrc, byte speed, byte loop, bool explode);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptDest, const CPointMap *ptSrc, byte speed, byte loop, bool explode, dword hue, dword render);
	PacketEffect(const CClient* target, EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptDest, const CPointMap *ptSrc, byte speed, byte loop, bool explode, dword hue, dword render, word effectid, dword explodeid, word explodesound, dword effectuid, byte type);
	void writeBasicEffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptSrc, const CPointMap *ptDest, byte speed, byte loop, bool explode);

	void writeHuedEffect(dword hue, dword render);
};

/***************************************************************************
 *
 *
 *	Packet 0x71 : PacketBulletinBoard		display a bulletin board or message (LOW)
 *
 *
 ***************************************************************************/
class PacketBulletinBoard : public PacketSend
{
public:
	PacketBulletinBoard(const CClient* target, const CItemContainer* board);
	PacketBulletinBoard(const CClient* target, BBOARDF_TYPE action, const CItemContainer* board, const CItemMessage* message);
};

/***************************************************************************
 *
 *
 *	Packet 0x72 : PacketWarMode				update war mode status (LOW)
 *
 *
 ***************************************************************************/
class PacketWarMode : public PacketSend
{
public:
	PacketWarMode(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0x73 : PacketPingAck				ping reply (IDLE)
 *
 *
 ***************************************************************************/
class PacketPingAck : public PacketSend
{
public:
	PacketPingAck(const CClient* target, byte value = 0);
};

/***************************************************************************
 *
 *
 *	Packet 0x74 : PacketVendorBuyList		show list of vendor items (LOW)
 *
 *
 ***************************************************************************/
class PacketVendorBuyList : public PacketSend
{
public:
	PacketVendorBuyList(void);
	uint fillBuyData(const CItemContainer* container, int iConvertFactor);
};

/***************************************************************************
 *
 *
 *	Packet 0x76 : PacketZoneChange			change server zone (LOW)
 *
 *
 ***************************************************************************/
class PacketZoneChange : public PacketSend
{
public:
	PacketZoneChange(const CClient* target, const CPointMap& pos);
};

/***************************************************************************
 *
 *
 *	Packet 0x77 : PacketCharacterMove		move a character (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCharacterMove : public PacketSend
{
public:
	PacketCharacterMove(const CClient* target, const CChar* character, byte direction);
};

/***************************************************************************
 *
 *
 *	Packet 0x78 : PacketCharacter			create a character (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCharacter : public PacketSend
{
private:
	CUID m_character;

public:
	PacketCharacter(CClient* target, const CChar* character);

	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x7C : PacketDisplayMenu			show a menu selection (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMenu : public PacketSend
{
public:
	PacketDisplayMenu(const CClient* target, CLIMODE_TYPE mode, const CMenuItem* items, uint count, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x81 : PacketChangeCharacter		allow client to change character (LOW)
 *
 *
 ***************************************************************************/
class PacketChangeCharacter : public PacketSend
{
public:
	PacketChangeCharacter(CClient* target);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return !(state->isClientKR() || state->isClientEnhanced());
	}
};

/***************************************************************************
 *
 *
 *	Packet 0x82 : PacketLoginError			login error response (HIGHEST)
 *
 *
 ***************************************************************************/
class PacketLoginError : public PacketSend
{
public:
	enum Reason
	{
		Invalid = 0x00, // no account
		InUse	= 0x01, // already in use
		Blocked = 0x02, // client blocked
		BadPass = 0x03, // incorrect password
		Other	= 0x04, // other (e.g. timeout)

		// the error codes below are not sent to or understood by the client,
		// and should be translated into one of the codes above
		BadVersion,     // version not permitted
		BadCharacter,   // invalid character selected
		BadAuthID,      // incorrect auth id
		BadAccount,     // bad account name (length, characters)
		BadPassword,    // bad password (length, characters)
		BadEncLength,   // bad message length
		EncUnknown,     // unknown encryption
		EncCrypt,       // crypted client not allowed
		EncNoCrypt,     // non-crypted client not allowed
		CharIdle,       // character is already ingame
		TooManyChars,   // account has too many characters
		CreationBlocked,// character creation is blocked in this moments.
		BlockedIP,      // ip is blocked
		MaxClients,     // max clients reached
		MaxGuests,      // max guests reached
		MaxPassTries,   // max password tries reached


		Success = 0xFF  // no error
	};

	PacketLoginError(const CClient* target, Reason reason);
};

/***************************************************************************
 *
 *
 *	Packet 0x85 : PacketDeleteError			delete character error response (LOW)
 *
 *
 ***************************************************************************/
class PacketDeleteError : public PacketSend
{
public:
	enum Reason
	{
		BadPass        = 0x00, // incorrect password
		NotExist       = 0x01, // character does not exist
		InUse          = 0x02, // character is being played right now
		NotOldEnough   = 0x03, // character is not old enough to delete
		BackupQueue    = 0x04, // character is currently queued for backup
		InvalidRequest = 0x05, // couldn't carry out the request

		Success        = 0xFF  // no error
	};

	PacketDeleteError(const CClient* target, Reason reason);
};

/***************************************************************************
 *
 *
 *	Packet 0x86 : PacketCharacterListUpdate	update character list (LOW)
 *
 *
 ***************************************************************************/
class PacketCharacterListUpdate : public PacketSend
{
public:
	PacketCharacterListUpdate(CClient* target, const CChar* lastCharacter);
};

/***************************************************************************
 *
 *
 *	Packet 0x88 : PacketPaperdoll			show paperdoll (LOW)
 *
 *
 ***************************************************************************/
class PacketPaperdoll : public PacketSend
{
public:
	PacketPaperdoll(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0x89 : PacketCorpseEquipment		send corpse equipment (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCorpseEquipment : public PacketSend
{
private:
	CUID m_corpse;

public:
	PacketCorpseEquipment(CClient* target, const CItemContainer* corpse);
	virtual bool onSend(const CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x8B : PacketSignGump			show a sign (LOW)
 *
 *
 ***************************************************************************/
class PacketSignGump : public PacketSend
{
public:
	PacketSignGump(const CClient* target, const CObjBase* object, GUMP_TYPE gump, lpctstr unknown, lpctstr text);
};

/***************************************************************************
 *
 *
 *	Packet 0x8C : PacketServerRelay			relay client to server (IDLE)
 *
 *
 ***************************************************************************/
class PacketServerRelay : public PacketSend
{
private:
	dword m_customerId;

public:
	PacketServerRelay(const CClient* target, dword ip, word port, dword customerId);
	virtual void onSent(CClient* client);
};

/***************************************************************************
 *
 *
 *	Packet 0x90 : PacketDisplayMap			display map (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMap : public PacketSend
{
public:
	PacketDisplayMap(const CClient* target, const CItemMap* map, const CRectMap& rect);
};

/***************************************************************************
 *
 *
 *	Packet 0x93 : PacketDisplayBook			display book (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayBook : public PacketSend
{
public:
	PacketDisplayBook(const CClient* target, CItem* book);
};

/***************************************************************************
 *
 *
 *	Packet 0x95 : PacketShowDyeWindow		show dye window (LOW)
 *
 *
 ***************************************************************************/
class PacketShowDyeWindow : public PacketSend
{
public:
	PacketShowDyeWindow(const CClient* target, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x98 : PacketAllNamesResponse	all names macro response (IDLE)
 *
 *
 ***************************************************************************/
class PacketAllNamesResponse : public PacketSend
{
public:
	PacketAllNamesResponse(const CClient* target, const CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0x9A : PacketAddPrompt			prompt for ascii text response (LOW)
 *	Packet 0xC2 : PacketAddPrompt			prompt for unicode text response (LOW)
 *
 *
 ***************************************************************************/
class PacketAddPrompt : public PacketSend
{
public:
	PacketAddPrompt(const CClient* target, CUID context1, CUID context2, bool useUnicode);
};

/***************************************************************************
 *
 *
 *	Packet 0x9E : PacketVendorSellList		show list of items to sell (LOW)
 *
 *
 ***************************************************************************/
class PacketVendorSellList : public PacketSend
{
public:
	PacketVendorSellList(const CChar* vendor);
	uint fillSellList(CClient* target, const CItemContainer* container, CItemContainer* stock1, CItemContainer* stock2, int iConvertFactor);
};

/***************************************************************************
 *
 *
 *	Packet 0xA1 : PacketHealthUpdate		update character health (LOW)
 *
 *
 ***************************************************************************/
class PacketHealthUpdate : public PacketSend
{
public:
	PacketHealthUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA2 : PacketManaUpdate			update character mana (LOW)
 *
 *
 ***************************************************************************/
class PacketManaUpdate : public PacketSend
{
public:
	PacketManaUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA3 : PacketStaminaUpdate		update character stamina (LOW)
 *
 *
 ***************************************************************************/
class PacketStaminaUpdate : public PacketSend
{
public:
	PacketStaminaUpdate(const CChar* character, bool full);
};

/***************************************************************************
 *
 *
 *	Packet 0xA5 : PacketWebPage				send client to a webpage (LOW)
 *
 *
 ***************************************************************************/
class PacketWebPage : public PacketSend
{
public:
	PacketWebPage(const CClient* target, lpctstr url);
};

/***************************************************************************
 *
 *
 *	Packet 0xA6 : PacketOpenScroll			open scroll message (LOW)
 *
 *
 ***************************************************************************/
class PacketOpenScroll : public PacketSend
{
public:
	PacketOpenScroll(const CClient* target, CResourceLock &s, SCROLL_TYPE type, dword context, lpctstr header);
};

/***************************************************************************
 *
 *
 *	Packet 0xA8 : PacketServerList			send server list (LOW)
 *
 *
 ***************************************************************************/
class PacketServerList : public PacketSend
{
public:
	PacketServerList(const CClient* target);
	void writeServerEntry(const CServerRef& server, int index, bool reverseIp);
};

/***************************************************************************
 *
 *
 *	Packet 0xA9 : PacketCharacterList		send character list (LOW)
 *
 *
 ***************************************************************************/
class PacketCharacterList : public PacketSend
{
public:
	PacketCharacterList(CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xAA : PacketAttack				set attack target (NORMAL)
 *
 *
 ***************************************************************************/
class PacketAttack : public PacketSend
{
public:
	PacketAttack(const CClient* target, CUID serial);
};

/***************************************************************************
 *
 *
 *	Packet 0xAB : PacketGumpValueInput		show input dialog (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpValueInput : public PacketSend
{
public:
	PacketGumpValueInput(const CClient* target, bool cancel, INPVAL_STYLE style, dword maxLength, lpctstr text, lpctstr caption, CObjBase* object);
};

/***************************************************************************
 *
 *
 *	Packet 0xAE: PacketMessageUNICODE			show message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageUNICODE : public PacketSend
{
public:
	PacketMessageUNICODE(const CClient* target, const nachar* pszText, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID language);
};

/***************************************************************************
 *
 *
 *	Packet 0xAF : PacketDeath				notifies about character death (NORMAL)
 *
 *
 ***************************************************************************/
class PacketDeath : public PacketSend
{
public:
	PacketDeath(CChar* dead, CItemCorpse* corpse, bool fFrontFall);
};

/***************************************************************************
 *
 *
 *	Packet 0xB0 : PacketGumpDialog			displays a dialog gump (LOW)
 *	Packet 0xDD : PacketGumpDialog			displays a dialog gump using compression (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpDialog : public PacketSend
{
public:
	PacketGumpDialog(int x, int y, CObjBase* object, dword context);
	void writeControls(const CClient* target, std::vector<CSString> const* controls, std::vector<CSString> const* texts);

protected:
	void writeCompressedControls(std::vector<CSString> const* controls, std::vector<CSString> const* texts);
	void writeStandardControls(std::vector<CSString> const* controls, std::vector<CSString> const* texts);
};

/***************************************************************************
 *
 *
 *	Packet 0xB2 : PacketChatMessage			send a chat system message (LOW)
 *
 *
 ***************************************************************************/
class PacketChatMessage : public PacketSend
{
public:
	PacketChatMessage(const CClient* target, CHATMSG_TYPE type, lpctstr param1, lpctstr param2, CLanguageID language);
};

/***************************************************************************
 *
 *
 *	Packet 0xB7 : PacketTooltip				send a tooltip (IDLE)
 *
 *
 ***************************************************************************/
class PacketTooltip : public PacketSend
{
public:
	PacketTooltip(const CClient* target, const CObjBase* object, lpctstr text);
};

/***************************************************************************
 *
 *
 *	Packet 0xB8 : PacketProfile				send a character profile (LOW)
 *
 *
 ***************************************************************************/
class PacketProfile : public PacketSend
{
public:
	PacketProfile(const CClient* target, const CChar* character);
};

/***************************************************************************
 *
 *
 *	Packet 0xB9 : PacketEnableFeatures		enable client features (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEnableFeatures : public PacketSend
{
public:
	PacketEnableFeatures(const CClient* target, dword flags);
};

/***************************************************************************
 *
 *
 *	Packet 0xBA : PacketArrowQuest			display onscreen arrow for client to follow (NORMAL)
 *
 *
 ***************************************************************************/
class PacketArrowQuest : public PacketSend
{
public:
	PacketArrowQuest(const CClient* target, int x, int y, int id);
};

/***************************************************************************
 *
 *
 *	Packet 0xBC : PacketSeason				change season (NORMAL)
 *
 *
 ***************************************************************************/
class PacketSeason : public PacketSend
{
public:
	PacketSeason(const CClient* target, SEASON_TYPE season, bool playMusic);
};

/***************************************************************************
*
*
*	Packet 0xBD : PacketClientVersionReq	request client version (HIGH)
*
*
***************************************************************************/
class PacketClientVersionReq : public PacketSend
{
public:
	PacketClientVersionReq(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF : PacketExtended			extended command
 *
 *
 ***************************************************************************/
class PacketExtended : public PacketSend
{
public:
	PacketExtended(EXTDATA_TYPE type, uint len = 0, Priority priority = PRI_NORMAL);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x04 : PacketGumpChange		change gump (LOW)
 *
 *
 ***************************************************************************/
class PacketGumpChange : public PacketExtended
{
public:
	PacketGumpChange(const CClient* target, dword context, int buttonId);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06 : PacketParty			party packet
 *
 *
 ***************************************************************************/
class PacketParty : public PacketExtended
{
public:
	PacketParty(PARTYMSG_TYPE type, uint len = 0, Priority priority = PRI_NORMAL);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x01 : PacketPartyList		send list of party members (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyList : public PacketParty
{
public:
	PacketPartyList(const CCharRefArray* members);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x02 : PacketPartyRemoveMember		remove member from party (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyRemoveMember : public PacketParty
{
public:
	PacketPartyRemoveMember(const CChar* member, const CCharRefArray* members);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x04 : PacketPartyChat		send party chat message (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyChat : public PacketParty
{
public:
	PacketPartyChat(const CChar* source, const nachar* text);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x06.0x07 : PacketPartyInvite	send party invitation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketPartyInvite : public PacketParty
{
public:
	PacketPartyInvite(const CClient* target, const CChar* inviter);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x08 : PacketMapChange			change map (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMapChange : public PacketExtended
{
public:
	PacketMapChange(const CClient* target, int map);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x10 : PacketPropertyListVersionOld		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
class PacketPropertyListVersionOld : public PacketExtended
{
protected:
	CUID m_object;

public:
	PacketPropertyListVersionOld(const CClient* target, const CObjBase* object, dword version);
	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_TOOLTIP);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x14 : PacketDisplayPopup		display popup menu (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayPopup : public PacketExtended
{
private:
	bool m_newPacketFormat;
	int m_popupCount;

public:
	PacketDisplayPopup(const CClient* target, CUID uid);

	void addOption(word entryTag, dword textId, word flags, word color);
	void finalise(void);

	int getOptionCount(void) const
	{
		return m_popupCount;
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x16 : PacketCloseUIWindow		Close Container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseUIWindow : public PacketExtended
{
public:
    enum UIWindow
    {
        Paperdoll = 1,
        Status = 2,
        Profile = 8,
        Container = 0xC
    };

	PacketCloseUIWindow(const CClient* target, const CObjBase* obj, UIWindow command);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x16.0x0C : PacketCloseContainer		Close Container (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCloseContainer : public PacketExtended
{
public:
	PacketCloseContainer(const CClient* target, const CObjBase* object);
};

/***************************************************************************
*
*
*	Packet 0xBF.0x17 : PacketCodexOfWisdom		open Codex of Wisdom (LOW)
*
*
***************************************************************************/
class PacketCodexOfWisdom : public PacketExtended
{
public:
    PacketCodexOfWisdom(const CClient *target, dword dwTopicID, bool fForceOpen);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x18 : PacketEnableMapDiffs		enable use of map diff files (NORMAL)
 *
 *
 ***************************************************************************/
class PacketEnableMapDiffs : public PacketExtended
{
public:
	PacketEnableMapDiffs(const CClient* target);
};

/***************************************************************************
*
*
*	Packet 0xBF.0x19.0x00 : PacketBondedStatus		set bonded status (NORMAL)
*
*
***************************************************************************/
class PacketBondedStatus : public PacketExtended
{
public:
	PacketBondedStatus(const CClient * target, const CChar * pChar, bool IsGhost);
};

/***************************************************************************
*
*
*	Packet 0xBF.0x19.0x02 : PacketStatLocks		update lock status of stats (NORMAL)
*
*
***************************************************************************/
class PacketStatLocks : public PacketExtended
{
public:
    PacketStatLocks(const CClient* target, const CChar* character);

    virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
    static bool CanSendTo(const CNetState* state)
    {
        return state->isClientVersionNumber(MINCLIVER_STATLOCKS);
    }
};

/***************************************************************************
*
*
*	Packet 0xBF.0x19.0x05 : PacketStatueAnimation	update character animation frame (NORMAL)
*
*
***************************************************************************/
class PacketStatueAnimation : public PacketExtended
{
public:
    PacketStatueAnimation(const CClient * target, const CChar * pChar, int iAnimation, int iFrame);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1B : PacketSpellbookContent	spellbook content (NORMAL)
 *
 *
 ***************************************************************************/
class PacketSpellbookContent : public PacketExtended
{
public:
	PacketSpellbookContent(const CClient* target, const CItem* spellbook, word offset);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
	    return state->isClientVersionNumber(MINCLIVER_SPELLBOOK);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x1D : PacketHouseDesignVersion			house design version (LOW)
 *
 *
 ***************************************************************************/
class PacketHouseDesignVersion : public PacketExtended
{
public:
	PacketHouseDesignVersion(const CClient* target, const CItemMultiCustom* house);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x04 : PacketHouseBeginCustomise	begin house customisation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketHouseBeginCustomise : public PacketExtended
{
public:
	PacketHouseBeginCustomise(const CClient* target, const CItemMultiCustom* house);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_CUSTOMMULTI) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x20.0x05 : PacketHouseEndCustomise		end house customisation (NORMAL)
 *
 *
 ***************************************************************************/
class PacketHouseEndCustomise : public PacketExtended
{
public:
	PacketHouseEndCustomise(const CClient* target, const CItemMultiCustom* house);
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x22 : PacketCombatDamageOld		[old] sends notification of got damage (NORMAL)
 *
 *
 ***************************************************************************/
class PacketCombatDamageOld : public PacketExtended
{
public:
	PacketCombatDamageOld(const CClient* target, byte damage, CUID defender);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_DAMAGE);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xBF.0x26 : PacketSpeedMode		set movement speed (HIGH)
 *
 *
 ***************************************************************************/
class PacketSpeedMode : public PacketExtended
{
public:
	PacketSpeedMode(const CClient* target, byte mode);
};

/***************************************************************************
 *
 *
 *	Packet 0xC1: PacketMessageLocalised		show localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageLocalised : public PacketSend
{
public:
	PacketMessageLocalised(const CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, lpctstr args);
};

/***************************************************************************
 *
 *
 *	Packet 0xC8: PacketVisualRange			set the visual range of the client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketVisualRange : public PacketSend
{
public:
	PacketVisualRange(const CClient* target, byte range);
};

/***************************************************************************
 *
 *
 *	Packet 0xCC: PacketMessageLocalisedEx	show extended localised message to client (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMessageLocalisedEx : public PacketSend
{
public:
	PacketMessageLocalisedEx(const CClient* target, int cliloc, const CObjBaseTemplate* source, HUE_TYPE hue, TALKMODE_TYPE mode, FONT_TYPE font, AFFIX_TYPE affixType, lpctstr affix, lpctstr args);
};

/***************************************************************************
 *
 *
 *	Packet 0xD1 : PacketLogoutAck			accept logout char (LOW)
 *
 *
 ***************************************************************************/
class PacketLogoutAck : public PacketSend
{
public:
	PacketLogoutAck(const CClient* target);
};

/***************************************************************************
 *
 *
 *	Packet 0xD4 : PacketDisplayBookNew		display book (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayBookNew : public PacketSend
{
public:
	PacketDisplayBookNew(const CClient* target, CItem* book);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_NEWBOOK) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xD6 : PacketPropertyList		property (tool tip) for objects (IDLE)
 *
 *
 ***************************************************************************/
class PacketPropertyList : public PacketSend
{
protected:
	CUID m_object;
	llong m_time;
	dword m_version;
	int m_entryCount;

public:
	PacketPropertyList(const CObjBase* object, dword version, const std::vector<std::unique_ptr<CClientTooltip>> &data);
	PacketPropertyList(const CClient* target, const PacketPropertyList* other);
	virtual bool onSend(const CClient* client);

	inline CUID getObject(void) const       { return m_object; }
	inline dword getVersion(void) const     { return m_version; }
	inline int getEntryCount(void) const    { return m_entryCount; }
	inline bool isEmpty(void) const         { return m_entryCount == 0; }

	bool hasExpired(int64 iTimeout) const;

	virtual inline bool canSendTo(const CNetState* state) const  { return CanSendTo(state); }
	static inline bool CanSendTo(const CNetState* state)         { return state->isClientVersionNumber(MINCLIVER_TOOLTIP);	}
};

/***************************************************************************
 *
 *
 *	Packet 0xD8 : PacketHouseDesign			house design (IDLE)
 *
 *
 ***************************************************************************/
class PacketHouseDesign : public PacketSend
{
#define PLANEDATA_BUFFER	1024	// bytes reserved for plane data
#define STAIRSPERBLOCK		750		// number of stair items per block
#define STAIRDATA_BUFFER    (sizeof(StairData) * STAIRSPERBLOCK) // bytes reserved for stair data

private:
	struct StairData
	{
		nword m_id;
		byte m_x;
		byte m_y;
		byte m_z;
	};

	StairData* m_stairBuffer;
	int m_stairCount;

protected:
	int m_itemCount;
	int m_dataSize;
	int m_planeCount;
	int m_stairPlaneCount;
	const CItemMultiCustom* m_house;

public:
	PacketHouseDesign(const CItemMultiCustom* house, int revision);
	PacketHouseDesign(const PacketHouseDesign* other);
	virtual ~PacketHouseDesign(void) override;

	bool writePlaneData(int plane, int itemCount, byte* data, int dataSize);
	bool writeStairData(ITEMID_TYPE id, int x, int y, int z);
	void flushStairData(void);
	void finalise(void);

	virtual bool canSendTo(const CNetState* state) const override { return CanSendToClient(state); }
	static bool CanSendToClient(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_CUSTOMMULTI) || state->isClientKR() || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xDC : PacketPropertyListVersion		property (tool tip) version (LOW)
 *
 *
 ***************************************************************************/
class PacketPropertyListVersion : public PacketSend
{
protected:
	CUID m_object;

public:
	PacketPropertyListVersion(const CClient* target, const CObjBase* object, dword version);
	virtual bool onSend(const CClient* client);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_TOOLTIPHASH);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xDF : PacketBuff				add/remove buff icon (LOW)
 *
 *
 ***************************************************************************/
class PacketBuff : public PacketSend
{
public:
	PacketBuff(const CClient* target, const BUFF_ICONS iconId, const dword clilocOne, const dword clilocTwo, const word durationSeconds, lpctstr* args, uint argCount); // add buff
	PacketBuff(const CClient* target, const BUFF_ICONS iconId); // remove buff

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_BUFFS);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xE3 : PacketKREncryption		kr encryption data (HIGH)
 *
 *
 ***************************************************************************/
class PacketKREncryption : public PacketSend
{
public:
	PacketKREncryption(const CClient* target);
};

/***************************************************************************
*
*
*	Packet 0xE5 : PacketWaypointAdd			add waypoint on radar map (LOW)
*
*
***************************************************************************/
class PacketWaypointAdd : public PacketSend
{
public:
    PacketWaypointAdd(const CClient *target, CObjBase *object, MAPWAYPOINT_TYPE type);

    virtual bool canSendTo(const CNetState *state) const { return CanSendTo(state); }
    static bool CanSendTo(const CNetState *state)
    {
        return state->isClientVersionNumber(MINCLIVER_MAPWAYPOINT) || state->isClientKR() || state->isClientEnhanced();
    }
};

/***************************************************************************
*
*
*	Packet 0xE6 : PacketWaypointRemove		remove waypoint on radar map (LOW)
*
*
***************************************************************************/
class PacketWaypointRemove : public PacketSend
{
public:
    PacketWaypointRemove(const CClient *target, CObjBase *object);

    virtual bool canSendTo(const CNetState *state) const { return CanSendTo(state); }
    static bool CanSendTo(const CNetState *state)
    {
        return state->isClientVersionNumber(MINCLIVER_MAPWAYPOINT) || state->isClientKR() || state->isClientEnhanced();
    }
};

/***************************************************************************
 *
 *
 *	Packet 0xEA : PacketToggleHotbar		toggle kr hotbar (NORMAL)
 *
 *
 ***************************************************************************/
class PacketToggleHotbar : public PacketSend
{
public:
	PacketToggleHotbar(const CClient* target, bool enable);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF2 : PacketTimeSyncResponse	time sync request (NORMAL)
 *
 *
 ***************************************************************************/
class PacketTimeSyncResponse : public PacketSend
{
public:
	PacketTimeSyncResponse(const CClient* target);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_SA) || state->isClientEnhanced() || state->isClientKR();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF3 : PacketItemWorldNew		sends item on ground (NORMAL)
 *
 *
 ***************************************************************************/
class PacketItemWorldNew : public PacketItemWorld
{
protected:
	PacketItemWorldNew(byte id, uint size, const CUID& uid);

public:
	enum DataSource
	{
		TileData	= 0x0,
		Character	= 0x1,
		Multi		= 0x2,
		Damageable	= 0x3
	};

	PacketItemWorldNew(const CClient* target, const CItem* item);
	PacketItemWorldNew(const CClient* target, const CChar* mobile);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_SA) || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF5 : PacketDisplayMapNew		display map (LOW)
 *
 *
 ***************************************************************************/
class PacketDisplayMapNew : public PacketSend
{
public:
	PacketDisplayMapNew(const CClient* target, const CItemMap* map, const CRectMap& rect);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
	    return state->isClientVersionNumber(MINCLIVER_NEWMAPDISPLAY) || state->isClientEnhanced();
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF6 : PacketMoveShip			move ship (NORMAL)
 *
 *
 ***************************************************************************/
class PacketMoveShip : public PacketSend
{
public:
	PacketMoveShip(const CClient* target, const CObjBase* movingObj, CObjBase** objects, uint objectCount, byte movedirection, byte boatdirection, byte speed);
};

/***************************************************************************
 *
 *
 *	Packet 0xF7 : PacketContainer			multiple packets (NORMAL)
 *
 *
 ***************************************************************************/
class PacketContainer : public PacketSend
{
public:
	PacketContainer(const CClient* target, CObjBase** objects, uint objectCount);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_HS);
	}
};

/***************************************************************************
 *
 *
 *	Packet 0xF9 : PacketGlobalChat			global chat (LOW) (INCOMPLETE)
 *
 *
 ***************************************************************************/
class PacketGlobalChat : public PacketSend
{
public:
	enum Action
	{
		FriendToggle = 0x01,		// client <- server
		FriendRemove = 0x10,		// client -> server
		FriendAddTarg = 0x76,		// client -> server
		StatusToggle = 0x8A,		// client -> server
		Connect = 0xB9,		// client <- server
		MessageSend = 0xC6		// client -> server
	};
	enum Stanza
	{
		Presence = 0x0,
		Message = 0x1,
		InfoQuery = 0x2
	};

	PacketGlobalChat(const CClient* target, byte unknown, byte action, byte stanza, lpctstr xml);

	virtual bool canSendTo(const CNetState* state) const { return CanSendTo(state); }
	static bool CanSendTo(const CNetState* state)
	{
		return state->isClientVersionNumber(MINCLIVER_GLOBALCHAT);
	}
};

#ifdef __clang__
    #pragma GCC diagnostic pop
#endif


#endif // _INC_SEND_H
