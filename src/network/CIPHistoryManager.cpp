#include "../common/CScriptTriggerArgs.h"
#include "../game/CServer.h"
#include "../game/CServerConfig.h"
#include "../game/CWorldGameTime.h"
#include "CIPHistoryManager.h"
#include <algorithm>

#define NETHISTORY_PINGDECAY	60		// time to decay 1 'ping'


/***************************************************************************
 *
 *
 *	struct HistoryIP			Simple interface for IP history maintainence
 *
 *
 ***************************************************************************/

void HistoryIP::update(void)
{
    // reset ttl
    m_ttl = g_Cfg.m_iNetHistoryTTL;
}

bool HistoryIP::checkPing(void)
{
    // ip is pinging, check if blocked
    update();

    return (m_blocked || (m_pings++ >= g_Cfg.m_iNetMaxPings));
}

void HistoryIP::setBlocked(bool isBlocked, int64 timeoutSeconds)
{
    // block ip
    ADDTOCALLSTACK("HistoryIP:setBlocked");
    if (isBlocked == true)
    {
        CScriptTriggerArgs args(m_ip.GetAddrStr());
        args.m_iN1 = timeoutSeconds;
        g_Serv.r_Call("f_onserver_blockip", &g_Serv, &args);
        timeoutSeconds = args.m_iN1;
    }

    m_blocked = isBlocked;

    if (isBlocked && timeoutSeconds >= 0)
        m_blockExpire = CWorldGameTime::GetCurrentTime().GetTimeRaw() + (timeoutSeconds * MSECS_PER_SEC);
    else
        m_blockExpire = 0;
}

/***************************************************************************
 *
 *
 *	class IPHistoryManager		Holds IP records (bans, pings, etc)
 *
 *
 ***************************************************************************/
IPHistoryManager::IPHistoryManager(void)
{
    m_lastDecayTime = 0;
}

IPHistoryManager::~IPHistoryManager(void)
{
    m_ips.clear();
}

void IPHistoryManager::tick(void)
{
    // periodic events
    ADDTOCALLSTACK("IPHistoryManager::tick");

    // check if ttl should decay (only do this once every second)
    const bool decayTTL = (!(m_lastDecayTime > 0) || CWorldGameTime::GetCurrentTime().GetTimeDiff(m_lastDecayTime) > 1 * MSECS_PER_SEC);
    if (decayTTL)
        m_lastDecayTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();

    for (IPHistoryList::iterator it = m_ips.begin(), end = m_ips.end(); it != end; ++it)
    {
        if (it->m_blocked)
        {
            // blocked ips don't decay, but check if the ban has expired
            if (it->m_blockExpire > 0 && (CWorldGameTime::GetCurrentTime().GetTimeRaw() > it->m_blockExpire))
                it->setBlocked(false);
        }
        else if (decayTTL)
        {
            if (it->m_connected == 0 && it->m_connecting == 0)
            {
                // start to forget about clients who aren't connected
                if (it->m_ttl >= 0)
                    --it->m_ttl;
            }

            // wait a 5th of TTL between each ping decay, but do not wait less than 30 seconds
            if (it->m_pings > 0 && --it->m_pingDecay < 0)
            {
                --it->m_pings;
                it->m_pingDecay = NETHISTORY_PINGDECAY;
            }
        }
    }

    // clear old ip history
    std::erase_if(m_ips, [](HistoryIP const& elem) {
        return elem.m_ttl < 0;
        });
}

HistoryIP& IPHistoryManager::getHistoryForIP(const CSocketAddressIP& ip) noexcept
{
    // get history for an ip

    // find existing entry
    for (IPHistoryList::iterator it = m_ips.begin(), end = m_ips.end(); it != end; ++it)
    {
        if (it->m_ip == ip)
            return *it;
    }

    // create a new entry
    HistoryIP hist = {
        .m_ip = ip,
        .m_pings = 0,
        .m_connecting = 0,
        .m_connected = 0,
        .m_blocked = 0,
        .m_ttl = 0,     // updated by update() method
        .m_connectionAttempts = 0,
        .m_timeLastConnectedMs = 0,  // set by the caller, if needed
        .m_blockExpire = 0,
        .m_pingDecay = NETHISTORY_PINGDECAY
    };
    hist.update();

    return m_ips.emplace_back(std::move(hist));
}

HistoryIP& IPHistoryManager::getHistoryForIP(const char* ip)
{
    // get history for an ip
    ADDTOCALLSTACK("IPHistoryManager::getHistoryForIP");

    CSocketAddressIP me(ip);
    return getHistoryForIP(me);
}
