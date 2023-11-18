#include "CTeleport.h"
#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "CSector.h"


//************************************************************************
// -CTeleport

CTeleport::CTeleport(tchar* pszArgs)
{
	// RES_TELEPORT
	// Assume valid iArgs >= 5

	tchar* ppCmds[4];
	size_t iArgs = Str_ParseCmds(pszArgs, ppCmds, ARRAY_COUNT(ppCmds), "=");
	if (iArgs < 2)
	{
		DEBUG_ERR(("Bad CTeleport Def\n"));
		_fNpc = false;
		return;
	}
	Read(ppCmds[0]);
	_ptDst.Read(ppCmds[1]);
	if (ppCmds[3])
		_fNpc = (Str_ToI(ppCmds[3]) != 0);
	else
		_fNpc = false;
}

bool CTeleport::RealizeTeleport()
{
	ADDTOCALLSTACK("CTeleport::RealizeTeleport");
	if (!IsCharValid() || !_ptDst.IsCharValid())
	{
		DEBUG_ERR(("CTeleport bad coords %s\n", WriteUsed()));
		return false;
	}
	CSector* pSector = GetSector();
	if (pSector)
		return pSector->AddTeleport(this);
	else
		return false;
}

CTeleport::~CTeleport() noexcept
{
	fprintf(stderr, "deleted %s.\n", _ptDst.WriteUsed());
}
