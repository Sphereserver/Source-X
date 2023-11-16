#ifndef _INC_CSTARTLOC_H
#define _INC_CSTARTLOC_H

#include "../common/sphere_library/CSString.h"
#include "../common/CPointBase.h"


class CStartLoc		// The start locations for creating a new char.
{
public:
	static const char* m_sClassName;

	CSString m_sArea;	// Area/City Name = Britain or Occlo
	CSString m_sName;	// Place name = Castle Britannia or Docks
	CPointMap m_pt;
	int iClilocDescription; //Only for clients 7.00.13 +

	explicit CStartLoc(lpctstr pszArea) noexcept :
		m_sArea(pszArea), iClilocDescription(1149559)
	{}
	~CStartLoc() noexcept = default;
	
	CStartLoc(const CStartLoc& copy) = delete;
	CStartLoc& operator=(const CStartLoc& other) = delete;
};


#endif // _INC_CSTARTLOC_H
