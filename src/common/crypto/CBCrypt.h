#ifndef _INC_CBCRYPT_H
#define _INC_CBCRYPT_H

#include "../sphere_library/CSString.h"

struct CBCrypt
{
    static CSString HashBCrypt(const char* password, int iPrefixCode = 2, int iCost = 4);
    static bool ValidateBCrypt(const char* password, const char* hash);
};

#endif	// _INC_CBCRYPT_H
