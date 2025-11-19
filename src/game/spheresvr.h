/**
* @file spheresvr.h
*
*/

#ifndef _INC_SPHERESVR_H
#define _INC_SPHERESVR_H

#include "../common/sphere_library/CSAssoc.h"

/* Globals */
// Generic. Simple data types.
extern std::string g_sServerDescription;

// Threading.
extern std::atomic_bool sg_inStartup;
extern std::atomic_bool sg_servClosing;

// Generic. Sphere classes (require the threading flags above.
extern CSStringList g_AutoComplete;


int Sphere_MainEntryPoint( int argc, char *argv[] );
int Sphere_InitServer( int argc, char *argv[] );
void Sphere_ExitServer();
bool Sphere_CheckMainStuckAndRestart();

#endif // _INC_SPHERESVR_H
