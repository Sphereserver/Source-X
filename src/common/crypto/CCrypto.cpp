//
// CCrypto.cpp
//

#include "../../sphere/threads.h"
#include "../sphereproto.h"
#include "../CExpression.h"
#include "../CScript.h"
#include "../CLog.h"
#include "CCrypto.h"

// For TwoFish and MD5 we only provide an interface, so we include the headers of the code doing all the related crypto stuff
#include "twofish/twofish.h"
#include "CMD5.h"


// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

/*		Login keys from SphereCrypt.ini		*/

std::vector<CCryptoClientKey> CCrypto::client_keys;

void CCrypto::SetClientVersion( dword iVer )
{
	m_iClientVersion = iVer;
}

void CCrypto::SetMasterKeys( dword hi, dword low )
{
	m_MasterHi = hi;
	m_MasterLo = low;
}

void CCrypto::SetCryptMask( dword hi, dword low )
{
	m_CryptMaskHi = hi;
	m_CryptMaskLo= low;
}

bool CCrypto::SetConnectType( CONNECT_TYPE ctWho )
{
	if ( ctWho > CONNECT_NONE && ctWho < CONNECT_QTY )
	{
		m_ConnectType = ctWho;
		return true;
	}

	return false;
}

bool CCrypto::SetEncryptionType( ENCRYPTION_TYPE etWho )
{
	if ( etWho >= ENC_NONE && etWho < ENC_QTY )
	{
		m_GameEnc = etWho;
		return true;
	}

	return false;
}

dword CCrypto::GetClientVer() const
{
	return m_iClientVersion;
}

bool CCrypto::IsValid() const
{
	return ( m_iClientVersion > 0 );
	//return ( (m_iClientVersion > 0) && (m_iClientVersion < 10000000) );	// should be safer? which versions do the Enhanced clients report?
}

bool CCrypto::IsInit() const
{
	return m_fInit;
}

CONNECT_TYPE CCrypto::GetConnectType() const
{
	return m_ConnectType;
}

ENCRYPTION_TYPE CCrypto::GetEncryptionType() const
{
	return m_GameEnc;
}

void CCrypto::LoadKeyTable(CScript & s)
{
	ADDTOCALLSTACK("CCrypto::LoadKeyTable");
	client_keys.clear();

	// Always add nocrypt
	addNoCryptKey();

	while ( s.ReadKeyParse() )
	{
		CCryptoClientKey c;
		c.m_client = ahextoi( s.GetKey() );
		c.m_key_1 = s.GetArgVal();
		c.m_key_2 = s.GetArgVal();
		c.m_EncType = (ENCRYPTION_TYPE)s.GetArgVal();
		client_keys.emplace_back(c);
	}
}

void CCrypto::addNoCryptKey(void)
{
	ADDTOCALLSTACK("CCrypto::addNoCryptKey");
	CCryptoClientKey c;
	c.m_client = 0;
	c.m_key_1 = 0;
	c.m_key_2 = 0;
	c.m_EncType = ENC_NONE;
	client_keys.emplace_back(c);
}

// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

/*
const word CCrypto::packet_size[0xde] = {
		0x0068, 0x0005, 0x0007, 0x8000, 0x0002, 0x0005, 0x0005, 0x0007, 0x000e, 0x0005, 0x000b, 0x0007, 0x8000, 0x0003, 0x8000, 0x003d,
		0x00d7, 0x8000, 0x8000, 0x000a, 0x0006, 0x0009, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0025, 0x8000, 0x0005, 0x0004, 0x0008,
		0x0013, 0x0008, 0x0003, 0x001a, 0x0007, 0x0014, 0x0005, 0x0002, 0x0005, 0x0001, 0x0005, 0x0002, 0x0002, 0x0011, 0x000f, 0x000a,
		0x0005, 0x8000, 0x0002, 0x0002, 0x000a, 0x028d, 0x8000, 0x0008, 0x0007, 0x0009, 0x8000, 0x8000, 0x8000, 0x0002, 0x0025, 0x8000,
		0x00c9, 0x8000, 0x8000, 0x0229, 0x02c9, 0x0005, 0x8000, 0x000b, 0x0049, 0x005d, 0x0005, 0x0009, 0x8000, 0x8000, 0x0006, 0x0002,
		0x8000, 0x8000, 0x8000, 0x0002, 0x000c, 0x0001, 0x000b, 0x006e, 0x006a, 0x8000, 0x8000, 0x0004, 0x0002, 0x0049, 0x8000, 0x0031,
		0x0005, 0x0009, 0x000f, 0x000d, 0x0001, 0x0004, 0x8000, 0x0015, 0x8000, 0x8000, 0x0003, 0x0009, 0x0013, 0x0003, 0x000e, 0x8000,
		0x001c, 0x8000, 0x0005, 0x0002, 0x8000, 0x0023, 0x0010, 0x0011, 0x8000, 0x0009, 0x8000, 0x0002, 0x8000, 0x000d, 0x0002, 0x8000,
		0x003e, 0x8000, 0x0002, 0x0027, 0x0045, 0x0002, 0x8000, 0x8000, 0x0042, 0x8000, 0x8000, 0x8000, 0x000b, 0x8000, 0x8000, 0x8000,
		0x0013, 0x0041, 0x8000, 0x0063, 0x8000, 0x0009, 0x8000, 0x0002, 0x8000, 0x001a, 0x8000, 0x0102, 0x0135, 0x0033, 0x8000, 0x8000,
		0x0003, 0x0009, 0x0009, 0x0009, 0x0095, 0x8000, 0x8000, 0x0004, 0x8000, 0x8000, 0x0005, 0x8000, 0x8000, 0x8000, 0x8000, 0x000d,
		0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x0040, 0x0009, 0x8000, 0x8000, 0x0003, 0x0006, 0x0009, 0x0003, 0x8000, 0x8000, 0x8000,
		0x0024, 0x8000, 0x8000, 0x8000, 0x0006, 0x00cb, 0x0001, 0x0031, 0x0002, 0x0006, 0x0006, 0x0007, 0x8000, 0x0001, 0x8000, 0x004e,
		0x8000, 0x0002, 0x0019, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x8000, 0x010c, 0x8000, 0x8000, 0x0009, 0x8000
};

int CCrypto::GetPacketSize(byte packet) // static
{
	if ( packet >= 0xde )
		return -1;

	return( packet_size[packet] );
}
*/

// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

int CCrypto::GetVerFromString( lpctstr pszVersion )
{
	ADDTOCALLSTACK("CCrypt::GetVerFromString");
	// Get version of old clients, which report the client version as ASCII string (eg: '5.0.2b')

	if ( (pszVersion == nullptr) || (*pszVersion == '\0') )
		return 0;

	byte iLetter = 0;
	size_t iMax = strlen(pszVersion);
	for ( size_t i = 0; i < iMax; ++i )
	{
		if ( IsAlpha(pszVersion[i]) )
		{
			iLetter = (pszVersion[i] - 'a') + 1;
			break;
		}
	}

	tchar *piVer[3];
	Str_ParseCmds(const_cast<tchar *>(pszVersion), piVer, ARRAY_COUNT(piVer), ".");

	// Don't rely on all values reported by client, because it can be easily faked. Injection users can report any
	// client version they want, and some custom clients may also report client version as "Custom" instead X.X.Xy
	if ( !piVer[0] || !piVer[1] || !piVer[2] || (iLetter > 26) )
		return 0;

	return (atoi(piVer[0]) * 1000000) + (atoi(piVer[1]) * 10000) + (atoi(piVer[2]) * 100) + iLetter;
}

int CCrypto::GetVerFromNumber( dword maj, dword min, dword rev, dword pat )
{
	ADDTOCALLSTACK("CCrypto::GetVerFromNumber");
	// Get version of new clients (5.0.6.5+), which report the client version as numbers (eg: 5,0,6,5)

	return (maj * 1000000) + (min * 10000) + (rev * 100) + pat;
}

char* CCrypto::WriteClientVerString( dword iClientVersion, char * pStr, uint uiBufLen)
{
	ADDTOCALLSTACK("CCrypto::WriteClientVerString");
	if ( iClientVersion >= MINCLIVER_NEWVERSIONING )
	{
		snprintf(pStr, uiBufLen, "%u.%u.%u.%u", iClientVersion / 1000000, (iClientVersion / 10000) % 100, (iClientVersion % 10000) / 100, iClientVersion % 100);
	}
	else
	{
		int iVer = snprintf(pStr, uiBufLen, "%u.%u.%u", iClientVersion / 1000000, (iClientVersion / 10000) % 100, (iClientVersion % 10000) / 100);
		char iPatch = iClientVersion % 100;
		if ( iPatch )
		{
			pStr[iVer++] = iPatch + 'a' - 1;
			pStr[iVer] = '\0';
		}
	}

	return pStr;
}

int CCrypto::GetVersionFromString( lpctstr pszVersion )
{
	ADDTOCALLSTACK("CCrypto::GetVersionFromString");
	return CCrypto::GetVerFromString( pszVersion );
}

char* CCrypto::WriteClientVer( char * pcStr, uint uiBufLen) const
{
	ADDTOCALLSTACK("CCrypto::WriteClientVer");
	return( CCrypto::WriteClientVerString( GetClientVer(), pcStr, uiBufLen ) );
}

bool CCrypto::SetClientVerEnum( dword iVer, bool bSetEncrypt )
{
	ADDTOCALLSTACK("CCrypto::SetClientVerEnum");
	for (size_t i = 0; i < client_keys.size(); ++i )
	{
		CCryptoClientKey & key = client_keys[i];

		if ( iVer == key.m_client )
		{
			if ( SetClientVerIndex( i, bSetEncrypt ))
				return true;
		}
	}

	return false;
}

bool CCrypto::SetClientVerIndex( size_t iVer, bool bSetEncrypt )
{
	ADDTOCALLSTACK("CCrypto::SetClientVerIndex");
	if ( iVer >= client_keys.size() )
		return false;

	CCryptoClientKey & key = client_keys[iVer];

	SetClientVersion(key.m_client);
	SetMasterKeys(key.m_key_1, key.m_key_2); // Hi - Lo
	if ( bSetEncrypt )
		SetEncryptionType(key.m_EncType);

	return true;
}

void CCrypto::SetClientVer( const CCrypto & crypt )
{
	ADDTOCALLSTACK("CCrypto::SetClientVer");
	m_fInit = false;
	m_iClientVersion = crypt.m_iClientVersion;
	m_MasterHi = crypt.m_MasterHi;
	m_MasterLo = crypt.m_MasterLo;
}

bool CCrypto::SetClientVer( lpctstr pszVersion )
{
	ADDTOCALLSTACK("CCrypto::SetClientVer");
	int iVer = 0;

	iVer = GetVersionFromString(pszVersion);

	m_fInit = false;

	if ( ! SetClientVerEnum( iVer ) )
	{
		DEBUG_ERR(( "Unsupported ClientVersion '%s'/'0x%x'. Use Ignition?\n", pszVersion, iVer ));
		return false;
	}

	return true;
}

// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

/*		Init encryption engine		*/

CCrypto::CCrypto()
{
	// Always at least one crypt code, for non encrypted clients!
	if ( ! client_keys.size() )
		addNoCryptKey();

	m_fInit = false;
	m_fRelayPacket = false;
	//SetClientVerEnum(client_keys[0][2]);
	SetClientVerEnum(0);

	tf_cipher	= new cipherInstance;
	tf_key		= new keyInstance;
	m_md5_engine	= new CMD5();

	m_CryptMaskHi = m_CryptMaskLo = 0;
	m_seed = 0;
	m_ConnectType = CONNECT_NONE;
	tf_position = 0;
	m_gameTable = 0;
	m_gameBlockPos = 0;
	m_gameStreamPos = 0;
	m_md5_position = 0;
}

CCrypto::~CCrypto()
{
	delete tf_cipher;
	delete tf_key;
	delete m_md5_engine;
}

bool CCrypto::Init( dword dwIP, const byte * pEvent, uint inLen, bool isclientKr )
{
	ADDTOCALLSTACK("CCrypto::Init");
	bool bReturn = true;

#ifdef DEBUG_CRYPT_MSGS
		DEBUG_MSG(("Called Init Seed(0x%x)\n", dwIP));
#endif

	if ( inLen == 62 ) // SERVER_Login 1.26.0
	{
		LoginCryptStart( dwIP, pEvent, inLen );
	}
	else
	{
		if ( isclientKr )
		{
			m_seed = dwIP;
			m_fInit = SetConnectType( CONNECT_GAME ) && SetEncryptionType( ENC_NONE );
		}
		else if ( inLen == 65 )	// Auto-registering server sending us info.
		{
			GameCryptStart( dwIP, pEvent, inLen );
		}
		else
		{
#ifdef DEBUG_CRYPT_MSGS
			DEBUG_MSG(("Odd login message length %" PRIuSIZE_T "? [CCrypto::Init()]\n", inLen));
#endif
			bReturn = false;
		}
	}

	m_fRelayPacket = false;
	return bReturn;
}

void CCrypto::InitFast( dword dwIP, CONNECT_TYPE ctInit, bool fRelay)
{
	/**
	 * Quickly set seed and connection type.
	 *  dwIP   = Seed
	 *  ctInit = Connection type
	 *  fRelay = Switched via relay packet (server selection) / Expect next game packet to be encrypted twice
	 **/
	ADDTOCALLSTACK("CCrypto::InitFast");

	SetConnectType( ctInit );
	m_seed = dwIP;

	if ( ctInit == CONNECT_GAME )
	{
		InitBlowFish();
		InitTwoFish();
	}

	if ( fRelay == true )
	{
		m_fRelayPacket = true;

		// no need to init game encryption here, we will only need to init again
		// with a new seed on the next game packet anyway
	}
}


// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

/*		Encryption utility methods		*/


bool CCrypto::RelayGameCryptStart( byte * pOutput, const byte * pInput, uint outLen, uint inLen )
{
	/**
	* When the client switches between login and game server without opening a new connection, the first game packet
	* is encrypted with both login and game server encryptions. This method attempts to initialize the game encryption
	* from this initial game packet.
	*
	* If this wasn't inconvenient enough, it seems that not all client versions behave like this and some earlier ones
	* do properly switch over to game encryption:
	* - Earliest Confirmed "Double Crypt" Client = 6.0.1.6
	* - Latest Confirmed "Single Crypt" Client   = 2.0.3.0
	**/
	ADDTOCALLSTACK("CCrypto::RelayGameCryptStart");

	// don't decrypt any future packets here
	m_fRelayPacket = false;

	// assume that clients prior to 2.0.4 do not double encrypt
	// (note: assumption has been made based on the encryption type in spherecrypt.ini, further testing is required!)
	if ( GetClientVer() < 2000400u )
	{
		InitBlowFish();
		InitTwoFish();
		if (!Decrypt(pOutput, pInput, outLen, inLen))
        {
            g_Log.EventError("NET-IN: RelayGameCryptStart failed (Decrypt, ClientVer < 2.00.04.0).\n");
            return false;
        }
        return true;
	}

	// calculate new seed
	dword dwNewSeed = m_MasterHi ^ m_MasterLo;
	dwNewSeed = ((dwNewSeed >> 24) & 0xFF) | ((dwNewSeed >> 8) & 0xFF00) | ((dwNewSeed << 8) & 0xFF0000) | ((dwNewSeed << 24) & 0xFF000000);
	dwNewSeed ^= m_seed;

	// set seed
	m_seed = dwNewSeed;

	// new seed requires a reset of the md5 engine
	m_md5_engine->reset();

	ENCRYPTION_TYPE etPrevious = GetEncryptionType();

	// decrypt packet as game
	// - rather than trust spherecrypt.ini, we can autodetect the encryption type
	//   by looking for the 0x91 command if the packet length is 65 (which it should be anyway)
	bool bFoundEncrypt = false;
	if ( inLen == 65 )
	{
		for (int i = ENC_NONE; i <= ENC_TFISH; ++i)
		{
			SetEncryptionType( (ENCRYPTION_TYPE)i );

			InitBlowFish();
			InitTwoFish();
			if (!Decrypt(pOutput, pInput, outLen, inLen))
            {
                g_Log.EventError("NET-IN: RelayGameCryptStart failed (Decrypt, autodetect).\n");
                return false;
            }

			if ((pOutput[0] ^ (byte) m_CryptMaskLo) == 0x91)
			{
				bFoundEncrypt = true;
				break;
			}
		}
	}

	if (bFoundEncrypt == false)
	{
		// auto-detect failed, assume spherecrypt was correct and see what happens
		SetEncryptionType(etPrevious);

		InitBlowFish();
		InitTwoFish();
		if (!Decrypt(pOutput, pInput, outLen, inLen))
        {
            g_Log.EventError("NET-IN: RelayGameCryptStart failed (Decrypt, not autodetected).\n");
            return false;
        }
	}

	// decrypt decrypted packet as login
	if (!DecryptLogin( pOutput, pOutput, outLen, inLen ))
    {
        g_Log.EventError("NET-IN: RelayGameCryptStart failed (DecryptLogin).\n");
        return false;
    }
    return true;
}

bool CCrypto::Encrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen )
{
	ADDTOCALLSTACK("CCrypto::Encrypt");
	if ( ! inLen )
		return false;

	if ( m_ConnectType == CONNECT_LOGIN )
		return false;

    const ENCRYPTION_TYPE enc = GetEncryptionType();
	if ( enc == ENC_TFISH )
	{
		if (!EncryptMD5( pOutput, pInput, outLen, inLen ))
            return false;
	}
    // It appears that prior to client version 1.26.04, the client performs no decryption of data received from any server.
    //  Should we send unencrypted data?
    /*
    else if (enc == ENC_LOGIN)
    {
        //const dword dwOCryptMaskLo = m_CryptMaskLo, dwOCryptMaskHi = m_CryptMaskHi;
        bool fRes = DecryptLogin(pOutput, pInput, outLen, inLen);
        //m_CryptMaskLo = dwOCryptMaskLo;
        //m_CryptMaskHi = dwOCryptMaskHi;
        if (!fRes)
            return false;
    }
    */
	memcpy( pOutput, pInput, inLen );
    return true;
}

bool CCrypto::Decrypt( byte * pOutput, const byte * pInput, uint outLen, uint inLen )
{
	ADDTOCALLSTACK("CCrypto::Decrypt");
	if ( ! inLen )
		return false;

    ENCRYPTION_TYPE enc = GetEncryptionType();
	if ( (m_ConnectType == CONNECT_LOGIN) || ( enc == ENC_LOGIN ) )
	{
        if (!DecryptLogin( pOutput, pInput, outLen, inLen ))
        {
            g_Log.EventError("NET-IN: Trying to decrypt (Login) too much data. Packet will not be parsed further.\n");
            return false;
        }
		return true;
	}

	if ( m_fRelayPacket == true )
	{
		if (!RelayGameCryptStart(pOutput, pInput, outLen, inLen ))
        {
            g_Log.EventError("NET-IN: RelayGameCryptStart returned an error. Packet will not be parsed further.\n");
            return false;
        }
        return true;
	}

	if ( enc == ENC_NONE )
	{
        if (outLen < inLen)
        {
            g_Log.EventError("NET-IN: Trying to decrypt (NoCrypt) too much data. Packet will not be parsed further.\n");
            return false;
        }
		memcpy( pOutput, pInput, inLen );
		return true;
	}

    if ( enc == ENC_TFISH || enc == ENC_BTFISH )
    {
        if (!DecryptTwoFish( pOutput, pInput, outLen, inLen ))
        {
            g_Log.EventError("NET-IN: Trying to decrypt (TFISH/BTFISH=%s) too much data. Packet will not be parsed further.\n", (enc == ENC_TFISH) ? "TFISH" : "BTFISH");
            return false;
        }
    }

	if ( enc == ENC_BFISH || enc == ENC_BTFISH )
	{
        if ( enc == ENC_BTFISH )
        {
            if (!DecryptBlowFish( pOutput, pOutput, outLen, inLen ))
            {
                g_Log.EventError("NET-IN: Trying to decrypt (BTFISH) too much data. Packet will not be parsed further.\n");
                return false;
            }
        }
        else
        {
            if (!DecryptBlowFish( pOutput, pInput, outLen, inLen ))
            {
                g_Log.EventError("NET-IN: Trying to decrypt (BFISH) too much data. Packet will not be parsed further.\n");
                return false;
            }
        }
	}

    return true;
}


// ===============================================================================================================
// ---------------------------------------------------------------------------------------------------------------
// ===============================================================================================================

/*		Handle login encryption		*/

bool CCrypto::LoginCryptStart( dword dwIP, const byte * pEvent, uint inLen )
{
	ADDTOCALLSTACK("CCrypto::LoginCryptStart");
	ASSERT(pEvent != nullptr);

    ASSERT(inLen <= MAX_BUFFER);
    std::unique_ptr<byte[]> pRaw = std::make_unique<byte[]>(MAX_BUFFER);
    memcpy(pRaw.get(), pEvent, inLen);
	
	m_seed = dwIP;
	SetConnectType( CONNECT_LOGIN );

	dword tmp_CryptMaskLo = (((~m_seed) ^ 0x00001357) << 16) | ((( m_seed) ^ 0xffffaaaa) & 0x0000ffff);
	dword tmp_CryptMaskHi = ((( m_seed) ^ 0x43210000) >> 16) | (((~m_seed) ^ 0xabcdffff) & 0xffff0000);

	SetClientVerIndex(0);
	SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo);

	for (uint i = 0, iAccountNameLen = 0;;)
	{
		if ( i >= client_keys.size() )
		{
			// Unknown client !!! Set as unencrypted and let Sphere do the rest.
#ifdef DEBUG_CRYPT_MSGS
			DEBUG_ERR(("Unknown client, i = %" PRIuSIZE_T "\n", i));
#endif
			SetClientVerIndex(0);
			SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo); // Hi - Lo
			break;
		}

		SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo);
		// Set client version properties
		SetClientVerIndex(i);

		// Test Decrypt
		if (!Decrypt(pRaw.get(), pEvent, MAX_BUFFER, inLen ))
        {
            g_Log.EventError("NET-IN: LoginCryptStart failed (decrypt).\n");
            return false;
        }

#ifdef DEBUG_CRYPT_MSGS
		DEBUG_MSG(("LoginCrypt %" PRIuSIZE_T " (%" PRIu32 ") type %" PRIx8 "-%" PRIx8 "\n", i, GetClientVer(), m_Raw[0], pEvent[0]));
#endif
		bool isValid = (pRaw[0] == 0x80 && pRaw[30] == 0x00 && pRaw[60] == 0x00 );
		if ( isValid )
		{
			// -----------------------------------------------------
			// This is a sanity check, sometimes client keys (like 4.0.0) can intercept, incorrectly,
			// a login packet. When is decrypted it is done not correctly (strange chars after
			// regular account name/password). This prevents that fact, choosing the right keys
			// to decrypt it correctly :)
			for (int toCheck = 21; toCheck <= 30; ++toCheck)
			{
				// no official client allows the account name or password to
				// exceed 20 chars (2d=16,kr=20), meaning that chars 21-30 must
				// always be 0x00 (some unofficial clients may allow the user
				// to enter more, but as they generally don't use encryption
				// it shouldn't be a problem)
				if (pRaw[toCheck] == 0x00 && pRaw[toCheck+30] == 0x00)
					continue;

				isValid = false;
				break;
			}

			if ( isValid == true )
			{
                char pszAccountNameCheck[MAX_ACCOUNT_NAME_SIZE];
				lpctstr sRawAccountName = reinterpret_cast<lpctstr>(pRaw.get() + 1 );
				iAccountNameLen = (uint)Str_GetBare(pszAccountNameCheck, sRawAccountName, MAX_ACCOUNT_NAME_SIZE, ACCOUNT_NAME_VALID_CHAR);

				// (matex) TODO: What for? We do not really need pszAccountNameCheck here do we?!
				if (iAccountNameLen > 0)
					pszAccountNameCheck[iAccountNameLen-1] = '\0';
				if (iAccountNameLen != strlen(sRawAccountName))
				{
					iAccountNameLen = 0;
					++i;

					continue;
				}

				// set seed, clientversion, cryptmask
				SetClientVerIndex(i);
				SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo);
				break;
			}
		}

		// Next one
		++i;
	}

	m_fInit = true;
    return true;
}

bool CCrypto::GameCryptStart( dword dwIP, const byte * pEvent, uint inLen )
{
	ADDTOCALLSTACK("CCrypto::GameCryptStart");
	ASSERT( pEvent != nullptr );

    ASSERT(inLen <= MAX_BUFFER);
    std::unique_ptr<byte[]> pRaw = std::make_unique<byte[]>(MAX_BUFFER);
	memcpy( pRaw.get(), pEvent, inLen );

	m_seed = dwIP;
	SetConnectType( CONNECT_GAME );

	bool bOut = false;

    // Auto-detect if the encryption is BFISH, BTFISH or TFISH
	for ( uint i = ENC_NONE; i <= ENC_TFISH; ++i )
	{
		SetEncryptionType( (ENCRYPTION_TYPE)i );
		if ( GetEncryptionType() == ENC_TFISH || GetEncryptionType() == ENC_BTFISH )
			InitTwoFish();
		if ( GetEncryptionType() == ENC_BFISH || GetEncryptionType() == ENC_BTFISH )
			InitBlowFish();

		if (!Decrypt( pRaw.get(), pEvent, MAX_BUFFER, inLen ))
        {
            g_Log.EventError("NET-IN: GameCryptStart failed (decrypt).\n");
            return false;
        }

#ifdef DEBUG_CRYPT_MSGS
		DEBUG_MSG(("GameCrypt %" PRIuSIZE_T " (%" PRIu32 ") type %" PRIx8 "-%" PRIx8 "\n", i, GetClientVer(), m_Raw[0], pEvent[0]));
#endif

		if ( pRaw[0] == 0x91 )
		{
            if ( pRaw[34] == 0x00 && pRaw[64] == 0x00)
            {
                bOut = true;    // Ok the new detected encryption is ok (legit post-login packet: 0x91)
                break;
            }
		}
	}

    // Auto-detect if the encryption is LOGIN (clients < 1.26.0 use as game encryption/decryption the same algorithm used for the login encryption)
    if (!bOut)
    {
        const dword tmp_CryptMaskLo = (((~m_seed) ^ 0x00001357) << 16) | ((( m_seed) ^ 0xffffaaaa) & 0x0000ffff);
        const dword tmp_CryptMaskHi = ((( m_seed) ^ 0x43210000) >> 16) | (((~m_seed) ^ 0xabcdffff) & 0xffff0000);
        SetClientVerIndex(0);

        for (size_t i = 0;;)
        {
            if ( i >= client_keys.size() )
            {
                // Unknown client !!! Set as unencrypted and let Sphere do the rest.
#ifdef DEBUG_CRYPT_MSGS
                DEBUG_ERR(("Unknown client, i = %" PRIuSIZE_T "\n", i));
#endif
                SetClientVerIndex(0);
				SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo); // Hi - Lo
                break;
            }

            // Set client version properties
			SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo); // Hi - Lo
            SetClientVerIndex(i);
            if (GetEncryptionType() != ENC_LOGIN)
            {
                ++i;
                continue;
            }

            // Test Decrypt
            if (!Decrypt( pRaw.get(), pEvent, MAX_BUFFER, inLen ))
            {
                g_Log.EventError("NET-IN: LoginCryptStart failed (decrypt).\n");
				SetCryptMask( tmp_CryptMaskHi, tmp_CryptMaskLo); // Hi - Lo
                SetClientVerIndex(0);
                return false;
            }

            if ( pRaw[0] == 0x91 )
            {
                if ( pRaw[34] == 0x00 && pRaw[64] == 0x00)
                {
                    bOut = true;    // Ok the new detected encryption is ok (legit post-login packet: 0x91)
                    SetCryptMask(tmp_CryptMaskHi, tmp_CryptMaskLo); 
                    break;
                }
            }
            ++i;
        }
    }

	// Well no ecryption guessed, set it to none and let Sphere do the dirty job :P
	if ( !bOut )
	{
		SetEncryptionType( ENC_NONE );
	}
	else
	{
		if ( GetEncryptionType() == ENC_TFISH || GetEncryptionType() == ENC_BTFISH )
			InitTwoFish();
		if ( GetEncryptionType() == ENC_BFISH || GetEncryptionType() == ENC_BTFISH )
			InitBlowFish();
	}

	m_fInit = true;
    return true;
}


