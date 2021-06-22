#include "../../sphere/threads.h"
#include "CCrypto.h"
#include "CMD5.h"


void CCrypto::InitMD5(byte * ucInitialize)
{
	ADDTOCALLSTACK("CCrypto::InitMD5");
	m_md5_position = 0;

	m_md5_engine->update( ucInitialize, TFISH_RESET );
	m_md5_engine->finalize();
	m_md5_engine->numericDigest( &m_md5_digest[0] );
}

bool CCrypto::EncryptMD5( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen )
{
	ADDTOCALLSTACK("CCrypto::EncryptMD5");

	for (size_t i = 0; i < inLen; ++i)
	{
        if (i >= outLen)
            return false; // error: i'm trying to write more bytes than the output buffer length
		pOutput[i] = pInput[i] ^ m_md5_digest[m_md5_position++];
		m_md5_position &= MD5_RESET;
	}
    return true;
}
