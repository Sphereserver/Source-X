#ifndef _INC_CONSOLEINTERFACE_H
#define _INC_CONSOLEINTERFACE_H

#include "../common/sphere_library/CSString.h"
#include "../common/basic_threading.h"
#include <queue>

class ConsoleOutput
{
    dword _iTextColor;
    CSString _sTextString;

public:
    friend class CNTWindow;
    ConsoleOutput(dword iLogColor, CSString sLogString);
    ConsoleOutput(CSString sLogString);
    ~ConsoleOutput();
    dword GetTextColor() const;
    const CSString& GetTextString() const;
};

class ConsoleInterface
{
    std::queue<ConsoleOutput*> _qStorage1;
    std::queue<ConsoleOutput*> _qStorage2;

protected:
    ConsoleInterface();
    ~ConsoleInterface();

    mutable std::mutex _inMutex;
    mutable std::mutex _outMutex;
    std::queue<ConsoleOutput*> *_qOutput;   // Active output queue

    void _SwitchQueues();
    void SwitchQueues();

public:
    void AddConsoleOutput(ConsoleOutput *output);
};
#endif //_INC_CONSOLEINTERFACE_H