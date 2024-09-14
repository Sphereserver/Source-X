/**
* @file CTextConsole.h
*
*/

#ifndef _INC_CTEXTCONSOLE_H
#define _INC_CTEXTCONSOLE_H

#include "sphere_library/CSString.h"


class CChar;

enum PLEVEL_TYPE : int			// Priviledge (priv) levels.
{
	PLEVEL_Guest = 0,		// 0 = This is just a guest account. (cannot PK)
	PLEVEL_Player,			// 1 = Player or NPC.
	PLEVEL_Counsel,			// 2 = Can travel and give advice.
	PLEVEL_Seer,			// 3 = Can make things and NPC's but not directly bother players.
	PLEVEL_GM,				// 4 = GM command clearance
	PLEVEL_Dev,				// 5 = Not bothererd by GM's
	PLEVEL_Admin,			// 6 = Can switch in and out of gm mode. assign GM's
	PLEVEL_Owner,			// 7 = Highest allowed level.
	PLEVEL_QTY
};

class CTextConsole
{
	// A base class for any class that can act like a console and issue commands.
	// CClient, CChar, CServer, CSFileConsole
protected:
	int OnConsoleKey( CSString & sText, tchar nChar, bool fEcho );

public:
	static const char *m_sClassName;

	// What privs do i have ?
	virtual PLEVEL_TYPE GetPrivLevel() const = 0;
	virtual lpctstr GetName() const = 0;	// ( every object must have at least a type name )
	virtual CChar * GetChar() const;	// are we also a CChar ? dynamic_cast ?

	virtual void SysMessage( lpctstr pszMessage ) const = 0;	// Feed back message.
	void VSysMessage( lpctstr pszFormat, va_list args ) const;
	void _cdecl SysMessagef( lpctstr pszFormat, ... ) const __printfargs(2,3);

public:
	CTextConsole() { };
	virtual ~CTextConsole() { };

private:
	CTextConsole(const CTextConsole& copy);
	CTextConsole& operator=(const CTextConsole& other);
};


#endif // _INC_CTEXTCONSOLE_H
