/**
* @file CDialogDef.h
*
*/

#ifndef _INC_CDIALOGDEF_H
#define _INC_CDIALOGDEF_H

#include "../CResourceLink.h"

class CClient;


class CDialogDef : public CResourceLink
{
    static lpctstr const sm_szLoadKeys[];

public:
    static const char *m_sClassName;
    bool GumpSetup( int iPage, CClient * pClientSrc, CObjBase * pObj, lpctstr Arguments = "" );
    size_t GumpAddText( lpctstr pszText );		// add text to the text section, return insertion index
    virtual bool r_Verb( CScript &s, CTextConsole * pSrc ) override;
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc ) override;

public:
    explicit CDialogDef( CResourceID rid );
    virtual ~CDialogDef() = default;

private:
    CDialogDef(const CDialogDef& copy);
    CDialogDef& operator=(const CDialogDef& other);

public:
    // temporary placeholders - valid only during dialog setup
    CObjBase *	m_pObj;
    CSString	m_sControls[1024];
    CSString	m_sText[512];
    size_t		m_iTexts;
    size_t		m_iControls;
    int			m_x;
    int			m_y;

    int			m_iOriginX;	// keep track of position when parsing
    int			m_iOriginY;
    word		m_iPage;		// page to open the dialog in

    bool		m_bNoDispose;	// contains 'nodispose' control
};

#endif // _INC_CDIALOGDEF_H
