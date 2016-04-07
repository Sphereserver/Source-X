
#pragma once
#ifndef _INC_CGMPAGE_H
#define _INC_CGMPAGE_H

#include "../common/sphere_library/CString.h"
#include "../common/CArray.h"
#include "../common/CScriptObj.h"
#include "../common/CRect.h"
#include "../CServTime.h"
#include "CAccount.h"


class CClient;	//fixme
//class CAccountRef;
class CGMPage : public CGObListRec, public CScriptObj
{
	// RES_GMPAGE
	// ONly one page allowed per account at a time.
	static lpctstr const sm_szLoadKeys[];
private:
	CGString m_sAccount;	// The account that paged me.
	CClient * m_pGMClient;	// assigned to a GM
	CGString m_sReason;		// Players Description of reason for call.

public:
	static const char *m_sClassName;
	// Queue a GM page. (based on account)
	CServTime  m_timePage;	// Time of the last call.
	CPointMap  m_ptOrigin;		// Origin Point of call.

public:
	CGMPage( lpctstr pszAccount );
	~CGMPage();

private:
	CGMPage(const CGMPage& copy);
	CGMPage& operator=(const CGMPage& other);

public:
	CAccountRef FindAccount() const;
	lpctstr GetAccountStatus() const;
	lpctstr GetName() const;
	lpctstr GetReason() const;
	void SetReason( lpctstr pszReason );
	CClient * FindGMHandler() const;
	void ClearGMHandler();
	void SetGMHandler( CClient * pClient );
	int64 GetAge() const;

	bool r_WriteVal( lpctstr pszKey, CGString &sVal, CTextConsole * pSrc );
	void r_Write( CScript & s ) const;
	bool r_LoadVal( CScript & s );

	CGMPage * GetNext() const;
};

#endif // _INC_CGMPAGE_H
