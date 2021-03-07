#include "../common/sphere_library/sstring.h"
#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "CObjBase.h"
#include "CServer.h"
#include "CTimedFunctionHandler.h"
#include "CTimedFunction.h"


CTimedFunction::CTimedFunction(CTimedFunctionHandler* pHandler, const CUID& uidAttached, const char* pcCommand) :
	CTimedObject(PROFILE_TIMEDFUNCTIONS),
	_pHandler(pHandler), _uidAttached(uidAttached)
{
	Str_CopyLimitNull(_ptcCommand, pcCommand, kuiCommandSize);
}

bool CTimedFunction::IsDeleted() const // virtual
{
	return false;
}

bool CTimedFunction::OnTick() // virtual
{
	ADDTOCALLSTACK("CTimedFunction::OnTick");

	const ProfileTask scriptsTask(PROFILE_TIMEDFUNCTIONS);

	const CUID uid(_uidAttached);
	CScript s(_ptcCommand);
	
	CTimedObject::OnTick();
	_pHandler->OnChildDestruct(this); // This has to be the last function call to ever access this object!

	// From now on, this object does NOT exist anymore!

	CObjBase* obj = uid.ObjFind();
	if (obj != nullptr) //just in case
	{
		CObjBaseTemplate* topobj = obj->GetTopLevelObj();
		ASSERT(topobj);

		CTextConsole* src;
		if (topobj->IsChar()) // only chars are derived classes from CTextConsole
			src = dynamic_cast <CTextConsole*> (topobj);
		else
			src = &g_Serv;

		obj->r_Verb(s, src); // Should we retrieve the result and show an error log if false is returned?
		return true;
	}
	else
	{
		if (g_Cfg.m_iDebugFlags & DEBUGF_SCRIPTS)
		{
			const uint uiUid = uid.GetObjUID();
			g_Log.EventDebug("[DEBUG_SCRIPTS] Blocked TIMERF(MS) execution on unused UID (0%x, dec %u'): '%s'.\n", uiUid, uiUid, _ptcCommand);
		}
	}

	return false;
}
