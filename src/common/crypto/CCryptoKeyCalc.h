/*
* @file CCryptoKeyCalc.h
* @brief Calculate client cryptographic keys.
*/

#ifndef _INC_CCRYPTOKEYCALC_H
#define _INC_CCRYPTOKEYCALC_H

#include "../CUOClientVersion.h"
#include "../sphereproto.h"
#include "crypto_common.h"
#include <string>


struct CCryptoKeyCalc
{
    static CCryptoClientKey CalculateLoginKeysReportedVer(CUOClientVersion ver, ENCRYPTION_TYPE forceCryptType = ENC_NONE) noexcept;
    static CCryptoClientKey CalculateLoginKeys(CUOClientVersion ver, GAMECLIENT_TYPE cliType, ENCRYPTION_TYPE forceCryptType = ENC_NONE) noexcept;
    static std::string FormattedLoginKey(CUOClientVersion ver, GAMECLIENT_TYPE cliType, CCryptoClientKey cryptoKey);
};

#endif // _INC_CCRYPTOKEYCALC_H
