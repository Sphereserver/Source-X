/**
* @file CCSpawn.h
*
*/

#ifndef _INC_CCSPAWN_H
#define _INC_CCSPAWN_H

#include "../../common/resource/CResourceHolder.h"
#include "../items/CItemBase.h"
#include "../uo_files/uofiles_enums_itemid.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../CComponent.h"
#include "../CTimedObject.h"
#include "../../sphere/threads.h"


class CCharBase;
class CItem;
class CUID;

class CCSpawn : public CComponent
{
    CItem* _pLink;
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];
    static lpctstr const sm_szRefKeys[];

    std::vector<CUID> _uidList; // Storing UIDs of the created items/chars.
    bool _fKillingChildren;     // For internal usage, set to true when KillChildren() is in proccess to prevent DelObj() being called from CObjBase when deleteing the objs.
    bool _fIsChampion;          // Is this a CHAMPION spawn?

    uint16 _iAmount;            // Maximum of objects to spawn.
    CResourceIDBase _idSpawn;   // legacy more1=ID of the Object to Spawn.

    uint16 _iPile;              // legacy more2=The max # of items to spawn per interval, if this is 0 spawn up to the total amount (Only for Items Spawn).
    uint16 _iTimeLo;            // legacy morex=Lo time in minutes.
    uint16 _iTimeHi;            // legacy morey=Hi time in minutes.
    uint8 _iMaxDist;            // legacy morez=How far from this spawns will be created the items/chars?

    static std::vector<CCSpawn*> _vBadSpawns;
    bool _fIsBadSpawn;

    void AddBadSpawn();
    void DelBadSpawn();

public:
    static const char *m_sClassName;
    static CCSpawn * GetBadSpawn(int index = -1);

    CCSpawn(CItem *pLink, bool fIsChampion = false);
    virtual ~CCSpawn();
    CItem *GetLink() const;
    virtual void Delete(bool fForce = false) override;
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;
    virtual void Copy(const CComponent *target) override;
    /*virtual bool IsDeleted() const override;
    virtual void GoAwake() override;
    virtual void GoSleep() override;*/

    /*  I don't want to inherit SetAmount, GetAmount and _iAmount from the parent CItem class. I need to redefine them for CCSpawn class
    *   so that when i set AMOUNT to the spawn item, i don't really set the "item amount/quantity" property, but the "spawn item AMOUNT" property.
    *   This way, even if there is a stackable spawn item (default in Enhanced Client), i won't increase the item stack quantity and i can't pick
    *   from pile the spawn item. Plus, since the max amount of spawnable objects per single spawn item is the max size of a byte, we can change
    *   the data type accepted/returned.
    */
    void SetAmount(uint16 iAmount);
    uint16 GetAmount() const;

    /**
    * @brief Returns how many Items/Chars are currently spawned.
    * @return count
    */
    uint16 GetCurrentSpawned() const;

    /**
    * @brief Returns the pile's size (Only for IT_SPAWN_ITEM).
    * @return count
    */
    uint16 GetPile() const;

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    * @return count
    */
    uint16 GetTimeLo() const;

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    * @return count
    */
    uint16 GetTimeHi() const;

    /**
    * @brief Returns how many Items/Chars are currently spawned from me.
    * @return count
    */
    uint8 GetDistanceMax() const;

    /**
    * @brief CCSpawn's custom alternative of CComponent::GetLink retrieving a direct link for the CItem, without const.
    * @return *SpawnItem
    */
    //CItem *GetSpawnItem();

    /**
    * @brief ID of the object to spawn.
    * @return CResourceID
    */
    const CResourceIDBase& GetSpawnID() const;

    /**
    * @brief Overrides onTick for this class.
    *
    * Setting time again
    * stoping if more2 >= amount
    * more1 Resource Check
    * resource (item/char) generation
    */
    virtual CCRET_TYPE OnTickComponent() override;
    /**
    * @brief Removes everything created by this spawn, if still belongs to the spawn.
    */
    void KillChildren();

    /**
    * @brief Setting display ID based on Character's Figurine or the default display ID if this is an IT_SPAWN_ITEM.
    */
    const CCharBase * SetTrackID();

    /**
    * @brief Generate a *pDef item from this spawn.
    */
    void GenerateItem();

    /**
    * @brief Generate a char with the given id from this spawn.
    * @params rid the id of the char.
    */
    CChar* GenerateChar(CResourceIDBase rid);
    /**
    * @brief Get a valid char id from the spawn.
    * @return the char id.
    */
    CResourceIDBase GetCharRid();
    /**
    * @brief Complete character generation.
    */
    void GenerateChar();

    /**
    * @brief Removing one UID in Spawn's m_obj[].
    * @param UID of the obj to remove.
    */
    void DelObj(const CUID& uid);

    /**
    * @brief Storing one UID in Spawn's m_obj[].
    * @param UID of the obj to add.
    */
    void AddObj(const CUID& uid);

    /**
    * @brief Get a proper CResourceID from the id provided.
    * @return a valid CResourceDef.
    */
 private:   const CResourceDef * _FixDef();
 public:    const CResourceDef * FixDef();

    /**
    * @brief Gets the name of the resource created (item or char).
    * @return the name of the resource.
    */
    uint WriteName(tchar * ptcOut) const;
};

#endif // _INC_CCSPAWN_H
