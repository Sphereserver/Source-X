/*
* @file CUOClientVersion.h
* @brief Utilities to manage UO client version strings and numbers.
*/

#ifndef _INC_CUOCLIENTVERSION_H
#define _INC_CUOCLIENTVERSION_H

#include "common.h"


struct CUOClientVersion
{
    static constexpr uint kuiECMajorVerOffset = 63u;

    /*  Members. */
    uint m_major, m_minor, m_revision, m_build /* build or patch, different names for the same thing */;
    char m_extrachar; // like 2.0.0[x] or 4.0.4b[2]

    /*  Methods. */
    bool Valid() const noexcept;
    uint GetLegacyVersionNumber() const noexcept;
    std::string GetVersionString() const noexcept;

    /*  Constructors. */
    // Pass the numbers as if those were reported by the client and stored in ReportedCliVer!
    // Pass the values with the offsets already applied! (like kuiECMajorVerOffset).
    // This explicit constructor has to be used for new version formats (without the trailing letter, thus clients >= 5.0.6.5, equivalent to > 5.0.6e),
    //  or old versions formats without a letter as build number (it means that build number is 0, because build 'a' is 1).
    explicit constexpr
    CUOClientVersion(const uint major, const uint minor, const uint revision, const uint build, const char extrachar = 0) noexcept :
        m_major(major), m_minor(minor), m_revision(revision), m_build(build), m_extrachar(extrachar)
    {}

    // Equivalent to ReportedCliVer! Offsets already applied! (like kuiECMajorVerOffset).
    // This explicit constructor has to be used for old version formats (with the trailing letter, thus clients < 5.0.6.5, equivalent to <= 5.0.6e),
    //  if this version has a letter in place of a build number.
    explicit constexpr
    CUOClientVersion(const uint major, const uint minor, const uint revision, const char build, const char extrachar = 0) noexcept :
        m_major(major), m_minor(minor), m_revision(revision), m_build(build - 'a'), m_extrachar(extrachar)
    {}

    CUOClientVersion(dword uiClientVersionNumber) noexcept;
    CUOClientVersion(lpctstr ptcVersionString) noexcept;

    /*  Operators. */
    bool operator ==(CUOClientVersion const& other) const noexcept;
    bool operator > (CUOClientVersion const& other) const noexcept;
    bool operator >=(CUOClientVersion const& other) const noexcept;
    bool operator < (CUOClientVersion const& other) const noexcept;
    bool operator <=(CUOClientVersion const& other) const noexcept;

    /*  Private methods. */
private:
    void ApplyVersionFromStringOldFormat(lpctstr ptcVersion) noexcept;
    void ApplyVersionFromStringNewFormat(lpctstr ptcVersion) noexcept;
};


/* Client Versions constants, converted from sphereproto.h to use CUOClientVersion */
struct CUOClientVersionConstants
{
#define SCDECL static constexpr auto
    /* Client versions (expansions) */
    SCDECL kMinCliver_LBR    = CUOClientVersion(3u, 0u, 7u, 'b');   // minimum client to activate LBR packets (3.0.7b), old vernum 3000702
    SCDECL kMinCliver_AOS    = CUOClientVersion(4u, 0u, 5u, 0u);    // minimum client to activate AOS packets (4.0.0a), old vernum 4000000
    SCDECL kMinCliver_SE     = CUOClientVersion(4u, 0u, 0u, 0u);    // minimum client to activate SE packets (4.0.5a), old vernum 4000500
    SCDECL kMinCliver_ML     = CUOClientVersion(5u, 0u, 0u, 0u);    // minimum client to activate ML packets (5.0.0a), old vernum 5000000
    SCDECL kMinCliver_SA     = CUOClientVersion(7u, 0u, 0u, 0u);    // minimum client to activate SA packets (7.0.0.0), old vernum 7000000
    SCDECL kMinCliver_HS     = CUOClientVersion(7u, 0u, 9u, 0u);    // minimum client to activate HS packets (7.0.9.0), old vernum 7000900
    SCDECL kMinCliver_TOL    = CUOClientVersion(7u, 0u, 45u, 65u);  // minimum client to activate TOL packets (7.0.45.65), old vernum 7004565

    /* client versions(extended status gump info) */
    SCDECL kMinCliver_StatusV2 = CUOClientVersion(3u, 0u, 8u, 'd'); // minimum client to receive v2 of 0x11 packet (3.0.8d), old vernum 3000804
    SCDECL kMinCliver_StatusV3 = CUOClientVersion(3u, 0u, 8u, 'j'); // minimum client to receive v3 of 0x11 packet (3.0.8j), old vernum 3000810
    SCDECL kMinCliver_StatusV4 = CUOClientVersion(4u, 0u, 0u, 0u);  // minimum client to receive v4 of 0x11 packet (4.0.0a), old vernum 4000000
    SCDECL kMinCliver_StatusV5 = CUOClientVersion(5u, 0u, 0u, 0u);  // minimum client to receive v5 of 0x11 packet (5.0.0a), old vernum 5000000
    SCDECL kMinCliver_StatusV6 = CUOClientVersion(7u, 0u, 30u, 0u); // minimum client to receive v6 of 0x11 packet (7.0.30.0), old vernum 7003000

    /* Client versions (behaviours) */
    SCDECL kMinCliver_CheckWalkCode     = CUOClientVersion(1u, 26u, 0u, 0u);    // minimum client to use walk crypt codes for fastwalk prevention (obsolete), old vernum 1260000
    SCDECL kMinCliver_PadCharList       = CUOClientVersion(3u, 0u, 0u, 'j');    // minimum client to pad character list to at least 5 characters, old vernum 3000010
    SCDECL kMinCliver_AutoAsync         = CUOClientVersion(4u, 0u, 0u, 0u);     // minimum client to auto-enable async networking, old vernum 4000000
    SCDECL kMinCliver_NotoInvul         = CUOClientVersion(4u, 0u, 0u, 0u);     // minimum client required to view noto_invul health bar, old vernum 4000000
    SCDECL kMinCliver_SkillCaps         = CUOClientVersion(4u, 0u, 0u, 0u);     // minimum client to send skill caps in 0x3A packet, old vernum 4000000
    SCDECL kMinCliver_CloseDialog       = CUOClientVersion(4u, 0u, 4u, 0u);     // minimum client where close dialog does not trigger a client response, old vernum 4000400
    SCDECL kMinCliver_CompressDialog    = CUOClientVersion(5u, 0u, 0u, 0u);     // minimum client to receive zlib compressed dialogs(5.0.0a), old vernum 5000000
    SCDECL kMinCliver_NewVersioning     = CUOClientVersion(5u, 0u, 6u, 5u);     // minimum client to use the new versioning format (after 5.0.6e it change to 5.0.6.5), old vernum 5000605
    SCDECL kMinCliver_ItemGrid          = CUOClientVersion(6u, 0u, 1u, 7u);     // minimum client to use grid index (6.0.1.7), old vernum 6000107
    SCDECL kMinCliver_NewChatSystemCC   = CUOClientVersion(7u, 0u, 4u, 1u);     // minimum client to use the new chat system (7.0.4.1) - classic client, old vernum 7000401
    SCDECL kMinCliver_GlobalChat        = CUOClientVersion(7u, 0u, 62u, 2u);    // minimum client to use global chat system (7.0.62.2), old vernum 7006202
    SCDECL kMinCliver_NewSecureTrade    = CUOClientVersion(7u, 0u, 45u, 65u);   // minimum client to use virtual gold/platinum on trade window (7.0.45.65), old vernum 7004565
    SCDECL kMinCliver_MapWaypoint       = CUOClientVersion(7u, 0u, 84u, 0u);    // minimum client to use map waypoints on classic client (7.0.84.0), old vernum 7008400

    // minimum client to use the new chat system (4.0.4.0) - enhanced client, old vernum 4000400
    SCDECL kMinCliverEC_NewChatSystem = CUOClientVersion(CUOClientVersion::kuiECMajorVerOffset + 4u, 0u, 4u, 0u);

    /* Client versions (packets) */
    SCDECL kMinCliver_ReverseIP         = CUOClientVersion(4u, 0u, 0u, 0u); // maximum client to reverse IP in 0xA8 packet, old vernum 4000000
    SCDECL kMinCliver_CustomMulti       = CUOClientVersion(4u, 0u, 0u, 0u); // minimum client to receive custom multi packets, old vernum 4000000
    SCDECL kMinCliver_Spellbook         = CUOClientVersion(4u, 0u, 0u, 0u); // minimum client to receive 0xBF.0x1B packet (4.0.0a), old vernum 4000000
    SCDECL kMinCliver_Damage            = CUOClientVersion(4u, 0u, 0u, 0u); // minimum client to receive 0xBF.0x22 packet, old vernum 4000000
    SCDECL kMinCliver_Tooltip           = CUOClientVersion(4u, 0u, 0u, 0u); // minimum client to receive tooltip packets, old vernum 4000000
    SCDECL kMinCliver_StatLocks         = CUOClientVersion(4u, 0u, 1u, 0u); // minimum client to receive 0xBF.0x19.0x02 packet, old vernum 4000100
    SCDECL kMinCliver_TooltipHash       = CUOClientVersion(4u, 0u, 5u, 0u); // minimum client to receive 0xDC packet, old vernum 4000500
    SCDECL kMinCliver_NewDamage         = CUOClientVersion(4u, 0u, 7u, 0u); // minimum client to receive 0x0B packet (4.0.7a), old vernum 4000700
    SCDECL kMinCliver_NewBook           = CUOClientVersion(5u, 0u, 0u, 0u); // minimum client to receive 0xD4 packet, old vernum 5000000
    SCDECL kMinCliver_Buffs             = CUOClientVersion(5u, 0u, 2u, 'b');// minimum client to receive buff packets (5.0.2b), old vernum 5000202
    SCDECL kMinCliver_NewContextMenu    = CUOClientVersion(6u, 0u, 0u, 0u); // minimun client to receive 0xBF.0x14.0x02 packet instead of 0xBF.0x14.0x01 (6.0.0.0), old vernum 6000000
    SCDECL kMinCliver_ExtraFeatures     = CUOClientVersion(6u, 0u, 14u, 2u);// minimum client to receive 4-byte feature mask (6.0.14.2), old vernum 6001402
    SCDECL kMinCliver_NewMobileAnim     = CUOClientVersion(7u, 0u, 0u, 0u); // minimum client to receive 0xE2 packet (7.0.0.0), old vernum 7000000
    SCDECL kMinCliver_SmoothShip        = CUOClientVersion(7u, 0u, 9u, 0u); // minimum client to receive 0xF6 packet (7.0.9.0), old vernum 7000900
    SCDECL kMinCliver_NewMapDisplay     = CUOClientVersion(7u, 0u, 13u, 0u);// minimum client to receive 0xF5 packet (7.0.13.0), old vernum 7001300
    SCDECL kMinCliver_ExtraStartInfo    = CUOClientVersion(7u, 0u, 13u, 0u);// minimum client to receive extra start info (7.0.13.0), old vernum 7001300
    SCDECL kMinCliver_NewMobIncoming    = CUOClientVersion(7u, 0u, 33u, 1u);// minimun client to receive 0x78 packet (7.0.33.1), old vernum 7003301

#undef SCDECL
};

#endif // _INC_CUOCLIENTVERSION_H
