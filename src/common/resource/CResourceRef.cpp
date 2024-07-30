/**
* @file CResourceRef.cpp
*
*/

#include "../../game/CServerConfig.h"
#include "../../game/CServer.h"
#include "../CException.h"
#include "../CScript.h"
#include "CResourceRef.h"

CResourceRef::CResourceRef()
{
    m_pLink = nullptr;
}

CResourceRef::CResourceRef(CResourceLink* pLink) : m_pLink(pLink)
{
    ASSERT(pLink);
    pLink->AddRefInstance();
}

CResourceRef::CResourceRef(const CResourceRef& copy)
{
    m_pLink = copy.m_pLink;
    if (m_pLink != nullptr)
        m_pLink->AddRefInstance();
}

CResourceRef::~CResourceRef()
{
    // TODO: Consider using not a bare pointer for m_pLink but a CResourceID, in order to safely check if the father
    //  resource was deleted or not.
    if (m_pLink != nullptr)
        m_pLink->DelRefInstance();
}


CResourceRef& CResourceRef::operator=(const CResourceRef& other)
{
    if (this != &other)
        SetRef(other.m_pLink);
    return *this;
}

void CResourceRef::SetRef(CResourceLink* pLink)
{
    //ASSERT(pLink != m_pLink);

    if (m_pLink != nullptr)
        m_pLink->DelRefInstance();

    m_pLink = pLink;

    if (m_pLink != nullptr)
        m_pLink->AddRefInstance();
}


//--


lpctstr CResourceRefArray::GetResourceName( size_t iIndex ) const
{
    // look up the name of the fragment given it's index.
    const CResourceLink * pResourceLink = operator[](iIndex).GetRef();
    ASSERT(pResourceLink);
    return pResourceLink->GetResourceName();
}

bool CResourceRefArray::r_LoadVal( CScript & s, RES_TYPE restype )
{
    ADDTOCALLSTACK("CResourceRefArray::r_LoadVal");
    EXC_TRY("LoadVal");
    // A bunch of CResourceLink (CResourceDef) pointers.
    // Add or remove from the list.
    // RETURN: false = it failed.

    // ? "TOWN" and "REGION" are special !

    bool fRet = true;

    tchar * pszCmd = s.GetArgStr();
    tchar * ppBlocks[128];	// max is arbitrary
    int iArgCount = Str_ParseCmds( pszCmd, ppBlocks, ARRAY_COUNT(ppBlocks));
    for ( int i = 0; i < iArgCount; ++i )
    {
        std::shared_ptr<CResourceDef> pResourceDefRef;
        CResourceLink* pResourceLink = nullptr;

        pszCmd = ppBlocks[i];
        if ( pszCmd[0] == '-' )
        {
            // remove a frag or all frags.
            ++pszCmd;
            if ( pszCmd[0] == '0' || pszCmd[0] == '*' )
            {
                clear();
                fRet = true;
                continue;
            }

            pResourceLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, pszCmd ));
            if (pResourceLink)
            {
                const iterator pos = std::find(begin(), end(), pResourceLink);
                const bool fFound = (end() != pos);
                if (fRet && !fFound)
                    fRet = false;
                if (fFound)
                    erase(pos);
            }
        }
        else
        {
            // Add a single knowledge fragment or appropriate group item.
            if ( pszCmd[0] == '+' )
                ++pszCmd;

            pResourceLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, pszCmd ));
            if ( pResourceLink )
            {
                // Check if it's already in the list, before adding it
                if (cend() == find(cbegin(), cend(), pResourceLink))
                    emplace_back(pResourceLink);
            }
        }

        if (pResourceLink == nullptr)
        {
            fRet = false;
            DEBUG_ERR(("Unknown '%s' Resource '%s'\n", CResourceHolder::GetResourceBlockName(restype), pszCmd));
        }
    }

    return fRet;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

void CResourceRefArray::WriteResourceRefList( CSString & sVal ) const
{
    ADDTOCALLSTACK("CResourceRefArray::WriteResourceRefList");
    TemporaryString tsVal;
    tchar * ptcVal = tsVal.buffer();
    size_t len = 0;
    for ( size_t j = 0, sz = size(); j < sz; ++j )
    {
        if ( j > 0 )
            ptcVal[len++] = ',';

        len += Str_CopyLimitNull(ptcVal + len, GetResourceName(j), tsVal.capacity() - len);
        if ( len >= SCRIPT_MAX_LINE_LEN-1 )
            break;
    }
    ptcVal[len] = '\0';
    sVal.Copy(ptcVal);
}

size_t CResourceRefArray::FindResourceType( RES_TYPE restype ) const
{
    ADDTOCALLSTACK("CResourceRefArray::FindResourceType");
    // Is this resource already in the list ?
    size_t iQty = size();
    for ( size_t i = 0; i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetRef()->GetResourceID();
        if ( ridtest.GetResType() == restype )
            return i;
    }
    return sl::scont_bad_index();
}

size_t CResourceRefArray::FindResourceID( const CResourceID & rid ) const
{
    ADDTOCALLSTACK("CResourceRefArray::FindResourceID");
    // Is this resource already in the list ?
    size_t iQty = size();
    for ( size_t i = 0; i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetRef()->GetResourceID();
        if ( ridtest == rid )
            return i;
    }
    return sl::scont_bad_index();
}

size_t CResourceRefArray::FindResourceName( RES_TYPE restype, lpctstr ptcKey ) const
{
    ADDTOCALLSTACK("CResourceRefArray::FindResourceName");
    // Is this resource already in the list ?
    CResourceLink * pResourceLink = dynamic_cast <CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, ptcKey ));
    if ( pResourceLink == nullptr )
        return sl::scont_bad_index();
    return FindResourceID(pResourceLink->GetResourceID());
}

void CResourceRefArray::r_Write( CScript & s, lpctstr ptcKey ) const
{
    ADDTOCALLSTACK_DEBUG("CResourceRefArray::r_Write");
    for ( size_t j = 0, sz = size(); j < sz; ++j )
    {
        s.WriteKeyStr( ptcKey, GetResourceName( j ));
    }
}
