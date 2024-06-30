/*
* @file CCrypto.h
* @brief Support for login encryption and MD5 hashing.
*/

#ifndef _INC_CCRYPTO_H
#define _INC_CCRYPTO_H

#include "crypto_common.h"
#include <vector>


class CMD5;
class CScript;


#define CRYPT_GAMESEED_LENGTH 8
struct CCryptoKeysHolder
{
	static CCryptoKeysHolder* get() noexcept;

	union CCryptoKey		// For internal encryption calculations, it has nothing to do with SphereCrypt.ini
	{
		byte  u_cKey[CRYPT_GAMESEED_LENGTH];
		dword u_iKey[2];
	};

	std::vector<CCryptoClientKey> client_keys;

	void LoadKeyTable(CScript& s);
	void addNoCryptKey(void);
};

// Each CClient has its instance (+1 instance for CServerDef, for SetCryptVersion() ??).
struct CCrypto
{
private:
	bool m_fInit;
	bool m_fRelayPacket;
	dword m_iClientVersion;
	ENCRYPTION_TYPE m_GameEnc;

	dword m_MasterHi;
	dword m_MasterLo;
	dword m_CryptMaskHi;
	dword m_CryptMaskLo;
	dword m_seed;	// seed ip we got from the client.
	
	CONNECT_TYPE m_ConnectType;

    CCryptoKeysHolder::CCryptoKey m_Key;    // crypt key used by this CClient


	// --------------- Two Fish ------------------------------
private:
	#define TFISH_RESET 0x100
	void* tf_key;      // keyInstance*
	void* tf_cipher;   // cipherInstance*
	byte tf_cipherTable[TFISH_RESET];
	int tf_position;

	void InitTwoFish();
	bool DecryptTwoFish( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	// --------------- EOF TwoFish ----------------------------
	
	
	// -------------- Blow Fish ------------------------------
private:
	static bool  sm_fBFishTablesReady;

	int	m_gameTable;
	int	m_gameBlockPos;		// 0-7
	size_t	m_gameStreamPos;	// use this to track the 21K move to the new Blowfish m_gameTable.

	static void InitTables();
	static void PrepareKey(CCryptoKeysHolder::CCryptoKey & key, int iTable );

    void InitBlowFish();
    void InitSeed(int iTable);
    bool DecryptBlowFish( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	byte DecryptBFByte( byte bEnc );
	// -------------- EOF BlowFish -----------------------

	// -------------------- MD5 ------------------------------
private:
    #define MD5_RESET 0x0F
	CMD5 * m_md5_engine;
	uint m_md5_position;
	byte m_md5_digest[MD5_RESET + 1]; // 16

	bool EncryptMD5( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	void InitMD5(byte * ucInitialize);
	// ------------------ EOF MD5 ----------------------------

	// ------------- Login Encryption ----------------------
private:
    bool DecryptLogin( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen );
	// ------------- EOF Login Encryption ------------------

private:
	void SetClientVerNumber( dword iVer );
	void SetMasterKeys( dword hi, dword low );
	void SetCryptMask( dword hi, dword low );
	bool SetConnectType( CONNECT_TYPE ctWho );
	bool SetEncryptionType( ENCRYPTION_TYPE etWho );

public:
	std::string GetClientVer() const;
	bool SetClientVerFromNumber( dword uiVer, bool fSetEncrypt = true );
	bool SetClientVerFromKeyIndex( uint uiVer, bool fSetEncrypt = true );
    bool SetClientVerFromString(lpctstr ptcVersion);
    void SetClientVerFromOther( const CCrypto & crypt );

    dword GetClientVerNumber() const;
	bool IsValid() const;
	bool IsInit() const;
	CONNECT_TYPE GetConnectType() const;
	ENCRYPTION_TYPE GetEncryptionType() const;

// --------- Basic
public:
	CCrypto();
	~CCrypto();

	CCrypto(const CCrypto& copy) = delete;
	CCrypto& operator=(const CCrypto& other) = delete;

	bool Init( dword dwIP, const byte * pEvent, uint inLen, bool isClientKR = false );
	void InitFast( dword dwIP, CONNECT_TYPE ctInit, bool fRelay = true );
	bool Decrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen );
	bool Encrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen );

private:
	bool LoginCryptStart( dword dwIP, const  byte * pEvent, uint inLen );
	bool GameCryptStart( dword dwIP, const byte * pEvent, uint inLen );
	bool RelayGameCryptStart( byte * pOutput, const byte * pInput, uint outLen, uint inLen );
   
};

#endif // _INC_CCRYPTO_H
