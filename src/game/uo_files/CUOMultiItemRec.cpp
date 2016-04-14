#include "CUOMultiItemRec.h"

ITEMID_TYPE CUOMultiItemRec::GetDispID() const
{
    return static_cast<ITEMID_TYPE>(m_wTileID);
}
