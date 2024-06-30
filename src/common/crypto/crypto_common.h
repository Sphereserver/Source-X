/*
* @file crypto_common.h
* @brief Common definition for crypto, connection and login management.
*/

#ifndef _INC_CRYPTO_COMMON_H
#define _INC_CRYPTO_COMMON_H

#include "../common.h"


enum CONNECT_TYPE	// What type of client connection is this ?
{
    CONNECT_NONE,		// There is no connection.
    CONNECT_UNK,		// client has just connected. waiting for first message.
    CONNECT_CRYPT,		// It's a game or login protocol but i don't know which yet.
    CONNECT_LOGIN,		// login client protocol
    CONNECT_GAME,		// game client protocol
    CONNECT_HTTP,		// We are serving web pages to this.
    CONNECT_TELNET,		// we are in telnet console mode.
    CONNECT_UOG,		// UOG needs special connection
    CONNECT_AXIS,		// Axis connection mode.
    CONNECT_QTY
};

enum ENCRYPTION_TYPE
{
    ENC_NONE = 0,
    ENC_BFISH,
    ENC_BTFISH,
    ENC_TFISH,
    ENC_LOGIN,      // the same login encryption is used by old clients (< 1.26.0) for game encryption/decryption
    ENC_QTY
};

// Encryption key stored in SphereCrypt.ini
struct CCryptoClientKey
{
    // TODO: add CUOClientVersion
    dword m_client;
    dword m_key_1;
    dword m_key_2;
    ENCRYPTION_TYPE m_EncType;
};


#endif // _INC_CRYPTO_COMMON_H
