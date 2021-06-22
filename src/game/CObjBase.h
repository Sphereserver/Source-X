/**
* @file CObjBase.h
*
*/

#ifndef _INC_COBJBASE_H
#define _INC_COBJBASE_H

#include "../common/resource/CResourceBase.h"
#include "../common/resource/CResourceRef.h"
#include "../common/CScriptObj.h"
#include "clients/CClientTooltip.h"
#include "CObjBaseTemplate.h"
#include "CTimedObject.h"
#include "CEntity.h"
#include "CBase.h"
#include "CServerConfig.h"


class PacketSend;
class PacketPropertyList;
class CCSpawn;

class CSector;
class CWorldTicker;

class CObjBase : public CObjBaseTemplate, public CScriptObj, public CEntity, public CEntityProps, public virtual CTimedObject
{
	static lpctstr const sm_szLoadKeys[];   // All Instances of CItem or CChar have these base attributes.
	static lpctstr const sm_szVerbKeys[];   // All Instances of CItem or CChar have these base attributes.
	static lpctstr const sm_szRefKeys[];    // All Instances of CItem or CChar have these base attributes.

    friend class CSector;
    friend class CWorldTicker;

private:
	int64 m_timestamp;          // TimeStamp

protected:
	CResourceRef m_BaseRef;     // Pointer to the resource that describes this type.
    bool _fDeleting;

    std::string _sRunningTrigger;   // Name of the running trigger (can be custom!) [use std::string instead of CSString because the former is allocated on-demand]
    short _iRunningTriggerId;       // Current trigger being run on this object. Used to prevent the same trigger being called over and over.
    short _iCallingObjTriggerId;    // I am running a trigger called via TRIGGER (CallPersonalTrigger method). In which trigger (OF THIS SAME OBJECT) was this call executed?

public:
    static const char *m_sClassName;
    static dword sm_iCount;    // how many total objects in the world ?

    
    int _iCreatedResScriptIdx;	// index in g_Cfg.m_ResourceFiles of the script file where this obj was created
    int _iCreatedResScriptLine;	// line in the script file where this obj was created

    CVarDefMap m_TagDefs;		// attach extra tags here.
    CVarDefMap m_BaseDefs;		// New Variable storage system
    dword	m_CanMask;			// Mask to be XORed to Can: enable or disable some Can Flags

    word	m_attackBase;       // dam for weapons
    word	m_attackRange;      // variable range of attack damage.

    word	m_defenseBase;	    // Armor for IsArmor items
    word	m_defenseRange;     // variable range of defense.
    int 	m_ModMaxWeight;		// ModMaxWeight prop.
    HUE_TYPE m_wHue;			// Hue or skin color. (CItems must be < 0x4ff or so)
    CUID 	_uidSpawn;          // SpawnItem for this item

    CResourceRefArray m_OEvents;
    
public:
    explicit CObjBase(bool fItem);
    virtual ~CObjBase();
private:
    CObjBase(const CObjBase& copy);
    CObjBase& operator=(const CObjBase& other);

protected:
    /**
     * @brief   Prepares to delete.
     */
    virtual void DeletePrepare();

    void DeleteCleanup(bool fForce);    // not virtual!

public:
    inline bool IsBeingDeleted() const noexcept
    {
        return _fDeleting;
    }

protected:  virtual bool _IsDeleted() const override;
public:     virtual bool  IsDeleted() const override;

    /**
     * @brief   Deletes this CObjBase from game (doesn't delete the raw class instance).
     * @param   bForce  Force deletion.
     * @return  Was deleted.
     */
    virtual bool Delete(bool fForce = false);

    /**
     * @brief   Dupe copy.
     * @param   pObj    The object.
     */
    virtual void DupeCopy(const CObjBase* pObj); // overridden by CItem

public:
	/**
	* @brief   Base get definition.
	* @return  null if it fails, else a pointer to a CBaseBaseDef.
	*/
    CBaseBaseDef* Base_GetDef() const noexcept;

	inline dword GetCanFlagsBase() const noexcept
	{
		return Base_GetDef()->m_Can;
	}

    inline dword GetCanFlags() const noexcept
	{
		// m_CanMask is XORed to m_Can:
		//  If a flag in m_CanMask is enabled in m_Can, it is ignored in this Can check
		//  If a flag in m_CanMask isn't enabled in m_Can, it is considered as enabled in this Can check
		// So m_CanMask is useful to dynamically switch on/off some of the read-only CAN flags in the ITEMDEF/CHARDEF.
		return (GetCanFlagsBase() ^ m_CanMask);
	}	

	bool Can(dword dwCan) const noexcept
	{
        return (GetCanFlags() & dwCan);
	}

    inline bool Can(dword dwCan, dword dwObjCanFlags) const noexcept
    {
        return (dwObjCanFlags & dwCan);
    }

    bool IsRunningTrigger() const;

	/**
	* @brief   Call personal trigger (from scripts).
	*
	* @param [in,out]  pArgs       If non-null, the arguments.
	* @param [in,out]  pSrc        If non-null, source for the.
	* @param [in,out]  trResult    The tr result.
	*
	* @return  true if it succeeds, false if it fails.
	*/
	bool CallPersonalTrigger(tchar * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult);


public:

    /**
    * @brief   Returns Spawn item.
    * @return  The CItem.
    */
    CCSpawn *GetSpawn();

    /**
    * @brief   sets the Spawn item.
    * @param  The CCSpawn.
    */
    void SetSpawn(CCSpawn *spawn);

    /**
    * @brief   Returns Faction CComponent.
    * @return  The CCFaction.
    */
    CCFaction *GetFaction();


    /**
     * @brief   Gets timestamp of the item (it's a property and not related at all with TIMER).
     * @return  The timestamp.
     */
	int64 GetTimeStamp() const;

    /**
     * @brief   Sets time stamp.
     * @param   t_time  The time.
     */
	void SetTimeStamp(int64 t_time);

    /*
    * @brief    Add iDelta to this object's timer (if active) and its child objects.
    */
    void TimeoutRecursiveResync(int64 iDelta);

    /**
    *@brief Returns the value of the string-type prop from the CComponentProps. Faster than the variant accepting a COMPPROPS_TYPE if you need to retrieve multiple props from the same CComponentProps
    *@param pCompProps The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param fZero If the prop val is an empty string, return "0" instead.
    *@param pBaseCompProps If nullptr and the prop doesn't exist, stop. Otherwise, this should point to the same type of CComponentProps, but in the object's CBaseBaseDef. So, check if the Base Property exists
    */
    CSString GetPropStr( const CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, bool fZero, const CComponentProps* pBaseCompProps = nullptr ) const;

    /**
    *@brief Returns the value of the string-type prop from the CComponentProps
    *@param iCompPropsType The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param fZero If the prop val is an empty string, return "0" instead
    *@param fDef If true, if the prop wasn't found, check the Base Prop (in my CBaseBaseDef).
    */
    CSString GetPropStr( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, bool fZero, bool fDef = false ) const;

    /**
    *@brief Returns the value of the numerical-type prop from the CComponentProps. Faster than the variant accepting a COMPPROPS_TYPE if you need to retrieve multiple props from the same CComponentProps
    *@param pCompProps The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param pBaseCompProps If nullptr and the prop doesn't exist, stop. Otherwise, this should point to the same type of CComponentProps, but in the object's CBaseBaseDef. So, check if the Base Property exists
    */
    CComponentProps::PropertyValNum_t GetPropNum( const CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, const CComponentProps* pBaseCompProps = nullptr ) const;

    /**
    *@brief Returns the value of the numerical-type prop from the CComponentProps.
    *@param iCompPropsType The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param fDef If true, if the prop wasn't found, check the Base Prop (in my CBaseBaseDef).
    */
    CComponentProps::PropertyValNum_t GetPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, bool fDef = false ) const;

    /**
    *@brief Sets the value of the string-type prop from the CComponentProps. Faster than the variant accepting a COMPPROPS_TYPE if you need to set multiple props from the same CComponentProps
    *@param pCompProps The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param ptcVal The property value
    *@param fDeleteZero If the prop val is an empty string, delete the prop
    */
    void SetPropStr( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, lpctstr ptcVal, bool fDeleteZero = true);

    /**
    *@brief Sets the value of the string-type prop from the CComponentProps.
    *@param iCompPropsType The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param ptcVal The property value
    *@param fDeleteZero If the prop val is an empty string, delete the prop
    */
    void SetPropStr( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, lpctstr ptcVal, bool fDeleteZero = true);

    /**
    *@brief Sets the value of the numerical-type prop from the CComponentProps. Faster than the variant accepting a COMPPROPS_TYPE if you need to set multiple props from the same CComponentProps
    *@param pCompProps The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param iVal The property value
    */
    void SetPropNum( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iVal );

    /*
    *@brief Sets the value of the numerical-type prop from the CComponentProps.
    *@param iCompPropsType The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param iVal The property value
    */
    void SetPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iVal );

    /**
    *@brief Sums a number to the value of the numerical-type prop from the CComponentProps. Faster than the variant accepting a COMPPROPS_TYPE if you need to set multiple props from the same CComponentProps
    *@param pCompProps The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param iMod The signed number to sum to the prop value
    *@param pBaseCompProps If nullptr and the prop doesn't exist, consider 0 as the previous value. Otherwise, this should point to the same type of CComponentProps, but in the object's CBaseBaseDef.
    *       So, check if the Base Property exists and use its value as the previous value. If it doesn't exists, use 0.
    */
    void ModPropNum( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iMod, const CComponentProps* pBaseCompProps = nullptr);

    /**
    *@brief Sums a number to the value of the numerical-type prop from the CComponentProps.
    *@param iCompPropsType The CComponentProps which the property belongs
    *@param iPropIndex The index (enum) of the property for that CComponentProps
    *@param iMod The signed number to sum to the prop value
    *@param fBaseDef If false and the prop doesn't exist, consider 0 as the previous value. Otherwise, check if the Base Property exists and use its value as the previous value. If it doesn't exists, use 0.
    */
    void ModPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iMod, bool fBaseDef = false);

    /**
     * @fn  lpctstr CObjBase::GetDefStr( lpctstr ptcKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets definition string from m_BaseDefs.
     *
     * @param   ptcKey      The key.
     * @param   fZero       true to zero.
     * @param   fBaseDef    if the def doesn't exist, then check for a base def.
     *
     * @return  The definition string.
     */
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false, bool fBaseDef = false ) const;

    /**
     * @fn  int64 CObjBase::GetDefNum( lpctstr ptcKey, bool fDef = false, int64 iDefault = 0 ) const;
     *
     * @brief   Gets definition number from m_BaseDefs.
     *
     * @param   ptcKey      The key.
     * @param   iDefault    Default value to return if not existant.
     * @param   fBaseDef    if the def doesn't exist, then check for a base def.
     *
     * @return  The definition number.
     */

	int64 GetDefNum( lpctstr ptcKey, bool fBaseDef = false, int64 iDefault = 0) const;

    /**
     * @fn  void CObjBase::SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true);
     *
     * @brief   Sets definition number from m_BaseDefs.
     *
     * @param   ptcKey  The key.
     * @param   iVal    Value.
     * @param   fZero   If iVal == 0, delete the def.
     */

	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero = true);

    /**
    * @fn  void CObjBase::ModDefNum(lpctstr ptcKey, int64 iMod, bool fZero = true);
    *
    * @brief   Add iVal to the numeric definition from m_BaseDefs.
    *
    * @param   ptcKey   The key.
    * @param   iMod     Value to sum to the current value of the def.
    * @param   fBaseDef if the def doesn't exist, then check for a base def and use that value to create a new def.
    * @param   fZero    If new def value == 0, delete the def.
    */

    void ModDefNum(lpctstr ptcKey, int64 iMod, bool fBaseDef = false, bool fZero = false);

    /**
     * @fn  void CObjBase::SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);
     *
     * @brief   Sets definition string to m_BaseDefs.
     *
     * @param   ptcKey  The key.
     * @param   pszVal  The value.
     * @param   fQuoted true if quoted.
     * @param   fZero   true to zero.
     */

	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);

    /**
     * @fn  void CObjBase::DeleteDef(lpctstr ptcKey);
     *
     * @brief   Deletes the definition from m_BaseDefs.
     *
     * @param   ptcKey  The key to delete.
     */
	void DeleteDef(lpctstr ptcKey);

    /**
     * @fn  CVarDefCont * CObjBase::GetDefKey( lpctstr ptcKey, bool fDef ) const;
     *
     * @brief   Gets definition key from m_BaseDefs.
     *
     * @param   ptcKey  The key.
     * @param   fDef    true to definition.
     *
     * @return  nullptr if it fails to find the def, else the pointer to the def.
     */
	CVarDefCont * GetDefKey( lpctstr ptcKey, bool fDef ) const;

    /**
    * @fn  CVarDefContNum * CObjBase::GetDefKeyNum( lpctstr ptcKey, bool fDef ) const;
    *
    * @brief   Gets definition key from m_BaseDefs.
    *
    * @param   ptcKey  The key.
    * @param   fDef    true to definition.
    *
    * @return  nullptr if it doesn't find a numeric def, else the pointer to the def.
    */
    inline CVarDefContNum * GetDefKeyNum(lpctstr ptcKey, bool fDef) const
    {
        return dynamic_cast<CVarDefContNum*>(GetDefKey(ptcKey, fDef));
    }

    /**
    * @fn  CVarDefContStr * CObjBase::GetDefKeyStr( lpctstr ptcKey, bool fDef ) const;
    *
    * @brief   Gets definition key from m_BaseDefs.
    *
    * @param   ptcKey  The key.
    * @param   fDef    true to definition.
    *
    * @return  nullptr if it doesn't find a string def, else the pointer to the def.
    */
    inline CVarDefContStr * GetDefKeyStr(lpctstr ptcKey, bool fDef) const
    {
        return dynamic_cast<CVarDefContStr*>(GetDefKey(ptcKey, fDef));
    }

    /**
     * @fn  lpctstr CObjBase::GetKeyStr( lpctstr ptcKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets key string from m_TagDefs.
     *
     * @param   ptcKey  The key.
     * @param   fZero   true to zero.
     * @param   fDef    true to definition.
     *
     * @return  The key string.
     */
	lpctstr GetKeyStr( lpctstr ptcKey, bool fZero = false, bool fDef = false ) const;

    /**
     * @fn  int64 CObjBase::GetKeyNum( lpctstr ptcKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets key number from m_TagDefs.
     *
     * @param   ptcKey  The key.
     * @param   fDef    true to definition.
     *
     * @return  The key number.
     */
	int64 GetKeyNum( lpctstr ptcKey, bool fDef = false ) const;

    /**
     * @fn  CVarDefCont * CObjBase::GetKey( lpctstr ptcKey, bool fDef ) const;
     *
     * @brief   Gets a key from m_TagDefs.
     *
     * @param   ptcKey  The key.
     * @param   fDef    Check also for base TagDefs (CHARDEF, ITEMDEF, etc).
     *
     * @return  null if it fails, else the key.
     */
	CVarDefCont * GetKey( lpctstr ptcKey, bool fDef ) const;

    /**
     * @fn  void CObjBase::SetKeyNum(lpctstr ptcKey, int64 iVal);
     *
     * @brief   Sets key number from m_TagDefs.
     *
     * @param   ptcKey  The key.
     * @param   iVal    Zero-based index of the value.
     */
	void SetKeyNum(lpctstr ptcKey, int64 iVal);

    /**
     * @fn  void CObjBase::SetKeyStr(lpctstr ptcKey, lpctstr pszVal);
     *
     * @brief   Sets key string from m_TagDefs.
     *
     * @param   ptcKey  The key.
     * @param   pszVal  The value.
     */
	void SetKeyStr(lpctstr ptcKey, lpctstr pszVal);

    /**
     * @fn  void CObjBase::DeleteKey( lpctstr ptcKey );
     *
     * @brief   Deletes the key described by ptcKey from m_TagDefs.
     *
     * @param   ptcKey  The key.
     */
	void DeleteKey( lpctstr ptcKey );


public:
    /**
     * @fn  virtual int CObjBase::FixWeirdness() = 0;
     *
     * @brief   Fix weirdness.
     *
     * @return  An int.
     */
	virtual int FixWeirdness() = 0;

    /**
     * @fn  virtual int CObjBase::GetWeight(word amount = 0) const = 0;
     *
     * @brief   Gets a weight.
     *
     * @param   amount  The amount.
     *
     * @return  The weight.
     */
	virtual int GetWeight(word amount = 0) const = 0;

    /**
     * @fn  virtual bool CObjBase::IsResourceMatch( RESOURCE_ID_BASE rid, dword dwArg ) = 0;
     *
     * @brief   Query if 'rid' is resource match.
     *
     * @param   rid     The rid.
     * @param   dwArg   The argument.
     *
     * @return  true if resource match, false if not.
     */
	virtual bool IsResourceMatch( const CResourceID& rid, dword dwArg ) const = 0;

    /**
     * @fn  virtual int CObjBase::IsWeird() const;
     *
     * @brief   Is weird.
     *
     * @return  An int.
     */
	virtual int IsWeird() const;

	// Accessors

    /**
     * @fn  virtual word CObjBase::GetBaseID() const = 0;
     *
     * @brief   Gets base identifier.
     *
     * @return  The base identifier.
     */

	virtual word GetBaseID() const = 0;

    /**
     * @fn  void CObjBase::SetUID( dword dwVal, bool fItem );
     *
     * @brief   Sets an UID.
     *
     * @param   dwVal   The value.
     * @param   fItem   true to item.
     */
	void SetUID( dword dwVal, bool fItem );

    /**
     * @fn  virtual lpctstr CObjBase::GetName() const;
     *
     * @brief   Gets the name. Resolves ambiguity w/CScriptObj.
     *
     * @return  The name.
     */
	virtual lpctstr GetName() const;

    /**
     * @fn  lpctstr CObjBase::GetResourceName() const;
     *
     * @brief   Gets resource name.
     *
     * @return  The resource name.
     */
	lpctstr GetResourceName() const;

protected:
     /**
     * @brief   Sets hue without calling triggers or additional checks (internally used by memory objects).
     * @param   wHue    The hue.
     */
     void SetHueQuick(HUE_TYPE wHue);

public:
    /**
     * @fn  void CObjBase::SetHue( HUE_TYPE wHue, bool bAvoidTrigger = true, CTextConsole *pSrc = nullptr, CObjBase *SourceObj = nullptr, llong sound = 0 );
     *
     * @brief   Sets Hue given.
     *
     * @param   wHue                The hue.
     * @param   fAvoidTrigger       true to avoid trigger.
     * @param [in,out]  pSrc        (Optional) If non-null, source for the.
     * @param [in,out]  SourceObj   (Optional) If non-null, source object.
     * @param   sound               The sound.
     */
	void SetHue( HUE_TYPE wHue, bool fAvoidTrigger = true, CTextConsole *pSrc = nullptr, CObjBase * pSourceObj = nullptr, llong iSound = 0 );

    /**
     * @fn  HUE_TYPE CObjBase::GetHue() const;
     *
     * @brief   Gets the hue.
     *
     * @return  The hue.
     */
	HUE_TYPE GetHue() const;


public:

    /**
     * @fn  virtual bool CObjBase::MoveTo(CPointMap pt, bool bForceFix = false) = 0;
     *
     * @brief   Move To Location.
     *
     * @param   pt          The point.
     * @param   fForceFix   true to force fix.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool MoveTo(const CPointMap& pt, bool fForceFix = false) = 0;	// Move to a location at top level.

    /**
     * @fn  bool CObjBase::MoveNear( CPointMap pt, word iSteps = 0 );
     *
     * @brief   Move near of a location.
     *
     * @param   pt      The point.
     * @param   iSteps  Zero-based index of the steps.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool MoveNear(CPointMap pt, ushort iSteps = 0 );

    /**
     * @fn  virtual bool CObjBase::MoveNearObj( const CObjBaseTemplate *pObj, word iSteps = 0 );
     *
     * @brief   Move near of an object.
     *
     * @param   pObj    The object.
     * @param   iSteps  Zero-based index of the steps.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, ushort iSteps = 0 );

    /**
     * @fn  void inline CObjBase::SetNamePool_Fail( tchar * ppTitles );
     *
     * @brief   SetNamePool() failed, so we set another name and throw an error.
     *
     * @param [in,out]  ppTitles    If non-null, the titles.
     */
	void SetNamePool_Fail( tchar * ppTitles );

    /**
     * @fn  bool CObjBase::SetNamePool( lpctstr pszName );
     *
     * @brief   Sets name pool.
     *
     * @param   pszName The name.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool SetNamePool( lpctstr pszName );

    /**
     * @fn  void CObjBase::Sound( SOUND_TYPE id, int iRepeat = 1 ) const;
     *
     * @brief   Play sound effect from this location.
     *
     * @param   id      The identifier.
     * @param   iRepeat Zero-based index of the repeat.
     */
	void Sound( SOUND_TYPE id, int iRepeat = 1 ) const;

    /**
     * @fn  void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = nullptr, byte bspeedseconds = 5, byte bloop = 1, bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;
     *
     * @brief   Adds an Effect.
     *
     * @param   motion          The motion.
     * @param   id              The identifier.
     * @param   pSource         Source for the.
     * @param   bspeedseconds   The bspeedseconds.
     * @param   bloop           The bloop.
     * @param   fexplode        true to fexplode.
     * @param   color           The color.
     * @param   render          The render.
     * @param   effectid        The effectid.
     * @param   explodeid       The explodeid.
     * @param   explodesound    The explodesound.
     * @param   effectuid       The effectuid.
     * @param   type            The type.
     */
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = nullptr, byte bspeedseconds = 5, byte bloop = 1,
        bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;
	
	/**
	* @fn  void CObjBase::EffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, CPointMap &pt, const CObjBase * pSource = nullptr, byte bspeedseconds = 5, byte bloop = 1, bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;
	*
	* @brief   Adds an Effect to a map point.
	* @param   motion          The motion.
	* @param   id              The identifier.
	* @param   pt			   The map point.
	* @param   pSource         Source for the.
	* @param   bspeedseconds   The bspeedseconds.
	* @param   bloop           The bloop.
	* @param   fexplode        true to fexplode.
	* @param   color           The color.
	* @param   render          The render.
	* @param   effectid        The effectid.
	* @param   explodeid       The explodeid.
	* @param   explodesound    The explodesound.
	* @param   effectuid       The effectuid.
	* @param   type            The type.
	*/
	void EffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *ptSrc, const CPointMap *ptDest, byte bspeedseconds = 5, byte bloop = 1,
        bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;

	void r_WriteSafe( CScript & s );

	virtual bool r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef ) override;
	virtual void r_Write( CScript & s );
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;	// some command on this object as a target

    /**
     * @fn  void CObjBase::Emote(lpctstr pText, CClient * pClientExclude = nullptr, bool fPossessive = false);
     *
     * @brief   Emote ( You %s, You see %s %s ).
     *
     * @param   pText                   The text.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fPossessive             true to possessive.
     */
	void Emote(lpctstr pText, CClient * pClientExclude = nullptr, bool fPossessive = false);
	void EmoteObj(lpctstr pText);

    /**
     * @fn  void CObjBase::Emote2(lpctstr pText, lpctstr pText2, CClient * pClientExclude = nullptr, bool fPossessive = false);
     *
     * @brief   Sends one Emote to this and a different text to others.
     *
     * @param   pText                   The text.
     * @param   pText2                  The second p text.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fPossessive             true to possessive.
     */
	void Emote2(lpctstr pText, lpctstr pText2, CClient * pClientExclude = nullptr, bool fPossessive = false);

    /**
     * @fn  virtual void CObjBase::Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
     *
     * @brief   Speaks.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     */
	virtual void Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );

    /**
     * @fn  virtual void CObjBase::SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );
     *
     * @brief   Speak UTF 8.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     * @param   lang    The language.
     */
	virtual void SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );

    /**
     * @fn  virtual void CObjBase::SpeakUTF8Ex( const nword * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );
     *
     * @brief   Speak UTF 8 ex.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     * @param   lang    The language.
     */
	virtual void SpeakUTF8Ex( const nword * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

    /**
     * @fn  void CObjBase::RemoveFromView( CClient * pClientExclude = nullptr , bool fHardcoded = true );
     *
     * @brief   Removes from view.

     *
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fHardcoded              true if hardcoded.
     */
	void RemoveFromView( CClient * pClientExclude = nullptr , bool fHardcoded = true );	// remove this item from all clients.

    /**
     * @fn  void CObjBase::ResendOnEquip( bool fAllClients = false );
     *
     * @brief   Resend on equip.
     *
     * @param   fAllClients true to all clients.
     */
	void ResendOnEquip( bool fAllClients = false );	// Fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again.

    /**
     * @fn  void CObjBase::ResendTooltip( bool fSendFull = false, bool fUseCache = false );
     *
     * @brief   Resend tooltip.
     *
     * @param   fSendFull   true to send full.
     * @param   fUseCache   true to use cache.
     */
	void ResendTooltip( bool fSendFull = false, bool fUseCache = false );	// force reload of tooltip for this object

    /**
     * @fn  void CObjBase::UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = nullptr ) const;
     *
     * @brief   Updates the can see.
     *
     * @param [in,out]  pPacket         If non-null, the packet.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     */
	void UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = nullptr ) const;

    /**
     * @fn  void CObjBase::UpdateObjMessage( lpctstr pTextThem, lpctstr pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;
     *
     * @brief   Updates the object message.
     *
     * @param   pTextThem               The text them.
     * @param   pTextYou                The text you.
     * @param [in,out]  pClientExclude  If non-null, the client exclude.
     * @param   wHue                    The hue.
     * @param   mode                    The mode.
     * @param   font                    The font.
     * @param   bUnicode                true to unicode.
     */
	void UpdateObjMessage( lpctstr pTextThem, lpctstr pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;

    /**
     * @fn  TRIGRET_TYPE CObjBase::OnHearTrigger(CResourceLock &s, lpctstr pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);
     *
     * @brief   Executes the hear trigger action.
     *
     * @param [in,out]  s       The CResourceLock to process.
     * @param   pCmd            The command.
     * @param [in,out]  pSrc    If non-null, source for the.
     * @param [in,out]  mode    The mode.
     * @param   wHue            The hue.
     *
     * @return  A TRIGRET_TYPE.
     */
	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, lpctstr pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

    /**
     * @fn  bool CObjBase::IsContainer() const;
     *
     * @brief   Query if this object is container.
     *
     * @return  true if container, false if not.
     */
	bool IsContainer() const;

    /**
     * @fn  virtual void CObjBase::Update(const CClient * pClientExclude = nullptr) = 0;
     *
     * @brief   send this new item to clients.
     *
     * @param   pClientExclude  Do not send to this CClient.
     */
	virtual void Update(const CClient * pClientExclude = nullptr) = 0;

    /**
     * @fn  virtual void CObjBase::Flip() = 0;
     *
     * @brief   Flips this object.
     */
	virtual void Flip()	= 0;

    /**
     * @fn  virtual bool CObjBase::OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false ) = 0;
     *
     * @brief   Executes the spell effect action.
     *
     * @param   spell               The spell.
     * @param [in,out]  pCharSrc    If non-null, the character source.
     * @param   iSkillLevel         Zero-based index of the skill level.
     * @param [in,out]  pSourceItem If non-null, source item.
     * @param   bReflecting         true to reflecting.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false )
		= 0;

    /**
     * @fn  virtual TRIGRET_TYPE CObjBase::Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );
     *
     * @brief   Spell's trigger (@Effect, @Start...).
     *
     * @param   spell           The spell.
     * @param   stage           The stage.
     * @param [in,out]  pSrc    If non-null, source for the.
     * @param [in,out]  pArgs   If non-null, the arguments.
     *
     * @return  A TRIGRET_TYPE.
     */
	virtual TRIGRET_TYPE Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );

public:
	//	Some global object variables
	int m_ModAr;

#define SU_UPDATE_HITS      0x01    // update hits to others
#define SU_UPDATE_MODE      0x02    // update mode to all
#define SU_UPDATE_TOOLTIP   0x04    // update tooltip to all
	uchar m_fStatusUpdate;  // update flags for next tick

 
protected:
    virtual void _GoAwake() override;
    virtual void _GoSleep() override;

protected:
    /**
     * @brief   Update Status window if any flag requires it on m_fStatusUpdate.
     */
    virtual void OnTickStatusUpdate();

    virtual bool _CanTick() const override;
    //virtual bool  _CanTick() const override;   // Not needed: the right virtual is called by CTimedObj::_CanTick.

public:
    std::vector<std::unique_ptr<CClientTooltip>> m_TooltipData; // Storage for tooltip data while in trigger
protected:
	PacketPropertyList* m_PropertyList;	// currently cached property list packet
	dword m_PropertyHash;				// latest property list hash
	dword m_PropertyRevision;			// current property list revision

public:

    /**
     * @fn  PacketPropertyList* CObjBase::GetPropertyList(void) const
     *
     * @brief   Gets property list.
     *
     * @return  null if it fails, else the property list.
     */
	PacketPropertyList* GetPropertyList(void) const { return m_PropertyList; }

    /**
     * @fn  void CObjBase::SetPropertyList(PacketPropertyList* propertyList);
     *
     * @brief   Sets property list.
     *
     * @param [in,out]  propertyList    If non-null, list of properties.
     */
	void SetPropertyList(PacketPropertyList* propertyList);

    /**
     * @fn  void CObjBase::FreePropertyList(void);
     *
     * @brief   Free property list.
     */
	void FreePropertyList(void);

    /**
     * @fn  dword CObjBase::UpdatePropertyRevision(dword hash);
     *
     * @brief   Updates the property revision described by hash.
     *
     * @param   hash    The hash.
     *
     * @return  The property revision number.
     */
	dword UpdatePropertyRevision(dword hash);

    /**
    * @fn  dword CObjBase::GetPropertyHash() const;
    *
    * @brief   Gets the property revision's hash.
    *
    * @return  The property hash.
    */
    dword GetPropertyHash() const;

    /**
     * @fn  void CObjBase::UpdatePropertyFlag();
     *
     * @brief   Updates the property status update flag.
     */
	void UpdatePropertyFlag();
};


/**
* @enum    MEMORY_TYPE
*
* @brief   Values that represent memory types ( IT_EQ_MEMORY_OBJ ).
*
* Types of memory a CChar has about a game object. (m_wHue)
*/
enum MEMORY_TYPE
{
	MEMORY_NONE = 0,
	MEMORY_SAWCRIME         = 0x0001,	// I saw them commit a crime or i was attacked criminally. I can call the guards on them. the crime may not have been against me.
	MEMORY_IPET             = 0x0002,	// I am a pet. (this link is my master) (never time out)
	MEMORY_FIGHT            = 0x0004,	// Active fight going on now. may not have done any damage. and they may not know they are fighting me.
	MEMORY_IAGGRESSOR       = 0x0008,	// I was the agressor here. (good or evil)
	MEMORY_HARMEDBY         = 0x0010,	// I was harmed by them. (but they may have been retaliating)
	MEMORY_IRRITATEDBY      = 0x0020,	// I saw them snoop from me or someone, or i have been attacked, or someone used Provocation on me.
	MEMORY_SPEAK            = 0x0040,	// We spoke about something at some point. (or was tamed) (NPC_MEM_ACT_TYPE)
	MEMORY_AGGREIVED        = 0x0080,	// I was attacked and was the inocent party here !
	MEMORY_GUARD            = 0x0100,	// Guard this item (never time out)
	MEMORY_LEGACY_ISPAWNED  = 0x0200,	// UNUSED (but keep this for backwards compatibility). I am spawned from this item. (never time out)
	MEMORY_GUILD            = 0x0400,	// This is my guild stone. (never time out) only have 1
	MEMORY_TOWN             = 0x0800,	// This is my town stone. (never time out) only have 1
	MEMORY_UNUSED           = 0x1000,	// UNUSED!!!! I am following this Object (never time out)
	MEMORY_UNUSED2          = 0x2000,	// UNUSED!!!! (MEMORY_WAR_TARG) This is one of my current war targets.
	MEMORY_FRIEND           = 0x4000,	// They can command me but not release me. (not primary blame)
	MEMORY_UNUSED3          = 0x8000	// UNUSED!!!! Gump record memory (More1 = Context, More2 = Uid)
};

enum NPC_MEM_ACT_TYPE	// A simgle primary memory about the object.
{
	NPC_MEM_ACT_NONE = 0,       // we spoke about something non-specific,
	NPC_MEM_ACT_SPEAK_TRAIN,    // I am speaking about training. Waiting for money
	NPC_MEM_ACT_SPEAK_HIRE,     // I am speaking about being hired. Waiting for money
	NPC_MEM_ACT_FIRSTSPEAK,     // I attempted (or could have) to speak to player. but have had no response.
	NPC_MEM_ACT_TAMED,          // I was tamed by this person previously.
	NPC_MEM_ACT_IGNORE          // I looted or looked at and discarded this item (ignore it)
};

enum STONEALIGN_TYPE // Types of Guild/Town stones
{
	STONEALIGN_STANDARD = 0,
	STONEALIGN_ORDER,
	STONEALIGN_CHAOS
};

enum ITRIG_TYPE
{
	// XTRIG_UNKNOWN = some named trigger not on this list.
    ITRIG_ADDREDCANDLE = 1,
    ITRIG_ADDWHITECANDLE,
	ITRIG_AfterClick,
	ITRIG_Buy,
    ITRIG_CarveCorpse,                //I am a corpse and i am going to be carved.
	ITRIG_Click,
	ITRIG_CLIENTTOOLTIP,        // Sending tooltip to client for this item
	ITRIG_CLIENTTOOLTIP_AFTERDEFAULT,
	ITRIG_ContextMenuRequest,   // A context menu was requested over me.
	ITRIG_ContextMenuSelect,    // A context menu option was selected, perform actions.
	ITRIG_Create,               // Item is being created.
	ITRIG_DAMAGE,               // I have been damaged in some way.
	ITRIG_DCLICK,               // I have been dclicked.
	ITRIG_DESTROY,              //+I am nearly destroyed.
	ITRIG_DROPON_CHAR,          // I have been dropped on this char.
	ITRIG_DROPON_GROUND,        // I have been dropped on the ground here.
	ITRIG_DROPON_ITEM,          // An item has been.
	ITRIG_DROPON_SELF,          // An item has been dropped upon me.
	ITRIG_DROPON_TRADE,         // Droping an item in a trade window.
	ITRIG_DYE,
	ITRIG_EQUIP,                // I have been equipped.
	ITRIG_EQUIPTEST,            // I'm not yet equiped, but checking if I can.
	ITRIG_MemoryEquip,          // I'm a memory and I'm being equiped.
	ITRIG_PICKUP_GROUND,        // I'm being picked up from ground.
	ITRIG_PICKUP_PACK,          // picked up from inside some container.
	ITRIG_PICKUP_SELF,          // picked up from this container
	ITRIG_PICKUP_STACK,         // picked up from a stack (ARGO)
    ITRIG_PreSpawn,             // Called before something is spawned, pre-check it's id and validity.
    ITRIG_Redeed,               // Redeeding a multi.
    ITRIG_RegionEnter,          // Ship entering a new region.
    ITRIG_RegionLeave,          // Ship leaving the region.
	ITRIG_Sell,                 // I'm being sold.
	ITRIG_Ship_Turn,            // I'm a ship and i'm turning around.
    ITRIG_Smelt,                // I'm going to be smelt.
    ITRIG_Spawn,                // This spawn is going to generate something.
	ITRIG_SPELLEFFECT,          // cast some spell on me.
    ITRIG_Start,                // Start trigger, right now used only on Champions.
	ITRIG_STEP,                 // I have been walked on. (or shoved)
	ITRIG_TARGON_CANCEL,        // Someone requested me (item) to target, now the targeting was canceled.
	ITRIG_TARGON_CHAR,          // I'm targeting a char.
	ITRIG_TARGON_GROUND,        // I'm targeting the ground.
	ITRIG_TARGON_ITEM,          // I am being combined with an item
	ITRIG_TIMER,                // My timer has expired.
	ITRIG_ToolTip,              // A tooltip is being requested from me.
	ITRIG_UNEQUIP,              // I'm being unequiped.
	ITRIG_QTY
};

enum WAR_SWING_TYPE	// m_Act_War_Swing_State
{
	WAR_SWING_INVALID = -1,
	WAR_SWING_EQUIPPING = 0,	// we are recoiling our weapon.
	WAR_SWING_READY,			// we can swing at any time.
	WAR_SWING_SWINGING,			// we are swinging our weapon.
    //--
    WAR_SWING_EQUIPPING_NOWAIT = 10 // Special return value for CChar::Fight_Hit, DON'T USE IT IN SCRIPTS!
};

enum CTRIG_TYPE : short
{
	CTRIG_AAAUNUSED		= 0,
	CTRIG_AfterClick,       // I'm not yet clicked, name should be generated before.
	CTRIG_Attack,           // I am attacking someone (SRC).
	CTRIG_CallGuards,       // I'm calling guards.
    // Here starts @charXXX section
	CTRIG_charAttack,           // Calling this trigger over other char.
	CTRIG_charClick,            // Calling this trigger over other char.
	CTRIG_charClientTooltip,    // Calling this trigger over other char.
	CTRIG_charClientTooltip_AfterDefault,
	CTRIG_charContextMenuRequest,// Calling this trigger over other char.
	CTRIG_charContextMenuSelect,// Calling this trigger over other char.
	CTRIG_charDClick,           // Calling this trigger over other char.
	CTRIG_charTradeAccepted,    // Calling this trigger over other char.

	CTRIG_Click,            // I got clicked on by someone.
	CTRIG_ClientTooltip,    // Sending tooltips for me to someone.
	CTRIG_ClientTooltip_AfterDefault,
	CTRIG_CombatAdd,        // I add someone to my attacker list.
	CTRIG_CombatDelete,     // delete someone from my list.
	CTRIG_CombatEnd,        // I finished fighting.
	CTRIG_CombatStart,      // I begin fighting.
	CTRIG_ContextMenuRequest,// Context menu requested on me.
	CTRIG_ContextMenuSelect,// An option was selected on the Context Menu.
	CTRIG_Create,           // Newly created (not in the world yet).
    CTRIG_CreateLoot,       // Create the loot (called on death)
	CTRIG_Criminal,         // Called before someone becomes 'gray' for someone.
	CTRIG_DClick,           // Someone has dclicked on me.
	CTRIG_Death,            // I just got killed.
	CTRIG_DeathCorpse,      // A Corpse is being created from my body.
	CTRIG_Destroy,          // I am nearly destroyed.
	CTRIG_Dismount,         // I'm dismounting.
	CTRIG_DYE,
	CTRIG_Eat,              // I'm eating something.
	CTRIG_EnvironChange,    // my environment changed somehow (light,weather,season,region)
	CTRIG_ExpChange,        // EXP is going to change
	CTRIG_ExpLevelChange,   // Experience LEVEL is going to change
	CTRIG_FameChange,       // Fame chaged
	CTRIG_FollowersUpdate,  // Adding or removing CurFollowers.

	CTRIG_GetHit,           // I just got hit.
	CTRIG_Hit,              // I just hit someone. (TARG)
	CTRIG_HitCheck,         // A check made before anything else in the Hit proccess, meant to completelly override combat system.
	CTRIG_HitIgnore,        // I should ignore this target, just giving a record to scripts.
	CTRIG_HitMiss,          // I just missed.
	CTRIG_HitParry,			// I succesfully parried an hit.
	CTRIG_HitTry,           // I am trying to hit someone. starting swing.
    CTRIG_HouseDesignBegin, // Starting to customize.
    CTRIG_HouseDesignCommit, // I committed a new house design
    CTRIG_HouseDesignCommitItem,// I committed an Item to the house design.
	CTRIG_HouseDesignExit,  // I exited house design mode

    // ITRIG_QTY
	CTRIG_itemAfterClick,       // I'm going to click one item.
	CTRIG_itemBuy,              // I'm going to buy one item.
    CTRIG_itemCarveCorpse,            // I am carving a corpse.
	CTRIG_itemClick,            // I clicked one item
	CTRIG_itemClientTooltip,    // Requesting ToolTip for one item.
	CTRIG_itemClientTooltip_AfterDefault,
	CTRIG_itemContextMenuRequest,// Requesting Context Menu.
	CTRIG_itemContextMenuSelect,// Selected one option from Context Menu.
	CTRIG_itemCreate,           // Created one item.
	CTRIG_itemDamage,           // Damaged one item.
	CTRIG_itemDCLICK,           // I have dclicked item.
	CTRIG_itemDestroy,          // Item is nearly destroyed.
	CTRIG_itemDROPON_CHAR,      // I have been dropped on this char.
	CTRIG_itemDROPON_GROUND,    // I dropped an item on the ground.
	CTRIG_itemDROPON_ITEM,      // I have been dropped on this item.
	CTRIG_itemDROPON_SELF,      // I have been dropped on this item.
	CTRIG_itemDROPON_TRADE,     // I have dropped this item on a Trade Window.
	CTRIG_itemEQUIP,            // I have equipped an item.
	CTRIG_itemEQUIPTEST,        // I'm checking if I can equip an item.
	CTRIG_itemMemoryEquip,      // I'm equiping a memory.
	CTRIG_itemPICKUP_GROUND,    // I'm picking up an item.
	CTRIG_itemPICKUP_PACK,      // picked up from inside some container.
	CTRIG_itemPICKUP_SELF,      // picked up from this (ACT) container.
	CTRIG_itemPICKUP_STACK,     // was picked up from a stack.
    CTRIG_itemRedeed,           // was redeeded (multis)
    CTRIG_itemRegionEnter,      // enter a region (ships)
    CTRIG_itemRegionLeave,      // leave a region (ships)
	CTRIG_itemSell,             // I am selling an item.
    CTRIG_itemSmelt,            // I am smelting an item.
	CTRIG_itemSPELL,            // cast some spell on the item.
	CTRIG_itemSTEP,             // stepped on an item
	CTRIG_itemTARGON_CANCEL,    // Canceled a target made from the item.
	CTRIG_itemTARGON_CHAR,      // Targeted a CChar with the item.
	CTRIG_itemTARGON_GROUND,    // Targeted the ground with the item.
	CTRIG_itemTARGON_ITEM,      // Targeted a CItem with the item.
	CTRIG_itemTimer,            // An item I have on is timing out.
	CTRIG_itemToolTip,          // Did tool tips on an item.
	CTRIG_itemUNEQUIP,          // i have unequipped (or try to unequip) an item.

	CTRIG_Jailed,               // I'm up to be send to jail, or to be forgiven.
	CTRIG_KarmaChange,          // Karma chaged
	CTRIG_Kill,         // I have just killed someone.
	CTRIG_LogIn,        // Client logs in.
	CTRIG_LogOut,       // Client logs out (21).
	CTRIG_Mount,        // I'm mounting some char.
	CTRIG_MurderDecay,  // I have decayed one of my kills.
	CTRIG_MurderMark,   // I am gonna to be marked as a murder.
	CTRIG_NotoSend,     // sending notoriety.

	CTRIG_NPCAcceptItem,    // (NPC only) i've been given an item i like (according to DESIRES).
	CTRIG_NPCActFight,      // (NPC only) I have to fight against my target.
	CTRIG_NPCActFollow,     // (NPC only) decided to follow someone.
	CTRIG_NPCAction,        // (NPC only) doing some action.
	CTRIG_NPCActWander,		// (NPC only) i'm wandering aimlessly
	CTRIG_NPCHearGreeting,  // (NPC only) i have been spoken to for the first time. (no memory of previous hearing).
	CTRIG_NPCHearUnknown,   // (NPC only) I heard something i don't understand.
	CTRIG_NPCLookAtChar,    // (NPC only) I found a char, i'm looking at it now.
	CTRIG_NPCLookAtItem,    // (NPC only) I found an item, i'm looking at it now.
	CTRIG_NPCLostTeleport,  // (NPC only) ready to teleport back to spawn.
	CTRIG_NPCRefuseItem,    // (NPC only) i've been given an item i don't want.
	CTRIG_NPCRestock,       // (NPC only) i'm restocking goods.
	CTRIG_NPCSeeNewPlayer,  // (NPC only) i see u for the first time. (in 20 minutes) (check memory time).
	CTRIG_NPCSeeWantItem,   // (NPC only) i see something good.
	CTRIG_NPCSpecialAction, // (NPC only) performing some special actions (spyder's web, dragon's breath...).

	CTRIG_PartyDisband, // I just disbanded my party.
	CTRIG_PartyInvite,  // SRC invited me to join a party, so I may chose.
	CTRIG_PartyLeave,   // I'm leaving this party.
	CTRIG_PartyRemove,  // I have ben removed from the party by SRC.

    CTRIG_PayGold,          // I'm going to give out money for a service (Skill Training, hiring...).
	CTRIG_PersonalSpace,	// i just got stepped on.
	CTRIG_PetDesert,        // I'm deserting from my owner ( starving, being hit by him ...).
	CTRIG_Profile,			// someone hit the profile button for me.
	CTRIG_ReceiveItem,		// I was just handed an item (Not yet checked if i want it).
	CTRIG_RegenStat,		// Hits/mana/stam/food regeneration.

	CTRIG_RegionEnter,          // I'm entering a region.
	CTRIG_RegionLeave,          // I'm leaving a region.
	CTRIG_RegionResourceFound,	// I just discovered a resource.
	CTRIG_RegionResourceGather, // I'm gathering a resource.

	CTRIG_Rename,       // Changing my name or pets one.
	CTRIG_Resurrect,    // I'm going to resurrect via function or spell.
	CTRIG_SeeCrime,     // I am seeing a crime.
	CTRIG_SeeHidden,    // I'm about to see a hidden char.
	CTRIG_SeeSnoop,     // I see someone Snooping something.

	// SKTRIG_QTY
	CTRIG_SkillAbort,       // SKTRIG_ABORT
	CTRIG_SkillChange,      // A skill's value is being changed.
	CTRIG_SkillFail,        // SKTRIG_FAIL
	CTRIG_SkillGain,        // SKTRIG_GAIN
	CTRIG_SkillMakeItem,    // An item is being created by a skill.
	CTRIG_SkillMenu,        // A skillmenu is being called.
	CTRIG_SkillPreStart,    // SKTRIG_PRESTART
	CTRIG_SkillSelect,      // SKTRIG_SELECT
	CTRIG_SkillStart,       // SKTRIG_START
	CTRIG_SkillStroke,      // SKTRIG_STROKE
	CTRIG_SkillSuccess,     // SKTRIG_SUCCESS
	CTRIG_SkillTargetCancel,// SKTRIG_TARGETCANCEL
	CTRIG_SkillUseQuick,    // SKTRIG_USEQUICK
	CTRIG_SkillWait,        // SKTRIG_WAIT

	CTRIG_SpellBook,        // Opening a spellbook
	CTRIG_SpellCast,        // Char is casting a spell.
	CTRIG_SpellEffect,      // A spell just hit me.
    CTRIG_SpellEffectAdd,        // A spell effected me, i'm getting bonus/penalties from it.
    CTRIG_SpellEffectRemove,		// Removing spell item from character.
    CTRIG_SpellEffectTick,  // A spell with SPELLFLAG_TICK just ticked.
	CTRIG_SpellFail,        // The spell failed.
	CTRIG_SpellSelect,      // selected a spell.
	CTRIG_SpellSuccess,     // The spell succeeded.
	CTRIG_SpellTargetCancel,// Cancelled spell target.
	CTRIG_StatChange,       // A stat value is being changed.
	CTRIG_StepStealth,      // Made a step while being in stealth .
	CTRIG_Targon_Cancel,    // Cancel target from TARGETF.
	CTRIG_ToggleFlying,     // Toggle between flying/landing.
	CTRIG_ToolTip,          // someone did tool tips on me.
	CTRIG_TradeAccepted,    // Everything went well, and we are about to exchange trade items
	CTRIG_TradeClose,       // Fired when a Trade Window is being deleted, no returns
	CTRIG_TradeCreate,      // Trade window is going to be created

	// Packet related triggers
	CTRIG_UserBugReport,    // (Client iteraction) Reported a bug
	CTRIG_UserChatButton,   // (Client iteraction) Pressed Chat button.
	CTRIG_UserExtCmd,       // (Client iteraction) Requested an special action (Open Door, cast spell, start skill...).
	CTRIG_UserExWalkLimit,  // (Client iteraction) Walked more than I could.
	CTRIG_UserGuildButton,  // (Client iteraction) Pressed Guild button.
	CTRIG_UserKRToolbar,    // (Client iteraction) Using KR's ToolBar.
	CTRIG_UserMailBag,      // (Client iteraction) Pressed Mail bag.
	CTRIG_UserQuestArrowClick,//(Client iteraction) Clicked ArrowQuest.
	CTRIG_UserQuestButton,  // (Client iteraction) Pressed Quest Button.
	CTRIG_UserSkills,       // (Client iteraction) Opened Skills' dialog.
	CTRIG_UserSpecialMove,  // (Client iteraction) Performing an special action from books.
	CTRIG_UserStats,        // (Client iteraction) Opening Stats' dialog.
    CTRIG_UserUltimaStoreButton,    // (Client iteraction) Using Ultima Store button.
	CTRIG_UserVirtue,       // (Client iteraction) Opening Virtue gump.
	CTRIG_UserVirtueInvoke, // (Client iteraction) Invoquing a Virtue.
	CTRIG_UserWarmode,      // (Client iteraction) Switching between War/Peace.

	CTRIG_QTY
};


/**
 * @brief   Gets dir string.
 * @param   pszDir  The dir.
 * @return  The dir string.
 */
DIR_TYPE GetDirStr( lpctstr pszDir );


#endif // _INC_COBJBASE_H
