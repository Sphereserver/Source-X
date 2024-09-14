#include "../game/clients/CClient.h"
#include "CNetState.h"
#include "CNetworkManager.h"
#include "CClientIterator.h"


ClientIterator::ClientIterator(const CNetworkManager* network)
{
    m_network = (network == nullptr ? &g_NetworkManager : network);
    m_nextClient = static_cast<CClient*> (m_network->m_clients.GetContainerHead());
}

ClientIterator::~ClientIterator()
{
    m_network = nullptr;
    m_nextClient = nullptr;
}

CClient* ClientIterator::next(bool includeClosing)
{
    for (CClient* current = m_nextClient; current != nullptr; current = current->GetNext())
    {
        // skip clients without a state, or whose state is invalid/closed
        const CNetState* ns = current->GetNetState();
        if (ns == nullptr || ns->isInUse(current) == false || ns->isClosed())
            continue;

        // skip clients whose connection is being closed
        if (includeClosing == false && ns->isClosing())
            continue;

        m_nextClient = current->GetNext();
        return current;
    }

    return nullptr;
}
