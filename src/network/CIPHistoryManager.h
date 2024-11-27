/**
* @file CIPHistoryManager.h
* @brief Manages IP connections data.
*/

#ifndef _INC_CIPHISTORYMANAGER_H
#define _INC_CIPHISTORYMANAGER_H

#include "CSocket.h"
#include <deque>


/***************************************************************************
 *
 *
 *	struct HistoryIP			Simple interface for IP history maintainence
 *
 *
 ***************************************************************************/
struct HistoryIP
{
    CSocketAddressIP m_ip;
    bool m_fBlocked;
    int m_iPingDecay;
    int m_iPings;        // How many times i tried to connect from this ip. Decays over time.
    int m_iConnectionRequests; // Like pings, but doesn't decay, it's forgotten when the IP is forgotten.
    int m_iPendingConnectionRequests;
    int m_iAliveSuccessfulConnections;
    int m_iTTLSeconds;
    int64 m_iTimeLastConnectedMs;
    int64 m_iBlockExpireMs;

    void update(void);
    bool checkPing(void); // IP is blocked -or- too many pings to it?
    void setBlocked(bool isBlocked, int64 timeoutSeconds = -1);
};

typedef std::deque<HistoryIP> IPHistoryList;



/***************************************************************************
 *
 *
 *	class IPHistoryManager		Holds IP records (bans, pings, etc)
 *
 *
 ***************************************************************************/
class IPHistoryManager
{
private:
    IPHistoryList m_ips;		// list of known ips
    int64 m_lastDecayTime;	// last decay time

public:
    IPHistoryManager(void);
    ~IPHistoryManager(void);

    IPHistoryManager(const IPHistoryManager& copy) = delete;
    IPHistoryManager& operator=(const IPHistoryManager& other) = delete;

public:
    void tick(void);	// periodic events

    HistoryIP& getHistoryForIP(const CSocketAddressIP& ip) noexcept;	// get history for an ip
    HistoryIP& getHistoryForIP(const char* ip);				// get history for an ip
};

#endif // _INC_CIPHISTORYMANAGER_H
