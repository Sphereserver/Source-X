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

class CChar;
class CItemStone;
class CItemContainer;
class CItemShip;
class CItemMulti : public CItem
{
	// IT_MULTI IT_SHIP
	// A ship or house etc.
private:
	static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];
    static lpctstr const sm_szRefKeys[];

    // house permissions
    CChar* _pOwner;     // Owner's UID
    CItemStone *_pGuild; // Linked Guild.
    std::vector<CChar*> _lCoowners;   // List of Coowners.
    std::vector<CChar*> _lFriends;    // List of Friends.
    std::vector<CChar*> _lVendors;    // List of Vendors.
    std::vector<CChar*> _lBans;        // List of Banned chars.
protected:
    std::vector<CItem*> _lLockDowns;  // List of Locked Down items.
private:
    std::vector<CItem*> _lComponents; // List of Components.

    // house general
    CItemContainer* _pMovingCrate;   // Moving Crate's UID.
    bool _fIsAddon;         // House AddOns are also multis
    HOUSE_TYPE _iHouseType; 
    uint8 _iMultiCount;         // Does this counts towars the char's house limit

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
    static CItemMulti *Multi_Create(CChar *pChar, const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed);
    // Owner
    void SetOwner(CChar* pOwner);
    bool IsOwner(CChar* uidTarget);
    CChar *GetOwner();
    // Guild
    void SetGuild(CItemStone* pGuild);
    bool IsGuild(CItemStone* pGuild);
    CItemStone *GetGuild();
    // Coowner
    void AddCoowner(CChar* uidCoowner);
    void DelCoowner(CChar* uidCoowner);
    size_t GetCoownerCount();
    int GetCoownerPos(CChar* uidTarget);
    // Friend
    void AddFriend(CChar* uidFriend);
    void DelFriend(CChar* uidFriend);
    size_t GetFriendCount();
    int GetFriendPos(CChar* uidTarget);
    // Ban
    void AddBan(CChar* uidBan);
    void DelBan(CChar* uidBan);
    size_t GetBanCount();
    int GetBanPos(CChar* uidBan);

    // House general:
    // Keys:
    CItem *GenerateKey(CChar *pTarget, bool fDupeOnBank = false);
    void RemoveKeys(CChar *pTarget);
    int16 GetMultiCount();
    // Redeed
    void Redeed(bool fDisplayMsg = true, bool fMoveToBank = true);
    //Moving Crate
    void SetMovingCrate(CItemContainer* uidCrate);
    CItemContainer* GetMovingCrate(bool fCreate);
    void TransferAllItemsToMovingCrate(TRANSFER_TYPE iType = TRANSFER_ALL);
    void TransferLockdownsToMovingCrate();
    void TransferMovingCrateToBank();
    // AddOn
    void SetAddon(bool fIsAddon);
    bool IsAddon();
    // Components
    void AddComponent(CItem* uidComponent);
    void DelComponent(CItem* uidComponent);
    int GetComponentPos(CItem* uidComponent);
    size_t GetComponentCount();
    void RemoveAllComponents();
    void GenerateBaseComponents(bool &fNeedKey, dword &dwKeyCode);

    // House storage:
    // - Limits & values
    // -- (Limits & values) Base Storage
    void SetBaseStorage(uint16 iLimit);
    uint16 GetBaseStorage();
    void SetIncreasedStorage(uint16 iIncrease);
    uint16 GetIncreasedStorage();
    uint16 GetMaxStorage();
    uint16 GetCurrentStorage();

    // -- (Limits & values)Vendors
    void SetBaseVendors(uint8 iLimit);
    uint8 GetBaseVendors();
    uint8 GetMaxVendors();
    // -- (Limits & values)LockDowns
    uint16 GetMaxLockdowns();
    uint8 GetLockdownsPercent();
    void SetLockdownsPercent(uint8 iPercent);
    // -Lockdowns
    void LockItem(CItem* uidItem, bool fUpdateFlags);
    void UnlockItem(CItem* uidItem, bool fUpdateFlags);
    int GetLockedItemPos(CItem* uidItem);
    size_t GetLockdownCount();
    //  -Vendors
    void AddVendor(CChar* uidVendor);
    void DelVendor(CChar* uidVendor);
    int GetHouseVendorPos(CChar* uidVendor);
    size_t GetVendorCount();

protected:
	virtual void OnComponentCreate(CItem * pComponent, bool fIsAddon = false);


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

	void Multi_Setup(CChar * pChar, dword dwKeyCode);
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};

/*
* Class related to storage/access the multis owned by the holder entity.
* This class adds multis to it's lists only checking for UID validity, nothing more,
* further checks must be added on multi placement, etc
*/
class CMultiStorage
{
private:
    std::vector<CItemMulti*> _lHouses; // List of stored houses.
    std::vector<CItemShip*> _lShips;  // List of stored ships.
    int16 _iHousesTotal;        // 
    int16 _iShipsTotal;

public:
    CMultiStorage();
    ~CMultiStorage();

    void AddMulti(CItemMulti *pMulti);
    void DelMulti(CItemMulti *pMulti);

    void AddHouse(CItemMulti *pHouse);
    void DelHouse(CItemMulti *pHouse);
    bool CanAddHouse(CChar *pChar, int16 iHouseCount);
    bool CanAddHouse(CItemStone *pStone, int16 iHouseCount);
    int16 GetHousePos(CItemMulti *pHouse);
    int16 GetHouseCountTotal();
    int16 GetHouseCountReal();
    CItemMulti *GetHouseAt(int16 iPos);
    void ClearHouses();

    void AddShip(CItemShip *pShip);
    void DelShip(CItemShip *pShip);
    bool CanAddShip(CChar *pChar, int16 iShipCount);
    bool CanAddShip(CItemStone *pStone, int16 iShipCount);
    int16 GetShipPos(CItemShip *pShip);
    int16 GetShipCountTotal();
    int16 GetShipCountReal();
    CItemShip *GetShipAt(int16 iPos);
    void ClearShips();


    virtual void r_Write(CScript & s);
};
#endif // _INC_CITEMMULTI_H
