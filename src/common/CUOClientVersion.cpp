#include "CUOClientVersion.h"
#include "CExpression.h"
#include "sphereproto.h"
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
    if (m_revision > 100) {
        factor_minor *= 10;
        factor_major *= 10;
    }
    if (m_minor > 100) {
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

    // Ranges algorithms not yet supported by Apple Clang...
    // const ptrdiff_t count = std::ranges::count(std::string_view(ptcVersion), '.');
    const auto svVersion = std::string_view(ptcVersion);
    const auto count = std::count(svVersion.cbegin(), svVersion.cend(), '_');
    if (count == 2)
        ApplyVersionFromStringOldFormat(ptcVersion);
    else if (count == 3)
        ApplyVersionFromStringOldFormat(ptcVersion);
    else
        ASSERT(false); // Malformed string?
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


void CUOClientVersion::ApplyVersionFromStringOldFormat(lpctstr ptcVersion) noexcept
{
    // Get version of old clients, which report the client version as ASCII string (eg: '5.0.2b')

    byte uiLetter = 0;
    size_t uiMax = strlen(ptcVersion);
    for (size_t i = 0; i < uiMax; ++i)
    {
        if (IsAlpha(ptcVersion[i]))
        {
            uiLetter = uchar(ptcVersion[i] - 'a') + 1u;
            break;
        }
    }

    tchar *piVer[3];
    Str_ParseCmds(const_cast<tchar *>(ptcVersion), piVer, ARRAY_COUNT(piVer), ".");

    // Don't rely on all values reported by client, because it can be easily faked. Injection users can report any
    // client version they want, and some custom clients may also report client version as "Custom" instead X.X.Xy
    if (!piVer[0] || !piVer[1] || !piVer[2] || (uiLetter > 26))
        return;

    m_major = uint(atoi(piVer[0]));
    m_minor = uint(atoi(piVer[1]));
    m_revision = uint(atoi(piVer[2]));
    m_build = uiLetter;
}

void CUOClientVersion::ApplyVersionFromStringNewFormat(lpctstr ptcVersion) noexcept
{
    // Get version of newer clients, which use only 4 numbers separated by dots (example: 6.0.1.1)

    std::string_view sw(ptcVersion);
    const size_t dot1 = sw.find_first_of('.', 0);
    const size_t dot2 = sw.find_first_of('.', dot1 + 1);
    const size_t dot3 = sw.find_first_of('.', dot2 + 1);
    const auto np = std::string_view::npos;
    if (np == dot1 || np == dot2 || np == dot3)
        return;

    const std::string_view sw1(sw.data(), dot1 - 1);
    const std::string_view sw2(sw1.data() + dot1, dot2 - 1);
    const std::string_view sw3(sw2.data() + dot2, dot3 - 1);
    const std::string_view sw4(sw3.data() + dot3);

    uint val1, val2, val3, val4;
    bool ok = true;
    ok = ok && svtonum(sw1, val1);
    ok = ok && svtonum(sw2, val2);
    ok = ok && svtonum(sw3, val3);
    ok = ok && svtonum(sw4, val4);
    if (!ok)
        return;

    m_major = val1;
    m_minor = val2;
    m_revision = val3;
    m_build = val4;
}
