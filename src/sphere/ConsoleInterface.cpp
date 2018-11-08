#ifdef _WINDOWS

#include "ConsoleInterface.h"
#include "../common/CLog.h"


ConsoleOutput::ConsoleOutput(COLORREF iLogColor, CSString sLogString)
{
    _iTextColor = iLogColor;
    _sTextString = sLogString;
}

ConsoleOutput::ConsoleOutput(CSString sLogString)
{
    _iTextColor = g_Log.GetColor(CTCOL_DEFAULT);
    _sTextString = sLogString;
}

ConsoleOutput::~ConsoleOutput()
{
}

COLORREF ConsoleOutput::GetTextColor()
{
    return _iTextColor;
}

CSString ConsoleOutput::GetTextString()
{
    return _sTextString;
}

ConsoleInterface::ConsoleInterface()
{
    _qStorage1 = new std::queue<ConsoleOutput*>;
    _qStorage2 = new std::queue<ConsoleOutput*>;
    _qOutput = &_qStorage1;
}

ConsoleInterface::~ConsoleInterface()
{
}

void ConsoleInterface::SwitchQueues()
{
    _inMutex.lock();
    _outMutex.lock();
    if (_qOutput == &_qStorage1)
    {
        _qOutput = &_qStorage2;
    }
    else
    {
        _qOutput = &_qStorage1;
    }
    _outMutex.unlock();
    _inMutex.unlock();
}

void ConsoleInterface::AddConsoleOutput(ConsoleOutput * output)
{
    _inMutex.lock();
    (*_qOutput)->push(output);
    _inMutex.unlock();
}

#endif