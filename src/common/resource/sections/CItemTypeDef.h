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

private:
    CItemTypeDef(const CItemTypeDef& copy);
    CItemTypeDef& operator=(const CItemTypeDef& other);

public:
    virtual bool r_LoadVal( CScript & s ) override;
    int GetItemType() const;
};

#endif // _INC_CITEMTYPEDEF_H
