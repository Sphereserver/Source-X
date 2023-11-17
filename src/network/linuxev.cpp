#ifdef _LIBEV

#include "../game/CServer.h"
#include "CNetState.h"
#include "CNetworkThread.h"
#include "linuxev.h"


// Call this function (cb = callback) when the socket is readable -> there may be new received data to read
//static void socketmain_cb(struct ev_loop * /* loop */, struct ev_io * /* w */, int /* revents */)
/*
{
	ev_io_stop(loop, w);
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{
			// warning: accepting a new connection here can result in a threading issue,
			// where the main thread can clear the connection before it has been fully initialised
			g_NetworkManager.acceptNewConnection();
		}
	}
	
	ev_io_start(loop, w);
}
*/

// Call this function when the socket is readable again -> we are not sending data anymore
// The data is sent (if the checks are passing) at each tick on a CNetworkThread, which sets also isSendingAsync to true. If in that tick
//  the CNetworkThread can't send the data (maybe because socketslave_cb wasn't called, so onAsyncSendComplete wasn't called), wait for the next tick.
static void socketslave_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
	ev_io_stop(loop, w);
	CNetState* state = reinterpret_cast<CNetState *>( w->data );
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			// g_NetworkOut.onAsyncSendComplete(state);
		}		
		else if ( revents & EV_WRITE )
		{
			CNetworkThread* thread = state->getParentThread();
			if (thread != nullptr)
				thread->onAsyncSendComplete(state, true);	// we can send (again) data
		}
	}
	
	if ( state->isSendingAsync() )
	{
		ev_io_start(loop, w);
	}
}

LinuxEv::LinuxEv(void) : AbstractSphereThread("T_NetLoop", IThread::High)
{
	m_eventLoop = ev_loop_new(EV_BACKEND_LIST);
	ASSERT(m_eventLoop != nullptr);	// libev probably couldn't find sys/poll.h, select.h and other includes (compiling on ubuntu with both x86_64 and i386 compilers? or more gcc versions?)
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

// Start to monitor the state of the socket with a CNetState
void LinuxEv::registerClient(CNetState * state, EventsID eventCheck)
{
	// we call it with eventCheck = LinuxEv::Write, so we check when the socket will be again writable

	ADDTOCALLSTACK("LinuxEv::registerClient");
	ASSERT(state != nullptr);
	
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

void LinuxEv::unregisterClient(CNetState * state)
{
	ADDTOCALLSTACK("LinuxEv::unregisterClient");
	ASSERT(state != nullptr);

	state->setSendingAsync(false);
	
	ev_io_stop(m_eventLoop, state->iocb());
}

void LinuxEv::forceClientevent(CNetState * state, EventsID eventForce)
{
	ADDTOCALLSTACK("LinuxEv::forceClientevent");
	ASSERT(state != nullptr);
	ev_invoke(m_eventLoop, state->iocb(), (int)eventForce);
}

void LinuxEv::forceClientread(CNetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientread");
	forceClientevent(state, LinuxEv::Read);
}

void LinuxEv::forceClientwrite(CNetState * state)
{
	ADDTOCALLSTACK("LinuxEv::forceClientwrite");
	forceClientevent(state, LinuxEv::Write);
}

void LinuxEv::registerMainsocket()
{
	// The socket will be checked for incoming data/connections by CNetworkManager::processAllInput.
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
