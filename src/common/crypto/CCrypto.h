/*
* @file CCrypto.h
* @brief Support for login encryption and MD5 hashing.
*/

#ifndef _INC_CCRYPT_H
#define _INC_CCRYPT_H

#include "../common.h"
#include <vector>

#define CLIENT_END 0x00000001


typedef struct keyInstance*		tf_keyInstance;
typedef struct cipherInstance*	tf_cipherInstance;
class CMD5;
class CScript;


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
    ENC_LOGIN,  // the same login encryption is used by old clients (< 1.26.0) for game encryption/decryption
	ENC_QTY
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

// HuffMan Compression Class

#define COMPRESS_TREE_SIZE (256+1)

class CHuffman
{
public:
    static const char* m_sClassName;
    CHuffman() = default;

    static uint Compress(byte* pOutput, const byte* pInput, uint outLen, uint inLen);

private:
	static const word sm_xCompress_Base[COMPRESS_TREE_SIZE];	

private:
	CHuffman(const CHuffman& copy);
	CHuffman& operator=(const CHuffman& other);
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

// Encryption key stored in SphereCrypt.ini
struct CCryptoClientKey
{
	dword m_client;
	dword m_key_1;
	dword m_key_2;
	ENCRYPTION_TYPE m_EncType;
};

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================


struct CCryptoKeysHolder
{
	static CCryptoKeysHolder* get() noexcept;

	union CCryptoKey		// For internal encryption calculations, it has nothing to do with SphereCrypt.ini
	{
#define CRYPT_GAMESEED_LENGTH	8
		byte  u_cKey[CRYPT_GAMESEED_LENGTH];
		dword u_iKey[2];
	};

	std::vector<CCryptoClientKey> client_keys;

	void LoadKeyTable(CScript& s);
	void addNoCryptKey(void);
};

struct CCrypto
{
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
	
	//static const word packet_size[0xde];

	// --------------- Generic -----------------------------------
	//static int GetPacketSize(byte packet);
	// --------------- EOF Generic -------------------------------

protected:
	// --------------- Two Fish ------------------------------
	#define TFISH_RESET 0x100
	tf_keyInstance tf_key;
	tf_cipherInstance tf_cipher;
	byte tf_cipherTable[TFISH_RESET];
	int tf_position;
private:
	void InitTwoFish();
	bool DecryptTwoFish( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	// --------------- EOF TwoFish ----------------------------
	
	
protected:
	// -------------- Blow Fish ------------------------------
	#define CRYPT_GAMEKEY_COUNT	25 // CRYPT_MAX_SEQ
	#define CRYPT_GAMEKEY_LENGTH	6
	
	#define CRYPT_GAMETABLE_START	1
	#define CRYPT_GAMETABLE_STEP	3
	#define CRYPT_GAMETABLE_MODULO	11
	#define CRYPT_GAMETABLE_TRIGGER	21036
	static const dword sm_dwInitData[18+1024];
	static const byte sm_key_table[CRYPT_GAMEKEY_COUNT][CRYPT_GAMEKEY_LENGTH];
	static const byte sm_seed_table[2][CRYPT_GAMEKEY_COUNT][2][CRYPT_GAMESEED_LENGTH];
	static bool  sm_fTablesReady;
public:
	int	m_gameTable;
	int	m_gameBlockPos;		// 0-7
	size_t	m_gameStreamPos;	// use this to track the 21K move to the new Blowfish m_gameTable.
private:
	CCryptoKeysHolder::CCryptoKey m_Key;
private:
	void InitSeed( int iTable );
	static void InitTables();
	static void PrepareKey(CCryptoKeysHolder::CCryptoKey & key, int iTable );
	bool DecryptBlowFish( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	byte DecryptBFByte( byte bEnc );
	void InitBlowFish();
	// -------------- EOF BlowFish -----------------------

protected:
	// -------------------- MD5 ------------------------------
	#define MD5_RESET 0x0F
	CMD5 * m_md5_engine;
	uint m_md5_position;
	byte m_md5_digest[16];
protected:
	bool EncryptMD5( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	void InitMD5(byte * ucInitialize);
	// ------------------ EOF MD5 ----------------------------

private:
	// ------------- Login Encryption ----------------------
	bool DecryptLogin( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	// ------------- EOF Login Encryption ------------------

private:
	int GetVersionFromString( lpctstr pszVersion );

private:
	void SetClientVersion( dword iVer );
	void SetMasterKeys( dword hi, dword low );
	void SetCryptMask( dword hi, dword low );
	bool SetConnectType( CONNECT_TYPE ctWho );
	bool SetEncryptionType( ENCRYPTION_TYPE etWho );

public:
	char* WriteClientVer( char * pcStr, uint uiBufLen) const;
	bool SetClientVerEnum( dword iVer, bool fSetEncrypt = true );
	bool SetClientVerIndex( size_t iVer, bool fSetEncrypt = true );
	void SetClientVer( const CCrypto & crypt );
	bool SetClientVer( lpctstr ptcVersion );
	static int GetVerFromString( lpctstr ptcVersion );
	static int GetVerFromNumber( dword maj, dword min, dword rev, dword pat );
	static char* WriteClientVerString( dword iClientVersion, char * pcStr, uint uiBufLen );

public:
	dword GetClientVer() const;
	bool IsValid() const;
	bool IsInit() const;
	CONNECT_TYPE GetConnectType() const;
	ENCRYPTION_TYPE GetEncryptionType() const;

// --------- Basic
public:
	CCrypto();
	~CCrypto();
private:
	CCrypto(const CCrypto& copy);
	CCrypto& operator=(const CCrypto& other);

public:
	bool Init( dword dwIP, const byte * pEvent, uint inLen, bool isClientKR = false );
	void InitFast( dword dwIP, CONNECT_TYPE ctInit, bool fRelay = true );
	bool Decrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen );
	bool Encrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen );
protected:
	bool LoginCryptStart( dword dwIP, const  byte * pEvent, uint inLen );
	bool GameCryptStart( dword dwIP, const byte * pEvent, uint inLen );
	bool RelayGameCryptStart( byte * pOutput, const byte * pInput, uint outLen, uint inLen );
   
};

#endif // _INC_CENCRYPT_H
