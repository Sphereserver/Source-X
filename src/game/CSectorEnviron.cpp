#include "uo_files/uofiles_macros.h"
#include "CSectorEnviron.h"


CSectorEnviron::CSectorEnviron() noexcept
{
    m_Light = LIGHT_BRIGHT;	// set based on time later.
    m_Season = SEASON_Summer;
    m_Weather = WEATHER_DRY;
}

void CSectorEnviron::SetInvalid() noexcept
{
    // Force a resync of all this. we changed location by teleport etc.
    m_Light = UINT8_MAX;	// set based on time later.
    m_Season = SEASON_QTY;
    m_Weather = WEATHER_DEFAULT;
}
