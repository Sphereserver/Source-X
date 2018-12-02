/**
* @file CRegionResourceDef.h
*
*/

#ifndef _INC_CREGIONRESOURCEDEF_H
#define _INC_CREGIONRESOURCEDEF_H

#include "../../game/uo_files/uofiles_enums_itemid.h"
#include "../CValueDefs.h"
#include "../CResourceLink.h"


/**
* @enum    RRTRIG_TYPE
*
* @brief   Region triggers
*/
enum RRTRIG_TYPE
{
    // XTRIG_UNKNOWN	= some named trigger not on this list.
    RRTRIG_RESOURCEFOUND=1,
    RRTRIG_RESOURCEGATHER,
    RRTRIG_RESOURCETEST,
    RRTRIG_QTY
};

/**
* @class   CRegionResourceDef
*
* @brief   A region resource definition. When mining/lumberjacking etc. What can we get?
*          RES_REGIONRESOURCE
*          [REGIONRESOURCE xx]
*/

class CRegionResourceDef : public CResourceLink
{
public:
    static const char *m_sClassName;
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szTrigName[RRTRIG_QTY+1];

    ITEMID_TYPE m_ReapItem;         // What item do we get when we try to mine this. ITEMID_ORE_1 most likely
    CValueCurveDef m_ReapAmount;    // How much can we reap at one time (based on skill)

    CValueCurveDef m_Amount;        // How is here total
    CValueCurveDef m_Skill;         // Skill levels required to mine this.
    CValueCurveDef m_iRegenerateTime;// tenth of seconds once found how long to regen this type.

public:
    explicit CRegionResourceDef( CResourceID rid );
    virtual ~CRegionResourceDef();

private:
    CRegionResourceDef(const CRegionResourceDef& copy);
    CRegionResourceDef& operator=(const CRegionResourceDef& other);

public:
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc = nullptr ) override;
    TRIGRET_TYPE OnTrigger( lpctstr pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs );
};

#endif // _INC_CREGIONRESOURCEDEF_H
