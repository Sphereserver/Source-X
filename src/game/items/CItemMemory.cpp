
#include "../chars/CChar.h"
#include "CItemMemory.h"
#include "CItemStone.h"

CItemMemory::CItemMemory( ITEMID_TYPE id, CItemBase * pItemDef ) :
	CItem( ITEMID_MEMORY, pItemDef )
{
	UNREFERENCED_PARAMETER(id);
}

CItemMemory::~CItemMemory()
{
	DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
}

word CItemMemory::SetMemoryTypes( word wType )	// For memory type objects.
{
	SetHueAlt( wType );
	return( wType );
}

word CItemMemory::GetMemoryTypes() const
{
	return( GetHueAlt());	// MEMORY_FIGHT
}

CItemStone *CItemMemory::Guild_GetLink()
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLink");
	if ( !IsMemoryTypes(MEMORY_TOWN|MEMORY_GUILD) )
		return NULL;
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

int CItemMemory::Guild_SetLoyalTo(CUID uid)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetLoyalTo");
	// Some other place checks to see if this is a valid member.
	return GetTagDefs()->SetNum("LoyalTo", (dword)uid, false);
}

CUID CItemMemory::Guild_GetLoyalTo() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetLoyalTo");
	CItemMemory *pObj = const_cast<CItemMemory *>(this);
	CUID iUid((dword)(pObj->GetTagDefs()->GetKeyNum("LoyalTo", true)));
	return iUid;
}

int CItemMemory::Guild_SetTitle(lpctstr pszTitle)
{
	ADDTOCALLSTACK("CItemMemory::Guild_SetTitle");
	return GetTagDefs()->SetStr("Title", false, pszTitle);
}

lpctstr CItemMemory::Guild_GetTitle() const
{
	ADDTOCALLSTACK("CItemMemory::Guild_GetTitle");
	return m_TagDefs.GetKeyStr("Title", false);
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
	return 0;
}
