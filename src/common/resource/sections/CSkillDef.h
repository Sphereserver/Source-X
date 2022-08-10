/**
* @file CSkillDef.h
*
*/

#ifndef _INC_CSKILLDEF_H
#define _INC_CSKILLDEF_H

#include "../../game/uo_files/uofiles_enums.h"
#include "../CResourceLink.h"
#include "../CValueDefs.h"


enum SKTRIG_TYPE
{
    // All skills may be scripted.
    // XTRIG_UNKNOWN    = some named trigger not on this list.
    SKTRIG_ABORT = 1,  	// Some odd thing went wrong.
    SKTRIG_FAIL,        // we failed the skill check.
    SKTRIG_GAIN,        // called when there is a chance to gain skill
    SKTRIG_PRESTART,    // called before any hardcoded messages
    SKTRIG_SELECT,      // just selecting params for the skill
    SKTRIG_START,       // params for skill are done. (stroke)
    SKTRIG_STROKE,      // Not really a trigger! Just a stage.
    SKTRIG_SUCCESS,     // we passed the skill check
    SKTRIG_TARGETCANCEL,// called when a target cursor is cancelled
    SKTRIG_USEQUICK,    // called when a 'quick' usage of the skill is made
    SKTRIG_WAIT,        // called when a test is made to see if the character must wait before starting
    SKTRIG_QTY
};

enum SKF_TYPE
{
    SKF_SCRIPTED = 0x0001,		// fully scripted, no hardcoded behaviour
    SKF_FIGHT = 0x0002,			// considered a fight skill, maintains fight active
    SKF_MAGIC = 0x0004,			// considered a magic skill
    SKF_CRAFT = 0x0008,			// considered a crafting skill, compatible with MAKEITEM function
    SKF_IMMOBILE = 0x0010,		// fails skill if character moves
    SKF_SELECTABLE = 0x0020,	// allows skill to be selected from the skill menu
    SKF_NOMINDIST = 0x0040,		// you can mine, fish, chop, hack on the same point you are standing on
    SKF_NOANIM = 0x0080,		// prevents hardcoded animation from playing
    SKF_NOSFX = 0x0100,			// prevents hardcoded sound from playing
    SKF_RANGED = 0x0200,		// Considered a ranged skill (combine with SKF_FIGHT)
    SKF_GATHER = 0x0400,		// Considered a gathering skill, using SkillStrokes as SKF_CRAFT
    SKF_DISABLED = 0x0800		// Disabled skill, can't be used.
};


/**
* @struct  CSkillDef
*
* @brief   A skill definition.
*          RES_SKILL
*          [SKILL n]
*/
struct CSkillDef : public CResourceLink // For skill def table
{
    static lpctstr const sm_szTrigName[SKTRIG_QTY+1];
    static lpctstr const sm_szLoadKeys[];
private:
    CSString m_sKey;        // script key word for skill.
public:
    CSString m_sTitle;      // title one gets if it's your specialty.
    CSString m_sName;       // fancy skill name
    CSString m_sTargetPrompt;// targetting prompt. (if needed)
    CSString m_sTargetPromptCliloc; //targetting prompt with cliloc support. (if needed)

    CValueCurveDef m_vcDelay; // Delay before skill complete (tenth of seconds).
    CValueCurveDef m_vcEffect;// Effectiveness of the skill, depends on skill.

    // Stat effects.
    // You will tend toward these stat vals if you use this skill a lot.
    byte m_Stat[STAT_BASE_QTY];     // STAT_STR, STAT_INT, STAT_DEX...
    byte m_StatPercent;             // BONUS_STATS = % of success depending on stats
    byte m_StatBonus[STAT_BASE_QTY];// % of each stat toward success at skill, total 100

    CValueCurveDef	m_AdvRate;      // ADV_RATE defined "skill curve" 0 to 100 skill levels.
    CValueCurveDef	m_Values;       // VALUES= influence for items made with 0 to 100 skill levels.
    int				m_GainRadius;	// GAINRADIUS= max. amount of skill above the necessary skill for a task to gain from it.
    int				m_Range;		// RANGE=n Used for SKF_GATHER skills represnting the max distace at which it can be used.

    dword			m_dwFlags;      // Skill Flags.
    dword			m_dwGroup;      // Skill Group.

public:
    explicit CSkillDef( SKILL_TYPE iSkill );
    virtual ~CSkillDef() = default;

private:
    CSkillDef(const CSkillDef& copy);
    CSkillDef& operator=(const CSkillDef& other);

public:

    /**
    * @fn  lpctstr GetKey() const
    *
    * @brief   Gets the key (ALCHEMY, BOWCRAFT...).
    *
    * @return  The key name.
    */
    lpctstr GetKey() const
    {
        return m_sKey;
    }

    lpctstr GetName() const
    {
        return GetKey();
    }
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
};

#endif // _INC_CSKILLDEF_H
