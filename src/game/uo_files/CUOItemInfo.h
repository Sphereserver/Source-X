/**
* @file CUOItemInfo.h
*
*/

#ifndef _INC_CUOITEMINFO_H
#define _INC_CUOITEMINFO_H


#include "CUOItemTypeRec.h"
#include "uofiles_enums_itemid.h"

struct CUOItemInfo : public CUOItemTypeRec_HS
{
    explicit CUOItemInfo( ITEMID_TYPE id, bool fNameNotNeeded = false);
};


#endif // _INC_CUOITEMINFO_H
