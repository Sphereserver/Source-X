#include "../common/sphere_library/sstring.h"
#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "CObjBase.h"
#include "CServer.h"
#include "CTimedFunctionHandler.h"
#include "CTimedFunction.h"


CTimedFunction::CTimedFunction(const CUID& uidAttached, const char* pcCommand) :
	CTimedObject(PROFILE_TIMEDFUNCTIONS),
	_uidAttached(uidAttached)
{
	Str_CopyLimitNull(_ptcCommand, pcCommand, kuiCommandSize);
}


bool CTimedFunction::_IsDeleted() const // virtual
{
	return false;
}

bool CTimedFunction::IsDeleted() const // virtual
{
	return false;
}


bool CTimedFunction::_CanTick(bool fParentGoingToSleep) const // virtual
{
	UnreferencedParameter(fParentGoingToSleep);
	return true;
}

bool CTimedFunction::CanTick(bool fParentGoingToSleep) const // virtual
{
	UnreferencedParameter(fParentGoingToSleep);
	return true;
}

static bool _ExecTimedFunction(CUID&& uid, CScript&& s)
{
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
			g_Log.EventDebug("[DEBUG_SCRIPTS] Blocked TIMERF(MS) execution on unused UID (0%x, dec %u'): '%s'.\n", uiUid, uiUid, s.GetKey());
		}
	}

	return false;
}

bool CTimedFunction::_OnTick() // virtual
{
	ADDTOCALLSTACK("CTimedFunction::_OnTick");

	CUID uid(_uidAttached);
	CScript s(_ptcCommand);

	delete this; // This has to be the last function call to ever access this object!

	// From now on, this object does NOT exist anymore!
	return _ExecTimedFunction(std::move(uid), std::move(s));
}

bool CTimedFunction::OnTick() // virtual
{
	ADDTOCALLSTACK("CTimedFunction::OnTick");

	CUID uid;
	CScript s;
	{
		MT_ENGINE_SHARED_LOCK_SET;
		uid.SetPrivateUID(_uidAttached);
		s.ParseKey(_ptcCommand);
	}

	delete this; // This has to be the last function call to ever access this object!

	// From now on, this object does NOT exist anymore!
	return _ExecTimedFunction(std::move(uid), std::move(s));
}
