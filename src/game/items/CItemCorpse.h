/**
* @file CItemCorpse.h
*
*/

#ifndef _INC_CITEMCORPSE_H
#define _INC_CITEMCORPSE_H

#include "../CBase.h"
#include "CItemContainer.h"


class CChar;

class CItemCorpse : public CItemContainer
{
	// IT_CORPSE
	// A corpse is a special type of item.
public:
	static const char *m_sClassName;
	CItemCorpse( ITEMID_TYPE id, CItemBase * pItemDef );
	virtual ~CItemCorpse();

private:
	CItemCorpse(const CItemCorpse& copy);
	CItemCorpse& operator=(const CItemCorpse& other);

public:
	bool IsCorpseResurrectable(CChar* pCharHealer, CChar* pCharGhost) const;
	CChar * IsCorpseSleeping() const;
	int GetWeight(word amount = 0) const;
};


#endif // _INC_CITEMCORPSE_H
