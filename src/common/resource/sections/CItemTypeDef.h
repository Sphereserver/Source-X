/**
* @file CItemTypeDef.h
*
*/

#ifndef _INC_CITEMTYPEDEF_H
#define _INC_CITEMTYPEDEF_H

#include "../CResourceLink.h"


class CItemTypeDef : public CResourceLink
{
public:
    static const char *m_sClassName;
    explicit CItemTypeDef( CResourceID rid ) : CResourceLink( rid )
    {
    }

    //CItemTypeDef(CItemTypeDef&&) = default;
    CItemTypeDef(const CItemTypeDef& copy) = delete;
    CItemTypeDef& operator=(const CItemTypeDef& other) = delete;

public:
    virtual bool r_LoadVal( CScript & s ) override;
    int GetItemType() const;
};

#endif // _INC_CITEMTYPEDEF_H
