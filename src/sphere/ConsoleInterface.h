#ifndef _INC_CONSOLEINTERFACE_H
#define _INC_CONSOLEINTERFACE_H

#include "../common/sphere_library/CSString.h"
#include "../common/basic_threading.h"
#ifndef _WIN32
    #include <condition_variable>
#endif
#include <queue>


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

class ConsoleOutput
{
    ConsoleTextColor _iTextColor;
    CSString _sTextString;

public:
    ConsoleOutput(ConsoleTextColor iLogColor, CSString sLogString);
    ConsoleOutput(CSString sLogString);
    ~ConsoleOutput();
    ConsoleTextColor GetTextColor() const;
    const CSString& GetTextString() const;
};

class ConsoleInterface
{
protected:
    mutable std::mutex _ciQueueMutex;
#ifndef _WIN32
    std::condition_variable _ciQueueCV;
#endif

    ConsoleInterface();
    ~ConsoleInterface();

    std::queue<ConsoleOutput*> _qOutput;

public:
    static uint CTColToRGB(ConsoleTextColor color);
    void AddConsoleOutput(ConsoleOutput *output);
};

#endif //_INC_CONSOLEINTERFACE_H
