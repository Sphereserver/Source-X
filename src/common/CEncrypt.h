/*
* @file CEncrypt.h
* @brief Support for login encryption and MD5 hashing.
*/

#pragma once
#ifndef _INC_CENCRYPT_H
#define _INC_CENCRYPT_H

#include "CScript.h"

// --- Two Fish ---
#include "twofish/aes.h"

// --- Blow Fish ---
// #include "blowfish/blowfish.h"

// --- MD5 ----
#include "CMD5.h"

#define CLIENT_END 0x00000001


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
	ENC_NONE=0,
	ENC_BFISH,
	ENC_BTFISH,
	ENC_TFISH,
	ENC_QTY
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

// HuffMan Compression Class

#define COMPRESS_TREE_SIZE (256+1)

class CHuffman
{
private:
	static const word sm_xCompress_Base[COMPRESS_TREE_SIZE];
public:
	static const char *m_sClassName;
	
	static size_t Compress( byte * pOutput, const byte * pInput, size_t inplen );

public:
	CHuffman() { };
private:
	CHuffman(const CHuffman& copy);
	CHuffman& operator=(const CHuffman& other);
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

struct CCryptClientKey
{
	dword m_client;
	dword m_key_1;
	dword m_key_2;
	ENCRYPTION_TYPE m_EncType;
};

struct CCrypt
{
public:
	union CCryptKey
	{
		#define CRYPT_GAMESEED_LENGTH	8
		byte  u_cKey[CRYPT_GAMESEED_LENGTH];
		dword u_iKey[2];
	};
private:
	bool m_fInit;
	bool m_fRelayPacket;
	dword m_iClientVersion;
	ENCRYPTION_TYPE m_GameEnc;

protected:
	dword m_MasterHi;
	dword m_MasterLo;

	dword m_CryptMaskHi;
	dword m_CryptMaskLo;

	dword m_seed;	// seed ip we got from the client.
	
	CONNECT_TYPE m_ConnectType;
	
//	#define TOTAL_CLIENTS 33
//	static const dword client_keys[TOTAL_CLIENTS+2][4];

	static const word packet_size[0xde];

public:
	static void LoadKeyTable(CScript & s);
	static std::vector<CCryptClientKey> client_keys;
	static void addNoCryptKey(void);

	// --------------- Generic -----------------------------------
	static int GetPacketSize(byte packet);
	// --------------- EOF Generic -------------------------------

protected:
	// --------------- Two Fish ------------------------------
	#define TFISH_RESET 0x100
	keyInstance tf_key;
	cipherInstance tf_cipher;
	byte tf_cipherTable[TFISH_RESET];
	int tf_position;
private:
	void InitTwoFish();
	void DecryptTwoFish( byte * pOutput, const byte * pInput, size_t iLen );
	// --------------- EOF TwoFish ----------------------------
	
	
protected:
	// -------------- Blow Fish ------------------------------
	#define CRYPT_GAMEKEY_COUNT	25 // CRYPT_MAX_SEQ
	#define CRYPT_GAMEKEY_LENGTH	6
	
	#define CRYPT_GAMETABLE_START	1
	#define CRYPT_GAMETABLE_STEP	3
	#define CRYPT_GAMETABLE_MODULO	11
	#define CRYPT_GAMETABLE_TRIGGER	21036
	static const byte sm_key_table[CRYPT_GAMEKEY_COUNT][CRYPT_GAMEKEY_LENGTH];
	static const byte sm_seed_table[2][CRYPT_GAMEKEY_COUNT][2][CRYPT_GAMESEED_LENGTH];
	static bool  sm_fTablesReady;
public:
	int	m_gameTable;
	int	m_gameBlockPos;		// 0-7
	size_t	m_gameStreamPos;	// use this to track the 21K move to the new Blowfish m_gameTable.
private:
	CCrypt::CCryptKey m_Key;
private:
	void InitSeed( int iTable );
	static void InitTables();
	static void PrepareKey( CCrypt::CCryptKey & key, int iTable );
	void DecryptBlowFish( byte * pOutput, const byte * pInput, size_t iLen );
	byte DecryptBFByte( byte bEnc );
	void InitBlowFish();
	// -------------- EOF BlowFish -----------------------

protected:
	// -------------------- MD5 ------------------------------
	#define MD5_RESET 0x0F
	CMD5 md5_engine;
	uint md5_position;
	byte md5_digest[16];
protected:
	void EncryptMD5( byte * pOutput, const byte * pInput, size_t iLen );
	void InitMD5(byte * ucInitialize);
	// ------------------ EOF MD5 ----------------------------

private:
	// ------------- Old Encryption ----------------------
	void DecryptOld( byte * pOutput, const byte * pInput, size_t iLen  );
	// ------------- EOF Old Encryption ------------------

private:
	int GetVersionFromString( lpctstr pszVersion );

private:
	void SetClientVersion( dword iVer );
	void SetMasterKeys( dword hi, dword low );
	void SetCryptMask( dword hi, dword low );
	bool SetConnectType( CONNECT_TYPE ctWho );
	bool SetEncryptionType( ENCRYPTION_TYPE etWho );

public:
	char* WriteClientVer( char * pStr ) const;
	bool SetClientVerEnum( dword iVer, bool bSetEncrypt = true );
	bool SetClientVerIndex( size_t iVer, bool bSetEncrypt = true );
	void SetClientVer( const CCrypt & crypt );
	bool SetClientVer( lpctstr pszVersion );
	static int GetVerFromString( lpctstr pszVersion );
	static int GetVerFromNumber( dword maj, dword min, dword rev, dword pat );
	static char* WriteClientVerString( dword iClientVersion, char * pStr );

public:
	dword GetClientVer() const;
	bool IsValid() const;
	bool IsInit() const;
	CONNECT_TYPE GetConnectType() const;
	ENCRYPTION_TYPE GetEncryptionType() const;

// --------- Basic
public:
	CCrypt();
private:
	CCrypt(const CCrypt& copy);
	CCrypt& operator=(const CCrypt& other);

public:
	bool Init( dword dwIP, byte * pEvent, size_t iLen, bool isclientKr = false );
	void InitFast( dword dwIP, CONNECT_TYPE ctInit, bool fRelay = true );
	void Decrypt( byte * pOutput, const byte * pInput, size_t iLen );
	void Encrypt( byte * pOutput, const byte * pInput, size_t iLen );
protected:
	void LoginCryptStart( dword dwIP, byte * pEvent, size_t iLen );
	void GameCryptStart( dword dwIP, byte * pEvent, size_t iLen );
	void RelayGameCryptStart( byte * pOutput, const byte * pInput, size_t iLen );
};

#endif // _INC_CENCRYPT_H
