/**
* @file CItemMulti.h
*
*/

#ifndef _INC_CITEMMULTI_H
#define _INC_CITEMMULTI_H

#include "CItem.h"

#define MAX_MULTI_LIST_OBJS 128
#define MAX_MULTI_CONTENT 1024


class CItemMulti : public CItem
{
	// IT_MULTI IT_SHIP
	// A ship or house etc.
private:
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];
    CUID _uidOwner;     // Owner's UID
    std::vector<CUID> _lCoowners;   // List of Coowners' UID.
    std::vector<CUID> _lFriends;    // List of Friends' UID.
    CUID _uidMovingCrate;   // Moving Crate's UID.

protected:
	CRegionWorld * m_pRegion;		// we own this region.
	bool Multi_IsPartOf( const CItem * pItem ) const;
	void MultiUnRealizeRegion();
	bool MultiRealizeRegion();
	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;
	const CItemBaseMulti * Multi_GetDef() const;
	bool Multi_CreateComponent( ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode );

public:
	int Multi_GetMaxDist() const;
	struct ShipSpeed // speed of a ship
	{
		uchar period;	// time between movement
		uchar tiles;	// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	byte m_SpeedMode;

    bool CanPlace(CChar *pChar);

    void SetOwner(CUID uidOwner);
    void AddCoowner(CUID uidCoowner);
    void DelCoowner(CUID uidCoowner);
    void AddFriend(CUID uidFriend);
    void DelFriend(CUID uidFriend);
    int GetCoownerCount();
    int GetFriendCount();
    bool IsOwner(CUID uidTarget);
    bool IsCoowner(CUID uidTarget);
    bool IsFriend(CUID uidTarget);
    CItem *GenerateKey(CChar *pTarget, bool fDupeOnBank = false);
    void RemoveKeys(CChar *pTarget);
    void SetMovingCrate(CUID uidCrate);
    void Redeed(bool fDisplayMsg = true, bool fMoveToBank = true);
    void TransferAllItemsToMovingCrate(CUID uidTargetContainer);

protected:
	virtual void OnComponentCreate( const CItem * pComponent );


public:
	static const char *m_sClassName;
	CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMulti();

private:
	CItemMulti(const CItemMulti& copy);
	CItemMulti& operator=(const CItemMulti& other);

public:
	virtual bool OnTick();
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false); // Put item on the ground here.
	virtual void OnMoveFrom();	// Moving from current location.
	void OnHearRegion( lpctstr pszCmd, CChar * pSrc );
	CItem * Multi_GetSign();	// or Tiller

	void Multi_Create( CChar * pChar, dword dwKeyCode );
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};


#endif // _INC_CITEMMULTI_H
