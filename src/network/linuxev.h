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
		// struct ev_io m_watchMainsock; // Watcher for Sphere's socket, to accept incoming connections (async read).
	
	public:
		LinuxEv(void);
		virtual ~LinuxEv(void);

		LinuxEv(const LinuxEv& copy) = delete;
		LinuxEv& operator=(const LinuxEv& other) = delete;
	
	public:
		virtual void onStart() override;
		virtual void tick() override;
		virtual void waitForClose() override;
		
	private:
		void forceClientevent(CNetState *, EventsID);
		
	public:
		void printInitInfo();
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
