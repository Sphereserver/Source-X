#ifndef _INC_CONSOLEINTERFACE_H
#define _INC_CONSOLEINTERFACE_H

#include "../common/sphere_library/CSString.h"
#include "../common/basic_threading.h"
#ifndef _WIN32
    #include <condition_variable>
#endif
#include <deque>


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
    explicit ConsoleOutput(ConsoleTextColor iLogColor, lpctstr ptcLogString) noexcept;
    explicit ConsoleOutput(lpctstr ptcLogString) noexcept;
    ~ConsoleOutput() noexcept = default;

    ConsoleTextColor GetTextColor() const noexcept {
        return _iTextColor;
    }
    const CSString& GetTextString() const noexcept {
        return _sTextString;
    }
};

class ConsoleInterface
{
protected:
    mutable std::mutex _ciQueueMutex;
#ifndef _WIN32
    std::condition_variable _ciQueueCV;
#endif

    ConsoleInterface() = default;
    ~ConsoleInterface() = default;

    std::deque<std::unique_ptr<ConsoleOutput>> _qOutput;

public:
    static uint CTColToRGB(ConsoleTextColor color) noexcept;
    void AddConsoleOutput(std::unique_ptr<ConsoleOutput>&& output) noexcept;
};

#endif //_INC_CONSOLEINTERFACE_H
