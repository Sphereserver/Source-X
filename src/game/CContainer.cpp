
#include "../common/CException.h"
#include "../common/CUID.h"
#include "../network/send.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "items/CItemContainer.h"
#include "items/CItemVendable.h"
#include "CObjBaseTemplate.h"
#include "triggers.h"
#include "CContainer.h"


CContainer::CContainer() :
	m_totalweight(0)
{
}


void CContainer::_GoAwake()
{
	ADDTOCALLSTACK("CContainer::_GoAwake");
	for (CSObjContRec* pObjRec : GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		//std::unique_lock<std::shared_mutex> lock(pItem->THREAD_CMUTEX);
		if (pItem->IsSleeping())
			pItem->GoAwake();
	}
}

void CContainer::_GoSleep()
{
	ADDTOCALLSTACK("CContainer::_GoSleep");
	for (CSObjContRec* pObjRec : GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		//std::unique_lock<std::shared_mutex> lock(pItem->THREAD_CMUTEX);
		if (!pItem->IsSleeping())
			pItem->GoSleep();
	}
}


void CContainer::ContentDelete(bool fForce)
{
    ADDTOCALLSTACK("CContainer::ContentDelete");
    if (_Contents.empty())
        return;

    // delete all entries.
    ASSERT(!_fIsClearing);
    _fIsClearing = true;

    // Loop through a copy of the current state of the container, since by deleting other container objects it could happen that
    //	other objects are deleted and appended to this list, thus invalidating the iterators used by the for loop.
    const auto stateCopy = GetIterationSafeContReverse();
    _Contents.clear();

    for (CSObjContRec* pRec : stateCopy)	// iterate the list.
    {
		EXC_TRY("Scheduling objects for deletion");

        ASSERT(pRec->GetParent() == this);
        CItem* pItem = static_cast<CItem*>(pRec);
        pItem->Delete(fForce);

		EXC_CATCH;
    }

    _fIsClearing = false;
}

void CContainer::ContentNotifyDelete()
{
	ADDTOCALLSTACK("CContainer::ContentNotifyDelete");
	if (!IsTrigUsed(TRIGGER_DESTROY)) // no point entering this loop if the trigger is not used
		return;

	// trigger @Destroy on contained items
	for (size_t i = 0; i < GetContentCount(); )
	{
		CItem* pItem = static_cast<CItem*>(GetContentIndex(i));
		bool fIncreaseIndex = true;
		if (!pItem->NotifyDelete())
		{
			// item shouldn't be destroyed and so cannot remain in this container,
			// drop it to the ground if it hasn't been moved already
			if (pItem->GetParent() == this)
			{
				fIncreaseIndex = !pItem->MoveToCheck(pItem->GetTopLevelObj()->GetTopPoint());
			}
		}

		if (fIncreaseIndex)
		{
			++i;
		}
	}
}

void CContainer::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CContainer::OnWeightChange");
	// Propagate the weight change up the stack if there is one.
	m_totalweight += iChange;
}

int	CContainer::GetTotalWeight() const
{
	return m_totalweight;
}

int CContainer::FixWeight()
{
	ADDTOCALLSTACK("CContainer::FixWeight");
	// If there is some sort of ASSERT during item add then this is used to fix it.
	m_totalweight = 0;

	for ( CSObjContRec *pObjRec : *this )
	{
		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pObjRec);
		if (!pCont)
		{
			//For every non-container item inside this container add its weight to it.
			CItem* pItem = dynamic_cast<CItem*>(pObjRec);
			if (pItem)
				m_totalweight += pItem->GetWeight();
			continue;
		}

        pCont->FixWeight();
        if (!pCont->IsWeighed())	// bank box doesn't count for weight.
            continue;
        m_totalweight += pCont->GetWeight();
	}
	return m_totalweight;
}

void CContainer::ContentAddPrivate( CItem *pItem )
{
	ADDTOCALLSTACK("CContainer::ContentAddPrivate");
	// We are adding to a CChar or a CItemContainer
	ASSERT(pItem);
	ASSERT(pItem->IsValidUID());	// it should be valid at this point.
	if ( pItem->GetParent() == this )
		return;

	CSObjCont::InsertContentTail( pItem );
	//pItem->RemoveUIDFlags(UID_O_DISCONNECT);

	if ( !pItem->IsType(IT_EQ_TRADE_WINDOW) )  //Don't apply trade window layer item weight on character weight.
		OnWeightChange(pItem->GetWeight());

	if (auto pThisObj = dynamic_cast<const CObjBase*>(this))
	{
		if (pThisObj->IsItem())
		{
			// prevent the timer from firing if the item is inside a container-type item
			if (!pItem->IsSleeping())
			{
				pItem->GoSleep();
			}
			pItem->SetDecayTime(-1);
		}
		else
		{
			// It's a char
			if (pThisObj->IsSleeping())
			{
				if (!pItem->IsSleeping())
				{
					pItem->GoSleep();
				}
			}
			else
			{
				if (pItem->IsSleeping())
				{
					pItem->GoAwake();
				}
			}
		}
	}
}

void CContainer::OnRemoveObj( CSObjContRec *pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CContainer::OnRemoveObj");
	// remove this object from the container list.
	// Overload the RemoveAt for general lists to come here.
	CItem *pItem = static_cast<CItem *>(pObRec);
	ASSERT(pItem);

	CSObjCont::OnRemoveObj(pItem);
	ASSERT(pItem->GetParent() == nullptr);

	pItem->SetUIDContainerFlags(UID_O_DISCONNECT);		// It is no place for the moment.
	if ( !pItem->IsType(IT_EQ_TRADE_WINDOW) ) //Don't apply trade window layer item weight on character weight.
		OnWeightChange(-pItem->GetWeight());
}

void CContainer::r_WriteContent( CScript &s ) const
{
	ADDTOCALLSTACK("CContainer::r_WriteContent");

	// Write out all the items in me.
	for (CSObjContRec* pObjRec : *this)
	{
		ASSERT(pObjRec->GetParent() == this);

		CItem* pItem = static_cast<CItem*>(pObjRec);
		pItem->r_WriteSafe(s);
	}
}

CItem *CContainer::ContentFind( CResourceID rid, dword dwArg, int iDecendLevels ) const
{
	ADDTOCALLSTACK("CContainer::ContentFind");
	// send all the items in the container.

	if ( rid.GetResIndex() == 0 )
		return nullptr;

	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( pItem->IsResourceMatch(rid, dwArg) )
			return pItem;

		if ( iDecendLevels <= 0 )
			continue;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )
		{
			if ( !pCont->IsSearchable() )
				continue;
			CItem *pItemInCont = pCont->ContentFind(rid, dwArg, iDecendLevels - 1);
			if ( pItemInCont )
				return pItemInCont;
		}
	}
	return nullptr;
}

TRIGRET_TYPE CContainer::OnContTriggerForLoop(
	CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs,
	CSString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, CResourceID rid, dword dwArg, int iDecendLevels )
{
	ADDTOCALLSTACK("CContainer::OnContTriggerForLoop");
	if ( rid.GetResIndex() != 0 )
	{
		for (CSObjContRec *pObjRec : GetIterationSafeContReverse())
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( pItem->IsResourceMatch(rid, dwArg) )
			{
				s.SeekContext(StartContext);
				TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
				if ( iRet == TRIGRET_BREAK )
				{
					EndContext = StartContext;
					break;
				}
				if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
					return iRet;
				if ( iRet == TRIGRET_CONTINUE )
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
			}
			if ( iDecendLevels <= 0 )
				continue;

			CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
			if ( pCont )
			{
				if ( pCont->IsSearchable() )
				{
					CContainer *pContBase = dynamic_cast<CContainer *>(pCont);
					TRIGRET_TYPE iRet = pContBase->OnContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, rid, dwArg, iDecendLevels - 1);
					if ( iRet != TRIGRET_ENDIF )
						return iRet;

					// Since the previous call has already found the EndContext, set it.
					EndContext = s.GetContext();
				}
			}
		}
	}

	if ( EndContext.m_iOffset <= StartContext.m_iOffset )
	{
		CScriptObj *pScript = dynamic_cast<CScriptObj *>(this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
    {
		s.SeekContext(EndContext);
    }
	return TRIGRET_ENDIF;
}

TRIGRET_TYPE CContainer::OnGenericContTriggerForLoop(
	CScript &s, CTextConsole *pSrc, CScriptTriggerArgs *pArgs,
	CSString *pResult, CScriptLineContext &StartContext, CScriptLineContext &EndContext, int iDecendLevels )
{
	ADDTOCALLSTACK("CContainer::OnGenericContTriggerForLoop");
	for (CSObjContRec* pObjRec : GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		s.SeekContext(StartContext);
		TRIGRET_TYPE iRet = pItem->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
		if ( iRet == TRIGRET_BREAK )
		{
			EndContext = StartContext;
			break;
		}
		if ( (iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE) )
			return iRet;
		if ( iRet == TRIGRET_CONTINUE )
			EndContext = StartContext;
		else
			EndContext = s.GetContext();
		if ( iDecendLevels <= 0 )
			continue;

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont && pCont->IsSearchable() )
		{
			CContainer *pContBase = dynamic_cast<CContainer *>(pCont);
			iRet = pContBase->OnGenericContTriggerForLoop(s, pSrc, pArgs, pResult, StartContext, EndContext, iDecendLevels - 1);
			if ( iRet != TRIGRET_ENDIF )
				return iRet;

			// Since the previous call has already found the EndContext, set it.
			EndContext = s.GetContext();
		}
	}

	if ( EndContext.m_iOffset <= StartContext.m_iOffset )
	{
		CScriptObj *pScript = dynamic_cast<CScriptObj *>(this);
		TRIGRET_TYPE iRet = pScript->OnTriggerRun(s, TRIGRUN_SECTION_FALSE, pSrc, pArgs, pResult);
		if ( iRet != TRIGRET_ENDIF )
			return iRet;
	}
	else
	{
		s.SeekContext(EndContext);
	}
	return TRIGRET_ENDIF;
}

bool CContainer::ContentFindKeyFor( CItem *pLocked ) const
{
	ADDTOCALLSTACK("CContainer::ContentFindKeyFor");
	// Look for the key that fits this in my possesion.
	const CUID& uidLock(pLocked->m_itContainer.m_UIDLock);
	return (uidLock.IsValidUID() && (nullptr != ContentFind(CResourceID(RES_TYPEDEF, IT_KEY), uidLock)));
}

CItem *CContainer::ContentFindRandom() const
{
	ADDTOCALLSTACK("CContainer::ContentFindRandom");
	// returns Pointer of random item, nullptr if player carrying none
	return static_cast<CItem *>(GetContentIndex(Calc_GetRandVal((int32)GetContentCount())));
}

int CContainer::ContentConsumeTest( const CResourceID& rid, int amount, dword dwArg ) const
{
    ADDTOCALLSTACK("CContainer::ContentConsumeTest");
    // ARGS:
    //  dwArg = a hack for ores.
    // RETURN:
    //  0 = all consumed ok.
    //  # = number left to be consumed. (still required)

    if ( rid.GetResIndex() == 0 )
        return amount;	// from skills menus.

	for (const CSObjContRec* pObjRec : *this)
	{
		const CItem* pItem = static_cast<const CItem*>(pObjRec);
        if ( pItem->IsResourceMatch(rid, dwArg) )
        {
            const word wAmountMax = pItem->GetAmount();
            const word wAmountToConsume = (word)minimum(amount,UINT16_MAX);
            amount -= (wAmountMax > wAmountToConsume ) ? wAmountToConsume : wAmountMax;
            if ( amount <= 0 )
                break;
        }

        const CItemContainer *pCont = dynamic_cast<const CItemContainer *>(pItem);
        if ( pCont )	// this is a sub-container.
        {
            if ( rid == CResourceID(RES_TYPEDEF, IT_GOLD) )
            {
                if ( pCont->IsType(IT_CONTAINER_LOCKED) )
                    continue;
            }
            else
            {
                if ( !pCont->IsSearchable() )
                    continue;
            }
            amount = pCont->ContentConsumeTest(rid, amount, dwArg);
            if ( amount <= 0 )
                break;
        }
    }
    return amount;
}

int CContainer::ContentConsume( const CResourceID& rid, int amount, dword dwArg )
{
	ADDTOCALLSTACK("CContainer::ContentConsume");
	// ARGS:
	//  dwArg = a hack for ores.
	// RETURN:
	//  0 = all consumed ok.
	//  # = number left to be consumed. (still required)

	if ( rid.GetResIndex() == 0 )
		return amount;	// from skills menus.

	for (size_t i = 0; i < GetContentCount();)
	{
		CItem* pItem = static_cast<CItem*>(GetContentIndex(i));
		ASSERT(pItem);
		if ( pItem->IsResourceMatch(rid, dwArg) )
		{
			amount -= pItem->ConsumeAmount( (word)minimum(amount,UINT16_MAX));
			if ( amount <= 0 )
				break;
		}

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )	// this is a sub-container.
		{
			if ( rid == CResourceID(RES_TYPEDEF, IT_GOLD) )
			{
				if (pCont->IsType(IT_CONTAINER_LOCKED))
				{
					++i;
					continue;
				}
			}
			else
			{
				if (!pCont->IsSearchable())
				{
					++i;
					continue;
				}
			}
			amount = pCont->ContentConsume(rid, amount, dwArg);
			if ( amount <= 0 )
				break;
		}

		if (!pItem->IsDeleted())
			++i;
	}
	return amount;
}

int CContainer::ContentCount( CResourceID rid, dword dwArg ) const
{
	ADDTOCALLSTACK("CContainer::ContentCount");
	// Calculate total (gold or other items) in this recursed container
	return INT32_MAX - ContentConsumeTest(rid, INT32_MAX, dwArg);
}

void CContainer::ContentAttrMod( uint64 iAttr, bool fSet )
{
	ADDTOCALLSTACK("CContainer::ContentAttrMod");
	// Mark the attr
	for (CSObjContRec* pObjRec : *this)
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( fSet )
			pItem->SetAttr(iAttr);
		else
			pItem->ClrAttr(iAttr);

		CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
		if ( pCont )	// this is a sub-container.
			pCont->ContentAttrMod(iAttr, fSet);
	}
}

void CContainer::ContentsDump( const CPointMap &pt, uint64 iAttrLeave )
{
	ADDTOCALLSTACK("CContainer::ContentsDump");
	// Just dump the contents onto the ground.

    iAttrLeave |= ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_CURSED2|ATTR_BLESSED2;
	
	for (size_t i = 0; i < GetContentCount(); )
	{
		CItem* pItem = static_cast<CItem*>(GetContentIndex(i));
		if ( pItem->IsAttr(iAttrLeave) )	// hair and newbie stuff.
		{
			++i;
			continue;
		}

		// ??? scatter a little ?
		if (!pItem->MoveToCheck(pt))
			++i;
	}
}

void CContainer::ContentsTransfer( CItemContainer *pCont, bool fNoNewbie )
{
	ADDTOCALLSTACK("CContainer::ContentsTransfer");
	// Move all contents to another container. (pCont)
	if ( !pCont )
		return;

	for (CSObjContRec* pObjRec : GetIterationSafeCont())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( fNoNewbie && pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER|ATTR_CURSED2|ATTR_BLESSED2) )	// keep newbie stuff.
			continue;
		pCont->ContentAdd(pItem);	// add content
	}
}

size_t CContainer::ResourceConsumePart( const CResourceQtyArray *pResources, int iReplicationQty, int iDamagePercent, bool fTest, dword dwArg )
{
	ADDTOCALLSTACK("CContainer::ResourceConsumePart");
	// Consume just some of the resources.
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product.
	//  iDamagePercent = 0-100
	// RETURN:
	//  BadIndex = all needed items where present.
	// index of the item we did not have.

	if ( iDamagePercent <= 0 )
		return SCONT_BADINDEX;

	size_t iMissing = SCONT_BADINDEX;
	size_t iQtyRes = pResources->size();
	for ( size_t i = 0; i < iQtyRes; ++i )
	{
		int iResQty = (int)((*pResources)[i].GetResQty());
		if ( iResQty <= 0 ) // not sure why this would be true
			continue;

		int iQtyTotal = (iResQty * iReplicationQty);
		if ( iQtyTotal <= 0 )
			continue;
		iQtyTotal = IMulDiv(iQtyTotal, iDamagePercent, 100);
		if ( iQtyTotal <= 0 )
			continue;

		const CResourceID rid = (*pResources)[i].GetResourceID();
		int iRet = fTest ? ContentConsumeTest(rid, iQtyTotal, dwArg) : ContentConsume(rid, iQtyTotal, dwArg);
		if ( iRet )
			iMissing = i;
	}

	return iMissing;
}

int CContainer::ResourceConsume( const CResourceQtyArray *pResources, int iReplicationQty, bool fTest )
{
	ADDTOCALLSTACK("CContainer::ResourceConsume");
	// Consume or test all the required resources.
	// ARGS:
	//	pResources = the resources i need to make 1 replication of this end product.
	// RETURN:
	//  how many whole objects can be made. <= iReplicationQty

	if ( iReplicationQty <= 0 )
		iReplicationQty = 1;
	if ( !fTest && iReplicationQty > 1 )
	{
		// Test what the max number we can really make is first !
		// All resources must be consumed with the same number.
		iReplicationQty = ResourceConsume(pResources, iReplicationQty, true);
	}

	CChar *pChar = dynamic_cast<CChar *>(this);
	int iQtyMin = INT32_MAX;
	for ( size_t i = 0; i < pResources->size(); ++i )
	{
		int iResQty = (int)((*pResources)[i].GetResQty());
		if ( iResQty <= 0 ) // not sure why this would be true
			continue;

		const int iQtyTotal = (iResQty * iReplicationQty);
		CResourceID rid = (*pResources)[i].GetResourceID();
		if ( rid.GetResType() == RES_SKILL )
		{
			if ( !pChar )
				continue;
			if ( pChar->Skill_GetBase((SKILL_TYPE)(rid.GetResIndex())) < iResQty )
				return 0;
			continue;
		}
		else if ( rid.GetResType() == RES_ITEMDEF )	// TAG.MATOVERRIDE_%s
		{
            if (pChar)
            {
                tchar * resOverride = Str_GetTemp();
                snprintf(resOverride, STR_TEMPLENGTH, "matoverride_%s", g_Cfg.ResourceGetName( CResourceID( RES_ITEMDEF, rid.GetResIndex() ) ));
                CResourceID ridOverride = CResourceID( RES_ITEMDEF , (dword)pChar->m_TagDefs.GetKeyNum(resOverride) );
                if ( ridOverride.GetResIndex() > 0 )
                    rid = ridOverride;
            }
		}

		int iQtyCur = iQtyTotal - (fTest ? ContentConsumeTest(rid, iQtyTotal) : ContentConsume(rid, iQtyTotal));
		iQtyCur /= iResQty;
		if ( iQtyCur < iQtyMin )
			iQtyMin = iQtyCur;
	}

	if ( iQtyMin == INT32_MAX )	// it has no resources ? So i guess we can make it from nothing ?
		return iReplicationQty;

	return iQtyMin;
}

size_t CContainer::ContentCountAll() const
{
	ADDTOCALLSTACK("CContainer::ContentCountAll");
	// RETURN:
	//  A count of all the items in this container and sub contianers.
	size_t iTotal = 0;
	for (const CSObjContRec* pObjRec : *this)
	{
		++ iTotal;
		const CItem* pItem = static_cast<const CItem*>(pObjRec);
		const CItemContainer *pCont = dynamic_cast<const CItemContainer *>(pItem);
		if ( !pCont )
			continue;
		//if ( !pCont->IsSearchable() )
		//	continue;	// found a sub
		iTotal += pCont->ContentCountAll();
	}
	return iTotal;
}

bool CContainer::r_GetRefContainer( lpctstr &ptcKey, CScriptObj *&pRef )
{
	ADDTOCALLSTACK("CContainer::r_GetRefContainer");
	if ( !strnicmp(ptcKey, "FIND", 4) )				// find*
	{
		ptcKey += 4;
		if ( !strnicmp(ptcKey, "ID", 2) )			// findid
		{
			ptcKey += 2;
			SKIP_SEPARATORS(ptcKey);
			pRef = ContentFind(g_Cfg.ResourceGetID_Advance(RES_ITEMDEF, ptcKey));
			SKIP_SEPARATORS(ptcKey);
			return true;
		}
		else if ( !strnicmp(ptcKey, "CONT", 4) )	// findcont
		{
			ptcKey += 4;
			SKIP_SEPARATORS(ptcKey);
			const size_t idx = Exp_GetSTSingle(ptcKey);
			if (idx >= GetContentCount())
				return false;
			pRef = dynamic_cast<CItem*>(GetContentIndex(idx));
			SKIP_SEPARATORS(ptcKey);
			return true;
		}
		else if ( !strnicmp(ptcKey, "TYPE", 4) )	// findtype
		{
			ptcKey += 4;
			SKIP_SEPARATORS(ptcKey);
			pRef = ContentFind(g_Cfg.ResourceGetID_Advance(RES_TYPEDEF, ptcKey));
			SKIP_SEPARATORS(ptcKey);
			return true;
		}
	}
	return false;
}

bool CContainer::r_WriteValContainer( lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc )
{
	UNREFERENCED_PARAMETER(pSrc);
	ADDTOCALLSTACK("CContainer::r_WriteValContainer");
	EXC_TRY("WriteVal");

	static lpctstr constexpr sm_szParams[] =
	{
		"COUNT",
		"FCOUNT",
		"RESCOUNT",
		"RESTEST"
	};

	int i = FindTableHeadSorted(ptcKey, sm_szParams, CountOf(sm_szParams));
	if ( i < 0 )
		return false;

	lpctstr	pKey = ptcKey + strlen(sm_szParams[i]);
	SKIP_SEPARATORS(pKey);
	switch ( i )
	{
		case 0:			//	count
		{
			sVal.FormatSTVal(CSObjCont::GetContentCount());
			break;
		}

		case 1:			//	fcount
			sVal.FormatSTVal(ContentCountAll());
			break;

		case 2:			//	rescount
			sVal.FormatSTVal(*pKey ? ContentCount(g_Cfg.ResourceGetID(RES_ITEMDEF, pKey)) : GetContentCount());
			break;

		case 3:			//	restest
		{
			CResourceQtyArray Resources;
			sVal.FormatSTVal(Resources.Load(pKey) ? ResourceConsume(&Resources, 1, true) : 0);
			break;
		}

		default:
			return false;
	}
	return true;

	EXC_CATCH;
	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}
