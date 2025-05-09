#include "CUOClientVersion.h"
#include "CLog.h"
#include "sphere_library/sstring.h"
#include <algorithm>
#include <string>
#include <string_view>


bool CUOClientVersion::Valid() const noexcept
{
    return (m_major != 0);
}

uint CUOClientVersion::GetLegacyVersionNumber() const noexcept
{
    // Get a number built to represent the client version (this should be discontinued since the versions got too big and > 100).
    //uint factor_build       = 1;
    uint factor_revision    = 100;
    uint factor_minor       = 100'00;
    uint factor_major       = 100'00'00;
    if (m_revision > 100)
    {
        factor_minor *= 10;
        factor_major *= 10;
    }
    if (m_minor > 100)
    {
        factor_major *= 10;
    }

    const uint retval =
        (m_major * factor_major) +
        (m_minor * factor_minor) +
        (m_revision * factor_revision) +
        m_build;
    return retval;
}

std::string CUOClientVersion::GetVersionString() const noexcept
{
    std::string ret;
    ret.reserve(30);

    if (*this >= CUOClientVersionConstants::kMinCliver_NewVersioning)
    {
        snprintf(ret.data(), ret.capacity(), "%u.%u.%u.%u", m_major, m_minor, m_revision, m_build);
    }
    else
    {
        const int iVer = snprintf(ret.data(), ret.capacity(), "%u.%u.%u", m_major, m_minor, m_revision);
        if (m_revision)
        {
            if (m_revision == 0)
                ret[iVer] = '0';
            else
                ret[iVer] = uchar(m_revision) + uchar('a' - 1); // 'a' = revision 1
            ret[iVer + 1] = '\0';
        }
    }

    return ret;
}


CUOClientVersion::CUOClientVersion(dword uiClientVersionNumber) noexcept :
    m_extrachar(0)
{
    // launch some tests for this function
    if (uiClientVersionNumber >= CUOClientVersionConstants::kMinCliver_NewVersioning.GetLegacyVersionNumber()) //MINCLIVER_NEWVERSIONING)
    {
        // We should really ditch this number, giving the difficulty to parse (imagine that in scripts...).
        //  We should just keep backwards compatibility with CLIVER/CLIVERREPORTED and provide just the string
        //  Maybe CLIVERSTR/CLIVERREPORTEDSTR ?
        if (uiClientVersionNumber > 100'00'000'00)
        {
            // Two extra digits: it's a EC (one more digit used for the bigger major version) with a greater rev number (like 4.00.101.00)
            m_major = uiClientVersionNumber / 100'000'00;
            m_minor = (uiClientVersionNumber / 1000'00) % 100;
            m_revision = (uiClientVersionNumber / 100 ) % 1000;

        }
        else if (uiClientVersionNumber > 10'00'000'00)
        {
            // The extra digit can be used for a bigger major (EC, like 67.01.00.00) or rev number (CC, like 7.101.00.00)
            const bool fEC = ((uiClientVersionNumber / 1000'00) < kuiECMajorVerOffset);
            if (fEC)
            {
                m_major = uiClientVersionNumber / 100'00'00;
                m_minor = (uiClientVersionNumber / 100'00) % 100;
                m_revision = (uiClientVersionNumber % 100'00) / 100;
            }
            else
            {
                m_major = uiClientVersionNumber / 100'000'00;
                m_minor = (uiClientVersionNumber / 1000'00) % 100;
                m_revision = (uiClientVersionNumber / 100) % 1000;
            }
        }
        else
        {
            m_major = uiClientVersionNumber / 100'00'00;
            m_minor = (uiClientVersionNumber / 100'00) % 100;
            m_revision = (uiClientVersionNumber / 100) % 100;
        }
        m_build = uiClientVersionNumber % 100;
    }
    else
    {
        m_major = uiClientVersionNumber / 100'00'00;
        m_minor = (uiClientVersionNumber / 100'00) % 100;
        m_revision = (uiClientVersionNumber % 100'00) / 100;
        m_build = uiClientVersionNumber % 100;
    }
}

CUOClientVersion::CUOClientVersion(lpctstr ptcVersion) noexcept :
    m_major(0), m_minor(0), m_revision(0), m_build(0), m_extrachar(0)
{
    if ((ptcVersion == nullptr) || (*ptcVersion == '\0'))
        return;

    tchar ptcVersionBuf[64];
    Str_CopyLimitNull(ptcVersionBuf, ptcVersion, sizeof(ptcVersionBuf));

    // Ranges algorithms not yet supported by Apple Clang...
    // const ptrdiff_t count = std::ranges::count(std::string_view(ptcVersion), '.');
    const auto svVersion = std::string_view(ptcVersion);
    const auto count = std::count(svVersion.cbegin(), svVersion.cend(), '.');
    if (count == 2)
        ApplyVersionFromStringOldFormat(ptcVersionBuf);
    else if (count == 3)
        ApplyVersionFromStringNewFormat(ptcVersionBuf);
    else
    {
        // Malformed string?
        g_Log.Event(LOGL_CRIT|LOGM_CLIENTS_LOG|LOGM_NOCONTEXT, "Received malformed client version string?.\n");
    }
}


bool CUOClientVersion::operator ==(CUOClientVersion const& other) const noexcept
{
    //return (0 == memcmp(this, &other, sizeof(CUOClientVersion)));
    const bool fSameVerNum = (m_major == other.m_major) && (m_minor == other.m_minor) && (m_revision == other.m_revision) && (m_build == other.m_build);
    return fSameVerNum && (m_extrachar == other.m_extrachar);
}
bool CUOClientVersion::operator > (CUOClientVersion const& other) const noexcept
{
    if (m_major > other.m_major)
        return true;
    if (m_minor > other.m_minor)
        return true;
    if (m_revision > other.m_revision)
        return true;
    if (m_build > other.m_build)
        return true;
    return false;
}
bool CUOClientVersion::operator >=(CUOClientVersion const& other) const noexcept
{
    return operator>(other) || operator==(other);
}
bool CUOClientVersion::operator < (CUOClientVersion const& other) const noexcept
{
    return other >= *this;
}
bool CUOClientVersion::operator <=(CUOClientVersion const& other) const noexcept
{
    return other > *this;
}


void CUOClientVersion::ApplyVersionFromStringOldFormat(lptstr ptcVersion) noexcept
{
    // Get version of old clients, which report the client version as ASCII string (eg: '5.0.2b')

    byte uiLetter = 0;
    const size_t uiMax = strnlen(ptcVersion, 20);
    for (size_t i = 0; i < uiMax; ++i)
    {
        if (IsAlpha(ptcVersion[i]))
        {
            uiLetter = uchar(ptcVersion[i] - 'a') + 1u;
            break;
        }
    }

    tchar *piVer[3];
    Str_ParseCmds(ptcVersion, piVer, ARRAY_COUNT(piVer), ".");

    // Don't rely on all values reported by client, because it can be easily faked. Injection users can report any
    // client version they want, and some custom clients may also report client version as "Custom" instead X.X.Xy
    if (!piVer[0] || !piVer[1] || !piVer[2] || (uiLetter > 26))
        return;

    m_major = uint(atoi(piVer[0]));
    m_minor = uint(atoi(piVer[1]));
    m_revision = uint(atoi(piVer[2]));
    m_build = uiLetter;
}

void CUOClientVersion::ApplyVersionFromStringNewFormat(lptstr ptcVersion) noexcept
{
    // Get version of newer clients, which use only 4 numbers separated by dots (example: 6.0.1.1)

    const std::string_view sv(ptcVersion);
    const size_t dot1 = sv.find_first_of('.', 0);
    const size_t dot2 = sv.find_first_of('.', dot1 + 1);
    const size_t dot3 = sv.find_first_of('.', dot2 + 1);
    constexpr auto np = std::string_view::npos;
    if (np == dot1 || np == dot2 || np == dot3)
        return;

    const std::string_view sv1(sv.data(), dot1 - 1);
    const std::string_view sv2(sv1.data() + dot1, dot2 - 1);
    const std::string_view sv3(sv2.data() + dot2, dot3 - 1);
    const std::string_view sv4(sv3.data() + dot3);

    std::optional<uint> val1, val2, val3, val4;
    bool ok = true;
    ok = ok && (val1 = Str_ToU(sv1.data(), 10, true)).has_value();
    ok = ok && (val2 = Str_ToU(sv1.data(), 10, true)).has_value();
    ok = ok && (val3 = Str_ToU(sv1.data(), 10, true)).has_value();
    ok = ok && (val4 = Str_ToU(sv1.data(), 10, true)).has_value();
    if (!ok)
        return;

    m_major     = val1.value();
    m_minor     = val2.value();
    m_revision  = val3.value();
    m_build     = val4.value();
}
