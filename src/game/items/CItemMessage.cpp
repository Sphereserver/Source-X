
#include "../../common/CException.h"
#include "CItemMessage.h"
#include "CItemVendable.h"

CItemMessage::CItemMessage( ITEMID_TYPE id, CItemBase * pItemDef ) :
    CTimedObject(PROFILE_ITEMS),
    CItemVendable( id, pItemDef )
{
}

CItemMessage::~CItemMessage()
{
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
    UnLoadSystemPages();
}

lpctstr const CItemMessage::sm_szLoadKeys[CIC_QTY+1] =
{
    "AUTHOR",
    "BODY",
    "PAGES",	// (W)
    "TITLE",	// same as name
    nullptr
};

void CItemMessage::r_Write(CScript & s)
{
    ADDTOCALLSTACK_DEBUG("CItemMessage::r_Write");
    CItemVendable::r_Write(s);
    s.WriteKeyStr("AUTHOR", m_sAuthor.GetBuffer());

    // Store the message body lines. MAX_BOOK_PAGES
    TemporaryString tsTemp;
    for ( word i = 0; i < GetPageCount(); ++i )
    {
        snprintf(tsTemp.buffer(), tsTemp.capacity(), "BODY.%" PRIu16, i);
        lpctstr pszText = GetPageText(i);
        s.WriteKeyStr(tsTemp.buffer(), ((pszText != nullptr) ? pszText : ""));
    }
}

bool CItemMessage::r_LoadVal(CScript &s)
{
    ADDTOCALLSTACK("CItemMessage::r_LoadVal");
    EXC_TRY("LoadVal");
        // Load the message body for a book or a bboard message.
        if ( s.IsKeyHead("BODY", 4) )
        {
            AddPageText(s.GetArgStr());
            return true;
        }

        switch ( FindTableSorted(s.GetKey(), sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1) )
        {
            case CIC_AUTHOR:
            {
                tchar* ptcArg = s.GetArgStr();
                if (ptcArg[0] != '0')
                    m_sAuthor = ptcArg;
            }
                return true;
            case CIC_BODY:		// handled above
                return false;
            case CIC_PAGES:		// not settable (used for resource stuff)
                return false;
            case CIC_TITLE:
                SetName(s.GetArgStr());
                return true;
        }
        return CItemVendable::r_LoadVal(s);
    EXC_CATCH;

    EXC_DEBUG_START;
            EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CItemMessage::r_WriteVal(lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CItemMessage::r_WriteVal");
    EXC_TRY("WriteVal");

    // Load the message body for a book or a bboard message.
    if (!strnicmp(ptcKey, "BODY", 4))
    {
        ptcKey += 4;
        const size_t uiPage = Exp_GetSTVal(ptcKey);
        if (m_sBodyLines.IsValidIndex(uiPage) == false)
            return false;
        sVal = *m_sBodyLines[uiPage];
        return true;
    }

    switch (FindTableSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1))
    {
        case CIC_AUTHOR:
            sVal = m_sAuthor;
            return true;
        case CIC_BODY:		// handled above
            return false;
        case CIC_PAGES:		// not settable (used for resource stuff)
            sVal.FormatSTVal(m_sBodyLines.size());
            return true;
        case CIC_TITLE:
            sVal = GetName();
            return true;
    }
    return (fNoCallParent ? false : CItemVendable::r_WriteVal(ptcKey, sVal, pSrc));
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}

lpctstr const CItemMessage::sm_szVerbKeys[] =
{
    "ERASE",
    "PAGE",
    nullptr
};

bool CItemMessage::r_Verb(CScript & s, CTextConsole *pSrc)
{
    ADDTOCALLSTACK("CItemMessage::r_Verb");
    EXC_TRY("Verb");
        ASSERT(pSrc);
        if ( s.IsKey(sm_szVerbKeys[0]) )
        {
            // 1 based pages
            word wPage = (s.GetArgStr()[0] && toupper(s.GetArgStr()[0]) != 'A') ? s.GetArgWVal() : 0;
            if ( wPage <= 0 )
            {
                m_sBodyLines.ClearFree();
                return true;
            }
            else if ( wPage <= m_sBodyLines.size() )
            {
                m_sBodyLines.erase_at(wPage - 1);
                return true;
            }
        }
        if ( s.IsKeyHead("PAGE", 4) )
        {
            word wPage = (word)atoi(s.GetKey() + 4);
            if ( wPage <= 0 )
                return false;

            SetPageText(wPage - 1, s.GetArgStr());
            return true;
        }
        return CItemVendable::r_Verb(s, pSrc);
    EXC_CATCH;

    EXC_DEBUG_START;
            EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}

void CItemMessage::DupeCopy(const CObjBase *pItemObj)
{
    ADDTOCALLSTACK("CItemMessage::DupeCopy");
    auto pItem = dynamic_cast<const CItem*>(pItemObj);
    ASSERT(pItem);

    CItemVendable::DupeCopy(pItem);

    const CItemMessage *pMsgItem = dynamic_cast<const CItemMessage *>(pItem);
    if ( pMsgItem == nullptr )
        return;

    m_sAuthor = pMsgItem->m_sAuthor;
    for ( word i = 0; i < pMsgItem->GetPageCount(); ++i )
        SetPageText(i, pMsgItem->GetPageText(i));
}

word CItemMessage::GetPageCount() const
{
    size_t sz = m_sBodyLines.size();
    ASSERT(sz < RES_PAGE_MAX);
    return (word)sz;
}

lpctstr CItemMessage::GetPageText( word wPage ) const
{
    if ( m_sBodyLines.IsValidIndex(wPage) == false )
        return nullptr;
    if ( m_sBodyLines[wPage] == nullptr )
        return nullptr;
    return m_sBodyLines[wPage]->GetBuffer();
}

void CItemMessage::SetPageText( word wPage, lpctstr pszText )
{
    if ( pszText == nullptr )
        return;
    m_sBodyLines.assign_at_grow(wPage, new CSString(pszText));
}

void CItemMessage::AddPageText( lpctstr pszText )
{
    m_sBodyLines.emplace_back(new CSString(pszText));
}

void CItemMessage::UnLoadSystemPages()
{
    m_sAuthor.Clear();
    m_sBodyLines.ClearFree();
}
