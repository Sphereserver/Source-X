/**
* @file CUOTerrainInfo.h
*
*/

#ifndef _INC_CUOTERRAININFO_H
#define _INC_CUOTERRAININFO_H

#include "CUOTerrainTypeRec.h"
#include "uofiles_types.h"


struct CUOTerrainInfo : public CUOTerrainTypeRec_HS
{
    CUOTerrainInfo( TERRAIN_TYPE id, bool fNameNeeded = true );
};


#endif //_INC_CUOTERRAININFO_H
