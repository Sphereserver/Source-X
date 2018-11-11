#include "ConsoleInterface.h"
#include "../common/CLog.h"


ConsoleOutput::ConsoleOutput(dword iLogColor, CSString sLogString)
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

dword ConsoleOutput::GetTextColor() const
{
    return _iTextColor;
}

const CSString& ConsoleOutput::GetTextString() const
{
    return _sTextString;
}

ConsoleInterface::ConsoleInterface()
{
    _qOutput = &_qStorage1;
}

ConsoleInterface::~ConsoleInterface()
{
}

void ConsoleInterface::_SwitchQueues()
{
    if (_qOutput == &_qStorage1)
    {
        _qOutput = &_qStorage2;
    }
    else
    {
        _qOutput = &_qStorage1;
    }
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
    _qOutput->push(output);
    _inMutex.unlock();
}