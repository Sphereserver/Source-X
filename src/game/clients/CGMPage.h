/**
* @file CGMPage.h
*
*/

#ifndef _INC_CGMPAGE_H
#define _INC_CGMPAGE_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/CScriptObj.h"
#include "../../common/CRect.h"
#include "CAccount.h"


class CClient;	//fixme
class CGMPage : public CSObjListRec, public CScriptObj
{
	// RES_GMPAGE
public:
	static const char* m_sClassName;
	static LPCTSTR const sm_szLoadKeys[];
	CGMPage(LPCTSTR pszAccount);
	~CGMPage();

public:
	CClient* m_pClientHandling;
	CSString m_sAccount;
	CUID m_uidChar;
	CPointMap m_pt;
	CSString m_sReason;
	CServerTime m_time;

public:
	void SetHandler(CClient* pClient);
	void ClearHandler();

	LPCTSTR GetName() const		{ return m_sAccount; }
	CGMPage* GetNext() const	{ return static_cast<CGMPage*>(CSObjListRec::GetNext()); }

	void r_Write(CScript& s) const;
	bool r_WriteVal(LPCTSTR pszKey, CSString& sVal, CTextConsole* pSrc);
	bool r_LoadVal(CScript& s);

private:
	CGMPage(const CGMPage& copy);
	CGMPage& operator=(const CGMPage& other);
};

#endif	// _INC_CGMPAGE_H