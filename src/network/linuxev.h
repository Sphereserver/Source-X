/**
* @file linuxev.h
*
*/

#ifndef _INC_LINUXEV_H
#define _INC_LINUXEV_H

#ifdef _LIBEV

	#include "../../lib/libev/src/ev.h"
	#include "../common/sphere_library/smutex.h"
	#include "../sphere/threads.h"

	#ifdef _BSD
		#define EV_BACKEND_LIST (EVBACKEND_SELECT | EVBACKEND_POLL | EVBACKEND_KQUEUE)
	#else
		#define EV_BACKEND_LIST (EVBACKEND_SELECT | EVBACKEND_POLL | EVBACKEND_EPOLL)
	#endif
	
	class CClient;
	class CNetState;
		
	class LinuxEv : public AbstractSphereThread
	{
	public:
		enum EventsID
		{
			Undefinied = -1,
			None = 0,
			Read = 1,
			Write = 2,
		
			Error = 0x80000000
		};
	
	private:
		struct ev_loop * m_eventLoop;
		struct ev_io m_watchMainsock;
	
	public:
		LinuxEv(void);
		virtual ~LinuxEv(void);

	private:
		LinuxEv(const LinuxEv& copy);
		LinuxEv& operator=(const LinuxEv& other);
	
	public:
		virtual void onStart();
		virtual void tick();
		virtual void waitForClose();
		
	private:
		void forceClientevent(CNetState *, EventsID);
		
	public:
		void forceClientread(CNetState *);
		void forceClientwrite(CNetState *);
		// --------------------------------------	
		void registerClient(CNetState *, EventsID);
		void unregisterClient(CNetState *);
		// --------------------------------------
		void registerMainsocket();
		void unregisterMainsocket();
	};

#endif
#endif // _INC_LINUXEV_H
