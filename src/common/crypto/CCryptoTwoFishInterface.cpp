#include "../../sphere/threads.h"
#include "../sphereproto.h"
#include "CCrypto.h"

extern "C" {
#include <twofish/twofish.h>
}


void CCrypto::InitTwoFish()
{
	ADDTOCALLSTACK("CCrypto::InitTwoFish");
	// Taken from t2tfish.cpp / CCryptNew.cpp

	dword dwIP = UNPACKDWORD( ((byte*) & m_seed ) );

#ifdef DEBUG_CRYPT_MSGS
	DEBUG_MSG(("GameCrypt Seed (%x)(%i-%x)\n", m_seed, dwIP, dwIP));
#endif

	// ---------------------------------------------
	memset(tf_key, 0, sizeof(keyInstance));
	memset(tf_cipher, 0, sizeof(cipherInstance));
	tf_position = 0;
	// ---------------------------------------------
    auto key_casted_ptr = static_cast<keyInstance*>(tf_key);
    auto cipher_casted_ptr = static_cast<cipherInstance*>(tf_cipher);

    ::makeKey(key_casted_ptr, 1 /*DIR_DECRYPT*/, 0x80); //, nullptr );
    ::cipherInit( cipher_casted_ptr, 1/*MODE_ECB*/, nullptr );

    key_casted_ptr->key32[0] = key_casted_ptr->key32[1] = key_casted_ptr->key32[2] = key_casted_ptr->key32[3] = dwIP; //0x7f000001;
    ::reKey( key_casted_ptr );

	for ( ushort i = 0; i < TFISH_RESET; i++ )
		tf_cipherTable[i] = (byte)(i);

	tf_position = 0;

	byte tmpBuff[TFISH_RESET];
    ::blockEncrypt( cipher_casted_ptr, key_casted_ptr, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
	memcpy( tf_cipherTable, &tmpBuff, TFISH_RESET );

	if ( GetEncryptionType() == ENC_TFISH )
	{
		InitMD5(&tmpBuff[0]);
	}
}

bool CCrypto::DecryptTwoFish( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen )
{
	ADDTOCALLSTACK("CCrypto::DecryptTwoFish");
	byte tmpBuff[TFISH_RESET];
    auto key_casted_ptr = static_cast<keyInstance*>(tf_key);
    auto cipher_casted_ptr = static_cast<cipherInstance*>(tf_cipher);

	for ( size_t i = 0; i < inLen; ++i )
	{
        if (i >= outLen)
            return false;

		if ( tf_position >= TFISH_RESET )
		{
            ::blockEncrypt( cipher_casted_ptr, key_casted_ptr, &tf_cipherTable[0], 0x800, &tmpBuff[0] ); // function09
			memcpy( &tf_cipherTable, &tmpBuff, TFISH_RESET );
			tf_position = 0;
		}

		pOutput[i] = pInput[i] ^ tf_cipherTable[tf_position++];
	}
    return true;
}
