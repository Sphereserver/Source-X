#include "CUOStaticItemRec.h"

ITEMID_TYPE CUOStaticItemRec::GetDispID() const
{
    return static_cast<ITEMID_TYPE>(m_wTileID);
}
