
#pragma once
#ifndef _INC_GRAYSVR_H
#define _INC_GRAYSVR_H

#if defined(_WIN32) && !defined(_MTNETWORK)
	// _MTNETWORK enabled via makefile for other systems
	#define _MTNETWORK
#endif

#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif

// Text mashers.

extern lpctstr GetTimeMinDesc( int dwMinutes );
extern size_t FindStrWord( lpctstr pTextSearch, lpctstr pszKeyWord );

////////////////////////////////////////////////////////////////////////////////////

class Main : public AbstractSphereThread
{
public:
	Main();
	virtual ~Main() { };

private:
	Main(const Main& copy);
	Main& operator=(const Main& other);

public:
	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
	virtual bool shouldExit();
};

//////////////////////////////////////////////////////////////

extern lpctstr g_szServerDescription;
extern int g_szServerBuild;
extern lpctstr const g_Stat_Name[STAT_QTY];
extern CGStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

// ---------------------------------------------------------------------------------------------



#endif // _INC_GRAYSVR_H
