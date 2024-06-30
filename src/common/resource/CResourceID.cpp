#include "CResourceID.h"
#include "../CLog.h"


static constexpr lpctstr _ptcWarnInvalidResource = "Expected a valid ResourceID, found invalid with index: 0%x (internal val: 0%x, warning code: %d).\n";
#define LOG_WARN_RES_INVALID(code)    g_Log.EventWarn(_ptcWarnInvalidResource, rid.GetResIndex(), rid.GetPrivateUID(), code);


// CResourceIDBase

CResourceIDBase::CResourceIDBase(RES_TYPE restype) // explicit
{
    // single instance type.
    ASSERT(restype < RES_QTY);
    m_dwInternalVal = UID_F_RESOURCE | (restype << RES_TYPE_SHIFT);
}

CResourceIDBase::CResourceIDBase(RES_TYPE restype, int iIndex) // explicit
{
    ASSERT(restype < RES_QTY);
    m_dwInternalVal = UID_F_RESOURCE | (restype << RES_TYPE_SHIFT) | (iIndex & RES_INDEX_MASK);
}

CResourceIDBase::CResourceIDBase(dword dwPrivateID) // explicit
{
    if (!CUID::IsValidUID(dwPrivateID))
    {
        DEBUG_MSG(("Trying to set invalid dwPrivateID (0x%x) to CResourceIDBase. Resetting it.\n", dwPrivateID));
        Init();
        return;
    }
    m_dwInternalVal = UID_F_RESOURCE | dwPrivateID;
}

CResourceIDBase::CResourceIDBase(const CResourceIDBase& rid) : CUID(rid) // copy constructor
{
    if (!rid.IsResource())
        LOG_WARN_RES_INVALID(1);

    m_dwInternalVal |= UID_F_RESOURCE;
    ASSERT(IsResource());
}

CResourceIDBase& CResourceIDBase::operator = (const CResourceIDBase& rid)  // assignment operator
{
    if (!rid.IsResource())
        LOG_WARN_RES_INVALID(2);

    CUID::operator=(rid);
    m_dwInternalVal |= UID_F_RESOURCE;
    ASSERT(IsResource());
    return *this;
}

void CResourceIDBase::FixRes()
{
    // When setting a TDATA* or a MORE*, a script could set a raw UID (valid/invalid resource) without calling the CResourceID(Base) methods
    // to set the value properly: for this reason it's better to ensure this is a valid resource before using it.
    m_dwInternalVal |= UID_F_RESOURCE;
    ASSERT(IsResource());
    ASSERT(IsValidUID());
}

bool CResourceIDBase::IsUIDItem() const
{
    // replacement for CUID::IsItem(), but don't be virtual, since we don't need that and the class size will increase due to the vtable

    // If it's both a resource and an item, and if it's the CResourceIDBase of a CRegion, it's a region from a multi
    if ((m_dwInternalVal & (UID_F_RESOURCE | UID_F_ITEM)) == (UID_F_RESOURCE | UID_F_ITEM))
        return IsValidUID();
    return false;
}

CItem* CResourceIDBase::ItemFindFromResource(bool fInvalidateBeingDeleted) const   // replacement for CUID::ItemFind()
{
    // Used by multis: when they are realized, a new CRegionWorld is created from a CResourceID with an internal value = to the m_dwInternalVal (private UID) of the multi, plus a | UID_F_RESOURCE.
    //  Remove the reserved UID_* flags (so also UID_F_RESOURCE), and find the item (in our case actually the multi) with that uid.
    ASSERT(IsResource());
    return CUID::ItemFindFromUID(m_dwInternalVal & UID_O_INDEX_MASK, fInvalidateBeingDeleted);
}


//CResourceID

CResourceID& CResourceID::operator = (const CResourceID& rid)   // assignment operator
{
    if (!rid.IsResource())
        LOG_WARN_RES_INVALID(3);

    CUID::operator=(rid);
    ASSERT(IsResource());
    m_wPage = rid.m_wPage;
    return *this;
}

CResourceID& CResourceID::operator = (const CResourceIDBase& rid)
{
    if (!rid.IsResource())
        LOG_WARN_RES_INVALID(4);

    CUID::operator=(rid);
    ASSERT(IsResource());
    m_wPage = 0;
    return *this;
}

bool CResourceID::operator == (const CResourceID & rid) const noexcept
{
    return ((rid.m_wPage == m_wPage) && (rid.m_dwInternalVal == m_dwInternalVal));
}

void CResourceID::Init() noexcept
{
    m_dwInternalVal = UID_UNUSED;
    m_wPage = 0;
}
void CResourceID::Clear() noexcept
{
    m_dwInternalVal = UID_PLAIN_CLEAR;
    m_wPage = 0;
}

bool CResourceID::IsEmpty() const noexcept
{
    return ((m_dwInternalVal & UID_O_INDEX_MASK) == 0 /* means also that restype = 0/RES_UNKNOWN */) && (m_wPage == 0);
}
