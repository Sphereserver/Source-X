/**
 * @file startup_monitor_thread.cpp
 *
 * Implementation of StartupMonitorThread: a Sphere thread context bound to the
 * bootstrap OS thread for startup work, then re-used as the monitor thread.
 */

#include "../common/CLog.h"
#include "../game/CServer.h"
#include "../game/CServerConfig.h"
#include "StartupMonitorThread.h"

// External globals used by the monitor loop
extern CServer      g_Serv;
extern CServerConfig g_Cfg;
extern CLog         g_Log;

StartupMonitorThread::StartupMonitorThread()
: AbstractSphereThread("T_SphereStartup", ThreadPriority::Highest)
{
    // Enable any profiles useful during startup/monitoring.
    m_profile.EnableProfile(PROFILE_OVERHEAD);
    m_profile.EnableProfile(PROFILE_STAT_FAULTS);
    m_profile.EnableProfile(PROFILE_IDLE);
}

void StartupMonitorThread::tick()
{
    // Not used: we don’t spawn a separate OS thread for this object.
    // The monitor loop is run manually via runMonitorLoop() on the same OS thread.
}

void StartupMonitorThread::renameAsMonitor()
{
    // Rename the current OS thread (bootstrap thread) to “T_Monitor”
    // for better visibility in debuggers and logs.
    AbstractThread::setThreadName("T_Monitor");
    // Keep our internal name in sync for logs using getName().
    overwriteInternalThreadName("T_Monitor");
}

void StartupMonitorThread::runMonitorLoop()
{
    constexpr const char *m_sClassName = "SphereMonitor";

    while (!g_Serv.GetExitFlag())
    {
        EXC_TRY("MainMonitorLoop");

        if (g_Cfg.m_iFreezeRestartTime <= 0)
        {
            DEBUG_ERR(("Freeze Restart Time cannot be cleared at run time\n"));
            g_Cfg.m_iFreezeRestartTime = 10;
        }

        EXC_SET_BLOCK("Sleep");
        for (int i = 0; i < g_Cfg.m_iFreezeRestartTime; ++i)
        {
            if (g_Serv.GetExitFlag())
                break;
            SLEEP(1000);
        }

        EXC_SET_BLOCK("Checks");

        // Don’t look for freezing when doing certain things.
        if (g_Serv.IsLoadingGeneric() || !g_Cfg.m_fSecure || g_Serv.IsValidBusy())
            continue;

        #ifndef _DEBUG
        EXC_SET_BLOCK("Check Stuck");
        // NOTE: We do not hold a pointer to g_Main here to avoid coupling;
        // the original code called g_Main.checkStuck(). Keep that in the caller
        // if needed (or expose a function to do it).
        extern bool Sphere_CheckMainStuckAndRestart(); // declare a tiny wrapper in spheresvr.cpp
        if (Sphere_CheckMainStuckAndRestart())
        {
            g_Log.Event(LOGL_CRIT, "'T_Main' thread hang, restarting...\n");
        }
        #endif

        EXC_CATCH;
    }
}
