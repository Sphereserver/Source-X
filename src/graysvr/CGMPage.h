#ifndef _INC_CGMPAGE_H
#define _INC_CGMPAGE_H

#include "../common/CString.h"
#include "../common/CArray.h"
#include "../common/CScriptObj.h"
#include "../common/CRect.h"
#include "CServTime.h"
#include "CAccount.h"


class CClient;	//fixme
//class CAccountRef;
class CGMPage : public CGObListRec, public CScriptObj
{
	// RES_GMPAGE
	// ONly one page allowed per account at a time.
	static LPCTSTR const sm_szLoadKeys[];
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
	CGMPage( LPCTSTR pszAccount );
	~CGMPage();

private:
	CGMPage(const CGMPage& copy);
	CGMPage& operator=(const CGMPage& other);

public:
	CAccountRef FindAccount() const;
	LPCTSTR GetAccountStatus() const;
	LPCTSTR GetName() const
	{
		return( m_sAccount );
	}
	LPCTSTR GetReason() const
	{
		return( m_sReason );
	}
	void SetReason( LPCTSTR pszReason )
	{
		m_sReason = pszReason;
	}
	CClient * FindGMHandler() const
	{
		return( m_pGMClient );
	}
	void ClearGMHandler();
	void SetGMHandler( CClient * pClient );
	INT64 GetAge() const;

	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );
	void r_Write( CScript & s ) const;
	bool r_LoadVal( CScript & s );

	CGMPage * GetNext() const
	{
		return( STATIC_CAST <CGMPage*>( CGObListRec::GetNext()));
	}
};

#endif // _INC_CGMPAGE_H
