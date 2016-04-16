#include "CUOMultiItemRec.h"

ITEMID_TYPE CUOMultiItemRec::GetDispID() const
{
    return static_cast<ITEMID_TYPE>(m_wTileID);
}

ITEMID_TYPE CUOMultiItemRec_HS::GetDispID() const
{
	return static_cast<ITEMID_TYPE>(m_wTileID);
}
