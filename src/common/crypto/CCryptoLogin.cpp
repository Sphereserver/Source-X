#include "../../sphere/threads.h"
#include "CCrypto.h"
#include <cstring> // for memcpy

// Encryption used when logging in, to access the server list
bool CCrypto::DecryptLogin( byte * pOutput, const byte * pInput, size_t outLen, size_t inLen )
{
	ADDTOCALLSTACK("CCrypto::DecryptLogin");
    // Algorithm: a kind of rotation cipher

    const dword dwCliVer = GetClientVer();
	if ( dwCliVer >= 1253700u )
	{
		for ( size_t i = 0; i < inLen; ++i )
		{
            if (i >= outLen)
                return false; // error: i'm trying to write more bytes than the output buffer length
			pOutput[i] = pInput[i] ^ (byte) m_CryptMaskLo;
			dword MaskLo = m_CryptMaskLo;
			dword MaskHi = m_CryptMaskHi;
			m_CryptMaskLo = ((MaskLo >> 1) | (MaskHi << 31)) ^ m_MasterLo;
			MaskHi = ((MaskHi >> 1) | (MaskLo << 31)) ^ m_MasterHi;
			m_CryptMaskHi = ((MaskHi >> 1) | (MaskLo << 31)) ^ m_MasterHi;
		}
		return true;
	}
    else if ( dwCliVer == 1253600u )
	{
        // Special multi key.
        /*
        #define CLIKEY_12536_HI1 0x387fc5cc
        #define CLIKEY_12536_HI2 0x35ce9581
        #define CLIKEY_12536_HI3 0x07afcc37
        #define CLIKEY_12536_LO1 0x021510c6
        #define CLIKEY_12536_LO2 0x4c3a1353
        #define CLIKEY_12536_LO3 0x16ef783f
        */
		for ( size_t i = 0; i < inLen; ++i )
		{
            if (i >= outLen)
                return false; // error: i'm trying to write more bytes than the output buffer length
			pOutput[i] = pInput[i] ^ (byte) m_CryptMaskLo;
			dword MaskLo = m_CryptMaskLo;
			dword MaskHi = m_CryptMaskHi;

			// FIXME: are these formulas right? because shifting by a number that can be max 0xFF (255) can return a huge number...
			//	and dword isn't obviously big enough, so it causes undefined behavior.
			m_CryptMaskHi =
				(m_MasterHi >> ((5 * MaskHi * MaskHi) & 0xff))
				+ (MaskHi * m_MasterHi)
				+ (MaskLo * MaskLo * 0x35ce9581)
				+ 0x07afcc37;
			m_CryptMaskLo =
				(m_MasterLo >> ((3 * MaskLo * MaskLo) & 0xff))
				+ (MaskLo * m_MasterLo)
				- (m_CryptMaskHi * m_CryptMaskHi * 0x4c3a1353)
				+ 0x16ef783f;
		}
		return true;
	}
    else if ( dwCliVer ) // CLIENT_VER <= 1253500
	{
		for ( size_t i = 0; i < inLen; ++i )
		{
            if (i >= outLen)
                return false; // error: i'm trying to write more bytes than the output buffer length
			pOutput[i] = pInput[i] ^ (byte) m_CryptMaskLo;
			dword MaskLo = m_CryptMaskLo;
			dword MaskHi = m_CryptMaskHi;
			m_CryptMaskLo = ((MaskLo >> 1) | (MaskHi << 31)) ^ m_MasterLo;
			m_CryptMaskHi = ((MaskHi >> 1) | (MaskLo << 31)) ^ m_MasterHi;
		}
		return true;
	}
    else if (pOutput != pInput)   // no crypt
    {
        if (inLen >= outLen)
            return false; // error: i'm trying to write more bytes than the output buffer length
        memcpy( pOutput, pInput, inLen );
    }

    return true;
}
