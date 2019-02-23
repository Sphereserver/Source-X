/**
* @file CResourceRef.cpp
*
*/

#include "../../game/CServerConfig.h"
#include "../../game/CServer.h"
#include "../CException.h"
#include "../CScript.h"
#include "CResourceRef.h"

lpctstr CResourceRefArray::GetResourceName( size_t iIndex ) const
{
    // look up the name of the fragment given it's index.
    const CResourceLink * pResourceLink = (*this)[iIndex];
    ASSERT(pResourceLink);
    return pResourceLink->GetResourceName();
}

bool CResourceRefArray::r_LoadVal( CScript & s, RES_TYPE restype )
{
    ADDTOCALLSTACK("CResourceRefArray::r_LoadVal");
    EXC_TRY("LoadVal");
    bool fRet = false;
    // A bunch of CResourceLink (CResourceDef) pointers.
    // Add or remove from the list.
    // RETURN: false = it failed.

    // ? "TOWN" and "REGION" are special !

    tchar * pszCmd = s.GetArgStr();

    tchar * ppBlocks[128];	// max is arbitrary
    int iArgCount = Str_ParseCmds( pszCmd, ppBlocks, CountOf(ppBlocks));

    for ( int i = 0; i < iArgCount; ++i )
    {
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

            CResourceLink * pResourceLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, pszCmd ));
            if ( pResourceLink == nullptr )
            {
                fRet = false;
                continue;
            }

            fRet = RemovePtr(pResourceLink);
        }
        else
        {
            // Add a single knowledge fragment or appropriate group item.
            if ( pszCmd[0] == '+' )
                ++pszCmd;

            CResourceLink * pResourceLink = dynamic_cast<CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, pszCmd ));
            if ( pResourceLink == nullptr )
            {
                fRet = false;
                DEBUG_ERR(( "Unknown '%s' Resource '%s'\n", CResourceBase::GetResourceBlockName(restype), pszCmd ));
                continue;
            }

            // Is it already in the list ?
            fRet = true;
            if ( ContainsPtr(pResourceLink) )
            {
                continue;
            }
            if ( g_Cfg.m_pEventsPetLink.ContainsPtr(pResourceLink) )
            {
                DEBUG_ERR(("'%s' already defined in sphere.ini - skipping\n", pResourceLink->GetName()));
                continue;
            }
            else if ( g_Cfg.m_pEventsPlayerLink.ContainsPtr(pResourceLink) )
            {
                DEBUG_ERR(("'%s' already defined in sphere.ini - skipping\n", pResourceLink->GetName()));
                continue;
            }
            else if ( restype == RES_REGIONTYPE && g_Cfg.m_pEventsRegionLink.ContainsPtr(pResourceLink) )
            {
                DEBUG_ERR(("'%s' already defined in sphere.ini - skipping\n", pResourceLink->GetName()));
                continue;
            }
            else if ( g_Cfg.m_iEventsItemLink.ContainsPtr(pResourceLink) )
            {
                DEBUG_ERR(("'%s' already defined in sphere.ini - skipping\n", pResourceLink->GetName()));
                continue;
            }

            emplace_back(pResourceLink);
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
    tchar * pszVal = static_cast<tchar *>(tsVal);
    size_t len = 0;
    for ( size_t j = 0, sz = size(); j < sz; ++j )
    {
        if ( j > 0 )
            pszVal[len++] = ',';

        len += strcpylen( pszVal + len, GetResourceName(j) );
        if ( len >= SCRIPT_MAX_LINE_LEN-1 )
            break;
    }
    pszVal[len] = '\0';
    sVal = pszVal;
}

CResourceRefArray::CResourceRefArray(const CResourceRefArray& copy) : CSPtrTypeArray<CResourceRef>(static_cast<const CSPtrTypeArray<CResourceRef> &>(copy))
{
}

CResourceRefArray& CResourceRefArray::operator=(const CResourceRefArray& other)
{
    static_cast<CSPtrTypeArray<CResourceRef> &>(*this) = static_cast<const CSPtrTypeArray<CResourceRef> &>(other);
    return *this;
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
            return( i );
    }
    return BadIndex();
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
    return BadIndex();
}

size_t CResourceRefArray::FindResourceName( RES_TYPE restype, lpctstr pszKey ) const
{
    ADDTOCALLSTACK("CResourceRefArray::FindResourceName");
    // Is this resource already in the list ?
    CResourceLink * pResourceLink = dynamic_cast <CResourceLink *>( g_Cfg.ResourceGetDefByName( restype, pszKey ));
    if ( pResourceLink == nullptr )
        return BadIndex();
    return FindPtr(pResourceLink);
}

void CResourceRefArray::r_Write( CScript & s, lpctstr pszKey ) const
{
    ADDTOCALLSTACK_INTENSIVE("CResourceRefArray::r_Write");
    for ( size_t j = 0, sz = size(); j < sz; ++j )
    {
        s.WriteKey( pszKey, GetResourceName( j ));
    }
}
