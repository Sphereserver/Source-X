/**
* @file CSkillClassDef.h
*
*/

#ifndef _INC_SKILLCLASSDEF_H
#define _INC_SKILLCLASSDEF_H

#include "../../../game/uo_files/uofiles_enums.h"
#include "../../sphere_library/CSString.h"
#include "../CResourceLink.h"


/**
* @class   CSkillClassDef
*
* @brief   A skill class definition.
*          RES_SKILLCLASS
*          [SKILLCLASS n]
*/
class CSkillClassDef : public CResourceLink // For skill def table
{
    static lpctstr const sm_szLoadKeys[];
public:
    static const char *m_sClassName;
    CSString m_sName;	// The name of this skill class.

    uint    m_StatSumMax;	// Max Stat sum.
    uint    m_SkillSumMax;	// Max Skill sum.

    ushort m_StatMax[STAT_BASE_QTY];      // Max Stat value.
    ushort m_SkillLevelMax[ SKILL_QTY ];  // Max Skill Value.

private:
    void Init();
public:
    explicit CSkillClassDef( CResourceID rid ) : CResourceLink( rid )
    {
        // If there was none defined in scripts.
        Init();
    }
    virtual ~CSkillClassDef() = default;

private:
    CSkillClassDef(const CSkillClassDef& copy);
    CSkillClassDef& operator=(const CSkillClassDef& other);

public:
    lpctstr GetName() const { return( m_sName ); }

    bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
    bool r_LoadVal( CScript & s ) override;
};

#endif // _INC_SKILLCLASSDEF_H
