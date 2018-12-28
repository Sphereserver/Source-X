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
	// ONly one page allowed per account at a time.
	static lpctstr const sm_szLoadKeys[];
private:
	CSString m_sAccount;	// The account that paged me.
	CClient * m_pGMClient;	// assigned to a GM
	CSString m_sReason;		// Players Description of reason for call.

public:
	static const char *m_sClassName;
	// Queue a GM page. (based on account)
	int64  m_timePage;      // Time of the last call.
	CPointMap  m_ptOrigin;  // Origin Point of call.

public:
	CGMPage( lpctstr pszAccount );
	~CGMPage();

private:
	CGMPage(const CGMPage& copy);
	CGMPage& operator=(const CGMPage& other);

public:
	CAccount * FindAccount() const;
	lpctstr GetAccountStatus() const;
	lpctstr GetName() const;
	lpctstr GetReason() const;
	void SetReason( lpctstr pszReason );
	CClient * FindGMHandler() const;
	void ClearGMHandler();
	void SetGMHandler( CClient * pClient );
	int64 GetAge() const;

	virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc ) override;
	void r_Write( CScript & s ) const;
	virtual bool r_LoadVal( CScript & s ) override;

	CGMPage * GetNext() const;
};


#endif // _INC_CGMPAGE_H
