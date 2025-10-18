/**
* @file spheresvr.h
*
*/

#ifndef _INC_SPHERESVR_H
#define _INC_SPHERESVR_H

#include "../common/sphere_library/CSAssoc.h"

extern std::string g_sServerDescription;
extern CSStringList g_AutoComplete;

int Sphere_MainEntryPoint( int argc, char *argv[] );
int Sphere_InitServer( int argc, char *argv[] );
void Sphere_ExitServer();
bool Sphere_CheckMainStuckAndRestart();

#endif // _INC_SPHERESVR_H
