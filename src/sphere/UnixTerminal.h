/**
* @file UnixTerminal.h
* @brief Unix terminal utilities (set colors, read text...)
*/


#pragma once
#ifndef _INC_UNIXTERMINAL_H
#define _INC_UNIXTERMINAL_H

enum ConsoleTextColor			// needed by both windows and unix
{
	// these are the basic colors supported by all unix terminals
	CTCOL_DEFAULT = 0,
	CTCOL_RED,
	CTCOL_GREEN,
	CTCOL_YELLOW,
	CTCOL_BLUE,
	CTCOL_MAGENTA,
	CTCOL_CYAN,
	CTCOL_WHITE,
	CTCOL_QTY
};


#ifndef _WIN32


#include "../common/common.h"

#ifdef _USECURSES
	#include <curses.h>
#else
	#include <termios.h>
#endif


class UnixTerminal
{
	private:
	#ifdef _USECURSES
		WINDOW * m_window;
	#else
		termios m_original;
	#endif
		tchar m_nextChar;
		bool m_isColorEnabled;
		bool m_prepared;

	public:
		UnixTerminal();
		~UnixTerminal();

	protected:
		UnixTerminal(const UnixTerminal & copy);
		UnixTerminal & operator=(const UnixTerminal & other);

	public:
		bool isReady();
		tchar read();
		void prepare();
		void print(lpctstr message);
		void setColor(ConsoleTextColor color);
		void setColorEnabled(bool enable);

	private:
		void prepareColor();
		void restore();

	public:
		bool isColorEnabled() const
		{
			return m_isColorEnabled;
		}
};

extern UnixTerminal g_UnixTerminal;


#endif // !_WIN32
#endif // _INC_UNIXTERMINAL_H
