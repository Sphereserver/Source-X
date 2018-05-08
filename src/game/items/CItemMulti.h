/**
* @file CItemMulti.h
*
*/

#ifndef _INC_CITEMMULTI_H
#define _INC_CITEMMULTI_H

#include "CItem.h"

#define MAX_MULTI_LIST_OBJS 128
#define MAX_MULTI_CONTENT 1024

enum HOUSE_TYPE
{
    HOUSE_PRIVATE,
    HOUSE_PUBLIC,
    HOUSE_GUILD
};

enum TRANSFER_TYPE
{
    TRANSFER_NOTHING    = 0x0,
    TRANSFER_LOCKDOWNS  = 0x1,  // Transfer Locked Down items
    TRANSFER_ADDONS     = 0x2,  // Transfer Addons
    TRANSFER_ALL        = 0x4   // Transfer Locked Down, Addons and normal items placed on ground (Not Components).
};

class CItemMulti : public CItem
{
	// IT_MULTI IT_SHIP
	// A ship or house etc.
private:
	static lpctstr const sm_szLoadKeys[];
	static lpctstr const sm_szVerbKeys[];

    // house permissions
    CUID _uidOwner;     // Owner's UID
    std::vector<CUID> _lCoowners;   // List of Coowners.
    std::vector<CUID> _lFriends;    // List of Friends.
    std::vector<CUID> _lLockDowns;  // List of Locked Down items.
    std::vector<CUID> _lVendors;    // List of Vendors.

    // house general
    CUID _uidMovingCrate;   // Moving Crate's UID.
    bool _fIsAddon;         // House AddOns are also multis
    HOUSE_TYPE _iHouseType; 

    // house storage
    uint16 _iBaseStorage;       // Base limit for secure storage (Max = 65535).
    uint8 _iBaseVendors;        // Base limit for player vendors (Max = 255).
    uint16 _iIncreasedStorage;  // % of increasd storage. Note: uint8 should be enough since the default max is 60%, but someone may want 300%, 1000% or even more.
    // Total Storage = _iBaseStorage + ( _iBaseStorage * _iIncreasedStorage )
    uint8 _iLockdownsPercent;   // % of Total Storage reserved for locked down items. (Default = 50%)

protected:
	CRegionWorld * m_pRegion;		// we own this region.
	bool Multi_IsPartOf( const CItem * pItem ) const;
	void MultiUnRealizeRegion();
	bool MultiRealizeRegion();
	CItem * Multi_FindItemType( IT_TYPE type ) const;
	CItem * Multi_FindItemComponent( int iComp ) const;
	const CItemBaseMulti * Multi_GetDef() const;
	bool Multi_CreateComponent( ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode, bool fIsAddon = false);

public:
	int Multi_GetMaxDist() const;
	struct ShipSpeed // speed of a ship
	{
		uchar period;	// time between movement
		uchar tiles;	// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	byte m_SpeedMode;

    // House permissions
    bool CanPlace(CChar *pChar);
    void SetOwner(CUID uidOwner);
    void AddCoowner(CUID uidCoowner);
    void DelCoowner(CUID uidCoowner);
    void AddFriend(CUID uidFriend);
    void DelFriend(CUID uidFriend);
    uint16 GetCoownerCount();
    uint16 GetFriendCount();
    bool IsOwner(CUID uidTarget);
    bool IsCoowner(CUID uidTarget);
    bool IsFriend(CUID uidTarget);

    // House general
    CItem *GenerateKey(CChar *pTarget, bool fDupeOnBank = false);
    void RemoveKeys(CChar *pTarget);
    void SetMovingCrate(CUID uidCrate);
    CUID GetMovingCrate(bool fCreate);
    void Redeed(bool fDisplayMsg = true, bool fMoveToBank = true);
    void TransferAllItemsToMovingCrate(CUID uidTargetContainer, bool fRemoveComponents = false, TRANSFER_TYPE iType = TRANSFER_NOTHING);
    void TransferLockdownsToMovingCrate();
    void SetAddon(bool fIsAddon);
    bool IsAddon();

    // House storage
    void SetBaseStorage(uint16 iLimit);
    uint16 GetBaseStorage();
    void SetBaseVendors(uint8 iLimit);
    uint8 GetBaseVendors();
    uint8 GetMaxVendors();
    uint8 GetCurrentVendors();
    void SetIncreasedStorage(uint16 iIncrease);
    uint16 GetIncreasedStorage();
    uint16 GetMaxStorage();
    uint16 GetCurrentStorage();
    uint16 GetMaxLockdowns();
    uint8 GetLockdownsPercent();
    void SetLockdownsPercent(uint8 iPercent);
    uint16 GetCurrentLockdowns();
    void LockItem(CUID uidItem, bool fUpdateFlags);
    void UnlockItem(CUID uidItem, bool fUpdateFlags);
    bool IsLockedItem(CUID uidItem);
    void AddVendor(CUID uidVendor);
    void DelVendor(CUID uidVendor);
    bool IsHouseVendor(CUID uidVendor);

protected:
	virtual void OnComponentCreate(const CItem * pComponent, bool fIsAddon = false);


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

	void Multi_Create(CChar * pChar, dword dwKeyCode);
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void  r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool  r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};


#endif // _INC_CITEMMULTI_H
