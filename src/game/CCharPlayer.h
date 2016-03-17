#ifndef _INC_CCHARPLAYER_H
#define _INC_CCHARPLAYER_H

#include "../common/graymul.h"
#include "../common/CResourceBase.h"
#include "../common/grayproto.h"
#include "CResource.h"
#include "CServTime.h"
#include "CAccount.h"


enum CPC_TYPE	// Player char.
{
#define ADD(a,b) CPC_##a,
#include "../tables/CCharPlayer_props.tbl"
#undef ADD
	CPC_QTY
};

struct CCharPlayer
{
	// Stuff that is specific to a player character.
private:
	BYTE m_SkillLock[SKILL_QTY];	// SKILLLOCK_TYPE List of skill lock states for this player character
	BYTE m_StatLock[STAT_BASE_QTY]; // SKILLLOCK_TYPE Applied to stats
	CResourceRef m_SkillClass;	// RES_SKILLCLASS CSkillClassDef What skill class group have we selected.
	bool m_bKrToolbarEnabled;

public:
	static const char *m_sClassName;
	CAccount * m_pAccount;	// The account index. (for idle players mostly)
	static LPCTSTR const sm_szVerbKeys[];

	CServTime m_timeLastUsed;	// Time the player char was last used.

	CGString m_sProfile;	// limited to SCRIPT_MAX_LINE_LEN-16

	WORD m_wMurders;		// Murder count.
	WORD m_wDeaths;			// How many times have i died ?
	BYTE m_speedMode;		// speed mode (0x0 = Normal movement, 0x1 = Fast movement, 0x2 = Slow movement, 0x3 and above = Hybrid movement)
	DWORD m_pflag;			// PFLAG

	static LPCTSTR const sm_szLoadKeys[];

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)

public:
	SKILL_TYPE Skill_GetLockType( LPCTSTR pszKey ) const;
	SKILLLOCK_TYPE Skill_GetLock( SKILL_TYPE skill ) const;
	void Skill_SetLock( SKILL_TYPE skill, SKILLLOCK_TYPE state );

	STAT_TYPE Stat_GetLockType( LPCTSTR pszKey ) const;
	SKILLLOCK_TYPE Stat_GetLock( STAT_TYPE stat ) const;
	void Stat_SetLock( STAT_TYPE stat, SKILLLOCK_TYPE state );

	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool SetSkillClass( CChar * pChar, RESOURCE_ID rid );
	CSkillClassDef * GetSkillClass() const;

	bool getKrToolbarStatus();

	CAccountRef GetAccount() const;

public:
	CCharPlayer( CChar * pChar, CAccount * pAccount );
	~CCharPlayer();

private:
	CCharPlayer(const CCharPlayer& copy);
	CCharPlayer& operator=(const CCharPlayer& other);
};


#endif //_INC_CCHARPLAYER_H
