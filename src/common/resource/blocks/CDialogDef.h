/**
* @file CDialogDef.h
*
*/

#ifndef _INC_CDIALOGDEF_H
#define _INC_CDIALOGDEF_H

#include "../../sphere_library/CSString.h"
#include "../CResourceLink.h"

class CClient;


class CDialogDef : public CResourceLink
{
    static lpctstr const sm_szLoadKeys[];

public:
    static const char *m_sClassName;
    bool GumpSetup( int iPage, CClient * pClientSrc, CObjBase * pObj, lpctstr Arguments = "" );
    uint GumpAddText( lpctstr pszText );		// add text to the text section, return insertion index
    virtual bool r_Verb( CScript &s, CTextConsole * pSrc ) override;
    virtual bool r_LoadVal( CScript & s ) override;
    virtual bool r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false) override;

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
    uint		m_uiTexts;
    uint		m_uiControls;
    int			m_x;
    int			m_y;

    int			m_iOriginX;	// keep track of position when parsing
    int			m_iOriginY;
    word		m_wPage;		// page to open the dialog in

    bool		m_fNoDispose;	// contains 'nodispose' control
};

#endif // _INC_CDIALOGDEF_H
