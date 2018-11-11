#include "ConsoleInterface.h"


// ConsoleOutput

ConsoleOutput::ConsoleOutput(ConsoleTextColor iLogColor, CSString sLogString)
{
    _iTextColor = iLogColor;
    _sTextString = sLogString;
}

ConsoleOutput::ConsoleOutput(CSString sLogString)
{
    _iTextColor = CTCOL_DEFAULT;
    _sTextString = sLogString;
}

ConsoleOutput::~ConsoleOutput()
{
}

ConsoleTextColor ConsoleOutput::GetTextColor() const
{
    return _iTextColor;
}

const CSString& ConsoleOutput::GetTextString() const
{
    return _sTextString;
}


// ConsoleInterface

ConsoleInterface::ConsoleInterface()
{
}

ConsoleInterface::~ConsoleInterface()
{
}

uint ConsoleInterface::CTColToRGB(ConsoleTextColor color) // static
{
    auto MakeRGB = [](uchar r, uchar g, uchar b) -> uint
    {
        return ((uint)r | (g << 8) | (b << 16));
    };
    switch (color)
    {
        case CTCOL_RED:
            return MakeRGB(255, 0, 0);
        case CTCOL_GREEN:
            return MakeRGB(0, 255, 0);
        case CTCOL_YELLOW:
            return MakeRGB(127, 127, 0);
        case CTCOL_BLUE:
            return MakeRGB(0, 0, 255);
        case CTCOL_MAGENTA:
            return MakeRGB(255, 0, 255);
        case CTCOL_CYAN:
            return MakeRGB(0, 127, 255);
        case CTCOL_WHITE:
            return MakeRGB(255, 255, 255);
        default:
            return MakeRGB(175, 175, 175);
    }
}

void ConsoleInterface::AddConsoleOutput(ConsoleOutput * output)
{
    std::unique_lock<std::mutex> lock(_ciQueueMutex);
    _qOutput.push(output);
#ifndef _WIN32
    _ciQueueCV.notify_one();
#endif
}
