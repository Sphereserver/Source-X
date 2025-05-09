
#ifndef _WIN32

#include "../common/CException.h"
#include "UnixTerminal.h"

#ifndef _USECURSES
	#include <cstdio>
	#include <unistd.h>
	#include <sys/time.h>
#endif


UnixTerminal::UnixTerminal() : AbstractSphereThread("T_UnixTerm", ThreadPriority::Highest),
#ifdef _USECURSES
	m_window(nullptr),
#else
	m_original{},
#endif
 m_nextChar('\0'), m_isColorEnabled(true), m_prepared(false)
{
}

UnixTerminal::~UnixTerminal()
{
    waitForClose();
    restore();
    //_thread_selfTerminateAfterThisTick = true;  // just to be sure
}

/*
bool isInputAvailable()
{
    fd_set fds;
    struct timeval tv = {0, 0}; // Non-blocking, immediate return
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}
*/

bool UnixTerminal::isReady()
{
	ADDTOCALLSTACK("UnixTerminal::isReady");
	if (m_nextChar != '\0')
		return true;

#ifdef _USECURSES
	int c = wgetch(m_window);
	if (c == ERR)
		return false;

	if (c >= 0 && c < 256)
	{
		// character input received
		m_nextChar = static_cast<tchar>(c);
		return true;
	}

	switch (c)
	{
		// map special characters to input
		case KEY_BACKSPACE:
			m_nextChar = '\b';
			return true;
		case KEY_ENTER:
			m_nextChar = '\n';
			return true;

		case KEY_RESIZE:
			// todo: resize window
			return false;
		case KEY_MOUSE:
			// todo: handle mouse event
			return false;
	}
#else
	// check if input is waiting
	fd_set consoleFds;
	FD_ZERO(&consoleFds);
	FD_SET(STDIN_FILENO, &consoleFds);

	timeval tvTimeout;
	tvTimeout.tv_sec = 0;
	tvTimeout.tv_usec = 1;

	if (select(1, &consoleFds, nullptr, nullptr, &tvTimeout) <= 0)
		return false;

	// get next character
	int c = fgetc(stdin);
	if (c < 0 || c > 255)
		return false;

	// map backspace
	if (c == m_original.c_cc[VERASE])
		c = '\b';

	// echo to console
	fputc(c, stdout);
    fflush(stdout);

	m_nextChar = static_cast<tchar>(c);
	return m_nextChar != '\0';
#endif

	return false;
}

tchar UnixTerminal::read()
{
	ADDTOCALLSTACK("UnixTerminal::read");
	tchar c = m_nextChar;
	m_nextChar = '\0';
	return c;
}

void UnixTerminal::setColor(ConsoleTextColor color)
{
	if (m_isColorEnabled == false)
		return;

	if (color < CTCOL_DEFAULT || color >= CTCOL_QTY)
		color = CTCOL_DEFAULT;

#ifdef _USECURSES
	wattrset(m_window, COLOR_PAIR(color));
#else
	if (color == CTCOL_DEFAULT)
		fprintf(stdout, "\033[0m");
	else
	{
		int bold = 1;
		if ((color == CTCOL_YELLOW) || (color == CTCOL_CYAN))
			bold = 0;
		int colorcode = 30 + (int)color;
		fprintf(stdout, "\033[%d;%dm", bold, colorcode);
	}
#endif
}

void UnixTerminal::print(lpctstr message)
{
	ADDTOCALLSTACK("UnixTerminal::print");
	ASSERT(message != nullptr);
#ifdef _USECURSES
	waddstr(m_window, message);
	wrefresh(m_window);
#else
	fputs(message, stdout);
    fflush(stdout);
#endif
}

void UnixTerminal::prepare()
{
	ADDTOCALLSTACK("UnixTerminal::prepare");
	ASSERT(m_prepared == false);

#ifdef _USECURSES
	initscr();	// init screen	-> THIS CAN FAIL AND MAKE SPHERE CRASH IF USING GDB (GNU Debugger)! If so, disable _USECURSES
	cbreak();	// read one character at a time
	echo();		// echo input

	// create a window, same size as terminal
	int lines, columns;
	getmaxyx(stdscr, lines, columns);
	m_window = newwin(lines, columns, 0, 0);

	keypad(m_window, TRUE);		// process special chars
	nodelay(m_window, TRUE);	// non-blocking input
	scrollok(m_window, TRUE);	// allow scrolling
	refresh();		// draw screen

#else
    // Am i running Sphere in a terminal?
    if (isatty(STDIN_FILENO))
    {
        // save existing attributes
        if (tcgetattr(STDIN_FILENO, &m_original) < 0)
            throw CSError(LOGL_WARN, 0, "failed to get terminal attributes");

        // set new terminal attributes
        termios term_caps = m_original;
        term_caps.c_lflag &= ~ unsigned(ICANON | ECHO);
        term_caps.c_cc[VMIN] = 1;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &term_caps) < 0)
            throw CSError(LOGL_WARN, 0, "failed to set terminal attributes");
    }
    else
    {
//#ifdef _POSIX_VERSION
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
//#endif
    }
	setbuf(stdin, nullptr);
#endif

	prepareColor();
	m_nextChar = '\0';
	m_prepared = true;
}

void UnixTerminal::prepareColor()
{
	ADDTOCALLSTACK("UnixTerminal::prepareColor");

#ifdef _USECURSES
	m_isColorEnabled = has_colors();
	if (m_isColorEnabled)
	{
		// initialize colors, and map our enum to the actual colour codes
		start_color();
		// init_pair (pair_id, curses foreground color id, curses background color id);
		// can define a max of 7 pairs? (for compatibility with 8-color terminals?)
		init_pair(CTCOL_RED,		COLOR_RED,		COLOR_BLACK);
		init_pair(CTCOL_GREEN,		COLOR_GREEN,	COLOR_BLACK);
		init_pair(CTCOL_YELLOW,		COLOR_YELLOW,	COLOR_BLACK);
		init_pair(CTCOL_BLUE,		COLOR_BLUE,		COLOR_BLACK);
		init_pair(CTCOL_CYAN,		COLOR_CYAN,		COLOR_BLACK);
		init_pair(CTCOL_MAGENTA,	COLOR_MAGENTA,	COLOR_BLACK);
		init_pair(CTCOL_WHITE,		COLOR_WHITE,	COLOR_BLACK);
	}

#else

	// Nowadays every terminal should support the main ASCII escape codes, so instead of keeping the list updated with
	//	every possible terminal we can use, i'd rather assume that every terminal we are using supports them.
/*
	// enable colour based on known terminal types
	if (m_isColorEnabled)
	{
		m_isColorEnabled = false;

		const char * termtype = getenv("TERM");
		if (termtype != nullptr)
		{
			static const char *sz_Supported_Terminals[] =
			{
				"aixterm", "ansi", "color_xterm",
				"con132x25", "con132x30", "con132x43", "con132x60",
				"con80x25", "con80x28", "con80x30", "con80x43", "con80x50", "con80x60",
				"cons25", "console", "gnome", "hft", "kon", "konsole", "kterm",
				"linux", "rxvt", "screen", "screen.linux", "vt100-color",
				"xterm", "xterm-color", "xterm-256color"
			};

			for (size_t i = 0; i < ARRAY_COUNT(sz_Supported_Terminals); ++i)
			{
				if (strcmp(termtype, sz_Supported_Terminals[i]) == 0)
				{
					m_isColorEnabled = true;
					break;
				}
			}
		}
	}
*/

#endif
}

void UnixTerminal::restore()
{
	ADDTOCALLSTACK("UnixTerminal::restore");
	ASSERT(m_prepared);

#ifdef _USECURSES
	endwin();	// clean up
	m_window = nullptr;
	m_nextChar = '\0';
#else
	// restore original terminal state
	if (tcsetattr(STDIN_FILENO, TCSANOW, &m_original) < 0)
	{
		fprintf(stderr, "SYSWARN: failed to restore terminal attributes.");
	}
#endif

	m_prepared = false;
}

void UnixTerminal::setColorEnabled(bool enable)
{
	m_isColorEnabled = enable;
}

void UnixTerminal::onStart()
{
    AbstractSphereThread::onStart();
    // Initialize nonblocking IO and disable readline on linux
    prepare();
}

void UnixTerminal::tick()
{
    while (!shouldExit())
    {
		std::deque<std::unique_ptr<ConsoleOutput>> outMessages;
		{
			// Better keep the mutex locked for the least time possible.
			std::unique_lock<std::mutex> lock(this->ConsoleInterface::_ciQueueMutex);

			while (_qOutput.empty())
			{
                this->ConsoleInterface::_ciQueueCV.wait(lock,
                    [this]() {
                        return (this->m_terminateRequested || !this->ConsoleInterface::_qOutput.empty());
                    });
			}

            if (this->ConsoleInterface::_qOutput.empty())
                break;

			outMessages.swap(this->ConsoleInterface::_qOutput);
		}

		for (std::unique_ptr<ConsoleOutput> const& co : outMessages)
        {
            setColor(co->GetTextColor());
            print(co->GetTextString().GetBuffer());
            setColor(CTCOL_DEFAULT);
        }
    }
}

void UnixTerminal::waitForClose()
{
    this->m_terminateRequested = true;
    this->ConsoleInterface::_ciQueueCV.notify_one();
    //AbstractSphereThread::waitForClose();
    //AbstractThread::terminate(true);
}


#endif // !_WIN32

