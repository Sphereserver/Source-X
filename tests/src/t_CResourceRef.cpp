#include <doctest/doctest.h>
#include "../../src/common/resource/CResourceLink.h"
#include "../../src/common/resource/CResourceRef.h"
#include "../../src/game/CServerConfig.h"
#include <string>
#include <vector>

// EVENTS=, TEVENTS= and the other lists of resources: what a script line adds and removes.

namespace
{
    // An [EVENTS name] section, as indexed from the scripts. The list only keeps the link, not the text.
    CResourceLink* GetTestEvent(int iIndex, lpctstr ptcName)
    {
        const CResourceID rid(RES_EVENTS, iIndex);
        const size_t i = g_Cfg.m_ResHash.FindKey(rid);
        if (i != sl::scont_bad_index())
            return dynamic_cast<CResourceLink*>(g_Cfg.m_ResHash.GetBarePtrAt(rid, i));
        auto* pLink = new CResourceLink(rid);
        pLink->SetResourceName(ptcName);    // Registers the name of the section, to find it by name.
        g_Cfg.m_ResHash.AddSortKey(rid, pLink);
        return pLink;
    }

    std::vector<CResourceLink*> Links(const CResourceRefArray& arr)
    {
        std::vector<CResourceLink*> vLinks;
        for (const CResourceRef& ref : arr)
            vLinks.emplace_back(ref.GetRef());
        return vLinks;
    }

    // The text of the script line: LoadValStr modifies it.
    bool Load(CResourceRefArray& arr, const char* pcLine)
    {
        std::string sLine(pcLine);
        return arr.LoadValStr(sLine.data(), RES_EVENTS);
    }
}

TEST_CASE("CResourceRefArray::LoadValStr adds and removes resources")
{
    CResourceLink* pA = GetTestEvent(0xFFF01, "e_unit_test_a");
    CResourceLink* pB = GetTestEvent(0xFFF02, "e_unit_test_b");
    CResourceLink* pC = GetTestEvent(0xFFF03, "e_unit_test_c");
    REQUIRE(pA != nullptr);
    REQUIRE(pB != nullptr);
    REQUIRE(pC != nullptr);

    CResourceRefArray arr;
    CHECK(Load(arr, "e_unit_test_a, +e_unit_test_b"));
    CHECK(Links(arr) == std::vector<CResourceLink*>{pA, pB});
    CHECK(pA->GetRefInstances() == 1u);

    SUBCASE("Adding one already in the list doesn't add it again")
    {
        CHECK(Load(arr, "+e_unit_test_a,e_unit_test_c"));
        CHECK(Links(arr) == std::vector<CResourceLink*>{pA, pB, pC});
        CHECK(pA->GetRefInstances() == 1u);
    }
    SUBCASE("Removing one")
    {
        CHECK(Load(arr, "-e_unit_test_a"));
        CHECK(Links(arr) == std::vector<CResourceLink*>{pB});
        CHECK(pA->GetRefInstances() == 0u);
        CHECK(pB->GetRefInstances() == 1u);
    }
    SUBCASE("Removing one not in the list fails, and changes nothing")
    {
        CHECK(!Load(arr, "-e_unit_test_c"));
        CHECK(Links(arr) == std::vector<CResourceLink*>{pA, pB});
    }
    SUBCASE("-0 removes all")
    {
        CHECK(Load(arr, "-0"));
        CHECK(arr.empty());
        CHECK(pA->GetRefInstances() == 0u);
        CHECK(pB->GetRefInstances() == 0u);
    }
    SUBCASE("-* removes all, and the line goes on")
    {
        CHECK(Load(arr, "-*, +e_unit_test_c"));
        CHECK(Links(arr) == std::vector<CResourceLink*>{pC});
    }
    SUBCASE("-0 followed by more digits is a resource number, not all")
    {
        CHECK(Load(arr, "-0FFF01"));    // The number of e_unit_test_a, in hexadecimal.
        CHECK(Links(arr) == std::vector<CResourceLink*>{pB});
    }
    SUBCASE("Removing all keeps the failure of an argument before it")
    {
        CHECK(!Load(arr, "+e_unit_test_unknown, -*"));
        CHECK(arr.empty());
    }
    SUBCASE("An unknown name fails, the rest of the line is applied")
    {
        CHECK(!Load(arr, "-e_unit_test_a, +e_unit_test_unknown, +e_unit_test_c"));
        CHECK(Links(arr) == std::vector<CResourceLink*>{pB, pC});
    }
}
