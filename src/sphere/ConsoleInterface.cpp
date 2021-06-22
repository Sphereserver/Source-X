#include "ConsoleInterface.h"


// ConsoleOutput

ConsoleOutput::ConsoleOutput(ConsoleTextColor iLogColor, lpctstr ptcLogString) noexcept :
    _iTextColor(iLogColor), _sTextString(ptcLogString)
{
}

ConsoleOutput::ConsoleOutput(lpctstr ptcLogString) noexcept :
    _iTextColor(CTCOL_DEFAULT), _sTextString(ptcLogString)
{
}


// ConsoleInterface

uint ConsoleInterface::CTColToRGB(ConsoleTextColor color) noexcept // static
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

void ConsoleInterface::AddConsoleOutput(std::unique_ptr<ConsoleOutput>&& output) noexcept
{
    std::unique_lock<std::mutex> lock(_ciQueueMutex);
    _qOutput.emplace_back(std::move(output));
#ifndef _WIN32
    _ciQueueCV.notify_one();
#endif
}
