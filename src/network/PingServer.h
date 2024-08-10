/**
* @file PingServer.h
*
*/

#ifndef _INC_PINGSERVER_H
#define _INC_PINGSERVER_H

#include "../sphere/threads.h"
#include "CSocket.h"


#define PINGSERVER_PORT		12000	// listen on this port for client pings (clients normally uses 12000)
#define PINGSERVER_BUFFER	64		// number of bytes to receive from clients (client normally sends 40)


class PingServer : public AbstractSphereThread
{
private:
	CSocket m_socket;

public:
	PingServer(void);
	virtual ~PingServer(void);

private:
	PingServer(const PingServer& copy);
	PingServer& operator=(const PingServer& other);

public:
	virtual void onStart();
	virtual void tick();
	virtual bool shouldExit();
	virtual void waitForClose();
};


#endif // _INC_PINGSERVER_H
