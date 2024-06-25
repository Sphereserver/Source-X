/**
* @file CCharPlayer.h
*
*/

#ifndef _INC_CCHARPLAYER_H
#define _INC_CCHARPLAYER_H

#include "../../common/resource/CResourceHolder.h"
#include "../../common/sphereproto.h"
#include "../clients/CAccount.h"
#include "../CServerConfig.h"

class CSkillClassDef;
class CMultiStorage;


enum CPC_TYPE	// Player char.
{
#define ADD(a,b) CPC_##a,
#include "../../tables/CCharPlayer_props.tbl"
#undef ADD
	CPC_QTY
};

struct CCharPlayer
{
	// Stuff that is specific to a player character.
private:
	byte m_SkillLock[SKILL_QTY];	// SKILLLOCK_TYPE List of skill lock states for this player character
	byte m_StatLock[STAT_BASE_QTY]; // SKILLLOCK_TYPE Applied to stats
	CResourceRef m_SkillClass;		// RES_SKILLCLASS CSkillClassDef What skill class group have we selected.
	bool m_fKrToolbarEnabled;

	// Multis
	CMultiStorage* _pMultiStorage;	// List of houses.

public:
	static const char *m_sClassName;
	CAccount * m_pAccount;		// The account index. (for idle players mostly)

	int64 _iTimeLastUsedMs;			// Time the player char was last used (connected).
	int64 _iTimeLastDisconnectedMs;

	CSString m_sProfile;	// limited to SCRIPT_MAX_LINE_LEN-16
    HUE_TYPE m_SpeechHue;	// speech hue used (sent by client)
    HUE_TYPE m_EmoteHue;	// emote hue used (sent by client)
    CUID m_uidWeaponLast;   // last equipped weapon (only used by 'EquipLastWeapon' client macro)

	word m_wMurders;		// Murder count.
	word m_wDeaths;			// How many times have i died ?
	byte m_speedMode;		// speed mode (0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement)
	dword m_pflag;			// PFLAG

	// Client's local light (might be useful in the future for NPCs also? keep it here for now)
	byte m_LocalLight;

	// Multis
	uint8 _iMaxHouses;              // Max houses this player (Client?) can have (Overriding CAccount::_iMaxHouses)
	uint8 _iMaxShips;               // Max ships this player (Client?) can have (Overriding CAccount::_iMaxShips)
	bool m_fRefuseGlobalChatRequests;

	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szLoadKeys[];

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

public:
	CAccount* GetAccount() const;

	SKILL_TYPE Skill_GetLockType( lpctstr ptcKey ) const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	void Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state );

	STAT_TYPE Stat_GetLockType( lpctstr ptcKey ) const;
	SKILLLOCK_TYPE Stat_GetLock( STAT_TYPE stat ) const;
	void Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state );

	bool getKrToolbarStatus() const noexcept;
	CMultiStorage* GetMultiStorage();

	bool SetSkillClass(CChar* pChar, CResourceID rid);
	CSkillClassDef* GetSkillClass() const;

	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, lpctstr ptcKey, CSString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );
	

public:
	CCharPlayer( CChar * pChar, CAccount * pAccount );
	~CCharPlayer();

private:
	CCharPlayer(const CCharPlayer& copy);
	CCharPlayer& operator=(const CCharPlayer& other);
};


#endif // _INC_CCHARPLAYER_H
