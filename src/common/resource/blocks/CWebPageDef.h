/**
* @file CWebPageDef.h
*
*/

#ifndef _INC_CWEBPAGEDEF_H
#define _INC_CWEBPAGEDEF_H

#include "../CResourceLink.h"
#include "../CValueDefs.h"

class CClient;


enum WTRIG_TYPE
{
    // XTRIG_UNKNOWN	= some named trigger not on this list.
    WTRIG_Load = 1,
    WTRIG_QTY
};

enum WEBPAGE_TYPE
{
    WEBPAGE_TEMPLATE,
    WEBPAGE_TEXT,
    WEBPAGE_BMP,
    WEBPAGE_GIF,
    WEBPAGE_JPG,
    WEBPAGE_QTY
};

class CWebPageDef : public CResourceLink
{
    // RES_WEBPAGE

    // This is a single web page we are generating or serving.
    static lpctstr const sm_szLoadKeys[];
    static lpctstr const sm_szVerbKeys[];
    static lpctstr const sm_szPageType[];
    static lpctstr const sm_szPageExt[];
private:
    WEBPAGE_TYPE m_type;        // What basic format of file is this ? 0=text
    CSString m_sSrcFilePath;    // source template for the generated web page.
private:
    PLEVEL_TYPE m_privlevel;    // What priv level to see this page ?

                                // For files that are being translated and updated.
    CSString m_sDstFilePath;		// where is the page served from ?
    int m_iUpdatePeriod;			// How often to update the web page in seconds. 0 = never.
    int m_iUpdateLog;				// create a daily log of the page.
    int64  m_timeNextUpdate;	// The time at will be done the next web update.

public:
    static const char *m_sClassName;
    static int sm_iListIndex;
    static lpctstr const sm_szTrigName[WTRIG_QTY + 1];
private:

    /**
    * @fn  int CWebPageDef::ServPageRequest( CClient * pClient, lpctstr pszURLArgs, CSTime * pdateLastMod );
    *
    * @brief   Serv page requested.
    *
    * @param [in,out]  pClient         If non-null, the client.
    * @param   pszURLArgs              The URL arguments.
    * @param [in,out]  pdateLastMod    If non-null, the pdate last modifier.
    *
    * @return  An int.
    */
    int ServPageRequest(CClient * pClient, lpctstr pszURLArgs, CSTime * pdateLastMod);
public:

    /**
    * @fn  lpctstr CWebPageDef::GetName() const
    *
    * @brief   Gets the source template for the generated web page (m_sSrcFilePath).
    *
    * @return  m_sSrcFilePath.
    */
    lpctstr GetName() const
    {
        return m_sSrcFilePath;
    }

    /**
    * @fn  lpctstr CWebPageDef::GetDstName() const
    *
    * @brief   Gets where is the page served from.
    *
    * @return  m_sDstFilePath.
    */
    lpctstr GetDstName() const
    {
        return m_sDstFilePath;
    }

    /**
    * @fn  bool CWebPageDef::IsMatch( lpctstr IsMatchPage ) const;
    *
    * @brief   Query if the requested page exists..
    *
    * @param   IsMatchPage The is match page.
    *
    * @return  true if match, false if not.
    */
    bool IsMatch(lpctstr IsMatchPage) const;

    /**
    * @fn  bool CWebPageDef::SetSourceFile( lpctstr pszName, CClient * pClient );
    *
    * @brief   Sets source file to be given.
    *
    * @param   pszName         The name.
    * @param [in,out]  pClient If non-null, the client.
    *
    * @return  true if it succeeds, false if it fails.
    */
    bool SetSourceFile(lpctstr pszName, CClient * pClient);

    /**
    * @fn  bool CWebPageDef::ServPagePost( CClient * pClient, lpctstr pszURLArgs, tchar * pPostData, size_t stContentLength );
    *
    * @brief   Translate scripted content.
    *
    * @param [in,out]  pClient     If non-null, the client.
    * @param   pszURLArgs          The URL arguments.
    * @param [in,out]  pPostData   If non-null, information describing the post.
    * @param   stContentLength     Length of the content.
    *
    * @return  true if it succeeds, false if it fails.
    */
    bool ServPagePost(CClient * pClient, lpctstr pszURLArgs, tchar * pPostData, size_t stContentLength);

    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;	// some command on this object as a target

    /**
    * @fn  void CWebPageDef::WebPageLog();
    *
    * @brief   Web page log.
    *
    */
    void WebPageLog();

    /**
    * @fn  bool CWebPageDef::WebPageUpdate( bool fNow, lpctstr pszDstName, CTextConsole * pSrc );
    *
    * @brief   Web page update proccess.
    *
    * @param   fNow            true to now.
    * @param   pszDstName      Name of the destination.
    * @param [in,out]  pSrc    If non-null, source for the.
    *
    * @return  true if it succeeds, false if it fails.
    */
    bool WebPageUpdate(bool fNow, lpctstr pszDstName, CTextConsole * pSrc);

    /**
    * @fn  static bool CWebPageDef::ServPage( CClient * pClient, tchar * pszPage, CSTime * pdateLastMod );
    *
    * @brief   Final checks: Control if page can be sent and does it (sends also 404 if not, and some other fails).
    *
    * @param [in,out]  pClient         If non-null, the client.
    * @param [in,out]  pszPage         If non-null, the page.
    * @param [in,out]  pdateLastMod    If non-null, the pdate last modifier.
    *
    * @return  true if it succeeds, false if it fails.
    */

    static bool ServPage(CClient * pClient, tchar * pszPage, CSTime * pdateLastMod);

public:
    explicit CWebPageDef(CResourceID id);
    virtual ~CWebPageDef()
    {
    }

private:
    CWebPageDef(const CWebPageDef& copy);
    CWebPageDef& operator=(const CWebPageDef& other);
};


#endif // _INC_CWEBPAGEDEF_H
