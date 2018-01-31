#if !defined(_WIN32) || defined(_LIBEV)

#include "../network/network.h"
#include "../game/CServer.h"
#include "linuxev.h"

// LibEv is used by Linux to notify when our main socket is readable or writable, so when i can read and send data again (async I/O).
// Windows supports async network I/O via WinSock.

LinuxEv g_NetworkEvent;


// Call this function (cb = callback) when the socket is readable -> there may be new received data to read
static void socketmain_cb(struct ev_loop * /* loop */, struct ev_io * /* w */, int /* revents */)
{
	/*
	ev_io_stop(loop, w);
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{
			// warning: accepting a new connection here can result in a threading issue,
			// where the main thread can clear the connection before it has been fully initialised
#ifndef _MTNETWORK
			g_NetworkIn.acceptConnection();
#else
			g_NetworkManager.acceptNewConnection();
#endif
		}
	}
	
	ev_io_start(loop, w);
	*/
}

// Call this function when the socket is readable again -> we are not sending data anymore
// The data is sent (if the checks are passing) at each tick on a NetworkThread, which sets also isSendingAsync to true. If in that tick
//  the NetworkThread can't send the data (maybe because socketslave_cb wasn't called, so onAsyncSendComplete wasn't called), wait for the next tick.
static void socketslave_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	NetState* state = reinterpret_cast<NetState *>( w->data );
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			// g_NetworkOut.onAsyncSendComplete(state);
		}		
		else if ( revents & EV_WRITE )
		{
#ifndef _MTNETWORK
			g_NetworkOut.onAsyncSendComplete(state, true);
#else
			NetworkThread* thread = state->getParentThread();
			if (thread != NULL)
				thread->onAsyncSendComplete(state, true);	// we can send (again) data
#endif
		}
	}
	
	if ( state->isSendingAsync() )
	{
		ev_io_start(loop, w);
	}
}

LinuxEv::LinuxEv(void) : AbstractSphereThread("T_NetworkEvents", IThread::High)
{
	m_eventLoop = ev_loop_new(EV_BACKEND_LIST);
	ASSERT(m_eventLoop != NULL);	// libev probably couldn't find sys/poll.h, select.h and other includes (compiling on ubuntu with both x86_64 and i386 compilers? or more gcc versions?)
	ev_set_io_collect_interval(m_eventLoop, 0.01);
	
	memset(&m_watchMainsock, 0, sizeof(ev_io));
}

LinuxEv::~LinuxEv(void)
{
	ev_loop_destroy(m_eventLoop);
}

void LinuxEv::onStart()
{
	// g_Log.Event(LOGM_CLIENTS_LOG, "Event start backend 0x%x\n", ev_backend(m_eventLoop));
	AbstractSphereThread::onStart();
}
	
void LinuxEv::tick()
{	
	ev_run(m_eventLoop, EVRUN_NOWAIT);
}

void LinuxEv::waitForClose()
{
	ev_break(m_eventLoop, EVBREAK_ALL);

	AbstractSphereThread::waitForClose();
}

// Start to monitor the state of the socket with a NetState
void LinuxEv::registerClient(NetState * state, EventsID eventCheck)
{
	// we call it with eventCheck = LinuxEv::Write, so we check when the socket will be again writable

	ADDTOCALLSTACK("LinuxEv::registerClient");
	ASSERT(state != NULL);
	
	memset(state->iocb(), 0, sizeof(ev_io));

#ifdef _WIN32
	int fd = EV_WIN32_HANDLE_TO_FD(state->m_socket.GetSocket());
	ev_io_init(state->iocb(), socketslave_cb, fd, (int)eventCheck);
#else
	ev_io_init(state->iocb(), socketslave_cb, state->m_socket.GetSocket(), (int)eventCheck);
#endif
	state->iocb()->data = state;
	state->setSendingAsync(true);	// set it true: when the socket will be again writable (so we have finished writing data in it)
									//  socketslave_cb will be called (even immediately after this function if we aren't actually sending data)
									//  and it will report that we are not sending async data anymore.
	
    ev_io_start(m_eventLoop, state->iocb());
}

void LinuxEv::unregisterClient(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::unregisterClient");
	ASSERT(state != NULL);

	state->setSendingAsync(false);
	
	ev_io_stop(m_eventLoop, state->iocb());
}

void LinuxEv::forceClientevent(NetState * state, EventsID eventForce)
{
	ADDTOCALLSTACK("LinuxEv::forceClientevent");
	ASSERT(state != NULL);
	ev_invoke(m_eventLoop, state->iocb(), (int)eventForce);
}

void LinuxEv::forceClientread(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientread");
	forceClientevent(state, LinuxEv::Read);
}

void LinuxEv::forceClientwrite(NetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientwrite");
	forceClientevent(state, LinuxEv::Write);
}

void LinuxEv::registerMainsocket()
{
	// The socket will be checked for incoming data/connections by NetworkManager::processAllInput.
	//	Right now neither Windows nor Linux support asynchronous input, but they do support async output.
	/*
#ifdef _WIN32
	int fd = EV_WIN32_HANDLE_TO_FD(g_Serv.m_SocketMain.GetSocket());
	ev_io_init(&m_watchMainsock, socketmain_cb, fd, EV_READ);
#else
	ev_io_init(&m_watchMainsock, socketmain_cb, g_Serv.m_SocketMain.GetSocket(), EV_READ);
#endif
    ev_io_start(m_eventLoop, &m_watchMainsock);		
	*/
}

void LinuxEv::unregisterMainsocket()
{
	//ev_io_stop(m_eventLoop, &m_watchMainsock);
}

#endif
