/**
* @file CContainer.h
*
*/

#pragma once
#ifndef _INC_CCONTAINER_H
#define _INC_CCONTAINER_H

#include "../common/sphere_library/CSArray.h"
#include "../common/CUID.h"
#include "../common/CRect.h"
#include "../common/CResourceBase.h"


class CItemContainer;

class CContainer : public CSObjList	// This class contains a list of items but may or may not be an item itself.
{
public:
	int	m_totalweight;	// weight of all the items it has. (1/WEIGHT_UNITS pound)
	virtual void OnRemoveObj( CSObjListRec* pObRec );	// Override this = called when removed from list.
	void ContentAddPrivate( CItem * pItem );

	void r_WriteContent( CScript & s ) const;

	bool r_WriteValContainer(lpctstr pszKey, CSString &sVal, CTextConsole *pSrc);
	bool r_GetRefContainer( lpctstr & pszKey, CScriptObj * & pRef );

public:
	static const char *m_sClassName;
	CContainer();
	virtual ~CContainer();

private:
	CContainer(const CContainer& copy);
	CContainer& operator=(const CContainer& other);

public:
	CItem * GetAt( size_t index ) const;
	int	GetTotalWeight() const;
	CItem* GetContentHead() const;
	CItem* GetContentTail() const;
	int FixWeight();

	bool ContentFindKeyFor( CItem * pLocked ) const;
	// bool IsItemInside( CItem * pItem ) const;

	CItem * ContentFindRandom() const;
	void ContentsDump( const CPointMap & pt, dword dwAttr = 0 );
	void ContentsTransfer( CItemContainer * pCont, bool fNoNewbie );
	void ContentAttrMod( dword dwAttr, bool fSet );
	void ContentNotifyDelete();

	// For resource usage and gold.
	CItem * ContentFind( RESOURCE_ID_BASE rid, dword dwArg = 0, int iDecendLevels = 255 ) const;
	TRIGRET_TYPE OnContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, RESOURCE_ID_BASE rid, dword dwArg = 0, int iDecendLevels = 255 );
	TRIGRET_TYPE OnGenericContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, int iDecendLevels = 255 );
	int ContentCount( RESOURCE_ID_BASE rid, dword dwArg = 0 );
	size_t ContentCountAll() const;
	int ContentConsume( RESOURCE_ID_BASE rid, int iQty = 1, bool fTest = false, dword dwArg = 0 );

	int ResourceConsume( const CResourceQtyArray * pResources, int iReplicationQty, bool fTest = false, dword dwArg = 0 );
	size_t ResourceConsumePart( const CResourceQtyArray * pResources, int iReplicationQty, int iFailPercent, bool fTest = false, dword dwArg = 0 );

	virtual void OnWeightChange( int iChange );
	virtual void ContentAdd( CItem * pItem ) = 0;
};


#endif // _INC_CCONTAINER_H
