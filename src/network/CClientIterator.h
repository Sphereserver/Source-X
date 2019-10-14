/**
* @file CClientIterator.h
* @brief Works as client iterator getting the clients.
*/

#ifndef _INC_CCLIENTITERATOR_H
#define _INC_CCLIENTITERATOR_H

#include "../game/clients/CClient.h"

class CNetworkManager;

class ClientIterator
{
protected:
    const CNetworkManager* m_network;	// network manager to iterate
    CClient* m_nextClient;

public:
    explicit ClientIterator(const CNetworkManager* network = nullptr);
    ~ClientIterator(void);

private:
    ClientIterator(const ClientIterator& copy);
    ClientIterator& operator=(const ClientIterator& other);

public:
    CClient* next(bool includeClosing = false); // finds next client
};


#endif // _INC_CCLIENTITERATOR_H