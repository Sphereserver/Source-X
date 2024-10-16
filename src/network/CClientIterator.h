/**
* @file CClientIterator.h
* @brief Works as client iterator getting the clients.
*/

#ifndef _INC_CCLIENTITERATOR_H
#define _INC_CCLIENTITERATOR_H

class CClient;
class CNetworkManager;

class ClientIterator
{
protected:
    const CNetworkManager* m_network;	// network manager to iterate
    CClient* m_nextClient;

public:
    explicit ClientIterator(const CNetworkManager* network = nullptr);
    ~ClientIterator();

    ClientIterator(const ClientIterator& copy) = delete;
    ClientIterator& operator=(const ClientIterator& other) = delete;

    CClient* next(bool includeClosing = false); // finds next client
};


#endif // _INC_CCLIENTITERATOR_H
