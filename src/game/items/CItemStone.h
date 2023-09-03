/**
* @file CItemStone.h
*
*/

#ifndef _INC_CITEMSTONE_H
#define _INC_CITEMSTONE_H

#include "../chars/CStoneMember.h"
#include "CItem.h"


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

class CMultiStorage;

class CItemStone : public CItem, public CSObjList
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
	CSString m_sCharter[6];
	CSString m_sWebPageURL;
	CSString m_sAbbrev;
    CMultiStorage *_pMultiStorage;
    int16 _iMaxShips;
    int16 _iMaxHouses;
private:

	void SetTownName();
	virtual bool SetName( lpctstr pszName ) override;
	virtual bool MoveTo(const CPointMap& pt, bool bForceFix = false) override;

	MEMORY_TYPE GetMemoryType() const;

	lpctstr GetCharter(uint iLine) const;
	void SetCharter( uint iLine, lpctstr pCharter );
	lpctstr GetWebPageURL() const;
	void SetWebPageURL( lpctstr pWebPageURL );
	void ElectMaster();
public:
	static const char *m_sClassName;
	CStoneMember * AddRecruit(const CChar * pChar, STONEPRIV_TYPE iPriv, bool bFull = false);
    CMultiStorage *GetMultiStorage();

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
	virtual int FixWeirdness() override;

public:
	CItemStone( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemStone();

private:
	CItemStone(const CItemStone& copy);
	CItemStone& operator=(const CItemStone& other);

public:
	virtual void r_Write( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & s, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override; // Execute command from script

	lpctstr GetTypeName() const;
	static bool IsUniqueName( lpctstr pName );
	CChar * GetMaster() const;
	bool NoMembers() const;
	CStoneMember * GetMasterMember() const;
	CStoneMember * GetMember( const CObjBase * pObj) const;
	bool IsPrivMember( const CChar * pChar ) const;

	// Simple accessors.
	STONEALIGN_TYPE GetAlignType() const;
	void SetALIGNTYPE(STONEALIGN_TYPE iAlign);
	lpctstr GetAlignName() const;
	lpctstr GetAbbrev() const;
	void SetAbbrev( lpctstr pAbbrev );
};


#endif // _INC_CITEMSTONE_H
