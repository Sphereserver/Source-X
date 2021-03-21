/**
* @file CBase.h
*
*/

#ifndef _INC_CBASE_H
#define _INC_CBASE_H

#include "../common/resource/CResourceBase.h"
#include "../common/resource/CResourceRef.h"
#include "../common/CVarDefMap.h"
#include "../common/sphere_library/CSString.h"
#include "uo_files/uofiles_types.h"
#include "components/CCFaction.h"
#include "CEntityProps.h"

struct CBaseBaseDef : public CResourceLink, public CEntityProps
{
	// Minimal amount of common info to define RES_ITEMDEF or RES_CHARDEF, (it might just be a DUPE)
	// The unique index id.	(WILL not be the same as artwork if outside artwork range)

	// TAGS
	static lpctstr const sm_szLoadKeys[];
	// Base type of both CItemBase and CCharBase

protected:
	dword       m_dwDispIndex;	// The base artwork id. (may be the same as GetResourceID() in base set.) but can also be "flipped"
	CSString    m_sName;		// default type name. (ei, "human" vs specific "Dennis")

private:
    dword       _dwInstances;   // How many CResourceRef objects refer to this ?

	height_t    m_Height;       // Height of the object.
	// -------------- ResLevel -------------
    byte        m_Expansion;
	byte        m_ResLevel;     // ResLevel required for players to see me
	HUE_TYPE    m_ResDispDnHue; // Hue shown to players who don't have my ResLevel
	word        m_ResDispDnId;  // ID shown to players who don't have my ResLevel
	// -------------------------------------

public:
	CVarDefMap  m_TagDefs;  // TAGs storage
	CVarDefMap  m_BaseDefs; // New Variable storage system

	// When events happen to the char. check here for reaction.
	CResourceRefArray   m_TEvents;          // Action or motivation type indexes. (NPC only)
	CResourceQtyArray   m_BaseResources;    // RESOURCES=10 MEAT (What is this made of)

	word    m_attackBase;   // base attack for weapons/chars. not magic plus.
	word    m_attackRange;  // variable range of attack damage.

	word    m_defenseBase;  // base defense bonus given by this (items)/having (chars).
	word    m_defenseRange; // variable range of defense.

	dword   m_Can;          // Base attribute flags. CAN_C_GHOST, etc
    RESDISPLAY_VERSION _iEraLimitProps;	// Don't allow to have properties newer than the given era.

    CFactionDef _pFaction;


public:
    CFactionDef GetFaction();

    /**
     * @brief   Gets definition string.
     * @param   ptcKey  The key.
     * @param   fZero   true to zero.
     * @return  The definition string.
     */
	lpctstr GetDefStr( lpctstr ptcKey, bool fZero = false ) const
	{
		return m_BaseDefs.GetKeyStr( ptcKey, fZero );
	}

    /**
     * @brief   Gets definition number.
     * @param   ptcKey  The key.
     * @return  The definition number.
     */
	int64 GetDefNum( lpctstr ptcKey ) const
	{
		return m_BaseDefs.GetKeyNum( ptcKey );
	}

    /**
     * @brief   Sets definition number.
     * @param   ptcKey      The key.
     * @param   iVal        Zero-based index of the value.
     * @param   fDeleteZero true to zero.
     */
	void SetDefNum(lpctstr ptcKey, int64 iVal, bool fDeleteZero = true, bool fWarnOverwrite = true)
	{
		m_BaseDefs.SetNum(ptcKey, iVal, fDeleteZero, fWarnOverwrite);
	}

    /**
     * @brief   Sets definition string.
     * @param   ptcKey      The key.
     * @param   pszVal      The value.
     * @param   fQuoted     true if quoted.
     * @param   fDeleteZero true to zero.
     */
	void SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted = false, bool fDeleteZero = true, bool fWarnOverwrite = true)
	{
		m_BaseDefs.SetStr(ptcKey, fQuoted, pszVal, fDeleteZero, fWarnOverwrite);
	}

    /**
     * @brief   Deletes the definition described by ptcKey.
     * @param   ptcKey  The key.
     */
	void DeleteDef(lpctstr ptcKey)
	{
		m_BaseDefs.DeleteKey(ptcKey);
	}

// Map Movement flags.
#define CAN_C_GHOST         0x0001  // Moves thru doors etc.
#define CAN_C_SWIM          0x0002	// dolphin, elemental or is water.
#define CAN_C_WALK          0x0004	// Can walk on land, climbed on walked over else Frozen by nature(Corpser).
#define CAN_C_PASSWALLS     0x0008	// Walk thru walls.
#define CAN_C_FLY           0x0010	// Can walk to a point with a much higher height difference from our current z.
#define CAN_C_FIRE_IMMUNE   0x0020	// Has some immunity to fire ? (will walk into it (lava)).
#define CAN_C_NOINDOORS     0x0040	// Can't go under roof.
#define CAN_C_HOVER         0x0080	// Can hover.

#define CAN_I_DOOR          0x0001	// Is a door UFLAG4_DOOR.
#define CAN_I_WATER			0x0002  // Need to swim in it. UFLAG1_WATER.
#define CAN_I_PLATFORM      0x0004  // we can walk on top of it. (even tho the item itself might block) UFLAG2_PLATFORM.
#define CAN_I_BLOCK         0x0008  // need to walk thru walls or fly over. UFLAG1_BLOCK.
#define CAN_I_CLIMB         0x0010  // Item: step up on it, UFLAG2_CLIMBABLE.
#define CAN_I_FIRE          0x0020  // Is a fire. Usually blocks as well. UFLAG1_DAMAGE.
#define CAN_I_ROOF          0x0040  // We are under a roof. can't rain on us. UFLAG4_ROOF.
#define CAN_I_HOVER         0x0080  // We are hovering. UFLAG4_HOVEROVER.

// CItemBase specific defs.
#define CAN_I_PILE              0x0100      // Can item be piled UFLAG2_STACKABLE (*.mul).
#define CAN_I_DYE               0x0200      // Can item be dyed UFLAG3_CLOTH? (sort of).
#define CAN_I_FLIP              0x0400      // will flip by default.
#define CAN_I_LIGHT             0x0800      // UFLAG3_LIGHT.
#define CAN_I_REPAIR            0x1000      // Is it repairable (difficulty based on value).
#define CAN_I_REPLICATE         0x2000      // Things like arrows are pretty much all the same.
#define CAN_I_DCIGNORELOS       0x4000      // when dclicked, ignore LOS checks.
#define CAN_I_DCIGNOREDIST      0x8000      // when dclicked, ignore distance checks.
#define CAN_I_BLOCKLOS          0x10000     // blocks LOS without blocking walkchecks.
#define CAN_I_EXCEPTIONAL       0x20000     // can items be exceptional.    -> TODO: convert to property, to free limited attr values
#define CAN_I_MAKERSMARK        0x40000     // can items hold makers mark.  -> TODO: convert to property, to free limited attr values
#define CAN_I_RETAINCOLOR       0x80000     // can items retain material colors.-> TODO: convert to property, to free limited attr values
#define CAN_I_ENCHANT           0x100000    // can items be enchanted (runic).  -> TODO: convert to property, to free limited attr values
#define CAN_I_IMBUE             0x200000    // can items be imbued (imbuing).   -> TODO: convert to property, to free limited attr values
#define CAN_I_RECYCLE           0x400000    // Can items be recycled.           -> TODO: convert to property, to free limited attr values
#define CAN_I_REFORGE           0x800000    // Can items be Runic Reforged.     -> TODO: convert to property, to free limited attr values
#define CAN_I_FORCEDC           0x1000000   // Can force DClick skipping other checks (LOS,Distance, Cont...).
#define CAN_I_DAMAGEABLE	    0x2000000	// Display item health bar on HS clients >= 7.0.30.0 (MORE1L = cur hitpoints / MORE1H = max hitpoints)
#define CAN_I_BLOCKLOS_HEIGHT   0x4000000   // blocks LOS without blocking walkchecks, but only if the item is too high for the viewer.

// (CItemBase) CanEquip specific defs.
#define CAN_U_ALL           0x000       // Can be used by everyone.
#define CAN_U_MALE          0x001       // Can be used by males.
#define CAN_U_FEMALE        0x002       // Can be used by females.
#define CAN_U_HUMAN         0x004       // Can be used by humans.
#define CAN_U_ELF           0x008       // Can be used by elfs.
#define CAN_U_GARGOYLE      0x010       // Can be used by gargoyles.
#define CAN_U_NONE          0x020       // Can not be used.

// CCharBase specific defs.
#define CAN_C_EQUIP         0x00100 // Can equip stuff. (humanoid).
#define CAN_C_USEHANDS      0x00200	// Can wield weapons (INT dependant), open doors ?, pick up/manipulate things.
#define CAN_C_MOUNT         0x00400	// can mount rides.
#define CAN_C_FEMALE        0x00800	// It is female by nature.
#define CAN_C_NONHUMANOID   0x01000	// Body type for combat messages.
#define CAN_C_RUN           0x02000	// Can run (not needed if they can fly).
#define CAN_C_DCIGNORELOS   0x04000	// when dclicking sth., ignore LOS checks.
#define CAN_C_DCIGNOREDIST  0x08000	// when dclicking sth., ignore distance checks.
#define CAN_C_NONMOVER      0x10000 // Just stay in place, avoid movement actions.
#define CAN_C_NOBLOCKHEIGHT 0x20000 // Do not consider char's height when checking if it can move to a point
#define CAN_C_STATUE        0x40000 // Statue: do not move, PacketStatueAnimation with tag.statue_animation and tag.statue_frame
#define CAN_C_NONSELECTABLE 0x80000 // Can't be selected/targeted (useful for chars with mt_statue which need to act as an item)

// Special flags common to items and chars (O = Object, generic)
#define CAN_O_NOSLEEP       0x10000000

// masks for movement-affecting flags
#define CAN_C_MOVEMENTCAPABLEMASK (CAN_C_SWIM| CAN_C_WALK|CAN_C_FLY|CAN_C_RUN|CAN_C_HOVER)
#define CAN_C_MOVEMASK      (CAN_C_GHOST|CAN_C_SWIM|CAN_C_WALK|CAN_C_PASSWALLS|CAN_C_FLY|CAN_C_FIRE_IMMUNE|CAN_C_NOINDOORS|CAN_C_HOVER|CAN_C_NOBLOCKHEIGHT)
#define CAN_I_MOVEMASK      (CAN_I_DOOR|CAN_I_WATER|CAN_I_PLATFORM|CAN_I_BLOCK|CAN_I_CLIMB|CAN_I_FIRE|CAN_I_ROOF|CAN_I_HOVER)


public:
	CBaseBaseDef( CResourceID id );
	virtual ~CBaseBaseDef();

private:
	CBaseBaseDef(const CBaseBaseDef& copy);
	CBaseBaseDef& operator=(const CBaseBaseDef& other);

public:
    /**
     * @brief   Gets the instances number.
     * @return  Count.
     */
    inline dword GetInstances() const noexcept
    {
        return _dwInstances;
    }

    inline void AddInstance() noexcept
    {
        ++_dwInstances;
    }

    void DelInstance();

    /**
     * @brief   Gets type name.
     * @return  The type name.
     */
	lpctstr GetTypeName() const;

    /**
     * @brief   Gets the name.
     * @return  The name.
     */
	virtual lpctstr GetName() const;

    /**
     * @brief   Query if this object has type name.
     * @return  true if type name, false if not.
     */
	bool HasTypeName() const;

    /**
     * @brief   Sets type name.
     * @param   pszName The name.
     */
	virtual void SetTypeName( lpctstr pszName );

    /**
     * @brief   Has the given Can flag.
     * @param   dwCan    The can flags.
     * @return  true if it succeeds, false if it fails.
     */
	bool Can( dword dwCan ) const;

    /**
     * @brief   Un link.
     */
	virtual void UnLink() override;

    //virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual bool r_LoadVal( CScript & s ) override;

    /**
     * @brief   Query if this object is valid.
     * @return  true if valid, false if not.
     */

	bool IsValid() const;

    /**
     * @brief   Gets the height.
     * @return  The height.
     */
	height_t GetHeight() const;

    /**
     * @brief   Sets a height.
     * @param   Height  The height.
     */
	void SetHeight( height_t Height );

	// -------------- ResLevel -------------

    /**
     * @brief   Gets ResLevel.
     * @return  The ResLevel.
     */
	byte GetResLevel() const;

    /**
     * @brief   Sets ResLevel.
     * @param   ResLevel    The ResLevel.
     * @return  true if it succeeds, false if it fails.
     */
	bool SetResLevel( byte ResLevel );

    /**
     * @brief   Gets ResDispDNHue.
     * @return  The ResDispDNHue.
     */
	HUE_TYPE GetResDispDnHue() const;

    /**
     * @brief   Sets ResDispDNHue.
     * @param   ResDispDnHue    The ResDispDNHue.
     */
	void SetResDispDnHue( HUE_TYPE ResDispDnHue );

    /**
     * @brief   Gets ResDispDnId.
     * @return  The ResDispDnId.
     */
	word GetResDispDnId() const;

    /**
     * @brief   Sets ResDispDnId.
     * @param   ResDispDnId Identifier for ResDispDnId.
     */
	void SetResDispDnId( word ResDispDnId );
	// -------------------------------------

    /**
     * @brief   Copies the basic described by pSrc.
     * @param   pSrc    Source for the.
     */
	void CopyBasic( const CBaseBaseDef * pSrc );

    /**
     * @brief   Copies and transfer what is described by pSrc.
     * @param   pSrc    If non-null, source for the.
     */
	void CopyTransfer( CBaseBaseDef * pSrc );


    /**
    * @brief   Returns the internal numeric range value from the string, used for value assignment.
    * @return  Value.
    */
    static ushort ConvertRangeStr(lpctstr ptcRange);
#define RANGE_MAKE(iHi, iLo)    (((iHi & 0xFF) << 8) | (iLo & 0xFF))   // Highest byte contains highest value, lowest contains the lowest value
#define RANGE_GET_HI(iRange)    ((iRange >> 8) & 0xFF)
#define RANGE_GET_LO(iRange)    (iRange & 0xFF)
};

#endif // _INC_CBASE_H
