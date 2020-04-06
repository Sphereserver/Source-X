#include "CWorld.h"
#include "CTimedFunctions.h"

void CTimedFunctions::Erase( CUID uid ) // static
{
	g_World._Ticker._TimedFunctions.Erase(uid);
}

int CTimedFunctions::IsTimer( CUID uid, lpctstr funcname ) // static
{
	return  g_World._Ticker._TimedFunctions.IsTimer(uid, funcname);
}

void CTimedFunctions::Stop( CUID uid, lpctstr funcname ) // static
{
	g_World._Ticker._TimedFunctions.Stop(uid, funcname);
}

void CTimedFunctions::Clear() // static
{
	g_World._Ticker._TimedFunctions.Clear();
}

TRIGRET_TYPE CTimedFunctions::Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext,
	CScript& s, CTextConsole* pSrc, CScriptTriggerArgs* pArgs, CSString* pResult) // static
{
	return  g_World._Ticker._TimedFunctions.Loop(funcname, LoopsMade, StartContext, s, pSrc, pArgs, pResult);
}

void CTimedFunctions::Add( CUID uid, int numSeconds, lpctstr funcname ) // static
{
	g_World._Ticker._TimedFunctions.Add(uid, numSeconds, funcname);
}

int CTimedFunctions::Load( const char *pszName, bool fQuoted, const char *pszVal) // static
{
	return g_World._Ticker._TimedFunctions.Load(pszName, fQuoted, pszVal);
}
