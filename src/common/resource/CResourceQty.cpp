/**
* @file CResourceQty.cpp
*
*/

#include "../../game/uo_files/uofiles_enums_itemid.h"
#include "../../game/items/CItemBase.h"
#include "../../game/chars/CChar.h"
#include "../../game/CObjBase.h"
#include "../../sphere/threads.h"
#include "../CLog.h"
#include "CResourceHolder.h"
#include "CResourceQty.h"


//**********************************************
// -CResourceQty

size_t CResourceQty::WriteKey( tchar * pszArgs, size_t uiBufSize, bool fQtyOnly, bool fKeyOnly ) const
{
    ADDTOCALLSTACK("CResourceQty::WriteKey");
    size_t i = 0;
    if ( (GetResQty() || fQtyOnly) && !fKeyOnly )
    {
        int ret = snprintf( pszArgs, uiBufSize, "%" PRId64 " ", GetResQty());
        i = (size_t)StrncpyCharBytesWritten(ret, uiBufSize);
    }
    if ( !fQtyOnly )
    {
        i += Str_CopyLen( pszArgs+i, g_Cfg.ResourceGetName( m_rid ));
    }
    return i;
}

size_t CResourceQty::WriteNameSingle( tchar * pszArgs, size_t uiBufLen, int iQty ) const
{
    ADDTOCALLSTACK("CResourceQty::WriteNameSingle");
    if ( GetResType() == RES_ITEMDEF )
    {
        const CItemBase * pItemBase = CItemBase::FindItemBase((ITEMID_TYPE)(m_rid.GetResIndex()));
        //DEBUG_ERR(("pItemBase 0x%x  m_rid 0%x  m_rid.GetResIndex() 0%x\n",pItemBase,m_rid,m_rid.GetResIndex()));
        if ( pItemBase )
        {
            lpctstr ptcSrc = pItemBase->GetNamePluralize(pItemBase->GetTypeName(),(( iQty > 1 ) ? true : false));
            return Str_CopyLimitNull(pszArgs, ptcSrc, uiBufLen);
        }
    }
    auto pResourceDef = static_cast<const CScriptObj *>(g_Cfg.RegisteredResourceGetDef(m_rid));
    if ( pResourceDef != nullptr )
        return Str_CopyLimitNull(pszArgs, pResourceDef->GetName(), uiBufLen);
    else
        return Str_CopyLimitNull(pszArgs, g_Cfg.ResourceGetName(m_rid), uiBufLen);
}

bool CResourceQty::Load(lpctstr &pszCmds)
{
    ADDTOCALLSTACK("CResourceQty::Load");
    // Can be either order.:
    // "Name Qty" or "Qty Name"

    lpctstr orig = pszCmds;
    GETNONWHITESPACE(pszCmds);	// Skip leading spaces.

    m_iQty = INT64_MIN;
    if ( !IsAlpha(*pszCmds) ) // might be { or .
    {
        m_iQty = Exp_GetVal(pszCmds);
        GETNONWHITESPACE(pszCmds);
    }

    if ( *pszCmds == '\0' )
    {
        g_Log.EventError("Bad resource list element in '%s'\n", orig);
        return false;
    }

    m_rid = g_Cfg.ResourceGetID_EatStr(RES_UNKNOWN, pszCmds);
    if ( m_rid.GetResType() == RES_UNKNOWN )
    {
        m_rid.Init();
        g_Log.EventError("Bad resource list id '%s'\n", orig);
        return false;
    }

    GETNONWHITESPACE(pszCmds);
    if ( m_iQty == INT64_MIN )	// trailing qty?
    {
        if ( *pszCmds == '\0' || *pszCmds == ',' )
            m_iQty = 1;
        else
        {
            m_iQty = Exp_GetVal(pszCmds);
            GETNONWHITESPACE(pszCmds);
        }
    }

    return true;
}


//**********************************************
// -CResourceQtyArray

CResourceQtyArray::CResourceQtyArray()
{
    m_mergeOnLoad = true;
}

CResourceQtyArray::CResourceQtyArray(lpctstr pszCmds)
{
    m_mergeOnLoad = true;
    Load(pszCmds);
}

void CResourceQtyArray::setNoMergeOnLoad()
{
    ADDTOCALLSTACK("CResourceQtyArray::setNoMergeOnLoad");
    m_mergeOnLoad = false;
}

size_t CResourceQtyArray::FindResourceType( RES_TYPE type ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::FindResourceType");
    // is this RES_TYPE in the array ?
    // BadIndex = fail
    for ( size_t i = 0, iQty = size(); i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetResourceID();
        if ( type == ridtest.GetResType() )
            return i;
    }
    return sl::scont_bad_index();
}

size_t CResourceQtyArray::FindResourceID( const CResourceID& rid ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::FindResourceID");
    // is this RESOURCE_ID in the array ?
    // BadIndex = fail
    for ( size_t i = 0, iQty = size(); i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetResourceID();
        if ( rid == ridtest )
            return i;
    }
    return sl::scont_bad_index();
}

size_t CResourceQtyArray::FindResourceMatch( const CObjBase * pObj ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::FindResourceMatch");
    // Is there a more vague match in the array ?
    // Use to find intersection with this pOBj raw material and BaseResource creation elements.
    for ( size_t i = 0, iQty = size(); i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetResourceID();
        if ( pObj->IsResourceMatch( ridtest, 0 ))
            return i;
    }
    return sl::scont_bad_index();
}

bool CResourceQtyArray::IsResourceMatchAll( const CChar * pChar ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::IsResourceMatchAll");
    // Check all required skills and non-consumable items.
    // RETURN:
    //  false = failed.
    ASSERT(pChar != nullptr);

    for ( size_t i = 0, iQty = size(); i < iQty; ++i )
    {
        const CResourceID& ridtest = (*this)[i].GetResourceID();

        if ( ! pChar->IsResourceMatch( ridtest, (uint)((*this)[i].GetResQty()) ))
            return false;
    }

    return true;
}

size_t CResourceQtyArray::Load(lpctstr pszCmds)
{
    ADDTOCALLSTACK("CResourceQtyArray::Load");
    //	clear-before-load in order not to mess with the previous data
    if ( !m_mergeOnLoad )
    {
        clear();
    }

    // 0 = clear the list.
    size_t iValid = 0;
    ASSERT(pszCmds);
    while ( *pszCmds )
    {
        if ( *pszCmds == '0' &&
            ( pszCmds[1] == '\0' || pszCmds[1] == ',' ))
        {
            clear();	// clear any previous stuff.
            reserve(4);
            ++pszCmds;
        }
        else
        {
            CResourceQty res;
            if ( !res.Load(pszCmds) )
                break;

            if ( res.GetResourceID().IsValidUID())
            {
                // Replace any previous refs to this same entry ?
                size_t i = FindResourceID( res.GetResourceID() );
                if ( i != sl::scont_bad_index() )
                {
                    operator[](i) = std::move(res);
                }
                else
                {
                    emplace_back(std::move(res));
                }
                ++iValid;
            }
        }

        if ( *pszCmds != ',' )
            break;

        ++pszCmds;
    }
    shrink_to_fit();

    return iValid;
}

void CResourceQtyArray::WriteKeys( tchar * pszArgs, size_t uiBufSize, size_t index, bool fQtyOnly, bool fKeyOnly ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::WriteKeys");
    size_t max = size();
    if ( index > 0 && index < max )
        max = index;

    for ( size_t i = (index > 0 ? index - 1 : 0); i < max; ++i )
    {
        if ( i > 0 && index == 0 )
        {
            pszArgs += sprintf( pszArgs, "," );
        }
        pszArgs += (*this)[i].WriteKey( pszArgs, uiBufSize, fQtyOnly, fKeyOnly );
    }
    *pszArgs = '\0';
}


void CResourceQtyArray::WriteNames( tchar * pszArgs, size_t uiBufSize, size_t index ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::WriteNames");
    int bytes_to_write;
    size_t max = size();
    if ( index > 0 && index < max )
        max = index;

    for ( size_t i = (index > 0 ? index - 1 : 0); i < max; ++i )
    {
        if ( i > 0 && index == 0 )
        {
            bytes_to_write = sprintf( pszArgs, ", " );
            pszArgs += StrncpyCharBytesWritten(bytes_to_write, uiBufSize);
        }

        const CResourceQty& resQty = (*this)[i];
        const int64 iQty = resQty.GetResQty();
        if ( iQty )
        {
            if (resQty.GetResType() == RES_SKILL )
            {
                bytes_to_write = sprintf( pszArgs, "%" PRId64 ".%" PRId64 " ", iQty / 10, iQty % 10);
                pszArgs += StrncpyCharBytesWritten(bytes_to_write, uiBufSize);
            }
            else
            {
                bytes_to_write = sprintf( pszArgs, "%" PRId64 " ", iQty);
                pszArgs += StrncpyCharBytesWritten(bytes_to_write, uiBufSize);
            }
        }

        pszArgs += resQty.WriteNameSingle( pszArgs, uiBufSize, (int)iQty );
    }
    *pszArgs = '\0';
}

bool CResourceQtyArray::operator == ( const CResourceQtyArray & array ) const
{
    ADDTOCALLSTACK("CResourceQtyArray::operator == ");
    if (size() != array.size())
        return false;

    for ( size_t i = 0, iQty = size(); i < iQty; ++i )
    {
        for ( size_t j = 0; ; ++j )
        {
            if ( j >= array.size() )
                return false;
            const CResourceQty& resQty = (*this)[i];
            if ( ! (resQty.GetResourceID() == array[j].GetResourceID() ) )
                continue;
            if (resQty.GetResQty() != array[j].GetResQty() )
                continue;
            break;
        }
    }
    return true;
}
