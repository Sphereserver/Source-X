#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../CSector.h"
#include "CItemVendable.h"
#include "CItemCommCrystal.h"

CItemCommCrystal::CItemCommCrystal( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
    CItemVendable( id, pItemDef )
{
}

CItemCommCrystal::~CItemCommCrystal()
{
    EXC_TRY("Cleanup in destructor");

    // / Must remove early because virtuals will fail in child destructor.
    DeletePrepare();

    EXC_CATCH;
}

lpctstr const CItemCommCrystal::sm_szLoadKeys[] =
{
	"SPEECH",
	nullptr,
};

void CItemCommCrystal::OnMoveFrom()
{
    ADDTOCALLSTACK("CItemCommCrystal::OnMoveFrom");
    // Being removed from the top level.
    CSector *pSector = GetTopSector();
    if ( pSector )
        pSector->RemoveListenItem();
}

// Move this item to it's point in the world. (ground/top level)
bool CItemCommCrystal::MoveTo(const CPointMap& pt, bool fForceFix)
{
    ADDTOCALLSTACK("CItemCommCrystal::MoveTo");
    CSector *pSector = pt.GetSector();
    ASSERT(pSector);
    pSector->AddListenItem();
    return CItem::MoveTo(pt, fForceFix);
}

void CItemCommCrystal::OnHear(lpctstr pszCmd, CChar *pSrc)
{
    ADDTOCALLSTACK("CItemCommCrystal::OnHear");
    // IT_COMM_CRYSTAL
    // STATF_COMM_CRYSTAL = if i am on a person.
    TALKMODE_TYPE mode = TALKMODE_SAY;
    for ( size_t i = 0; i < m_Speech.size(); ++i )
    {
        CResourceLink *pLink = m_Speech[i].GetRef();
        ASSERT(pLink);
        CResourceLock s;
        if ( !pLink->ResourceLock(s) )
            continue;
        TRIGRET_TYPE iRet = OnHearTrigger(s, pszCmd, pSrc, mode);
        if ( iRet == TRIGRET_ENDIF || iRet == TRIGRET_RET_FALSE )
            continue;
        break;
    }

    // That's prevent @ -1 crash speech :P
    if ( *pszCmd == '@' )
        return;

    if ( m_uidLink.IsValidUID() )
    {
        // I am linked to something ?
        // Transfer the sound.
        CItem *pItem = m_uidLink.ItemFind();
        if ( pItem && pItem->IsType(IT_COMM_CRYSTAL) )
            pItem->Speak(pszCmd);
    }
    else if (m_Speech.size() <= 0 )
        Speak(pszCmd);
}

void CItemCommCrystal::r_Write(CScript & s)
{
    ADDTOCALLSTACK_DEBUG("CItemCommCrystal::r_Write");
    CItemVendable::r_Write(s);
    m_Speech.r_Write(s, "SPEECH");
}

bool CItemCommCrystal::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CItemCommCrystal::r_WriteVal");
    switch ( FindTableSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1) )
    {
        case 0:
            m_Speech.WriteResourceRefList(sVal);
            break;
        default:
            return (fNoCallParent ? false : CItemVendable::r_WriteVal(ptcKey, sVal, pSrc));
    }
    return true;
}

bool CItemCommCrystal::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemCommCrystal::r_LoadVal");
    switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1) )
    {
        case 0:
            return m_Speech.r_LoadVal(s, RES_SPEECH);
        default:
            return CItemVendable::r_LoadVal(s);
    }
}

void CItemCommCrystal::DupeCopy(const CObjBase *pItemObj)
{
    ADDTOCALLSTACK("CItemCommCrystal::DupeCopy");
    auto pItem = dynamic_cast<const CItem*>(pItemObj);
    ASSERT(pItem);

    CItemVendable::DupeCopy(pItem);

    const CItemCommCrystal *pItemCrystal = dynamic_cast<const CItemCommCrystal *>(pItem);
    if ( !pItemCrystal )
        return;

    m_Speech = pItemCrystal->m_Speech;
}
