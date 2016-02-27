#ifndef _INC_CITEMMESSAGE_H
#define _INC_CITEMMESSAGE_H

#include "graysvr.h"

class CItemMessage : public CItemVendable	// A message for a bboard or book text.
{
    // IT_BOOK, IT_MESSAGE = can be written into.
    // the name is the title for the message. (ITEMID_BBOARD_MSG)
protected:
    static LPCTSTR const sm_szVerbKeys[];
private:
    CGObArray<CGString*> m_sBodyLines;	// The main body of the text for bboard message or book.
public:
    static const char *m_sClassName;
    CGString m_sAuthor;					// Should just have author name !
    static LPCTSTR const sm_szLoadKeys[CIC_QTY+1];

public:
    CItemMessage( ITEMID_TYPE id, CItemBase * pItemDef );
    virtual ~CItemMessage();

private:
    CItemMessage(const CItemMessage& copy);
    CItemMessage& operator=(const CItemMessage& other);

public:
    virtual void r_Write( CScript & s );
    virtual bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
    virtual bool r_LoadVal( CScript & s );
    virtual bool r_Verb( CScript & s, CTextConsole * pSrc ); // Execute command from script

    size_t GetPageCount() const;
    LPCTSTR GetPageText( size_t iPage ) const;
    void SetPageText( size_t iPage, LPCTSTR pszText );
    void AddPageText( LPCTSTR pszText );

    virtual void DupeCopy( const CItem * pItem );
    void UnLoadSystemPages();
};


#endif //_INC_CITEMMESSAGE_H
