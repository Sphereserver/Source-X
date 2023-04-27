#include "../common/CUID.h"
#include "CWorld.h"
#include "CWorldTimedFunctions.h"

void CWorldTimedFunctions::ClearUID(const CUID& uid ) // static
{
	g_World._Ticker._TimedFunctions.ClearUID(uid);
}

int64 CWorldTimedFunctions::IsTimer(const CUID& uid, lpctstr ptcCommand) // static
{
	return  g_World._Ticker._TimedFunctions.IsTimer(uid, ptcCommand);
}

void CWorldTimedFunctions::Stop(const CUID& uid, lpctstr ptcCommand) // static
{
	g_World._Ticker._TimedFunctions.Stop(uid, ptcCommand);
}

void CWorldTimedFunctions::Clear() // static
{
	g_World._Ticker._TimedFunctions.Clear();
}

TRIGRET_TYPE CWorldTimedFunctions::Loop(lpctstr ptcCommand, int LoopsMade, CScriptLineContext StartContext,
	CScript& s, CTextConsole* pSrc, CScriptTriggerArgs* pArgs, CSString* pResult) // static
{
	return  g_World._Ticker._TimedFunctions.Loop(ptcCommand, LoopsMade, StartContext, s, pSrc, pArgs, pResult);
}

void CWorldTimedFunctions::Add(const CUID& uid, int64 iTimeout, lpctstr ptcCommand) // static
{
	g_World._Ticker._TimedFunctions.Add(uid, iTimeout, ptcCommand);
}

int CWorldTimedFunctions::Load(lpctstr ptcKeyword, bool fQuoted, lpctstr ptcArg) // static
{
	return g_World._Ticker._TimedFunctions.Load(ptcKeyword, fQuoted, ptcArg);
}
