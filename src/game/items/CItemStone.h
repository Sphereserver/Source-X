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
	CGrayUID m_uidLinkTo;			// My char uid or enemy stone UID

									// Only apply to members.
	CGrayUID m_uidLoyalTo;	// Who am i loyal to? invalid value = myself.
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
	CStoneMember( CItemStone * pStone, CGrayUID uid, STONEPRIV_TYPE iType, LPCTSTR pTitle = "", CGrayUID loyaluidLink = 0, bool fArg1 = false, bool fArg2 = false, int nAccountGold = 0);
	virtual ~CStoneMember();

private:
	CStoneMember(const CStoneMember& copy);
	CStoneMember& operator=(const CStoneMember& other);

public:
	CStoneMember* GetNext() const;
	CItemStone * GetParentStone() const;

	CGrayUID GetLinkUID() const;

	STONEPRIV_TYPE GetPriv() const;
	void SetPriv(STONEPRIV_TYPE iPriv);
	bool IsPrivMaster() const;
	bool IsPrivMember() const;
	LPCTSTR GetPrivName() const;

	// If the member is really a war flag (STONEPRIV_ENEMY)
	void SetWeDeclared(bool f);
	bool GetWeDeclared() const;
	void SetTheyDeclared(bool f);
	bool GetTheyDeclared() const;

	// Member
	bool IsAbbrevOn() const;
	void ToggleAbbrev();
	void SetAbbrev(bool mode);

	LPCTSTR GetTitle() const;
	void SetTitle( LPCTSTR pTitle );
	CGrayUID GetLoyalToUID() const;
	bool SetLoyalTo( const CChar * pChar);
	int GetAccountGold() const;
	void SetAccountGold( int iGold );
	// ---------------------------------

	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szVerbKeys[];

	LPCTSTR GetName() const { return m_sClassName; }
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_WriteVal( LPCTSTR pKey, CGString & sVal, CTextConsole * pSrc );
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
	static LPCTSTR const sm_szVerbKeys[];
	static LPCTSTR const sm_szLoadKeys[];
	static LPCTSTR const sm_szLoadKeysM[];
	static LPCTSTR const sm_szLoadKeysG[];
private:
	CGString m_sCharter[6];
	CGString m_sWebPageURL;
	CGString m_sAbbrev;

private:

	void SetTownName();
	bool SetName( LPCTSTR pszName );
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false);

	MEMORY_TYPE GetMemoryType() const;

	LPCTSTR GetCharter(unsigned int iLine) const;
	void SetCharter( unsigned int iLine, LPCTSTR pCharter );
	LPCTSTR GetWebPageURL() const;
	void SetWebPage( LPCTSTR pWebPage );
	void ElectMaster();
public:
	static const char *m_sClassName;
	CStoneMember * AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull = false);

	// War
private:
	void TheyDeclarePeace( CItemStone* pEnemyStone, bool fForcePeace );
	bool WeDeclareWar(CItemStone * pEnemyStone);
	void WeDeclarePeace(CGrayUID uidEnemy, bool fForcePeace = false);
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
	virtual bool r_WriteVal( LPCTSTR pszKey, CGString & s, CTextConsole * pSrc );
	virtual bool r_GetRef( LPCTSTR & pszKey, CScriptObj * & pRef );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	LPCTSTR GetTypeName() const;
	static bool IsUniqueName( LPCTSTR pName );
	CChar * GetMaster() const;
	bool NoMembers() const;
	CStoneMember * GetMasterMember() const;
	CStoneMember * GetMember( const CObjBase * pObj) const;
	bool IsPrivMember( const CChar * pChar ) const;

	// Simple accessors.
	STONEALIGN_TYPE GetAlignType() const;
	void SetAlignType(STONEALIGN_TYPE iAlign);
	LPCTSTR GetAlignName() const;
	LPCTSTR GetAbbrev() const;
	void SetAbbrev( LPCTSTR pAbbrev );
};

#endif // _INC_CITEMSTONE_H
