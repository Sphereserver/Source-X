#include "../common/CLog.h"
#include "../common/CUID.h"
#include "../sphere/ProfileTask.h"
#include "CObjBase.h"
#include "CServerConfig.h"
#include "CServerTime.h"
#include "CTimedFunctionHandler.h"


#define TF_TICK_MAGIC_NUMBER		99


CTimedFunctionHandler::CTimedFunctionHandler() :
	_strLoadBufferCommand(CTimedFunction::kuiCommandSize, '\0'),
	_strLoadBufferNumbers(CTimedFunction::kuiCommandSize, '\0')
{
}

void CTimedFunctionHandler::OnChildDestruct(CTimedFunction* tf)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::OnChildDestruct");
	for (auto it = _timedFunctions.begin(), itEnd = _timedFunctions.end(); it != itEnd; ++it)
	{
		if (it->get() == tf)
		{
			_timedFunctions.erase(it);
			return;
		}
	}
	ASSERT(false);
}

int64 CTimedFunctionHandler::IsTimer(const CUID& uid, lpctstr ptcCommand) const
{
	ADDTOCALLSTACK("CTimedFunctionHandler::IsTimer");
	for (std::unique_ptr<CTimedFunction> const& tf : _timedFunctions)	// the end iterator changes at each stl container erase call
	{
		if ((tf->GetUID() == uid) && (Str_Match(ptcCommand, tf->GetCommand()) == MATCH_VALID))
			return tf->GetTimerAdjusted();
	}
	return 0;
}

void CTimedFunctionHandler::ClearUID( const CUID& uid )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Erase");
	for (auto it = _timedFunctions.begin(); it != _timedFunctions.end(); )	// the end iterator changes at each stl container erase call
	{
		std::unique_ptr<CTimedFunction>& tf = *it;
		if (tf->GetUID() == uid)
			it =_timedFunctions.erase(it);
		else
			++it;
	}
}

void CTimedFunctionHandler::Stop(const CUID& uid, lpctstr ptcCommand)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Stop");
	for (auto it = _timedFunctions.begin(); it != _timedFunctions.end(); )	// the end iterator changes at each stl container erase call
	{
		std::unique_ptr<CTimedFunction>& tf = *it;
		if ((tf->GetUID() == uid) && (Str_Match(ptcCommand, tf->GetCommand()) == MATCH_VALID))
		{
			it = _timedFunctions.erase(it);
		}
		else
			++it;
	}
}

void CTimedFunctionHandler::Clear()
{
    ADDTOCALLSTACK("CTimedFunctionHandler::Clear");

    _timedFunctions.clear();
}

TRIGRET_TYPE CTimedFunctionHandler::Loop(lpctstr ptcCommand, int iLoopsMade, CScriptLineContext StartContext,
    CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Loop");
	for (auto it = _timedFunctions.begin(); it != _timedFunctions.end(); )	// the end iterator changes at each stl container erase call
	{
		++iLoopsMade;
		if (g_Cfg.m_iMaxLoopTimes && (iLoopsMade >= g_Cfg.m_iMaxLoopTimes))
		{
			g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed).\n", iLoopsMade);
			return TRIGRET_ENDIF;
		}

		std::unique_ptr<CTimedFunction>& tf = *it;
		if (!strcmpi(tf->GetCommand(), ptcCommand))
		{
			CObjBase* pObj = tf->GetUID().ObjFind();
			if (!pObj)
			{
			LoopStop:
				break;
			}

			TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);

			if (iRet == TRIGRET_BREAK)
			{
				goto LoopStop;
			}
			if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
				return iRet;
			s.SeekContext(StartContext);
		}

		++it;
	}

	return TRIGRET_ENDIF;
}

void CTimedFunctionHandler::Add(const CUID& uid, int64 iTimeout, const char* pcCommand)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Add");
	ASSERT(pcCommand != nullptr);
	ASSERT(strlen(pcCommand) < CTimedFunction::kuiCommandSize);

	auto& tf = _timedFunctions.emplace_back(std::make_unique<CTimedFunction>(this, uid, pcCommand));
	tf->SetTimeout(iTimeout);
}

int CTimedFunctionHandler::Load(lpctstr ptcKeyword, bool fQuoted, lpctstr ptcArg)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Load");
	UNREFERENCED_PARAMETER(fQuoted);
	if (!ptcKeyword)
		return -1;

	static constexpr lpctstr ptcErrorPair = "Invalid TimerF in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n";
	if (!strnicmp(ptcKeyword, "TimerFCall", 11))
	{
		if (_strLoadBufferCommand[0] != '\0')
		{
			// A TimerFCall wasn't called before this TimerFNumbers, so tf doesn't contain a valid CTimedFunction object.
			g_Log.Event(LOGM_INIT | LOGL_ERROR, ptcErrorPair, SPHERE_FILE);
			return -1;
		}
		ASSERT(_strLoadBufferNumbers[0] == '\0');
		Str_CopyLimitNull(_strLoadBufferCommand.data(), ptcArg, _strLoadBufferCommand.size());
	}
	else if ( !strnicmp(ptcKeyword, "TimerFNumbers", 13) )
	{
		if (_strLoadBufferCommand[0] == '\0')
		{
			// A TimerFCall wasn't called before this TimerFNumbers, so tf doesn't contain a valid CTimedFunction object.
			g_Log.Event(LOGM_INIT | LOGL_ERROR, ptcErrorPair, SPHERE_FILE);
			return -1;
		}

		Str_CopyLimitNull(_strLoadBufferNumbers.data(), ptcArg, _strLoadBufferNumbers.size());	// because ptcArg is constant and Str_ParseCmds wants a non-constant string
		tchar* ppVal[3];
		const size_t iArgs = Str_ParseCmds(_strLoadBufferNumbers.data(), ppVal, CountOf(ppVal), " ,\t" );
		if ( iArgs != 3 )
        {
            g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid TimerF line in %sdata.scp: %s=%s (arguments mismatch: 3 needed).\n", SPHERE_FILE, ptcKeyword, ptcArg);
            return -1;
        }

        const auto oldErrno = errno;
		errno = 0;

		// The "tick" value was used on the old 56* timerf ticking system. It always was a positive number.
		//  Now X replaces this value in the save files to TF_TICK_MAGIC_NUMBER, so that we know that it comes from a X save file.
		//  We need that info because on X we save the timeout in milliseconds, on 0.56 it was stored in tenths of second.
        const int tick = (int)std::strtol(ppVal[0], nullptr, 10);
		if (errno == ERANGE)
		{
			g_Log.Event(LOGM_INIT | LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Invalid legacy tick (first value=%s).\n", SPHERE_FILE, ppVal[0]);
			errno = oldErrno;
			return -1;
		}

		const unsigned long uidTest = std::strtoul(ppVal[1], nullptr, 10);
        if ((errno == ERANGE) || (uidTest > UINT32_MAX))
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Invalid UID (second value=%s).\n", SPHERE_FILE, ppVal[1]);
            errno = oldErrno;
            return -1;
        }
		const uint uid = (uint)uidTest;

        errno = 0;
		int64 elapsed = (int64)std::strtoll(ppVal[2], nullptr, 10);
        if (errno == ERANGE)
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Invalid elapsed time (third value=%s).\n", SPHERE_FILE, ppVal[2]);
            errno = oldErrno;
            return -1;
        }
        errno = oldErrno;

		if (tick < TF_TICK_MAGIC_NUMBER)
		{
			// It's a 0.56 save file. Convert this timeout from ticks (tenths of second) in milliseconds.
			elapsed *= MSECS_PER_TENTH;
		}
		Add(CUID(uid), elapsed, _strLoadBufferCommand.c_str());
		_strLoadBufferCommand[0] = '\0';
		_strLoadBufferNumbers[0] = '\0';
	}

	return 0;
}

void CTimedFunctionHandler::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::r_Write");
	for (std::unique_ptr<CTimedFunction> const& tf : _timedFunctions)
	{
		const CUID& uid = tf->GetUID();
		if (uid.IsValidUID())
		{
			s.WriteKeyFormat("TimerFCall", "%s", tf->GetCommand());
			s.WriteKeyFormat("TimerFNumbers", STRINGIFY(TF_TICK_MAGIC_NUMBER) ",%" PRIu32 ",%" PRId64, uid.GetObjUID(), tf->GetTimerAdjusted());
		}
	}
}
