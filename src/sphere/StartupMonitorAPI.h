// sphere/StartupMonitorAPI.h
#ifndef _INC_STARTUP_MONITOR_API_H
#define _INC_STARTUP_MONITOR_API_H

extern "C" {
    void Sphere_AttachBootstrapContext();
    void Sphere_RenameBootstrapToMonitor();
    void Sphere_RunMonitorLoop();
}

#endif
