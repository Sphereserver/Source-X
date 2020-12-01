#include "CServerConfig.h"
#include "CServerTime.h"

int64 CServerTime::GetTimeRaw() const
{
	if ( m_llPrivateTime < 0 )
		return 0;

	return m_llPrivateTime;
}

void CServerTime::Init()
{
	m_llPrivateTime = 0;
}

void CServerTime::InitTime( int64 llTimeBase )
{
	if ( llTimeBase < 0 )
		llTimeBase = 0;

	m_llPrivateTime = llTimeBase;
}

bool CServerTime::IsTimeValid() const
{
	return ( m_llPrivateTime > 0 ? true : false );
}

CServerTime CServerTime::operator+( int64 llTimeDiff ) const
{
	CServerTime time;
	time.m_llPrivateTime = m_llPrivateTime + llTimeDiff;
	if ( time.m_llPrivateTime < 0 )
		time.m_llPrivateTime = 0;

	return time;
}

CServerTime CServerTime::operator-( int64 llTimeDiff ) const
{
	CServerTime time;
	time.m_llPrivateTime = m_llPrivateTime - llTimeDiff;
	if ( time.m_llPrivateTime < 0 )
		time.m_llPrivateTime = 0;

	return time;
}


lpctstr CServerTime::GetTimeMinDesc(int minutes) // static
{
    tchar* pTime = Str_GetTemp();

    int minute = minutes % 60;
    int hour = (minutes / 60) % 24;

    lpctstr pMinDif;
    if (minute < 15)
    {
        //		pMinDif = "";
        pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_FIRST);
    }
    else if ((minute >= 15) && (minute < 30))
    {
        //		pMinDif = "a quarter past";
        pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_SECOND);
    }
    else if ((minute >= 30) && (minute < 45))
    {
        //pMinDif = "half past";
        pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_THIRD);
    }
    else
    {
        //		pMinDif = "a quarter till";
        pMinDif = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_QUARTER_FOURTH);
        hour = (hour + 1) % 24;
    }

    /*
        static lpctstr constexpr sm_ClockHour[] =
        {
            "midnight",
            "one",
            "two",
            "three",
            "four",
            "five",
            "six",
            "seven",
            "eight",
            "nine",
            "ten",
            "eleven",
            "noon"
        };
    */
    static const lpctstr sm_ClockHour[] =
    {
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ZERO),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ONE),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TWO),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_THREE),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_FOUR),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_FIVE),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_SIX),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_SEVEN),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_EIGHT),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_NINE),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TEN),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_ELEVEN),
        g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_HOUR_TWELVE),
    };

    lpctstr pTail;
    if (hour == 0 || hour == 12)
        pTail = "";
    else if (hour > 12)
    {
        hour -= 12;
        if ((hour >= 1) && (hour < 6))
            pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_13_TO_18);
        //			pTail = " o'clock in the afternoon";
        else if ((hour >= 6) && (hour < 9))
            pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_18_TO_21);
        //			pTail = " o'clock in the evening.";
        else
            pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_21_TO_24);
        //			pTail = " o'clock at night";
    }
    else
    {
        pTail = g_Cfg.GetDefaultMsg(DEFMSG_CLOCK_24_TO_12);
        //		pTail = " o'clock in the morning";
    }

    snprintf(pTime, STR_TEMPLENGTH, "%s %s %s", pMinDif, sm_ClockHour[hour], pTail);
    return pTime;
}