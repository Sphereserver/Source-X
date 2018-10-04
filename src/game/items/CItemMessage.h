/**
* @file CItemMessage.h
*
*/

#ifndef _INC_CITEMMESSAGE_H
#define _INC_CITEMMESSAGE_H

#include "../../common/sphere_library/CSString.h"
#include "CItemVendable.h"


enum CIC_TYPE
{
	CIC_AUTHOR,
	CIC_BODY,
	CIC_PAGES,
	CIC_TITLE,
	CIC_QTY
};

class CItemMessage : public CItemVendable	// A message for a bboard or book text.
{
    // IT_BOOK, IT_MESSAGE = can be written into.
    // the name is the title for the message. (ITEMID_BBOARD_MSG)
protected:
    static lpctstr const sm_szVerbKeys[];
private:
    CSObjArray<CSString*> m_sBodyLines;	// The main body of the text for bboard message or book.
public:
    static const char *m_sClassName;
    CSString m_sAuthor;					// Should just have author name !
    static lpctstr const sm_szLoadKeys[CIC_QTY+1];

public:
    CItemMessage( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemMessage();

private:
    CItemMessage(const CItemMessage& copy);
    CItemMessage& operator=(const CItemMessage& other);

public:
    virtual void r_Write( CScript & s );
    virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc );
    virtual bool r_LoadVal( CScript & s );
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script
    virtual bool IsDeleted()
    {
        return CItem::IsDeleted();
    }

    size_t GetPageCount() const;
    lpctstr GetPageText( size_t iPage ) const;
    void SetPageText( size_t iPage, lpctstr pszText );
    void AddPageText( lpctstr pszText );

    virtual void DupeCopy( const CItem * pItem );
    void UnLoadSystemPages();
};


#endif // _INC_CITEMMESSAGE_H
