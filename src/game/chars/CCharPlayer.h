/**
* @file CCharPlayer.h
*
*/

#ifndef _INC_CCHARPLAYER_H
#define _INC_CCHARPLAYER_H

#include "../../common/resource/CResourceBase.h"
#include "../../common/sphereproto.h"
#include "../clients/CAccount.h"
#include "../CServerConfig.h"

class CSkillClassDef;


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
	bool m_bKrToolbarEnabled;

public:
	static const char *m_sClassName;
	CAccount * m_pAccount;		// The account index. (for idle players mostly)
	static lpctstr const sm_szVerbKeys[];

	int64 m_timeLastUsed;	// Time the player char was last used.

	CSString m_sProfile;	// limited to SCRIPT_MAX_LINE_LEN-16
    HUE_TYPE m_SpeechHue;	// speech hue used (sent by client)
    CUID m_uidWeaponLast;   // last equipped weapon (only used by 'EquipLastWeapon' client macro)

	word m_wMurders;		// Murder count.
	word m_wDeaths;			// How many times have i died ?
	byte m_speedMode;		// speed mode (0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement)
	dword m_pflag;			// PFLAG

	static lpctstr const sm_szLoadKeys[];

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

public:
	SKILL_TYPE Skill_GetLockType( lpctstr pszKey ) const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	void Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state );

	STAT_TYPE Stat_GetLockType( lpctstr pszKey ) const;
	SKILLLOCK_TYPE Stat_GetLock( STAT_TYPE stat ) const;
	void Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state );

	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, lpctstr pszKey, CSString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool SetSkillClass( CChar * pChar, CResourceID rid );
	CSkillClassDef * GetSkillClass() const;

	bool getKrToolbarStatus();

	CAccount * GetAccount() const;

public:
	CCharPlayer( CChar * pChar, CAccount * pAccount );
	~CCharPlayer();

private:
	CCharPlayer(const CCharPlayer& copy);
	CCharPlayer& operator=(const CCharPlayer& other);
};


#endif // _INC_CCHARPLAYER_H
