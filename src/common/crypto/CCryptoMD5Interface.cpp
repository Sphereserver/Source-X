#include "../../sphere/threads.h"
#include "CCrypto.h"
#include "CMD5.h"


void CCrypto::InitMD5(byte * ucInitialize)
{
	ADDTOCALLSTACK("CCrypto::InitMD5");
	md5_position = 0;

	md5_engine->update( ucInitialize, TFISH_RESET );
	md5_engine->finalize();
	md5_engine->numericDigest( &md5_digest[0] );
}

void CCrypto::EncryptMD5( byte * pOutput, const byte * pInput, size_t iLen )
{
	ADDTOCALLSTACK("CCrypto::EncryptMD5");
	for (size_t i = 0; i < iLen; i++)
	{
		pOutput[i] = pInput[i] ^ md5_digest[md5_position++];
		md5_position &= MD5_RESET;
	}
}
