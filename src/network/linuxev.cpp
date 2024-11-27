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

// Call this callback function when the socket is readable or writable again -> we are not sending data anymore
// The data is sent when there's some queued data to send.
static void socketslave_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	// libev could call this function aliasing ev_io as a ev_watcher.
	// ev_watcher is a "parent" struct of ev_io, they share the first member variables.
	// it's a evil trick, but does the job since C doesn't have struct inheritance
	
	ev_io_stop(loop, watcher);
	CNetState* state = reinterpret_cast<CNetState *>( watcher->data );
	
	if ( !g_Serv.IsLoading() )
	{
		if ( revents & EV_READ )
		{	
			// This happens when the client socket is readable (i can try to retrieve data), this does NOT mean
			//  that i have data to read. It might also mean that i have done writing to the socket?
			// g_NetworkOut.onAsyncSendComplete(state);
		}		
		else if ( revents & EV_WRITE )
		{
			CNetworkThread* thread = state->getParentThread();
			if (thread != nullptr)
				thread->onAsyncSendComplete(state, true);	// we can send (again) data
		}
	}
	
	if (state->isSendingAsync())
	{
		ev_io_start(loop, watcher);
	}
}

// intervals: the second is the unit, use decimals for smaller intervals
static constexpr double kLoopCollectNetworkInputSeconds = 0.002;
static constexpr double kLoopWaitBeforeNextCycleSeconds = 0.008;

LinuxEv::LinuxEv(void) : AbstractSphereThread("T_NetLoopOut", ThreadPriority::High)
	//, m_watchMainsock{}
{
	// Right now, we use libev to send asynchronously packets to clients.
	m_eventLoop = ev_loop_new(ev_recommended_backends() | EVFLAG_NOENV);
	ASSERT(m_eventLoop != nullptr);	// if fails, libev config.h probably was not configured properly
	ev_set_io_collect_interval(m_eventLoop, kLoopCollectNetworkInputSeconds);
}

LinuxEv::~LinuxEv(void)
{
	ev_loop_destroy(m_eventLoop);
}

void LinuxEv::printInitInfo()
{
	g_Log.Event(LOGM_CLIENTS_LOG, "Networking: Libev. Initialized with backend 0x%x.\n", ev_backend(m_eventLoop));
}

void LinuxEv::onStart()
{
	AbstractSphereThread::onStart();
}

static void periodic_cb(struct ev_loop* /*loop*/, ev_periodic* /*w*/, int /*revents*/) noexcept
{
	;
}

void LinuxEv::tick()
{	
	/*
	A flags value of EVRUN_NOWAIT will look for new events,
	will handle those events and any already outstanding ones,
	but will not wait and block your process in case there are no events and will return after one iteration of the loop.
	*/
	//ev_run(m_eventLoop, EVRUN_NOWAIT);

	// Trying a different approach: enter the event loop, run again if exits.
	// ev_run will keep handling events until either no event watchers are active anymore or "ev_break" was called

#ifdef _DEBUG
	g_Log.EventDebug("Networking: Libev. Starting event loop.\n");
#endif

	// This periodic timer keeps awake the event loop. We could have used ev_ref but it had its problems...
	// Don't ask me why (maybe i don't get how this actually should work, and this is only a workaround),
	//  but if we rely on ev_ref to increase the event loop reference counter to keep it alive without this periodic timer/callback,
	//  the loop will ignore the io_collect_interval. Moreover, it will make the polling backend in use (like most frequently epoll) wait the maximum 
	//  time (MAX_BLOCKTIME in ev.c, circa 60 seconds) to collect incoming data, only then the callback will be called. So each batch of packets
	//  would be processed every 60 seconds...
	struct ev_periodic periodic_check;
	ev_periodic_init(&periodic_check, periodic_cb, 0, kLoopWaitBeforeNextCycleSeconds, nullptr);
	ev_periodic_start(m_eventLoop, &periodic_check);

	ev_run(m_eventLoop, 0);

#ifdef _DEBUG
	g_Log.EventDebug("Networking: Libev. Event loop STOPPED.\n");
#endif
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

	// Right now we support only async writing to the socket.
	// Pure async read would mean to call functions and access data typically managed by the main thread,
	//  but the core isn't designed for such usage, nor we have all the thread synchronization methods for every possible stuff we might need.
	// A fair compromise TODO would be to async read incoming data, parse that in packets that will be stored in a buffer periodically accessed and processed
	//  by the main thread.
	ASSERT(0 == (eventCheck & ~EV_WRITE));

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
