/**
* @file UnixTerminal.h
* @brief Unix terminal utilities (set colors, read text...)
*/

#ifndef _INC_UNIXTERMINAL_H
#define _INC_UNIXTERMINAL_H

#ifndef _WIN32

#include "threads.h"
#include "ConsoleInterface.h"

#ifdef _USECURSES
	#include <curses.h>
#else
	#include <termios.h>
#endif


struct UnixTerminal : public AbstractSphereThread, public ConsoleInterface
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

        UnixTerminal(const UnixTerminal & copy) = delete;
        UnixTerminal & operator=(const UnixTerminal & other) = delete;

	public:
        virtual void onStart() override;
        virtual void tick() override;
        virtual void waitForClose() override;
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
