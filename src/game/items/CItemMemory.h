
#pragma once
#ifndef CITEMMEMORY_H
#define CITEMMEMORY_H

#include "CItem.h"


class CItemMemory : public CItem
{
	// IT_EQ_MEMORY
	// Allow extra tags for the memory
public:
	static const char *m_sClassName;
	CItemMemory( ITEMID_TYPE id, CItemBase * pItemDef );

	virtual ~CItemMemory();

private:
	CItemMemory(const CItemMemory& copy);
	CItemMemory& operator=(const CItemMemory& other);

public:
	WORD SetMemoryTypes( WORD wType );

	WORD GetMemoryTypes() const;

	bool Guild_IsAbbrevOn() const;
	void Guild_SetAbbrev( bool fAbbrevShow );
	WORD Guild_GetVotes() const;
	void Guild_SetVotes( WORD wVotes );
	int Guild_SetLoyalTo( CGrayUID uid );
	CGrayUID Guild_GetLoyalTo() const;
	int Guild_SetTitle( LPCTSTR pszTitle );
	LPCTSTR Guild_GetTitle() const;
	CItemStone * Guild_GetLink();

	virtual int FixWeirdness();
};


#endif // CITEMMEMORY_H
