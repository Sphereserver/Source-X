#ifndef _INC_CTELEPORT_H
#define _INC_CTELEPORT_H

#include "../common/CPointBase.h"


class CTeleport : public CPointSort	// The static world teleporters.
{
	// Put a built in trigger here ? can be Array sorted by CPointMap.
public:
	static const char* m_sClassName;

	bool _fNpc;
	CPointMap _ptDst;


	explicit CTeleport(const CPointMap& pt) : CPointSort(pt)
	{
		ASSERT(pt.IsValidPoint());
		_ptDst = pt;
		_fNpc = false;
	}
	explicit CTeleport(tchar* pszArgs);
	~CTeleport() noexcept = default;

	CTeleport(const CTeleport& copy) = delete;
	CTeleport& operator=(const CTeleport& other) = delete;


	bool RealizeTeleport();
};


#endif // _INC_CTELEPORT_H