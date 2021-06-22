/**
* @file CSpellDef.h
*
*/

#ifndef _INC_CSPELLDEF_H
#define _INC_CSPELLDEF_H

#include "../../game/uo_files/uofiles_enums_itemid.h"
#include "../../game/uo_files/uofiles_types.h"
#include "../CResourceQty.h"
#include "../CResourceLink.h"


/**
* @enum    SPTRIG_TYPE
*
* @brief   Values that represent spell trigger types.
*/
enum SPTRIG_TYPE
{
    SPTRIG_EFFECT	= 1,
    SPTRIG_EFFECTADD,
    SPTRIG_EFFECTREMOVE,
    SPTRIG_EFFECTTICK,
    SPTRIG_FAIL,
    SPTRIG_SELECT,
    SPTRIG_START,
    SPTRIG_SUCCESS,
    SPTRIG_TARGETCANCEL,
    SPTRIG_QTY
};

/**
* @class   CSpellDef
*
* @brief   A spell definition.
*          RES_SPELL
*          [SPELL n]
*
*/
class CSpellDef : public CResourceLink
{
private:
    dword	m_dwFlags;  // Spell Flags.
    dword	m_dwGroup;  // Spell group.

    CSString m_sName;	// Spell name

public:
    static const char *m_sClassName;
    static lpctstr const sm_szTrigName[SPTRIG_QTY+1];
    static lpctstr const sm_szLoadKeys[];
    CSString m_sTargetPrompt;       // targetting prompt. (if needed)
    SOUND_TYPE m_sound;             // What noise does it make when done.
    CSString m_sRunes;              // Letter Runes for Words of power.
    CResourceQtyArray m_Reags;      // What reagents does it take ?
    CResourceQtyArray m_SkillReq;   // What skills/unused reagents does it need to cast.
    ITEMID_TYPE m_idSpell;          // The rune graphic for this.
    ITEMID_TYPE m_idScroll;         // The scroll graphic item for this.
    ITEMID_TYPE m_idEffect;         // Animation effect ID
    word m_wManaUse;                // How much mana does it need.
    word m_wTithingUse;             // Tithing points required for this spell.
    LAYER_TYPE m_idLayer;           // Where the layer buff/debuff/data is stored.

    CValueCurveDef m_CastTime;      // In tenths of second.
    CValueCurveDef m_vcEffect;        // Damage or effect level based on skill of caster.100% magery
    CValueCurveDef m_Duration;      // length of effect. in tenth of second
    CValueCurveDef m_Interrupt;     // chance to interrupt a spell

public:

    /**
    * @fn  bool CSpellDef::IsSpellType( dword wFlags ) const
    *
    * @brief   Check if this Spell has the given flags.
    *
    * @param   wFlags  The flags.
    *
    * @return  true if match, false if not.
    */
    bool IsSpellType( dword wFlags ) const
    {
        return (( m_dwFlags & wFlags ) ? true : false );
    }

public:
    explicit CSpellDef( SPELL_TYPE id );
    virtual ~CSpellDef() = default;

private:
    CSpellDef(const CSpellDef& copy);
    CSpellDef& operator=(const CSpellDef& other);

public:
    lpctstr GetName() const { return( m_sName ); }
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;

    bool GetPrimarySkill( int * piSkill = nullptr, int * piQty = nullptr ) const;
};

#endif // _INC_CSPELLDEF_H
