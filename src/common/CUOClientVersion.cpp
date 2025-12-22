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

    if (m_build >= 100)
    {
        factor_revision *= 10;
        factor_minor *= 10;
        factor_major *= 10;
    }
    if (m_revision >= 100)
    {
        factor_minor *= 10;
        factor_major *= 10;
    }
    if (m_minor >= 100)
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
    if (m_major > 1000 || m_minor > 1000 || m_revision > 1000 || m_build > 1000 || m_build_sub > 1000)
        return {};  // Invalid at best, a malicious attempt of writing past the buffer at worst.

    std::string ret;
    ret.resize(20);

    if (   *this >= CUOClientVersionConstants::kMinCliver_ModernVersioning
        || *this <= CUOClientVersionConstants::kMinCliver_LetterVersioning)
    {
        snprintf(ret.data(), ret.capacity(), "%u.%u.%u.%u", m_major, m_minor, m_revision, m_build);
    }
    else
    {
        // snprintf returns the number of characters that would have been written had n been sufficiently large,
        //  not counting the terminating null character, or a negative value if an encoding error occurred
        int iWrittenChars = snprintf(ret.data(), ret.capacity(), "%u.%u.%u", m_major, m_minor, m_revision);
        if (iWrittenChars < 0)
            return {};  // ?!

        if (m_build)
        {
            ret[iWrittenChars] = uchar(m_build) + uchar('a' - 1); // 'a' = 'patch' 1
            ++iWrittenChars;
        }
        if (m_build_sub)
        {
            // Append the number to the string buffer
            Str_FromUI(m_build_sub, &ret[iWrittenChars], ret.capacity() - (size_t)iWrittenChars - 1);
        }
        // ret[iVer] = '\0'; // resize already zeroes everything
    }

    return ret;
}


CUOClientVersion::CUOClientVersion(dword uiClientVersionNumber) noexcept :
    m_build_sub(kuiBuildSubCatchAllVal)
    // build_sub isn't encoded in the version number, so use a special value to inform
    //  that we got this CUOClientVersion from a ver number.
{
    // launch some tests for this function
    if (uiClientVersionNumber >= CUOClientVersionConstants::kMinCliver_ModernVersioning.GetLegacyVersionNumber()) //MINCLIVER_NEWVERSIONING)
    {
        // We should really ditch this number, giving the difficulty to parse (imagine that in scripts...).
        //  We should just keep backwards compatibility with CLIVER/CLIVERREPORTED and provide just the string
        //  Maybe CLIVERSTR/CLIVERREPORTEDSTR ?
        if (uiClientVersionNumber > 10'00'000'00)
        {
            // Two extra digits: it's a EC (one more digit used for the bigger major version) with a greater rev number (like 4.00.101.00)
            m_major = uiClientVersionNumber / 100'000'00;
            m_minor = (uiClientVersionNumber / 1000'00) % 100;
            m_revision = (uiClientVersionNumber / 100 ) % 1000;

        }
        else if (uiClientVersionNumber > 1'00'000'00)
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

CUOClientVersion::CUOClientVersion(lpctstr ptcVersion, bool fEnhancedClient) noexcept :
    m_major(0), m_minor(0), m_revision(0), m_build(0), m_build_sub(0)
{
    if ((ptcVersion == nullptr) || (*ptcVersion == '\0'))
        return;

    tchar ptcVersionBuf[64];
    Str_CopyLimitNull(ptcVersionBuf, ptcVersion, sizeof(ptcVersionBuf));

    // Ranges algorithms not yet supported by Apple Clang...
    // const size_t count = std::ranges::count(std::string_view(ptcVersion), '.');
    const auto svVersion = std::string_view(ptcVersion);
    const auto count = std::count(svVersion.cbegin(), svVersion.cend(), '.');
    if (count == 2)
    {
        if (fEnhancedClient)
        {
            g_Log.Event(LOGL_CRIT|LOGM_CLIENTS_LOG|LOGM_NOCONTEXT, "Wrong string passed to CUOClientVersion: Enhanced Client version but with old format?.\n");
            return;
        }
        ApplyVersionFromStringOldFormat(ptcVersionBuf);
    }
    else if (count == 3)
    {
        ApplyVersionFromStringNewFormat(ptcVersionBuf, fEnhancedClient);
    }
    else
    {
        // Malformed string?
        g_Log.Event(LOGL_CRIT|LOGM_CLIENTS_LOG|LOGM_NOCONTEXT, "Received malformed client version string?.\n");
    }
}


bool CUOClientVersion::operator ==(CUOClientVersion const& other) const noexcept
{
    const bool fSameVerNum = (m_major == other.m_major) && (m_minor == other.m_minor) && (m_revision == other.m_revision) && (m_build == other.m_build);

    // Ignore this comparison is m_build_sub is kuiBuildSubCatchAllVal
    // (so, if one of the two CUOClientVersion was calculated from a version number, which doesn't carry this information)
    const bool fSameBuildSub =
        ((m_build_sub == kuiBuildSubCatchAllVal) || (other.m_build_sub == kuiBuildSubCatchAllVal))
            ? true : (m_build_sub == other.m_build_sub);

    return fSameVerNum && fSameBuildSub;
}
bool CUOClientVersion::operator > (CUOClientVersion const& other) const noexcept
{
    if (m_major > other.m_major)
        return true;

    if (m_major < other.m_major)
        return false;
    if (m_minor > other.m_minor)
        return true;

    if (m_minor < other.m_minor)
        return false;
    if (m_revision > other.m_revision)
        return true;

    if (m_revision < other.m_revision)
        return false;
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

    const size_t uiMax = strnlen(ptcVersion, 20);
    size_t uiLetterPos = 0;
    byte uiLetter = 0;
    for (; uiLetterPos < uiMax; ++uiLetterPos)
    {
        if (IsAlpha(ptcVersion[uiLetterPos]))
        {
            uiLetter = uchar(ptcVersion[uiLetterPos] - 'a') + 1u;
            break;
        }
    }

    tchar *piVer[3]{};
    lptstr ptcVersionParsed = ptcVersion;
    Str_ParseCmds(ptcVersionParsed, piVer, ARRAY_COUNT(piVer), ".");

    // Don't rely on all values reported by client, because it can be easily faked. Injection users can report any
    // client version they want, and some custom clients may also report client version as "Custom" instead X.X.Xy
    if (!piVer[0] || !piVer[1] || !piVer[2] || (uiLetter > 26))
    {
        g_Log.EventDebug("Invalid version string '%s' passed to CUOClientVersion::ApplyVersionFromStringOldFormat.\n", ptcVersion);
        return;
    }

    m_major = uint(atoi(piVer[0]));
    m_minor = uint(atoi(piVer[1]));
    m_revision = uint(atoi(piVer[2]));
    m_build = uiLetter;

    ++ uiLetterPos;
    if (ptcVersion[uiLetterPos] != '\0')
        m_build_sub = uint(atoi(ptcVersion + uiLetterPos));
}

void CUOClientVersion::ApplyVersionFromStringNewFormat(lptstr ptcVersion, bool fEnhancedClient) noexcept
{
    // Get version of newer clients, which use only 4 numbers separated by dots (example: 6.0.1.1)

    constexpr auto np = std::string_view::npos;
    const std::string_view sv(ptcVersion);

    const size_t dot1 = sv.find_first_of('.', 0);
    if (dot1 == np)
    {
    ret_err:
        g_Log.EventDebug("Invalid version string '%s' passed to CUOClientVersion::ApplyVersionFromStringNewFormat.\n", ptcVersion);
        return;
    }

    const size_t dot2 = sv.find_first_of('.', dot1 + 1);
    if (dot2 == np)
        goto ret_err;

    const size_t dot3 = sv.find_first_of('.', dot2 + 1);
    if (dot3 == np)
        goto ret_err;

    const std::string_view sv1(sv.data(), dot1);
    const std::string_view sv2(sv.data() + dot1 + 1, dot2 - dot1 - 1);
    const std::string_view sv3(sv.data() + dot2 + 1, dot3 - dot2 - 1);
    const std::string_view sv4(sv.data() + dot3 + 1);

    bool ok = true;
    try
    {
        std::optional<uint> val1, val2, val3, val4;
        ok = ok && (val1 = Str_ToU(sv1.data(), 10, sv1.length(), false)).has_value();
        ok = ok && (val2 = Str_ToU(sv2.data(), 10, sv2.length(), false)).has_value();
        ok = ok && (val3 = Str_ToU(sv3.data(), 10, sv3.length(), false)).has_value();
        ok = ok && (val4 = Str_ToU(sv4.data(), 10, sv4.length(), false)).has_value();
        if (!ok)
            return;

        m_major     = val1.value();
        m_minor     = val2.value();
        m_revision  = val3.value();
        m_build     = val4.value();

        if ((m_major < kuiECMajorVerOffset) && fEnhancedClient)
            m_major += kuiECMajorVerOffset;
    }
    catch (std::bad_optional_access const&)
    {
        // Shouldn't really happen...
        m_major = m_minor = m_revision = m_build = 0;
        DEBUG_MSG(("std::bad_optional_access at CUOClientVersion::ApplyVersionFromStringNewFormat.\n"));
    }
}
