/**
 * @file startup_monitor_thread.h
 *
 * StartupMonitorThread
 *  - Provides a proper Sphere thread context on the bootstrap OS thread during startup.
 *  - Later, it can act as the monitor thread on that same OS thread.
 *
 * Usage pattern:
 *  - Very early in main(): g_StartupMonitor.attachToCurrentThread("T_SphereStartup");
 *  - Run all startup/bootstrap work (LoadIni, scripts, world loading).
 *  - If running core on separate OS thread:
 *      g_Main.start();
 *      g_StartupMonitor.renameAsMonitor();
 *      g_StartupMonitor.runMonitorLoop(); // blocking, replaces Sphere_MainMonitorLoop()
 *    Else (inline core on bootstrap thread):
 *      g_StartupMonitor.detachFromCurrentThread();
 *      AbstractThread::ThreadBindingScope bind(g_Main, "T_Main");
 *      while (!g_Serv.GetExitFlag()) g_Main.tick();
 */

#ifndef _INC_STARTUP_MONITOR_THREAD_H
#define _INC_STARTUP_MONITOR_THREAD_H

#include "threads.h"

class StartupMonitorThread final : public AbstractSphereThread
{
public:
    StartupMonitorThread();
    ~StartupMonitorThread() override = default;

    // Not used in attached/inline mode (we don’t spawn an OS thread for this class),
    // but we must still define it.
    void tick() override;

    // Rename the current OS thread for visibility once we switch from startup duties to monitor duties.
    void renameAsMonitor();

    // Blocking monitor loop (runs on the bootstrap OS thread after startup).
    // Mirrors the previous Sphere_MainMonitorLoop() logic.
    void runMonitorLoop();
};

#endif // _INC_STARTUP_MONITOR_THREAD_H
