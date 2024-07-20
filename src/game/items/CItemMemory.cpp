
#include "../chars/CChar.h"
#include "../components/CCSpawn.h"
#include "CItemMemory.h"
#include "CItemStone.h"

CItemMemory::CItemMemory( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
	CItem( ITEMID_MEMORY, pItemDef )
{
	UnreferencedParameter(id);
}

word CItemMemory::SetMemoryTypes( word wType )	// For memory type objects.
{
	SetHueQuick( wType );
	return( wType );
}

word CItemMemory::GetMemoryTypes() const
{
	return GetHue();	// MEMORY_FIGHT
}

CItemStone *CItemMemory::Guild_GetLink()
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLink");
	if ( !IsMemoryTypes(MEMORY_TOWN|MEMORY_GUILD) )
		return nullptr;
	return dynamic_cast<CItemStone*>(m_uidLink.ItemFind());
}

bool CItemMemory::Guild_IsAbbrevOn() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_IsAbbrevOn");
	return (m_itEqMemory.m_Action != 0);
}

void CItemMemory::Guild_SetAbbrev(bool fAbbrevShow)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetAbbrev");
	m_itEqMemory.m_Action = fAbbrevShow;
}

word CItemMemory::Guild_GetVotes() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetVotes");
	return m_itEqMemory.m_Skill;
}

void CItemMemory::Guild_SetVotes(word wVotes)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetVotes");
	m_itEqMemory.m_Skill = wVotes;
}

bool CItemMemory::Guild_SetLoyalTo(CUID uid)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetLoyalTo");
	// Some other place checks to see if this is a valid member.
	return (m_TagDefs.SetNum("LoyalTo", (dword)uid, false) != nullptr) ? true : false;
}

CUID CItemMemory::Guild_GetLoyalTo() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLoyalTo");
	CUID iUid((dword)(GetKeyNum("LoyalTo", true)));
	return iUid;
}

bool CItemMemory::Guild_SetTitle(lpctstr pszTitle)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetTitle");
	return (m_TagDefs.SetStr("Title", false, pszTitle) != nullptr) ? true : false;
}

lpctstr CItemMemory::Guild_GetTitle() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetTitle");
	return GetKeyStr("Title", false);
}

int CItemMemory::FixWeirdness()
{
	ADDTOCALLSTACK("CItemMemory::FixWeirdness");
	int iResultCode = CItem::FixWeirdness();
	if ( iResultCode )
		return iResultCode;

	if ( !IsItemEquipped() || GetEquipLayer() != LAYER_SPECIAL || !GetMemoryTypes() )	// has to be a memory of some sort.
	{
		iResultCode = 0x4222;
		return iResultCode;	// get rid of it.
	}

	CChar *pChar = dynamic_cast<CChar*>(GetParent());
	if ( !pChar )
	{
		iResultCode = 0x4223;
		return iResultCode;	// get rid of it.
	}

    // Automatic transition from old to new spawn engine
    dword dwFlags = GetHue();
    if (dwFlags & MEMORY_LEGACY_ISPAWNED)
    {
        CCSpawn *pSpawn = static_cast<CCSpawn*>(m_uidLink.ItemFind()->GetComponent(COMP_SPAWN));
        if (pSpawn && pChar)
        {
            pSpawn->AddObj(pChar->GetUID());
        }
        iResultCode = 0x4226;
        return iResultCode;
    }

	// If it is my guild make sure I am linked to it correctly !
	if ( IsMemoryTypes(MEMORY_GUILD|MEMORY_TOWN) )
	{
		const CItemStone *pStone = pChar->Guild_Find(static_cast<MEMORY_TYPE>(GetMemoryTypes()));
		if ( !pStone || pStone->GetUID() != m_uidLink )
		{
			iResultCode = 0x4224;
			return iResultCode;	// get rid of it.
		}
		if ( !pStone->GetMember(pChar) )
		{
			iResultCode = 0x4225;
			return iResultCode;	// get rid of it.
		}
	}

	// Make sure guard memories are linked correctly (this is not an ERROR, just make the item decay on next tick)
	if ( IsMemoryTypes(MEMORY_GUARD) && !m_uidLink.ObjFind() )
	{
		SetAttr(ATTR_DECAY);
		_SetTimeout(0);
	}

	return 0;
}
