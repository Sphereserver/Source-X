
#include "../common/CObjBaseTemplate.h"
#include "../sphere/ProfileTask.h"
#include "CObjBase.h"
#include "CServer.h"
#include "CServerConfig.h"
#include "CTimedFunction.h"

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
	ProfileTask scriptsTask(PROFILE_TIMEDFUNCTIONS);


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
				CScript s(tf->funcname);
				CObjBase * obj = tf->uid.ObjFind();

				if ( obj != NULL ) //just in case
				{
					CObjBaseTemplate * topobj = obj->GetTopLevelObj();
					CTextConsole* src;

					if ( topobj->IsChar() )
						src = dynamic_cast <CTextConsole*> ( topobj );
					else
						src = &g_Serv;

					m_tfRecycled.push_back( tf );
					it = m_timedFunctions[tick].erase( it );

					obj->r_Verb( s, src );
				}
				else
				{
					m_tfRecycled.push_back( tf );
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
		m_timedFunctions[tick].push_back( tf );
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
				m_tfRecycled.push_back( tf );
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
		for ( auto it = m_timedFunctions[tick].begin(), end = m_timedFunctions[tick].end(); it != end; )
		{
			TimedFunction* tf = *it;
			if ( (tf->uid == uid) && (!strcmpi( tf->funcname, funcname)) )
				return tf->elapsed;

			++it;
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
			if (( tf->uid == uid) && (!strcmpi( tf->funcname, funcname)))
			{
				m_tfRecycled.push_back( tf );
				it = m_timedFunctions[tick].erase( it );
			}
			else
				++it;
		}
	}
}

TRIGRET_TYPE CTimedFunctionHandler::Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext, CScriptLineContext EndContext, CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult)
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
				TRIGRET_TYPE iRet = pObj->OnTriggerRun(s, TRIGRUN_SECTION_TRUE, pSrc, pArgs, pResult);
				if (iRet == TRIGRET_BREAK)
				{
					EndContext = StartContext;
					endLooping = true;
					break;
				}
				if ((iRet != TRIGRET_ENDIF) && (iRet != TRIGRET_CONTINUE))
					return iRet;
				if (iRet == TRIGRET_CONTINUE)
					EndContext = StartContext;
				else
					EndContext = s.GetContext();
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
	ASSERT(funcname != NULL);
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
	strcpy( tf->funcname, funcname );
	if ( m_isBeingProcessed )
		m_tfQueuedToBeAdded.push_back( tf );
	else
		m_timedFunctions[tick].push_back( tf );
}

int CTimedFunctionHandler::Load( const char *pszName, bool fQuoted, const char *pszVal)
{
	ADDTOCALLSTACK("CTimedFunctionHandler::Load");
	UNREFERENCED_PARAMETER(fQuoted);
	static char tempBuffer[1024];
	static TimedFunction *tf = NULL;

	if ( !pszName )
		return -1;

	if ( !strnicmp( pszName, "CurTick", 7 ) )
	{
		if ( IsDigit(pszVal[0] ) )
			m_curTick = ATOI(pszVal);
		else
			g_Log.Event( LOGM_INIT|LOGL_ERROR,	"Invalid NextTick line in save file: %s=%s\n", pszName, pszVal );
	}
	else if ( !strnicmp( pszName, "TimerFNumbers", 13 ) )
	{
		tchar * ppVal[4];
		strcpy( tempBuffer, pszVal );	//because pszVal is constant and Str_ParseCmds wants a non-constant string
		size_t iArgs = Str_ParseCmds( tempBuffer, ppVal, CountOf( ppVal ), " ,\t" );
		if ( iArgs == 3 )
		{
			if ( IsDigit( ppVal[0][0] ) && IsDigit( ppVal[1][0] ) && IsDigit( ppVal[2][0] ) )
			{
				int tick = ATOI(ppVal[0]);
				int uid = ATOI(ppVal[1]);
				int elapsed = ATOI(ppVal[2]);
				int isNew = 0;
				if ( tf == NULL )
				{
					tf = new TimedFunction;
					tf->funcname[0] = 0;
					isNew = 1;
				}
				tf->elapsed = elapsed;
				tf->uid.SetPrivateUID( uid );
				if ( !isNew )
				{
					m_timedFunctions[tick].push_back( tf );
					tf = NULL;
				}
				else
				{
					g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE );
				}
			}
			else
			{
				g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf line in %sdata.scp: %s=%s\n", SPHERE_FILE, pszName, pszVal );
			}
		}
	}
	else if ( !strnicmp( pszName, "TimerFCall", 11 ) )
	{
		int isNew = 0;
		if ( tf == NULL )
		{
			tf = new TimedFunction;
			isNew = 1;
		}
		strcpy( tf->funcname, pszVal );
		if ( !isNew )
			g_Log.Event( LOGM_INIT|LOGL_ERROR, "Invalid Timerf in %sdata.scp. Each TimerFCall and TimerFNumbers pair must be in that order.\n", SPHERE_FILE );
	}

	return 0;
}

void CTimedFunctionHandler::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CTimedFunctionHandler::r_Write");
	s.WriteKeyFormat( "CurTick", "%i", m_curTick );
	for ( int tick = 0; tick < TICKS_PER_SEC; ++tick )
	{
		for ( auto it = m_timedFunctions[tick].begin(), end = m_timedFunctions[tick].end(); it != end; ++it )
		{
			TimedFunction* tf = *it;
			if ( tf->uid.IsValidUID() )
			{
				s.WriteKeyFormat( "TimerFCall", "%s", tf->funcname );
				s.WriteKeyFormat( "TimerFNumbers", "%i,%u,%i", tick, tf->uid.GetObjUID(), tf->elapsed );
			}
		}
	}
}