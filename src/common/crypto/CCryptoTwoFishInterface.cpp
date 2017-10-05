#include "../../sphere/threads.h"
#include "../sphereproto.h"
#include "CCrypto.h"

void CCrypto::InitTwoFish()
{
	ADDTOCALLSTACK("CCrypto::InitTwoFish");
	// Taken from t2tfish.cpp / CCryptNew.cpp

	dword dwIP = UNPACKDWORD( ((byte*) & m_seed ) );

#ifdef DEBUG_CRYPT_MSGS
	DEBUG_MSG(("GameCrypt Seed (%x)(%i-%x)\n", m_seed, dwIP, dwIP));
#endif

	// ---------------------------------------------
	memset(&tf_key, 0, sizeof(keyInstance));
	memset(&tf_cipher, 0, sizeof(cipherInstance));
	tf_position = 0;
	// ---------------------------------------------

	makeKey( &tf_key, 1 /*DIR_DECRYPT*/, 0x80, NULL );
	cipherInit( &tf_cipher, 1/*MODE_ECB*/, NULL );

	tf_key.key32[0] = tf_key.key32[1] = tf_key.key32[2] = tf_key.key32[3] = dwIP; //0x7f000001;
	reKey( &tf_key );

	for ( ushort i = 0; i < TFISH_RESET; i++ )
		tf_cipherTable[i] = (byte)(i);

	tf_position = 0;

	byte tmpBuff[TFISH_RESET];
	blockEncrypt( &tf_cipher, &tf_key, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
	memcpy( &tf_cipherTable, &tmpBuff, TFISH_RESET );

	if ( GetEncryptionType() == ENC_TFISH )
	{
		InitMD5(&tmpBuff[0]);
	}
}

void CCrypto::DecryptTwoFish( byte * pOutput, const byte * pInput, size_t iLen )
{
	ADDTOCALLSTACK("CCrypto::DecryptTwoFish");
	byte tmpBuff[TFISH_RESET];

	for ( size_t i = 0; i < iLen; ++i )
	{
		if ( tf_position >= TFISH_RESET )
		{
			blockEncrypt( &tf_cipher, &tf_key, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
			memcpy( &tf_cipherTable, &tmpBuff, TFISH_RESET );
			tf_position = 0;
		}

		pOutput[i] = pInput[i] ^ tf_cipherTable[tf_position++];
	}
}
