#include <doctest/doctest.h>
#include "../../src/common/CUOClientVersion.h"
//#include "../../src/common/sphereproto.h"
#include <stdlib.h> // atoi
#include <sstream>

namespace { namespace ref_impl
{
#ifdef _WIN32
#   define FMTdword     "lu"	// Windows uses '%lu' to format dec dword (unsigned long)
#   define FMTdwordH	"lx"	// Windows uses '%lx' to format hex dword (unsigned long)
#else
#   define FMTdword     "u"		// Linux uses '%u' to format dec dword (unsigned long)
#   define FMTdwordH	"x"		// Linux uses '%x' to format hex dword (unsigned int)
#endif

    // From 0.56d
    dword CCrypt_GetVerFromString(lpctstr pszVer)
    {
        // Get version of old clients, which report the client version as ASCII string (eg: '5.0.2b')
        if ( (pszVer == nullptr) || (*pszVer == '\0') )
            return 0;

        byte bLetter = 0;
        for ( size_t i = 0; i < strlen(pszVer); ++i )
        {
            if ( IsAlpha(pszVer[i]) )
            {
                bLetter = (pszVer[i] - 'a') + 1;
                break;
            }
        }

        tchar *ppVer[3];
        Str_ParseCmds(const_cast<tchar *>(pszVer), ppVer, ARRAY_COUNT(ppVer), ".");

        // Don't rely on all values reported by client, because it can be easily faked. Injection users can report any
        // client version they want, and some custom clients may also report client version as "Custom" instead X.X.Xy
        if ( !ppVer[0] || !ppVer[1] || !ppVer[2] || (bLetter > 26) )
            return 0;

        return (atoi(ppVer[0]) * 1000000) + (atoi(ppVer[1]) * 10000) + (atoi(ppVer[2]) * 100) + bLetter;
    }

    // From 0.56d
    dword CCrypt_GetVerFromNumber(dword dwMajor, dword dwMinor, dword dwRevision, dword dwPatch)
    {
        // Get version of new clients (5.0.6.5+), which report the client version as numbers (eg: 5,0,6,5)

        return (dwMajor * 1000000) + (dwMinor * 10000) + (dwRevision * 100) + dwPatch;
    }

    // From 0.56d
    tchar *CCrypt_WriteClientVerString(dword dwVer, tchar *pszOutput)
    {
        if ( dwVer >= MINCLIVER_NEWVERSIONING )
        {
            snprintf(pszOutput, 44, "%" FMTdword ".%" FMTdword ".%" FMTdword ".%" FMTdword, dwVer / 1000000, (dwVer / 10000) % 100, (dwVer % 10000) / 100, dwVer % 100);
        }
        else
        {
            int iVer = snprintf(pszOutput, 33, "%" FMTdword ".%" FMTdword ".%" FMTdword, dwVer / 1000000, (dwVer / 10000) % 100, (dwVer % 10000) / 100);
            int iPatch = static_cast<int>(dwVer % 100);
            if ( iPatch )
            {
                pszOutput[iVer++] = static_cast<tchar>(iPatch + 'a' - 1);
                pszOutput[iVer] = '\0';
            }
        }
        return pszOutput;
    }

}}


TEST_CASE("CUOClientVersion parsing")\
// clazy:exclude=non-pod-global-static
{
    constexpr std::pair<const char *, CUOClientVersion> data_pair_str_old_format[]
        {
            {"1.0.1.0",    CUOClientVersion(1u, 0u, 1u, 0u)},       // Ancient versioning, no letters.
            {"2.0.0a",     CUOClientVersion(2u, 0u, 0u, 'a')},
            {"2.0.0x",     CUOClientVersion(2u, 0u, 0u, 'x')},
            {"2.0.1j",     CUOClientVersion(2u, 0u, 1u, 'j')},
            {"2.0.8z",     CUOClientVersion(2u, 0u, 8u, 'z')},
            {"4.0.4b2",    CUOClientVersion(4u, 0u, 4u, 'b', 2)},
            {"5.0.2b",     CUOClientVersion(5u, 0u, 2u, 'b')}
        };
    constexpr std::pair<const char *, CUOClientVersion> data_pair_str_new_format[]
        {
            {"6.0.0.0",    CUOClientVersion(6u, 0u, 0u, 0u)},
            {"7.1.2.34",   CUOClientVersion(7u, 1u, 2u, 34u)},
            {"7.12.3.4",   CUOClientVersion(7u, 12u, 3u, 4u)},
            {"7.1.23.4",   CUOClientVersion(7u, 1u, 23u, 4u)},
            {"7.1.2.34",   CUOClientVersion(7u, 1u, 2u, 34u)},
            {"7.12.34.5",  CUOClientVersion(7u, 12u, 34u, 5u)},
            {"7.12.34.56", CUOClientVersion(7u, 12u, 34u, 56u)},
            {"7.100.0.0",  CUOClientVersion(7u, 100u, 0u, 0u)},
            {"7.100.0.1",  CUOClientVersion(7u, 100u, 0u, 1u)},
            {"7.101.1.5",  CUOClientVersion(7u, 101u, 1u, 5u)}
        };
    constexpr std::pair<const char *, CUOClientVersion> data_pair_str_enhanced_client[]
        {
            {"67.1.0.0",   CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 1u, 0u, 0u)},
            {"67.5.0.5",   CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 5u, 0u, 5u)},
            {"67.100.0.0", CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 100u, 0u, 0u)},
            {"67.101.1.5", CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 101u, 1u, 5u)}
        };
    constexpr std::pair<uint, CUOClientVersion> data_pair_num[]
        {
            {1253500,       CUOClientVersion(1u, 25u, 35u, 0u)},    // Ancient versioning, no letters.
            {2000000,       CUOClientVersion(2u, 0u, 0u, 0u, 'x')}, // build_sub
            {3000702,       CUOClientVersion(3u, 0u, 7u, 'b')},     // Letter versioning.
            {3000810,       CUOClientVersion(3u, 0u, 8u, 'j')},
            {5000000,       CUOClientVersion(5u, 0u, 0u, 0u)},
            {5000605,       CUOClientVersion(5u, 0u, 6u, 'e')},
            {5000700,       CUOClientVersion(5u, 0u, 7u, 0u)},
            {7004565,       CUOClientVersion(7u, 0u, 45u, 65u)},
            {70010200,      CUOClientVersion(7u, 0u, 102u, 0u)}
    };
    constexpr std::pair<uint, CUOClientVersion> data_pair_num_enhanced_client[]
        {
            {67000900,      CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 0u, 9u, 0u)},
            {67009900,      CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 0u, 99u, 0u)},
            {670010000,     CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 0u, 100u, 0u)},
            {670110800,     CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 1u, 108u, 0u)},
            {670110801,     CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 1u, 108u, 1u)},
            {670110810,     CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 1u, 108u, 10u)}
        };

    SUBCASE("Read from string, old format")
    {
        for (auto const& elem : data_pair_str_old_format)
        {
            INFO("Test: " << std::string_view(elem.first) << " -> " << elem.second.GetVersionString());
            CUOClientVersion cv(elem.first);
            CHECK_EQ(cv, elem.second);
            //CHECK_EQ(ref_impl::CCrypt_GetVerFromString(elem.first), elem.second);
        }
    }

    SUBCASE("Read from string, new format")
    {
        for (auto const& elem : data_pair_str_new_format)
        {
            CUOClientVersion cv(elem.first);
            CHECK_EQ(cv, elem.second);
        }
    }

    SUBCASE("Read from string, Enhanced Client")
    {
        for (auto const& elem : data_pair_str_enhanced_client)
        {
            CUOClientVersion cv(elem.first);
            CHECK_EQ(cv, elem.second);
        }
    }

    SUBCASE("Read from number, all formats")
    {
        for (auto const& elem : data_pair_num)
        {
            CUOClientVersion cv(elem.first);
            CHECK_EQ(cv, elem.second);
        }
    }

    SUBCASE("Write as string, old format")
    {
        std::string strVer;
        for (auto const& elem : data_pair_str_old_format)
        {
            strVer = elem.second.GetVersionString();
            CHECK(!strcmp(strVer.c_str(), elem.first));
        }
    }

    SUBCASE("Write as string, new format")
    {
        std::string strVer;
        for (auto const& elem : data_pair_str_new_format)
        {
            strVer = elem.second.GetVersionString();
            CHECK(!strcmp(strVer.c_str(), elem.first));
        }
    }

    SUBCASE("Write as string, Enhanced Client")
    {
        std::string strVer;
        for (auto const& elem : data_pair_str_enhanced_client)
        {
            strVer = elem.second.GetVersionString();
            CHECK(!strcmp(strVer.c_str(), elem.first));
        }
    }

    SUBCASE("Write as number, all formats")
    {
        for (auto const& elem : data_pair_num)
        {
            uint ver = elem.second.GetLegacyVersionNumber();
            CHECK_EQ(ver, elem.first);

            // Doesn't work for revision > 100
            //uint ver_ref_impl = ref_impl::CCrypt_GetVerFromNumber(elem.second.m_major, elem.second.m_minor, elem.second.m_revision, elem.second.m_build);
            //CHECK_EQ(ver, ver_ref_impl);
        }
    }

    SUBCASE("Write as number, Enhanced Client")
    {
        for (auto const& elem : data_pair_num_enhanced_client)
        {
            uint ver = elem.second.GetLegacyVersionNumber();
            CHECK_EQ(ver, elem.first);
        }
    }
}
