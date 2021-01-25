#include "../common/CLog.h"
#include "../sphere/ProfileTask.h"
#include "CObjBase.h"
#include "CServer.h"
#include "CServerConfig.h"
#include "CTimedFunctionHandler.h"

CTimedFunctionHandler::CTimedFunctionHandler()
{
	m_curTick = 0;
	m_processedFunctionsPerTick = 0;
	m_isBeingProcessed = false;
}

void CTimedFunctionHandler::OnTick()
{
	ADDTOCALLSTACK("CTimedFunctionHandler::OnTick");
	m_isBeingProcessed = true;

	++m_curTick;

	if ( m_curTick >= TICKS_PER_SEC)
		m_curTick = 0;

	int tick = m_curTick;
	const ProfileTask scriptsTask(PROFILE_TIMEDFUNCTIONS);


	if ( !m_timedFunctions[tick].empty() )
	{
		for ( auto it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); )	
		{
			++m_processedFunctionsPerTick;
			if (g_Cfg.m_iMaxLoopTimes && (m_processedFunctionsPerTick >= g_Cfg.m_iMaxLoopTimes))
			{
				g_Log.EventError("Terminating TIMERF executions for this tick, since it seems being dead-locked (%d iterations already passed)\n", m_processedFunctionsPerTick);
				break;
			}
			TimedFunction* tf = *it;
			tf->elapsed -= 1;
			if ( tf->elapsed <= 1 )
			{
				CObjBase * obj = tf->uid.ObjFind();

				if ( obj != nullptr ) //just in case
				{
                    CScript s(tf->funcname);
					CObjBaseTemplate * topobj = obj->GetTopLevelObj();
                    ASSERT(topobj);
					CTextConsole* src;

					if ( topobj->IsChar() ) // only chars are derived classes from CTextConsole
						src = dynamic_cast <CTextConsole*> ( topobj );
					else
						src = &g_Serv;

					m_tfRecycled.emplace_back( tf );
					it = m_timedFunctions[tick].erase( it );

					obj->r_Verb( s, src );
				}
				else
				{
					m_tfRecycled.emplace_back( tf );
					it = m_timedFunctions[tick].erase( it );
				}
			}
			else
			{
				++it;
			}
		}
	}

	m_isBeingProcessed = false;
	m_processedFunctionsPerTick = 0;

	while ( !m_tfQueuedToBeAdded.empty() )
	{
		TimedFunction *tf = m_tfQueuedToBeAdded.back();
		m_tfQueuedToBeAdded.pop_back();
		m_timedFunctions[tick].emplace_back( tf );
	}
}

void CTimedFunctionHandler::Erase( CUID uid )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Erase");
	for ( int tick = 0; tick < TICKS_PER_SEC; ++tick )
	{
		for ( auto it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); )	// the end iterator changes at each stl container erase call
		{
			TimedFunction* tf = *it;
			if ( tf->uid == uid)
			{
				m_tfRecycled.emplace_back( tf );
				it = m_timedFunctions[tick].erase( it );
			}
			else
				++it;
		}
	}
}

int CTimedFunctionHandler::IsTimer( CUID uid, lpctstr funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::IsTimer");
	for ( int tick = 0; tick < TICKS_PER_SEC; ++tick )
	{
		for ( auto it = m_timedFunctions[tick].begin(), end = m_timedFunctions[tick].end(); it != end; ++it)
		{
			TimedFunction* tf = *it;
			if ( (tf->uid == uid) && (Str_Match(funcname, tf->funcname) == MATCH_VALID))
				return tf->elapsed;
		}
	}
	return 0;
}

void CTimedFunctionHandler::Stop( CUID uid, lpctstr funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Stop");
	for ( int tick = 0; tick < TICKS_PER_SEC; ++tick )
	{
		for ( auto it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); )	// the end iterator changes at each erase call
		{
			TimedFunction* tf = *it;
            if ((tf->uid == uid) && (Str_Match(funcname, tf->funcname) == MATCH_VALID))
			{
				m_tfRecycled.emplace_back( tf );
				it = m_timedFunctions[tick].erase( it );
			}
			else
				++it;
		}
	}
}

void CTimedFunctionHandler::Clear()
{
    ADDTOCALLSTACK("CTimedFunctionHandler::Clear");

    m_curTick = 0;
    m_processedFunctionsPerTick = 0;

    for (uint i = 0; i < TICKS_PER_SEC; ++i)
    {
        m_timedFunctions[i].clear();
    }
    m_tfQueuedToBeAdded.clear();
    m_tfRecycled.clear();
}

TRIGRET_TYPE CTimedFunctionHandler::Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext,
    CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Loop");
	bool endLooping = false;
	for (int tick = 0; (tick < TICKS_PER_SEC) && !endLooping; ++tick)
	{
		for (auto it = m_timedFunctions[tick].begin(); it != m_timedFunctions[tick].end(); )
		{
			++LoopsMade;
			if (g_Cfg.m_iMaxLoopTimes && (LoopsMade >= g_Cfg.m_iMaxLoopTimes))
			{
				g_Log.EventError("Terminating loop cycle since it seems being dead-locked (%d iterations already passed)\n", LoopsMade);
				return TRIGRET_ENDIF;
			}

			TimedFunction* tf = *it;
			if (!strcmpi(tf->funcname, funcname))
			{
				CObjBase * pObj = tf->uid.ObjFind();
                if (!pObj)
                {
                LoopStop:
                    endLooping = true;
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
	}
	return TRIGRET_ENDIF;
}

void CTimedFunctionHandler::Add( CUID uid, int numSeconds, lpctstr funcname )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Add");
	ASSERT(funcname != nullptr);
	ASSERT(strlen(funcname) < 1024);

	int tick = m_curTick;
	TimedFunction *tf;
	if ( !m_tfRecycled.empty() )
	{
		tf = m_tfRecycled.back();
		m_tfRecycled.pop_back();
	}
	else
	{
		tf = new TimedFunction;
	}
	tf->uid = uid;
	tf->elapsed = numSeconds;
	Str_CopyLimitNull( tf->funcname, funcname, sizeof(tf->funcname) );
	if ( m_isBeingProcessed )
		m_tfQueuedToBeAdded.emplace_back( tf );
	else
		m_timedFunctions[tick].emplace_back( tf );
}

int CTimedFunctionHandler::Load( const char *pszName, bool fQuoted, const char *pszVal)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Load");
	UNREFERENCED_PARAMETER(fQuoted);
	static char tempBuffer[1024];
	static TimedFunction *tf = nullptr;

	if ( !pszName )
		return -1;

	if ( !strnicmp( pszName, "CurTick", 7 ) )
	{
		if ( !IsDigit(pszVal[0] ) )
        {
            g_Log.Event( LOGM_INIT|LOGL_ERROR,"Invalid CurTick line in %sdata.scp (value=%s).\n", SPHERE_FILE, pszVal );
            return -1;
        }

        auto oldErrno = errno;
        int tick = (int)std::strtol(pszVal, nullptr, 10);
        if (tick >= TICKS_PER_SEC)
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid CurTick in %sdata.scp (value=%d is too high).\n", SPHERE_FILE, tick);
            errno = oldErrno;
            return -1;
        }
        errno = oldErrno;

        m_curTick = tick;
	}
	else if ( !strnicmp( pszName, "TimerFNumbers", 13 ) )
	{
		tchar * ppVal[4];
		Str_CopyLimitNull( tempBuffer, pszVal, sizeof(tempBuffer) );	//because pszVal is constant and Str_ParseCmds wants a non-constant string
		size_t iArgs = Str_ParseCmds( tempBuffer, ppVal, CountOf( ppVal ), " ,\t" );
		if ( iArgs != 3 )
        {
            g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s (too few values)\n", SPHERE_FILE, pszName, pszVal );
            return -1;
        }
        if (!IsDigit(ppVal[0][0]) || !IsDigit(ppVal[1][0]) || !IsDigit(ppVal[2][0]))
        {
            g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s (encountered a non numeric value)\n", SPHERE_FILE, pszName, pszVal );
            return -1;
        }
        
        auto oldErrno = errno;
        int tick = (int)std::strtol(ppVal[0], nullptr, 10);
        if (tick > TICKS_PER_SEC)
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Tick (first value=%d) is too high.\n", SPHERE_FILE, tick);
            errno = oldErrno;
            return -1;
        }

        errno = 0;
        unsigned long uidTest = std::strtoul(ppVal[1], nullptr, 10);
        if ((errno == ERANGE) || (uidTest > UINT32_MAX))
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Invalid UID (second value=%lu).\n", SPHERE_FILE, uidTest);
            errno = oldErrno;
            return -1;
        }
        uint uid = (uint)uidTest;

        errno = 0;
        int elapsed = (int)std::strtol(ppVal[2], nullptr, 10);
        if (errno == ERANGE)
        {
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid TimerFNumbers in %sdata.scp. Invalid elapsed time (third value=%d).\n", SPHERE_FILE, elapsed);
            errno = oldErrno;
            return -1;
        }
        errno = oldErrno;

        if (tf == nullptr)
        {
            // A TimerFCall wasn't called before this TimerFNumbers, so tf doesn't contain a valid TimedFunction object.
            g_Log.Event(LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE);
            return -1;
        }
        tf->elapsed = elapsed;
        tf->uid.SetPrivateUID(uid);
        m_timedFunctions[tick].emplace_back(tf);
        tf = nullptr;
	}
	else if ( !strnicmp( pszName, "TimerFCall", 11 ) )
	{
		bool isNew = false;
		if ( tf == nullptr )
		{
			tf = new TimedFunction;
			isNew = true;
		}
		Str_CopyLimitNull( tf->funcname, pszVal, sizeof(tf->funcname) );

		if ( !isNew )
        {
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE );
            return -1;
        }
	}

	return 0;
}

void CTimedFunctionHandler::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::r_Write");
	s.WriteKeyFormat( "CurTick", "%d", m_curTick );
	for ( int tick = 0; tick < TICKS_PER_SEC; ++tick )
	{
		for ( auto it = m_timedFunctions[tick].begin(), end = m_timedFunctions[tick].end(); it != end; ++it )
		{
			TimedFunction* tf = *it;
			if ( tf->uid.IsValidUID() )
			{
				s.WriteKeyFormat( "TimerFCall", "%s", tf->funcname );
				s.WriteKeyFormat( "TimerFNumbers", "%d,%u,%d", tick, tf->uid.GetObjUID(), tf->elapsed );
			}
		}
	}
}