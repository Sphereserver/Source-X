/**
* @file CContainer.h
*
*/

#ifndef _INC_CCONTAINER_H
#define _INC_CCONTAINER_H

#include "../common/sphere_library/CSObjCont.h"
#include "../common/resource/CResourceBase.h"
#include "../common/CUID.h"
#include "../common/CRect.h"


class CItemContainer;
class CObjBase;

class CContainer : public CSObjCont	// This class contains a list of items but may or may not be an item itself.
{
public:
	static const char *m_sClassName;
	CContainer();
	virtual ~CContainer() = default;

private:
	CContainer(const CContainer& copy);
	CContainer& operator=(const CContainer& other);

public:
    int	m_totalweight;      // weight of all the items it has. (1/WEIGHT_UNITS pound)

protected:
    friend class CObjBase;
    // Not virtuals!
    void _GoAwake();
    void _GoSleep();

public:
    void ContentDelete(bool fForce);

    /**
     * @fn  virtual void CContainer::OnRemoveObj( CSObjListRec* pObRec );
     * @brief   Override this = called when removed from list.
     * @param [in,out]  pObRec  If non-null, the ob record.
     */
    virtual void OnRemoveObj(CSObjContRec* pObRec) override;

    /**
     * @fn  void CContainer::ContentAddPrivate( CItem * pItem );
     * @brief  Adds an item to this CContainer.
     * @param [in,out]  pItem   If non-null, the item.
     */
    void ContentAddPrivate(CItem* pItem);

    void r_WriteContent(CScript& s) const;

    bool r_WriteValContainer(lpctstr ptcKey, CSString& sVal, CTextConsole* pSrc);
    bool r_GetRefContainer(lpctstr& ptcKey, CScriptObj*& pRef);


    /**
     * @fn  int CContainer::GetTotalWeight() const;
     * @brief   Gets total weight.
     * @return  The total weight.
     */
	int	GetTotalWeight() const;

    /**
     * @fn  int CContainer::FixWeight();
     * @brief   Fix weight.
     * @return  An int.
     */
	int FixWeight();

    /**
     * @fn  bool CContainer::ContentFindKeyFor( CItem * pLocked ) const;
     * @brief   Content find key for.
     * @param [in,out]  pLocked If non-null, the locked.
     * @return  true if it succeeds, false if it fails.
     */
	bool ContentFindKeyFor( CItem * pLocked ) const;
	// bool IsItemInside( CItem * pItem ) const;

    /**
     * @fn  CItem * CContainer::ContentFindRandom() const;
     * @brief   Content find random.
     * @return  null if it fails, else a pointer to a CItem.
     */
	CItem * ContentFindRandom() const;

    /**
     * @fn  void CContainer::ContentsDump( const CPointMap & pt, uint64 iAttr = 0 );
     * @brief   Contents dump.
     * @param   pt      The point.
     * @param   dwAttr  The attribute.
     */
	void ContentsDump( const CPointMap & pt, uint64 uiAttr = 0 );

    /**
     * @fn  void CContainer::ContentsTransfer( CItemContainer * pCont, bool fNoNewbie );
     * @brief   Contents transfer.
     * @param [in,out]  pCont   If non-null, the container.
     * @param   fNoNewbie       true to no newbie.
     */
	void ContentsTransfer( CItemContainer * pCont, bool fNoNewbie );

    /**
     * @fn  void CContainer::ContentAttrMod( uint64 iAttr, bool fSet );
     * @brief   Content attribute modifier.
     * @param   dwAttr  The attribute.
     * @param   fSet    true to set.
     */
	void ContentAttrMod( uint64 uiAttr, bool fSet );

    /**
     * @fn  void CContainer::ContentNotifyDelete();
     * @brief   Content notify delete.
     */
	void ContentNotifyDelete();

	// For resource usage and gold.

    /**
     * @fn  CItem * CContainer::ContentFind( RESOURCE_ID_BASE rid, dword dwArg = 0, int iDecendLevels = 255 ) const;
     * @brief   Content find.
     * @param   rid             The rid.
     * @param   dwArg           The argument.
     * @param   iDecendLevels   Zero-based index of the decend levels.
     * @return  null if it fails, else a pointer to a CItem.
     */
	CItem * ContentFind( CResourceID rid, dword dwArg = 0, int iDecendLevels = 255 ) const;

    /**
     * @fn  TRIGRET_TYPE CContainer::OnContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, RESOURCE_ID_BASE rid, dword dwArg = 0, int iDecendLevels = 255 );
     * @brief   Executes the container trigger for loop action.
     * @param [in,out]  s               The CScript to process.
     * @param [in,out]  pSrc            If non-null, source for the.
     * @param [in,out]  pArgs           If non-null, the arguments.
     * @param [out] pResult             If non-null, the result.
     * @param [in,out]  StartContext    Context for the start.
     * @param [in,out]  EndContext      Context for the end.
     * @param   rid                     The rid.
     * @param   dwArg                   The argument.
     * @param   iDecendLevels           Zero-based index of the decend levels.
     *
     * @return  A TRIGRET_TYPE.
     */
	TRIGRET_TYPE OnContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, CResourceID rid, dword dwArg = 0, int iDecendLevels = 255 );

    /**
     * @fn  TRIGRET_TYPE CContainer::OnGenericContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, int iDecendLevels = 255 );
     * @brief   Executes the generic container trigger for loop action.
     * @param [in,out]  s               The CScript to process.
     * @param [in,out]  pSrc            If non-null, source for the.
     * @param [in,out]  pArgs           If non-null, the arguments.
     * @param [out] pResult             If non-null, the result.
     * @param [in,out]  StartContext    Context for the start.
     * @param [in,out]  EndContext      Context for the end.
     * @param   iDecendLevels           Zero-based index of the decend levels.
     *
     * @return  A TRIGRET_TYPE.
     */
	TRIGRET_TYPE OnGenericContTriggerForLoop( CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult, CScriptLineContext & StartContext, CScriptLineContext & EndContext, int iDecendLevels = 255 );

    /**
     * @fn  int CContainer::ContentCount( RESOURCE_ID_BASE rid, dword dwArg = 0 );
     * @brief   Content count of rid.
     * @param   rid     The rid.
     * @param   dwArg   The argument.
     * @return  An int.
     */
	int ContentCount( CResourceID rid, dword dwArg = 0 ) const;

    /**
     * @fn  size_t CContainer::ContentCountAll() const;
     * @brief   Content count all.
     * @return  A size_t.
     */
	size_t ContentCountAll() const;

    /**
     * @fn  int CContainer::ContentConsume( RESOURCE_ID_BASE rid, int iQty = 1, bool fTest = false, dword dwArg = 0 );
     * @brief   Content consume.
     * @param   rid     The rid.
     * @param   iQty    Zero-based index of the qty.
     * @param   dwArg   The argument.
     *
     * @return  0 = all consumed, # = number left to be consumed.
     */
	int ContentConsume( const CResourceID& rid, int iQty = 1, dword dwArg = 0 );

    /**
    * @fn  int CContainer::ContentConsumeTest( RESOURCE_ID_BASE rid, int iQty = 1, dword dwArg = 0 );
    * @brief   Content consume.
    * @param   rid     The rid.
    * @param   iQty    Zero-based index of the qty.
    * @param   dwArg   The argument.
    *
    * @return  0 = all consumed, # = number left to be consumed.
    */
    int ContentConsumeTest( const CResourceID& rid, int amount, dword dwArg = 0) const;

    /**
     * @fn  int CContainer::ResourceConsume( const CResourceQtyArray * pResources, int iReplicationQty, bool fTest = false, dword dwArg = 0 );
     * @brief   Resource consume.
     * @param   pResources      The resources.
     * @param   iReplicationQty Zero-based index of the replication qty.
     * @param   fTest           true to test.
     *
     * @return  An int.
     */
	int ResourceConsume( const CResourceQtyArray * pResources, int iReplicationQty, bool fTest = false );

    /**
     * @fn  size_t CContainer::ResourceConsumePart( const CResourceQtyArray * pResources, int iReplicationQty, int iFailPercent, bool fTest = false, dword dwArg = 0 );
     * @brief   Resource consume part.
     * @param   pResources      The resources.
     * @param   iReplicationQty Zero-based index of the replication qty.
     * @param   iFailPercent    Zero-based index of the fail percent.
     * @param   fTest           true to test.
     * @param   dwArg           The argument.
     *
     * @return  A size_t.
     */
	size_t ResourceConsumePart( const CResourceQtyArray * pResources, int iReplicationQty, int iFailPercent, bool fTest = false, dword dwArg = 0 );

    /**
     * @fn  virtual void CContainer::OnWeightChange( int iChange );
     * @brief   Executes the weight change action.
     * @param   iChange Zero-based index of the change.
     */
	virtual void OnWeightChange( int iChange );

    /**
     * @fn  virtual void CContainer::ContentAdd( CItem * pItem ) = 0;
     * @brief   Content add.
     * @param [in,out]  pItem   If non-null, the item.
	 * @param [in,out]  bForceNoStack	Do not stack on other identical items, even if it's a stackable type.
     */
	virtual void ContentAdd( CItem * pItem, bool bForceNoStack = false ) = 0;
};


#endif // _INC_CCONTAINER_H
