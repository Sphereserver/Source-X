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
    CChar* _pOwner;     // Owner's character
    CItemStone *_pGuild; // Linked Guild.
    std::vector<CChar*> _lCoowners;   // List of Coowners.
    std::vector<CChar*> _lFriends;    // List of Friends.
    std::vector<CChar*> _lVendors;    // List of Vendors.
    std::vector<CChar*> _lBans;        // List of Banned chars.
	std::vector<CChar*> _lAccesses;    // List of Accesses chars.
protected:
    std::vector<CItem*> _lLockDowns;  // List of Locked Down items.
    std::vector<CItemContainer*> _lSecureContainers; // List of Secured containers.
private:
    std::vector<CItem*> _lComps; // List of Components.
    std::vector<CItemMulti*> _lAddons;  // List of AddOns.

    // house general
    CItemContainer* _pMovingCrate;   // Moving Crate's container.
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
    /**
    * @brief Checks if the given item is part of this multi.
    * @param pItem the item to check
    * @return true if the pItem is part of this multi.
    */
	bool Multi_IsPartOf( const CItem * pItem ) const;
    /**
    * @brief Removes the region created by this multi and updates inside char's region
    */
	void MultiUnRealizeRegion();
    /**
    * @brief Creates a new region for this multi and update the world with it.
    */
	bool MultiRealizeRegion();
    /**
    * @brief Searchs for any item of the given type.
    * @param type The type of item to search.
    * @return the CItem (if any).
    */
	CItem * Multi_FindItemType( IT_TYPE type ) const;
    /**
    * @brief Searchs for any item with the given id.
    * @param iComp The id of the item to search.
    * @return the CItem (if any).
    */
	CItem * Multi_FindItemComponent( int iComp ) const;
    /**
    * @brief Searchs for the CItemBase of this multi.
    * @return the CItemBaseMulti.
    */
	const CItemBaseMulti * Multi_GetDef() const;
    /**
    * @brief Components creation.
    * @param id The id of the item
    * @param dx p.x
    * @param dy p.y
    * @param dz p.z
    * @param dwKeyCode the lock hash to apply on the item (if is lockable).
    * @param fIsAddon true when the created component is an addon.
    * @return true if the component need a key (wether the key is created or not is not checked here).
    */
	bool Multi_CreateComponent( ITEMID_TYPE id, short dx, short dy, char dz, dword dwKeyCode);

public:
    /**
    * @brief Calculates the largest part of the multi.
    * @return the value.
    */
	int Multi_GetMaxDist() const;

	struct ShipSpeed // speed of a ship
	{
		uchar period;	// time between movement
		uchar tiles;	// distance to move
	};
	ShipSpeed m_shipSpeed; // Speed of ships (IT_SHIP)
	byte m_SpeedMode;

    /**
    * @brief Checks if a the multi can be created and create it if so.
    *
    * Checks about the player placing it, the terrain, and any possible obstacles are done here (More info in the definition).
    * @param pChar the char placing the Multi.
    * @param pItemDef the CItemBaseMulti of this multi.
    * @param pt The position on the world.
    * @param pDeed The deed used to create this multi.
    * @return the CItemMulti if the creation succeeded.
    */
    static CItemMulti *Multi_Create(CChar *pChar, const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed);

    /** House Permissions
    */
    ///@{

    /**
    * @brief Revokes any privileges from this house.
    * @param pSrc the object whom privileges to revoke.
    */
    void RevokePrivs(CChar *pSrc);
    //Owner
    /**
    * @brief Set's the owner.
    * @param pOwner the new owner.
    */
    void SetOwner(CChar* pOwner);
    /**
    * @brief Checks if the given CChar in the param is the owner.
    * @param pTarget the char to check.
    * @return true if true.
    */
    bool IsOwner(CChar* pTarget);
    /**
    * @brief Returns the owner.
    * @return the Owner's CChar.
    */
    CChar *GetOwner();

    // Guilds
    /**
    * @brief Links this multi to a guild.
    * @param pGuild the guild on which to link.
    */
    void SetGuild(CItemStone* pGuild);
    /**
    * @brief Checks if the given guild is the one linked to this multi.
    * @param pGuild the guild to check on.
    * @return true if match.
    */
    bool IsGuild(CItemStone* pGuild);
    /**
    * @brief Returns the guild linked.
    * @return the guild
    */
    CItemStone *GetGuild();

    // Coowner
    /**
    * @brief Adds a coowner to the _lCoowners list.
    * @param pCoowner the coowner
    */
    void AddCoowner(CChar* pCoowner);
    /**
    * @brief Deletes a coowner to the _lCoowners list.
    * @param pCoowner the coowner
    */
    void DelCoowner(CChar* pCoowner);
    /**
    * @brief Returns the total count of coowners on the list.
    * @return the count.
    */
    size_t GetCoownerCount();
    /**
    * @brief Get's the position on the _lCoowners list of the given char.
    * @param pTarget the coowner.
    * @return the position.
    */
    int GetCoownerPos(CChar* pTarget);

    // Friend
    /**
    * @brief Adds a friend to the _lFriends list.
    * @param pFriend the friend
    */
    void AddFriend(CChar* pFriend);
    /**
    * @brief Deletes a friend to the _lFriends list.
    * @param pFriend the friend
    */
    void DelFriend(CChar* pFriend);
    /**
    * @brief Returns the total count of friends on the list.
    * @return the count.
    */
    size_t GetFriendCount();
    /**
    * @brief Get's the position on the _lFriends list of the given char.
    * @param pTarget the friend.
    * @return the position.
    */
    int GetFriendPos(CChar* pTarget);
    // Ban
    /**
    * @brief Adds a char to the _lBans list.
    * @param pBan the char to ban.
    */
    void AddBan(CChar* pBan);
    /**
    * @brief Deletes a char from the _lBans list.
    * @param pBan the char.
    */
    void DelBan(CChar* pBan);
    /**
    * @brief Returns the total count of banned chars.
    * @return the count
    */
    size_t GetBanCount();
    /**
    * @brief Returns the position on the _lBans list of the given char.
    * @param pBan the char.
    * @return the pos.
    */
    int GetBanPos(CChar* pBan);
	// Access
    /**
    * @brief Adds access to this house to the given char.
    * @param pAccess the char.
    */
	void AddAccess(CChar* pAccess);
    /**
    * @brief Revokes the access to this house to the given char.
    *
    * Note: This removes the char from the list, but won't prevent it from enter like a Ban.
    * @param pAccess the char.
    */
	void DelAccess(CChar* pAccess);
    /**
    * @brief Returns the count of chars with access.
    * @return the count.
    */
	size_t GetAccessCount();
    /**
    * @brief Returns the position on the _lAccess list of the given char.
    * @param pAccess the char.
    * @return the position.
    */
	int GetAccessPos(CChar* pAccess);
    /**
    * @brief Ejects a char from inside the house to the House Sign.
    * @param pChar the char.
    */
    void Eject(CChar* pChar);
    /**
    * @brief Ejects all chars from this house
    * @param pChar this char will not be ejected (eg: the owner kicking all people)
    */
    void EjectAll(CChar* pChar = nullptr);
    ///@}

    /** House general:
    */
    ///@{
    // Keys:
    /**
    * @brief Creates a key for the given character.
    * @param pTarget the target that receives the key.
    * @param  fDupeOnBank creates a copy on the bank if true.
    * @return the Key.
    */
    CItem *GenerateKey(CChar *pTarget, bool fDupeOnBank = false);
    /**
    * @brief Removes all the keys of this house from the given char.
    * @param pTarget the char.
    */
    void RemoveKeys(CChar *pTarget);
    // Misc
    /**
    * @brief Returns the multi count.
    *
    * Checks how many multis count this multi towards the character's houses/ships list.
    * eg: a Castle can count as 3 multis instead of 1.
    * @return the count.
    */
    int16 GetMultiCount();
    // Redeed
    /**
    * @brief Redeeds this multi
    * @param fDisplayMsg Display the bounce message on the player.
    * @param fMoveToBank Places the deed on the bank.
    * @param pChar the char doing the redeed (if any).
    */
    void Redeed(bool fDisplayMsg = true, bool fMoveToBank = true, CChar *pChar = nullptr);
    //Moving Crate
    /**
    * @brief Sets the Moving Crate to the target.
    * @param pCrate the new Moving Crate.
    */
    void SetMovingCrate(CItemContainer* pCrate);
    /**
    * @brief Returns the Moving Crate if any.
    * @param fCreate creates a new crate if there is no one already.
    * @return the Moving Crate.
    */
    CItemContainer* GetMovingCrate(bool fCreate);
    /**
    * @brief Transfer all the items to the Moving Crate.
    * @param iType the types of items to transfer.
    */
    void TransferAllItemsToMovingCrate(TRANSFER_TYPE iType = TRANSFER_ALL);
    /**
    * @brief Transfers Locked Down items to Moving Crate.
    */
    void TransferLockdownsToMovingCrate();
    /**
    * @brief Transfers Secured containers to Moving Crate.
    */
    void TransferSecuredToMovingCrate();
    /**
    * @brief Transfers Moving Crate to owner's bank.
    */
    void TransferMovingCrateToBank();

    // AddOns
    /**
    * @brief Adds an Addon to the addons list.
    * @param pAddon the Addon
    */
    void AddAddon(CItemMulti *pAddon);
    /**
    * @brief Removes an Addon from the addons list.
    * @param pAddon the Addon
    */
    void DelAddon(CItemMulti *pAddon);
    /**
    * @brief Returns the position of a given Addon.
    * @param pAddon the Addon
    * @return the position
    */
    int GetAddonPos(CItemMulti *pAddon);
    /**
    * @brief Returns the total count of Addons.
    * @return the count.
    */
    size_t GetAddonCount();
    /**
    * @brief Redeeds all addons.
    */
    void RedeedAddons();

    // Components
    /**
    * @brief Adds a Component to the components list.
    * @param pComponent the component.
    */
    void AddComp(CItem* pComponent);
    /**
    * @brief Removes a Component from the components list.
    * @param pComponent the component.
    */
    void DelComp(CItem* pComponent);
    /**
    * @brief Returns the position of a given Component.
    * @param pComponent the component
    * @return the position
    */
    int GetCompPos(CItem* pComponent);
    /**
    * @brief Returns the total count of Components.
    * @return the count.
    */
    size_t GetCompCount();
    /**
    * @brief Removes all Components.
    */
    void RemoveAllComps();
    /**
    * @brief Generates the base components of the multi.
    * @param fNeedKey wether a key is required or not.
    * @param dwKeyCode the code hash for keys.
    */
    void GenerateBaseComponents(bool &fNeedKey, dword &dwKeyCode);
    ///@}

    // House storage:
    // - Limits & values
    // -- (Limits & values) Base Storage
    /**
    * @brief Sets the Base Storage.
    * @param iLimit the total storage.
    */
    void SetBaseStorage(uint16 iLimit);
    /**
    * @brief Returns the Base Storage.
    * @return the total.
    */
    uint16 GetBaseStorage();
    /**
    * @brief Sets the modifier of Base Storage.
    * @param iIncrease the modifier.
    */
    void SetIncreasedStorage(uint16 iIncrease);
    /**
    * @brief Returns the modifier for Base Storage.
    * @return the modifier.
    */
    uint16 GetIncreasedStorage();
    /**
    * @brief returns the Max Storage.
    * @return the value.
    */
    uint16 GetMaxStorage();
    /**
    * @brief returns the current Storage used.
    * @return the value.
    */
    uint16 GetCurrentStorage();

    // -- (Limits & values)Vendors
    /**
    * @brief Sets the Base Vendors.
    * @param iLimit the limit.
    */
    void SetBaseVendors(uint8 iLimit);
    /**
    * @brief Returns the Base Vendors.
    * @param the value.
    */
    uint8 GetBaseVendors();
    /**
    * @brief Returns the max allowed vendors.
    * @return the total
    */
    uint8 GetMaxVendors();
    // -- (Limits & values)LockDowns
    /**
    * @brief Returns the maximum lockdowns allowed.
    * @return the max
    */
    uint16 GetMaxLockdowns();
    /**
    * @brief Returns the Lockdowns Percent.
    * @return the percent
    */
    uint8 GetLockdownsPercent();
    /**
    * @brief Sets the Lockdowns Percent.
    * @param iPercent the percent.
    */
    void SetLockdownsPercent(uint8 iPercent);
    // -Lockdowns
    /**
    * @brief Locks an item and add it to the Lockdowns list.
    * @param pItem the item.
    */
    void LockItem(CItem* pItem);
    /**
    * @brief Unlocks an item and remove it from the Lockdowns list.
    * @param pItem the item.
    */
    void UnlockItem(CItem* pItem);
    /**
    * @brief Returns the position of the given item.
    * @param pItem the item.
    */
    int GetLockedItemPos(CItem* pItem);
    /**
    * @brief Returns the total locked down items.
    * @return the count.
    */
    size_t GetLockdownCount();
    // -Secured storage (Containers + items).
    /**
    * @brief Secures a container and adds it to the containers list.
    * @param pContainer the container.
    */
    void Secure(CItemContainer *pContainer);
    /**
    * @brief Releases a container and removes it from the containers list.
    * @param pContainer the container.
    */
    void Release(CItemContainer *pContainer);
    /**
    * @brief Returns the position of the given container
    * @param pContainer the container
    * @return the pos.
    */
    int GetSecuredContainerPos(CItemContainer *pContainer);
    /**
    * @brief Returns the total items secured on containers.
    * @return the count
    */
    int16 GetSecuredItemsCount();
    /**
    * @brief Returns the total containers secured.
    * @return the count.
    */
    size_t GetSecuredContainersCount();
    // -Vendors
    /**
    * @brief Adds a char to the vendors list.
    * @param pVendor the vendor.
    */
    void AddVendor(CChar* pVendor);
    /**
    * @brief Removes a char from the vendors list.
    * @param pVendor the vendor
    */
    void DelVendor(CChar* pVendor);
    /**
    * @brief Returns the position of the given char.
    * @param pVendor the char
    * @return the pos.
    */
    int GetHouseVendorPos(CChar* pVendor);
    /**
    * @brief Returns the total vendors.
    * @return the count.
    */
    size_t GetVendorCount();

protected:
    /**
    * @brief Called when a component is created.
    * @param pComponent the component
    * @param fIsAddon true if the component is an addon.
    */
	virtual void OnComponentCreate(CItem * pComponent, bool fIsAddon = false);


public:
	static const char *m_sClassName;
	CItemMulti( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemMulti();

private:
	CItemMulti(const CItemMulti& copy);
	CItemMulti& operator=(const CItemMulti& other);

public:
    /**
    * @brief Tick override.
    * @return true
    */
	virtual bool OnTick();
    /**
    * @brief Place the multi.
    * @param pt Position.
    * @param bForceFix force a fix.
    * @return bool if can be moved.
    */
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false);
    /**
    * @brief Moving from current location.
    */
	virtual void OnMoveFrom();
    /**
    * @brief Speech commands on the multi.
    * @param pszCmd the speech.
    * @param pSrc who talked.
    */
	void OnHearRegion( lpctstr pszCmd, CChar * pSrc );
    /**
    * @brief Gets the Sign or Tiller item
    * @return the item
    */
	CItem * Multi_GetSign();
    
    /**
    * @brief General setup.
    * @param pChar the char placing the multi.
    * @param dwKeyCode code hash for keys.
    */
	void Multi_Setup(CChar * pChar, dword dwKeyCode);
    /**
    * @brief Gets the CItemBaseMulti linked to the given id
    * @param id the id.
    * @return the CItemBaseMulti
    */
	static const CItemBaseMulti * Multi_GetDef( ITEMID_TYPE id );

    // Scripts virtuals.

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );

	virtual void r_Write( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc );
	virtual bool r_LoadVal( CScript & s  );
	virtual void DupeCopy( const CItem * pItem );
};

/*
* Privileges/Access type for multis, mutually exclusive.
*/
enum HOUSE_PRIV
{
    HP_NONE,
    HP_OWNER,
    HP_COOWNER,
    HP_FRIEND,
    HP_ACCESSONLY,
    HP_BAN,
    HP_VENDOR,
    HP_GUILD,
    HP_QTY
};

/*
* Class related to storage/access the multis owned by the holder entity.
* This class adds multis to it's lists only checking for UID validity, nothing more,
* further checks must be added on multi placement, etc
*/
class CMultiStorage
{
private:
    std::map<CItemMulti*,HOUSE_PRIV> _lHouses;  // List of stored houses.
    std::map<CItemShip*,HOUSE_PRIV> _lShips;    // List of stored ships.
    int16 _iHousesTotal;    // Max of houses.
    int16 _iShipsTotal;     // Max of ships.
    CObjBase *_pSrc;
public:
    CMultiStorage(CObjBase *pSrc);
    virtual ~CMultiStorage();

    /**
    * @brief Adds a multi
    * @param pMulti the multi
    */
    void AddMulti(CItemMulti *pMulti, HOUSE_PRIV ePriv);
    /**
    * @brief Removes a multi
    * @param pMulti the multi
    */
    void DelMulti(CItemMulti *pMulti);
    /**
    * @brief Gets the priv
    * @param pMulti the multi to retrieve my privs.
    */
    HOUSE_PRIV GetPriv(CItemMulti* pMulti);

    /**
    * @brief Adds a house multi.
    * @param pHouse the house.
    */
    void AddHouse(CItemMulti *pHouse, HOUSE_PRIV ePriv);
    /**
    * @brief Removes a house multi.
    * @param pHouse the house.
    */
    void DelHouse(CItemMulti *pHouse);
    /**
    * @brief Checks if the char can place a house.
    * @param pChar the char.
    * @param iHouseCount the MultiCount of the house.
    * @return true if the char has space for the house.
    */
    bool CanAddHouse(CChar *pChar, int16 iHouseCount);
    /**
    * @brief Checks if the Guild/Town Stone can place a house.
    * @param pStone the stone.
    * @param iHouseCount the MultiCount of the house.
    * @return true if the stone has space for the house.
    */
    bool CanAddHouse(CItemStone *pStone, int16 iHouseCount);
    /**
    * @brief Returns the position of the given house.
    * @return the position.
    */
    int16 GetHousePos(CItemMulti *pHouse);
    /**
    * @brief Returns the total count of houses.
    *
    * This method counts the real houses and the value each one
    * adds with MultiCount.
    * @return the count.
    */
    int16 GetHouseCountTotal();
    /**
    * @brief Returns the real count of houses.
    * @return the count.
    */
    int16 GetHouseCountReal();
    /**
    * @brief Returns the house at the given position.
    * @param iPos the position
    * @return the house
    */
    CItemMulti *GetHouseAt(int16 iPos);
    /**
    * @brief Clears the list of houses.
    */
    void ClearHouses();


    /**
    * @brief Adds a ship.
    * @param pShip the ship.
    */
    void AddShip(CItemShip *pShip, HOUSE_PRIV ePriv);
    /**
    * @brief Removes a ship.
    * @param pShip the ship.
    */
    void DelShip(CItemShip *pShip);
    /**
    * @brief Checks if the char can place a ship.
    * @param pChar the char.
    * @param iHouseCount the MultiCount of the ship.
    * @return true if the char has space for the ship.
    */
    bool CanAddShip(CChar *pChar, int16 iShipCount);
    /**
    * @brief Checks if the Guild/Town Stone can place a ship.
    * @param pStone the stone.
    * @param iHouseCount the MultiCount of the ship.
    * @return true if the stone has space for the ship.
    */
    bool CanAddShip(CItemStone *pStone, int16 iShipCount);
    /**
    * @brief Returns the position of the given ship.
    * @param pShip the ship.
    * @return the position.
    */
    int16 GetShipPos(CItemShip *pShip);
    /**
    * @brief Returns the total count of ships.
    *
    * This method counts the real ships and the value each one
    * adds with MultiCount.
    * @return the count.
    */
    int16 GetShipCountTotal();
    /**
    * @brief The real count of ships.
    * @return the count.
    */
    int16 GetShipCountReal();
    /**
    * @brief Gets the ship at the given position.
    * @param iPos the position.
    * @return the ship.
    */
    CItemShip *GetShipAt(int16 iPos);
    /**
    * @brief Clears the ships list.
    */
    void ClearShips();

    /**
    * @brief Writes the houses/ships on the worldsave.
    * @param s The datastream.
    */
    virtual void r_Write(CScript & s);
};
#endif // _INC_CITEMMULTI_H
