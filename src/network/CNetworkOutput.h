/**
* @file CNetworkOutput.h
* @brief  Handles network output to clients.
*/

#ifndef _INC_NETWORKOUTPUT_H
#define _INC_NETWORKOUTPUT_H

#include "../common/common.h"

class CNetState;
class CNetworkThread;
class PacketSend;
class PacketTransaction;


#define NETWORK_MAXQUEUESIZE	g_Cfg.m_iNetMaxQueueSize		// max packets to hold per queue (comment out for unlimited)


class CNetworkOutput
{
private:
	static constexpr inline size_t _failed_result(void) { return INTPTR_MAX; }

private:
	CNetworkThread* m_thread;	// owning network thread
	byte* m_encryptBuffer;		// buffer for encrpyted data

public:
	static const char* m_sClassName;
	CNetworkOutput(void);
	~CNetworkOutput(void);

private:
	CNetworkOutput(const CNetworkOutput& copy);
	CNetworkOutput& operator=(const CNetworkOutput& other);

public:
	void setOwner(CNetworkThread* thread) { m_thread = thread; }			// set owner thread
	bool processOutput(void);											// process output to clients, returns true if data was sent
	size_t flush(CNetState* state);										// process all queues for a client
	void onAsyncSendComplete(CNetState* state, bool success);			// notify that async operation completed

	static void QueuePacket(PacketSend* packet, bool appendTransaction);// queue a packet for sending
	static void QueuePacketTransaction(PacketTransaction* transaction);	// queue a packet transaction for sending

private:
	void checkFlushRequests(void);										// check for clients who need data flushing
	size_t processPacketQueue(CNetState* state, uint priority);	// process a client's packet queue
	size_t processAsyncQueue(CNetState* state);							// process a client's async queue
	bool processByteQueue(CNetState* state);								// process a client's byte queue

	bool sendPacket(CNetState* state, PacketSend* packet);				// send packet to client (can be queued for async operation)
	bool sendPacketData(CNetState* state, PacketSend* packet);			// send packet data to client
	size_t sendData(CNetState* state, const byte* data, size_t length);	// send raw data to client
};



#endif // _INC_NETWORKOUTPUT_H
