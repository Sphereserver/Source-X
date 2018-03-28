#include "CUOMultiItemRec.h"

ITEMID_TYPE CUOMultiItemRec::GetDispID() const
{
    return (ITEMID_TYPE)(m_wTileID);
}

ITEMID_TYPE CUOMultiItemRec_HS::GetDispID() const
{
	return (ITEMID_TYPE)(m_wTileID);
}
