/**
* @file CStoneMember.h
*
*/

#ifndef _INC_CSTONEMEMBER_H
#define _INC_CSTONEMEMBER_H

#include "../../common/sphere_library/CSObjListRec.h"
#include "../../common/CScriptObj.h"
#include "../../common/CUID.h"

class CItemStone;


enum STONEPRIV_TYPE // Priv level for this char
{
	STONEPRIV_CANDIDATE = 0,
	STONEPRIV_MEMBER,
	STONEPRIV_MASTER,
	STONEPRIV_UNUSED,
	STONEPRIV_ACCEPTED,		// The candidate has been accepted. But they have not dclicked on the stone yet.
	STONEPRIV_ENEMY = 100,	// This is an enemy town/guild.
	STONEPRIV_ALLY			// This is an ally town/guild.
};

class CStoneMember : public CSObjListRec, public CScriptObj	// Members for various stones, and links to stones at war with
{
	// NOTE: Chars are linked to the CItemStone via a memory object.
	friend class CItemStone;
private:
	STONEPRIV_TYPE m_iPriv;	// What is my status level in the guild ?
	CUID m_uidLinkTo;			// My char uid or enemy stone UID

								// Only apply to members.
	CUID m_UIDLoyalTo;	// Who am i loyal to? invalid value = myself.
	CSString m_sTitle;		// What is my title in the guild?

	union	// Depends on m_iPriv
	{
		struct	// Unknown type.
		{
			int m_Val1;
			int m_Val2;
			int m_Val3;
		} m_UnDef;

		struct // STONEPRIV_ALLY
		{
			int m_fTheyDeclared;
			int m_fWeDeclared;
		} m_Ally;

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
	CStoneMember(CItemStone* pStone, CUID uid, STONEPRIV_TYPE iType, lpctstr pTitle = "", CUID loyaluidLink = CUID(UID_CLEAR), bool fArg1 = false, bool fArg2 = false, int nAccountGold = 0);
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

	// If the member is really a ally flag (STONEPRIV_ALLY)
	void SetWeDeclaredAlly(bool f);
	bool GetWeDeclaredAlly() const;
	void SetTheyDeclaredAlly(bool f);
	bool GetTheyDeclaredAlly() const;

	// If the member is really a war flag (STONEPRIV_ENEMY)
	void SetWeDeclaredWar(bool f);
	bool GetWeDeclaredWar() const;
	void SetTheyDeclaredWar(bool f);
	bool GetTheyDeclaredWar() const;

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
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_WriteVal( lpctstr pKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute command from script
	virtual bool r_LoadVal( CScript & s ) override;
};


#endif // _INC_CSTONEMEMBER_H
