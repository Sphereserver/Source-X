
#pragma once
#ifndef CSECTORENVIRON_H
#define CSECTORENVIRON_H

#include "../common/grayproto.h"


struct CSectorEnviron	// When these change it is an CTRIG_EnvironChange,
{
#define LIGHT_OVERRIDE 0x80
public:
    BYTE m_Light;		// the calculated light level in this area. |0x80 = override.
    SEASON_TYPE m_Season;		// What is the season for this sector.
    WEATHER_TYPE m_Weather;		// the weather in this area now.
public:
    CSectorEnviron();
    void SetInvalid();
};


#endif // CSECTORENVIRON_H
