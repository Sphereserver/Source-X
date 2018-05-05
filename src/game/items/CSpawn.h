/**
* @file CSpawn.h
*
*/

#ifndef _INC_CSPAWN_H
#define _INC_CSPAWN_H

//#include "CItem.h"
#include "../CComponent.h"


class CCharBase;

class CSpawn : public CComponent
{
private:
    CItem * _pLink;             ///< Link this object to it's parent CItem.
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];
    static lpctstr const sm_szRefKeys[];
    std::vector<CUID> _uidList; // Storing UIDs of the created items/chars.
    bool _fKillingChildren;     // For internal usage, set to true when KillChildren() is in proccess to prevent DelObj() being called from CObjBase when deleteing the objs.

    uint16 _iAmount;            // Maximum of objects to spawn.
    CResourceIDBase _idSpawn;   // ID of the Object to Spawn.

    uint16 _iPile;              // more2=The max # of items to spawn per interval, if this is 0 spawn up to the total amount (Only for Item's Spawn).
    uint16 _iTimeLo;            // morex=Lo time in minutes.
    uint16 _iTimeHi;            // morey=Hi time in minutes.
    uint8 _iMaxDist;            // morez=How far from this spawns will be created the items/chars?

public:
    static const char *m_sClassName;

    /*  I don't want to inherit SetAmount, GetAmount and _iAmount from the parent CItem class. I need to redefine them for CSpawn class
    *   so that when i set AMOUNT to the spawn item, i don't really set the "item amount/quantity" property, but the "spawn item AMOUNT" property.
    *   This way, even if there is a stackable spawn item (default in Enhanced Client), i won't increase the item stack quantity and i can't pick
    *   from pile the spawn item. Plus, since the max amount of spawnable objects per single spawn item is the max size of a byte, we can change
    *   the data type accepted/returned.
    */
    void SetAmount(word iAmount);
    word GetAmount();

    /**
    * @brief Returns how many Items/Chars are currently spawned.
    *
    * @return count
    */
    word GetCurrentSpawned();

    /**
    * @brief Returns the pile's size (Only for IT_SPAWN_ITEM).
    *
    * @return count
    */
    word GetPile();

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    *
    * @return count
    */
    uint16 GetTimeLo();

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    *
    * @return count
    */
    uint16 GetTimeHi();

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    *
    * @return count
    */
    uint8 GetMaxDist();

    /**
    * @brief CSpawn's custom alternative of CComponent::GetLink retrieving a direct link for the CItem, without const.
    *
    * @return *SpawnItem
    */
    //CItem *GetSpawnItem();

    /**
    * @brief ID of the object to spawn.
    *
    * @return *CResourceIDBase
    */
    CResourceIDBase GetSpawnID();

    /**
    * @brief Overrides onTick for this class.
    *
    * Setting time again
    * stoping if more2 >= amount
    * more1 Resource Check
    * resource (item/char) generation
    */
    void OnTick(bool fExec);

    /**
    * @brief Removes everything created by this spawn, if still belongs to the spawn.
    */
    void KillChildren();

    /**
    * @brief Setting display ID based on Character's Figurine or the default display ID if this is an IT_SPAWN_ITEM.
    */
    CCharBase * SetTrackID();

    /**
    * @brief Generate a *pDef item from this spawn.
    *
    * @param pDef resource to create
    */
    void GenerateItem(CResourceDef * pDef);

    /**
    * @brief Generate a *pDef char from this spawn.
    *
    * @param pDef resource to create
    */
    void GenerateChar(CResourceDef * pDef);

    /**
    * @brief Removing one UID in Spawn's m_obj[].
    *
    * @param UID of the obj to remove.
    */
    void DelObj(CUID uid);

    /**
    * @brief Storing one UID in Spawn's m_obj[].
    *
    * @param UID of the obj to add.
    */
    void AddObj(CUID uid);

    /**
    * @brief Test if the character from more1 exists.
    *
    * @param ID of the char to check.
    */
    inline CCharBase * TryChar(CREID_TYPE &id);

    /**
    * @brief Test if the item from more1 exists.
    *
    * @param ID of the item to check.
    */
    inline CItemBase * TryItem(ITEMID_TYPE &id);

    /**
    * @brief Get a proper RESOURCE_ID from the id provided.
    *
    * @return a valid RESOURCE_ID.
    */
    CResourceDef * FixDef();

    /**
    * @brief Gets the name of the resource created (item or char).
    *
    * @return the name of the resource.
    */
    uint WriteName(tchar * pszOut) const;

    virtual CItem *GetLink() override;
    CSpawn(const CObjBase *pLink);
    virtual ~CSpawn();
    virtual void Delete(bool fForce = false) override;
    virtual bool r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_GetRef(lpctstr & pszKey, CScriptObj * & pRef) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;
    virtual void Copy(CComponent *target) override;
};

#endif // _INC_CSPAWN_H
