#include "../CUOClientVersion.h"
#include "CCryptoKeyCalc.h"
#include <iomanip>
#include <string_view>
#include <sstream>


/* Static functions (not methods) */

ENCRYPTION_TYPE
static GetEncryptionTypeForClient(CUOClientVersion ver) noexcept
{
    if (ver.m_major == 1)
    {
        if (ver.m_minor >= 23 && ver.m_minor <= 25)
        {
            //if (ver.minor == 25 && ver.revision == 36)
            //    return ENC_LOGIN/INDIVIDUAL;  // 1.25.36 (special multikey encryption, handled differently in-place)
            return ENC_LOGIN; // "old blowfish"?
        }
        if (ver.m_minor == 26)
            return ENC_BFISH;

        //return ENC_QTY; // unknown
    }
    if (ver.m_major == 2)
    {
        if (ver.m_minor == 0 && ver.m_revision == 0 && ver.m_build == 0)
            return (ver.m_extrachar == 'x') ? ENC_BTFISH : ENC_BFISH; // 2.0.0x or 2.0.0
        if (ver.m_minor == 0 && ver.m_revision <= 3)
            return ENC_BTFISH;
    }
    return ENC_BFISH;
}

std::string_view
static EncryptionTypeToString(ENCRYPTION_TYPE enc) noexcept
{
    switch (enc)
    {
        case ENC_NONE:
            return "ENC_NONE";
        case ENC_BFISH:
            return "ENC_BFISH";
        case ENC_BTFISH:
            return "ENC_BTFISH";
        case ENC_TFISH:
            return "ENC_TFISH";
        case ENC_LOGIN:
            return "ENC_LOGIN";
        case ENC_QTY:
        default:
            return "ENC_UNK";
    }
}

/* Methods */

CCryptoClientKey
CCryptoKeyCalc::CalculateLoginKeysReportedVer(CUOClientVersion ver, ENCRYPTION_TYPE forceCryptType) noexcept   // static
{
    uint key1, key2;
    key1 = (ver.m_major << 23) | (ver.m_minor << 14) | (ver.m_revision << 4);
    key1 ^= (ver.m_revision * ver.m_revision) << 9;
    key1 ^= (ver.m_minor * ver.m_minor);
    key1 ^= (ver.m_minor * 11) << 24;
    key1 ^= (ver.m_revision * 7) << 19;
    key1 ^= 0x2C13A5FD;

    key2 = (ver.m_major << 22) | (ver.m_revision << 13) | (ver.m_minor << 3);
    key2 ^= (ver.m_revision * ver.m_revision * 3) << 10;
    key2 ^= (ver.m_minor * ver.m_minor);
    key2 ^= (ver.m_minor * 13) << 23;
    key2 ^= (ver.m_revision * 7) << 18;
    key2 ^= 0xA31D527F;

    return CCryptoClientKey{
        .m_client = ver.GetLegacyVersionNumber(),
        .m_key_1 = key1,
        .m_key_2 = key2,
        .m_EncType = ((forceCryptType <= ENC_NONE) || (forceCryptType >= ENC_QTY)) ? GetEncryptionTypeForClient(ver) : forceCryptType
    };
}

CCryptoClientKey
CCryptoKeyCalc::CalculateLoginKeys(CUOClientVersion ver, GAMECLIENT_TYPE cliType, ENCRYPTION_TYPE forceCryptType) noexcept   // static
{
    if (cliType == CLIENTTYPE_EC)
        ver.m_major += CUOClientVersion::kuiECMajorVerOffset;
    return CalculateLoginKeysReportedVer(ver, forceCryptType);
}

std::string
CCryptoKeyCalc::FormattedLoginKey(CUOClientVersion ver, GAMECLIENT_TYPE cliType, CCryptoClientKey cryptoKey) // static
{
    // Outputs a line ready to be inserted in SphereCrypt.ini
    // Accepts standard client version (the one shown in the client), not the reported client version (the one the client sends in a packet to the server).
    // We do here the conversion to standard-type to reported-type.
    std::stringstream ss;
    ss << std::setfill('0') << std::left;
    // std::setw sets the minimum field length
    if (cliType == CLIENTTYPE_EC)
    {
        ss  << std::setw(2) << ver.m_major + CUOClientVersion::kuiECMajorVerOffset
            << std::setw(2) << ver.m_minor
            << std::setw(2) << ver.m_revision
            << std::setw(2) << 0; //ver.m_build;
    }
    else
    {
        ss  << std::setw(1) << ver.m_major
            << std::setw(2) << ver.m_minor
            << std::setw(2) << ver.m_revision
            << std::setw(2) << 0; //ver.m_build;
    }

    ss << ' ';
    ss << std::uppercase << std::hex
        << '0' << std::setw(8) << cryptoKey.m_key_1 /* hi */ << ' '
        << '0' << std::setw(8) << cryptoKey.m_key_2 /* lo */;

    ss << ' ';
    ss << EncryptionTypeToString(cryptoKey.m_EncType);

    ss << " // ";
    ss << ver.GetVersionString().c_str();

    return ss.str();
}

