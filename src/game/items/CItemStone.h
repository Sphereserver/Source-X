
#pragma once
#ifndef _INC_CITEMSTONE_H
#define _INC_CITEMSTONE_H

#include "CItem.h"


enum STONEPRIV_TYPE // Priv level for this char
{
	STONEPRIV_CANDIDATE = 0,
	STONEPRIV_MEMBER,
	STONEPRIV_MASTER,
	STONEPRIV_UNUSED,
	STONEPRIV_ACCEPTED,	// The candidate has been accepted. But they have not dclicked on the stone yet.
	STONEPRIV_ENEMY = 100	// this is an enemy town/guild.
};

class CStoneMember : public CGObListRec, public CScriptObj	// Members for various stones, and links to stones at war with
{
	// NOTE: Chars are linked to the CItemStone via a memory object.
	friend class CItemStone;
private:
	STONEPRIV_TYPE m_iPriv;	// What is my status level in the guild ?
	CUID m_uidLinkTo;			// My char uid or enemy stone UID

									// Only apply to members.
	CUID m_uidLoyalTo;	// Who am i loyal to? invalid value = myself.
	CGString m_sTitle;		// What is my title in the guild?

	union	// Depends on m_iPriv
	{
		struct	// Unknown type.
		{
			int m_Val1;
			int m_Val2;
			int m_Val3;
		} m_UnDef;

		struct // STONEPRIV_ENEMY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Enemy;

		struct	// a char member (NOT STONEPRIV_ENEMY)
		{
			int m_fAbbrev;			// Do they have their guild abbrev on or not ?
			int m_iVoteTally;		// Temporary space to calculate votes for me.
			int m_iAccountGold;		// how much i still owe to the guild or have surplus (Normally negative).
		} m_Member;
	};

public:
	static const char *m_sClassName;
	CStoneMember( CItemStone * pStone, CUID uid, STONEPRIV_TYPE iType, lpctstr pTitle = "", CUID loyaluidLink = 0, bool fArg1 = false, bool fArg2 = false, int nAccountGold = 0);
	virtual ~CStoneMember();

private:
	CStoneMember(const CStoneMember& copy);
	CStoneMember& operator=(const CStoneMember& other);

public:
	CStoneMember* GetNext() const;
	CItemStone * GetParentStone() const;

	CUID GetLinkUID() const;

	STONEPRIV_TYPE GetPriv() const;
	void SetPriv(STONEPRIV_TYPE iPriv);
	bool IsPrivMaster() const;
	bool IsPrivMember() const;
	lpctstr GetPrivName() const;

	// If the member is really a war flag (STONEPRIV_ENEMY)
	void SetWeDeclared(bool f);
	bool GetWeDeclared() const;
	void SetTheyDeclared(bool f);
	bool GetTheyDeclared() const;

	// Member
	bool IsAbbrevOn() const;
	void ToggleAbbrev();
	void SetAbbrev(bool mode);

	lpctstr GetTitle() const;
	void SetTitle( lpctstr pTitle );
	CUID GetLoyalToUID() const;
	bool SetLoyalTo( const CChar * pChar);
	int GetAccountGold() const;
	void SetAccountGold( int iGold );
	// ---------------------------------

	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

	lpctstr GetName() const { return m_sClassName; }
	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( lpctstr pKey, CGString & sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
	virtual bool r_LoadVal( CScript & s );
};

enum STONEDISP_TYPE	// Hard coded Menus
{
	STONEDISP_NONE = 0,
	STONEDISP_ROSTER,
	STONEDISP_CANDIDATES,
	STONEDISP_FEALTY,
	STONEDISP_ACCEPTCANDIDATE,
	STONEDISP_REFUSECANDIDATE,
	STONEDISP_DISMISSMEMBER,
	STONEDISP_VIEWCHARTER,
	STONEDISP_SETCHARTER,
	STONEDISP_VIEWENEMYS,
	STONEDISP_VIEWTHREATS,
	STONEDISP_DECLAREWAR,
	STONEDISP_DECLAREPEACE,
	STONEDISP_GRANTTITLE,
	STONEDISP_VIEWBANISHED,
	STONEDISP_BANISHMEMBER
};

class CItemStone : public CItem, public CGObList
{
	// IT_STONE_GUILD
	// IT_STONE_TOWN
	// ATTR_OWNED = auto promote to member.

	friend class CStoneMember;
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szLoadKeysM[];
	static lpctstr const sm_szLoadKeysG[];
private:
	CGString m_sCharter[6];
	CGString m_sWebPageURL;
	CGString m_sAbbrev;

private:

	void SetTownName();
	bool SetName( lpctstr pszName );
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false);

	MEMORY_TYPE GetMemoryType() const;

	lpctstr GetCharter(uint iLine) const;
	void SetCharter( uint iLine, lpctstr pCharter );
	lpctstr GetWebPageURL() const;
	void SetWebPage( lpctstr pWebPage );
	void ElectMaster();
public:
	static const char *m_sClassName;
	CStoneMember * AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull = false);

	// War
private:
	void TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace );
	bool WeDeclareWar(CItemStone * pEnemyStone);
	void WeDeclarePeace(CUID uidEnemy, bool fForcePeace = false);
	void AnnounceWar( const CItemStone * pEnemyStone, bool fWeDeclare, bool fWar );
public:
	bool IsAtWarWith( const CItemStone * pStone ) const;
	bool IsAlliedWith( const CItemStone * pStone ) const;

	bool CheckValidMember(CStoneMember * pMember);
	int FixWeirdness();

public:
	CItemStone( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemStone();

private:
	CItemStone(const CItemStone& copy);
	CItemStone& operator=(const CItemStone& other);

public:
	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	lpctstr GetTypeName() const;
	static bool IsUniqueName( lpctstr pName );
	CChar * GetMaster() const;
	bool NoMembers() const;
	CStoneMember * GetMasterMember() const;
	CStoneMember * GetMember( const CObjBase * pObj) const;
	bool IsPrivMember( const CChar * pChar ) const;

	// Simple accessors.
	STONEALIGN_TYPE GetAlignType() const;
	void SetAlignType(STONEALIGN_TYPE iAlign);
	lpctstr GetAlignName() const;
	lpctstr GetAbbrev() const;
	void SetAbbrev( lpctstr pAbbrev );
};

#endif // _INC_CITEMSTONE_H
