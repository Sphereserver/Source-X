#include "../common/sphere_library/CSTime.h"
#include "../common/CLog.h"
#include "../game/clients/CClient.h"
#include "../game/CServerConfig.h"
#include "CNetState.h"
#include "CNetworkManager.h"
#include "CNetworkInput.h"
#include "CNetworkOutput.h"
#include "CNetworkThread.h"


static const char* GenerateNetworkThreadName(size_t id)
{
    char* name = Str_GetTemp();
    snprintf(name, AbstractThread::m_nameMaxLength, "T_Net #%" PRIuSIZE_T, id);
    return name;
}


CNetworkThread::CNetworkThread(CNetworkManager* manager, size_t id)
    : AbstractSphereThread(GenerateNetworkThreadName(id), ThreadPriority::Disabled),
    m_manager(manager), m_id(id), _iTimeLastStateDataCheck(0)
{
}

CNetworkThread::~CNetworkThread(void)
{
}

void CNetworkThread::assignNetworkState(CNetState* state)
{
    ADDTOCALLSTACK("CNetworkThread::assignNetworkState");
    m_assignQueue.push(state);
    if (getPriority() == ThreadPriority::Disabled)
        awaken();
}

void CNetworkThread::queuePacket(PacketSend* packet, bool appendTransaction)
{
    // queue a packet for sending
    CNetworkOutput::QueuePacket(packet, appendTransaction);
}

void CNetworkThread::queuePacketTransaction(PacketTransaction* transaction)
{
    // queue a packet transaction for sending
    CNetworkOutput::QueuePacketTransaction(transaction);
}

void CNetworkThread::checkNewStates(void)
{
    // check for states that have been assigned but not moved to our list
    ADDTOCALLSTACK("CNetworkThread::checkNewStates");
    ASSERT(!isActive() || isCurrentThread());

    while (m_assignQueue.empty() == false)
    {
        CNetState* state = m_assignQueue.front();
        m_assignQueue.pop();

        ASSERT(state != nullptr);
        state->setParentThread(this);
        m_states.emplace_back(state);
    }
}

void CNetworkThread::dropInvalidStates(void)
{
    // check for states in our list that don't belong to us
    ADDTOCALLSTACK("CNetworkThread::dropInvalidStates");
    ASSERT(!isActive() || isCurrentThread());

    for (NetworkStateList::iterator it = m_states.begin(); it != m_states.end(); )
    {
        CNetState* state = *it;
        if (state->getParentThread() != this)
        {
            // state has been unassigned or reassigned elsewhere
            it = m_states.erase(it);
        }
        else if (state->isInUse() == false)
        {
            // state is invalid
            state->setParentThread(nullptr);
            it = m_states.erase(it);
        }
        else
        {
            // state is good
            ++it;
        }
    }
}

void CNetworkThread::onStart(void)
{
    AbstractSphereThread::onStart();
    m_input.setOwner(this);
    m_output.setOwner(this);
    m_profile.EnableProfile(PROFILE_NETWORK_RX);
    m_profile.EnableProfile(PROFILE_DATA_RX);
    m_profile.EnableProfile(PROFILE_NETWORK_TX);
    m_profile.EnableProfile(PROFILE_DATA_TX);
}

void CNetworkThread::tick(void)
{
    // process periodic actions
    ADDTOCALLSTACK("CNetworkThread::tick");
    checkNewStates();
    dropInvalidStates();

    if (m_states.empty())
    {
        // we haven't been assigned any clients, so go idle for now
        if (getPriority() != ThreadPriority::Disabled)
            setPriority(ThreadPriority::Low);
        return;
    }

    processInput();
    processOutput();

    // we're active, take priority
    setPriority(static_cast<ThreadPriority>(g_Cfg._iNetworkThreadPriority));

    static constexpr int64 kiStateDataCheckPeriod = 10 * 1000; // 10 seconds, expressed in milliseconds
    const int64 iTimeCur = CSTime::GetMonotonicSysTimeMilli();
    if (iTimeCur - _iTimeLastStateDataCheck > kiStateDataCheckPeriod)
    {
        _iTimeLastStateDataCheck = iTimeCur;


		for (CNetState* pCurState : m_states)
		{
			if ((g_Cfg._iMaxSizeClientOut != 0) || (g_Cfg._iMaxSizeClientIn != 0))
			{
				uchar uiKick = 0;
				int64 iBytes = 0, iQuota = 0;
				if ((g_Cfg._iMaxSizeClientOut != 0) && (pCurState->_iOutByteCounter > g_Cfg._iMaxSizeClientOut))
				{
					uiKick = 1;
					iBytes = pCurState->_iOutByteCounter, iQuota = g_Cfg._iMaxSizeClientOut;
				}
				else if ((g_Cfg._iMaxSizeClientIn != 0) && (pCurState->_iInByteCounter > g_Cfg._iMaxSizeClientIn))
				{
					uiKick = 2;
					iBytes = pCurState->_iInByteCounter, iQuota = g_Cfg._iMaxSizeClientIn;
				}

				if (uiKick)
				{
					bool fLog = true;
					CClient* pCurClient = pCurState->getClient();
					if (pCurClient)
					{
						fLog = pCurClient->Event_ExceededNetworkQuota(uiKick, iBytes, iQuota);
					}
					else
					{
						pCurState->markReadClosed();
						pCurState->markWriteClosed();
					}

					if (fLog)
					{
						const CAccount* pAccount = (pCurClient ? pCurClient->GetAccount() : nullptr);
						g_Log.EventWarn(
							"NetState id %d (IP: %s, Account: %s) exceeded its %s quota (%" PRId64 "/% " PRId64 ").\n",
							pCurState->id(),
							pCurState->m_socket.GetPeerName().GetAddrStr(), (pAccount ? pAccount->GetName() : "NA"),
							((uiKick == 2) ? "input" : "output"),
							((uiKick == 2) ? pCurState->_iInByteCounter : pCurState->_iOutByteCounter),
							((uiKick == 2) ? g_Cfg._iMaxSizeClientIn : g_Cfg._iMaxSizeClientOut)
						);
					}
				}
			}

			pCurState->_iInByteCounter = pCurState->_iOutByteCounter = 0;
		}
	}
}

void CNetworkThread::flushAllClients(void)
{
	ADDTOCALLSTACK("CNetworkThread::flushAllClients");
	NetworkThreadStateIterator states(this);
    while (CNetState* state = states.next())
    {
        m_output.flush(state);
    }
}


/***************************************************************************
*
*	class NetworkThreadStateIterator		Works as network state iterator getting the states
*											for a thread, safely.
*
***************************************************************************/
NetworkThreadStateIterator::NetworkThreadStateIterator(const CNetworkThread* thread)
{
    ASSERT(thread != nullptr);
    m_thread = thread;
    m_nextIndex = 0;
    m_safeAccess = m_thread->isActive() && !m_thread->isCurrentThread();
}

NetworkThreadStateIterator::~NetworkThreadStateIterator(void)
{
    m_thread = nullptr;
}

CNetState* NetworkThreadStateIterator::next(void)
{
    if (m_safeAccess == false)
    {
        // current thread, we can use the thread's state list directly
        // find next non-null state
        const int sz = int(m_thread->m_states.size());
        while (m_nextIndex < sz)
        {
            CNetState* state = m_thread->m_states[m_nextIndex];
            ++m_nextIndex;

            if (state != nullptr)
                return state;
        }
    }
    else
    {
        // different thread, we have to use the manager's states
        while (m_nextIndex < m_thread->m_manager->m_stateCount)
        {
            CNetState* state = m_thread->m_manager->m_states[m_nextIndex];
            ++m_nextIndex;

            if (state != nullptr && state->getParentThread() == m_thread)
                return state;
        }
    }

    return nullptr;
}
