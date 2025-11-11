#include "MainThread.h"
#include "../game/CWorld.h"
#include "../game/CServer.h"
#include "../network/CNetworkManager.h"

#ifdef _WIN32
#include "../sphere/ntservice.h"	// g_Service
#endif

//*******************************************************************
//	Main server loop

static int Sphere_OnTick()
{
    // Give the world (CMainTask) a single tick. RETURN: 0 = everything is fine.
    constexpr const char *m_sClassName = "SphereTick";
    EXC_TRY("Tick");
#ifdef _WIN32
    EXC_SET_BLOCK("service");
    g_NTService._OnTick();
#endif

    EXC_SET_BLOCK("world");
    g_World._OnTick();

    // process incoming data
    EXC_SET_BLOCK("network-in");
    g_NetworkManager.processAllInput();

    EXC_SET_BLOCK("server");
    g_Serv._OnTick();

    // push outgoing data
    EXC_SET_BLOCK("network-out");
    g_NetworkManager.processAllOutput();

    // don't put the network-tick between in.tick and out.tick, otherwise it will clean the out queue!
    EXC_SET_BLOCK("network-tick");
    g_NetworkManager.tick();	// then this thread has to call the network tick

    EXC_CATCH;
    return g_Serv.GetExitFlag();
}

MainThread::MainThread()
: AbstractSphereThread("T_Main", ThreadPriority::RealTime)
{
    m_profile.EnableProfile(PROFILE_NETWORK_RX);
    m_profile.EnableProfile(PROFILE_CLIENTS);
    //m_profile.EnableProfile(PROFILE_NETWORK_TX);
    m_profile.EnableProfile(PROFILE_CHARS);
    m_profile.EnableProfile(PROFILE_ITEMS);
    m_profile.EnableProfile(PROFILE_MAP);
    m_profile.EnableProfile(PROFILE_MULTIS);
    m_profile.EnableProfile(PROFILE_NPC_AI);
    m_profile.EnableProfile(PROFILE_SCRIPTS);
    m_profile.EnableProfile(PROFILE_SHIPS);
    m_profile.EnableProfile(PROFILE_TIMEDFUNCTIONS);
    m_profile.EnableProfile(PROFILE_TIMERS);
}

void MainThread::onStart()
{
    AbstractSphereThread::onStart();
}

void MainThread::tick()
{
    Sphere_OnTick();
}

bool MainThread::shouldExit() noexcept
{
    if (g_Serv.GetExitFlag() != 0)
        return true;
    return AbstractSphereThread::shouldExit();
}

