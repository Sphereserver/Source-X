#include <doctest/doctest.h>
#include "../../src/common/CExpression.h"
#include <string>
#include <string_view>
#include <cctype> // toupper

#ifndef MSVC_COMPILER
#define ATTR_NO_UBSAN __attribute__((no_sanitize("undefined")))
#else
#define ATTR_NO_UBSAN //__declspec(no_sanitize_address)
#endif


namespace { namespace ref_impl
{
/*
    // From 0.56d, known to be faulty... Copying it here just for historical reasons.
    int ahextoi(lpctstr pszArgs)	// convert hex string to int
    {
        // Unfortunately the library func can't handle UINT_MAX
        //tchar *pszEnd; return strtol(s, &pszEnd, 16);

        if ( !pszArgs || !*pszArgs )
            return 0;

        GETNONWHITESPACE(pszArgs);

        bool fHex = false;
        if ( *pszArgs == '0' )
        {
            if ( *++pszArgs != '.' )
                fHex = true;
            --pszArgs;
        }

        int iVal = 0;
        for (;;)
        {
            tchar ch = static_cast<tchar>(toupper(*pszArgs));
            if ( IsDigit(ch) )
                ch -= '0';
            else if ( fHex && (ch >= 'A') && (ch <= 'F') )
                ch -= 'A' - 10;
            else if ( !fHex && (ch == '.') )
            {
                ++pszArgs;
                continue;
            }
            else
                break;

            iVal *= (fHex ? 0x10 : 10);
            iVal += ch;
            ++pszArgs;
        }
        return iVal;
    }

    // From 0.56d, known to be faulty... Copying it here just for historical reasons.
    int64 ahextoi64(lpctstr pszArgs)	// convert hex string to int64
    {
        if ( !pszArgs || !*pszArgs )
            return 0;

        GETNONWHITESPACE(pszArgs);

        bool fHex = false;
        if ( *pszArgs == '0' )
        {
            if ( *++pszArgs != '.' )
                fHex = true;
            --pszArgs;
        }

        int64 iVal = 0;
        for (;;)
        {
            tchar ch = static_cast<tchar>(toupper(*pszArgs));
            if ( IsDigit(ch) )
                ch -= '0';
            else if ( fHex && (ch >= 'A') && (ch <= 'F') )
                ch -= 'A' - 10;
            else if ( !fHex && (ch == '.') )
            {
                ++pszArgs;
                continue;
            }
            else
                break;

            iVal *= (fHex ? 0x10 : 10);
            iVal += ch;
            ++pszArgs;
        }
        return iVal;
    }
*/

}}

// Small helper to check value and how far the input pointer advanced.
// Expects expr.GetSingle to consume the token in-place and leave 'in' pointing
// at the first non-numeric char following the parsed number.
#define CHECK_PARSE_AND_REMAINDER(input, expected_val, expected_remainder) {                                                                \
    lpctstr parsed_input_str = input;                                                                                                       \
    const int64 actual_val = expr.GetSingle(parsed_input_str);                                                                                 \
    DOCTEST_CHECK_MESSAGE(actual_val == (int64)expected_val,                                                                                       \
        "Value mismatch!\nInput: '" << input << "'\nExpected: '" << expected_val << "'\nGot: '" << actual_val << "'");                      \
    const std::string_view sv_post_parse(parsed_input_str);                                                                                 \
    const std::string_view sv_expected(expected_remainder);                                                                                 \
    DOCTEST_CHECK_MESSAGE(sv_expected == sv_post_parse,                                                                                     \
        "Remainder mismatch!\nInput: '" << input << "'\nExpected remainder: '" << sv_expected << "'\nGot: '" << sv_post_parse << "'");      \
}

TEST_CASE("Numerical string parsing")\
// clazy:exclude=non-pod-global-static
{
    CExpression& expr = CExpression::GetExprParser();
    lpctstr in_str_consumable;
    llong out_cur_impl, out_ref_impl;

    SUBCASE("Decimal integer")
    {
        constexpr std::pair<char const * const, int64> test_data_ref_ok[]
        {
            {"0",   0},
            {"-1",  -1},
            {"100", 100},
            {"2147483647",  INT32_MAX},
            {"-2147483647", -INT32_MAX},
            {"-2147483648", INT32_MIN},
            {"4294967295",  UINT32_MAX},
            {"9223372036854775807",     INT64_MAX},
            {"-9223372036854775807",    -INT64_MAX},
            //{"-9223372036854775808",    INT64_MIN}
        };

        for (auto const& p : test_data_ref_ok)
        {
            in_str_consumable = p.first, out_cur_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);

            in_str_consumable = p.first, out_ref_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, out_ref_impl);
        }

        // With this data the old reference implementation failed
        constexpr std::pair<char const * const, int64> test_data_ref_fail[]
            {
                {"9223372036854775808",  /* INT64_MAX + 1 */ -1}     // Overflow
            };

        for (auto const& p : test_data_ref_fail)
        {
            in_str_consumable = p.first, out_cur_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);
        }

    }

    SUBCASE("Hexadecimal")
    {
        constexpr std::pair<char const * const, int64> test_data_ref_ok[]
            {
                {"0",   0},
                {"07FFFFFFF",           INT32_MAX},
                {"0100000000",          0x100000000},
                {"07FFFFFFFFFFFFFFF",   0x7FFFFFFFFFFFFFFF /* INT64_MAX */},
                {"0FFFFFFFFFFFFFFFF",   -1},
                {"0FFFFFFFFFFFFFFFE",   -2}
            };

        for (auto const& p : test_data_ref_ok)
        {
            in_str_consumable = p.first, out_cur_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);

            in_str_consumable = p.first, out_ref_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, out_ref_impl);
        }

        // With this data the old reference implementation failed
        constexpr std::pair<char const * const, int64> test_data_ref_fail[]
        {
            {"0FFFFFFFF",           -1},    // This is a SphereServer quirk for historical reasons
            {"0FFFFFFFFFFFFFFFF1",  -1}     // Overflow
        };

        for (auto const& p : test_data_ref_fail)
        {
            in_str_consumable = p.first, out_cur_impl = expr.GetSingle(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);
        }
    }

    SUBCASE("Whitespace and trailing junk consumption")
    {
        CHECK_PARSE_AND_REMAINDER("   42", 42, "");           // consume entire string
        CHECK_PARSE_AND_REMAINDER("   42   ", 42, "   ");     // stop at trailing space
        CHECK_PARSE_AND_REMAINDER("   3]", 3, "]");           // stop right before ']'
        CHECK_PARSE_AND_REMAINDER("   0F]", 15, "]");         // hex with trailing junk
        CHECK_PARSE_AND_REMAINDER("123abc", 123, "abc");      // decimal then junk
        CHECK_PARSE_AND_REMAINDER("0F  xyz", 15, "  xyz");    // hex then spaces+text
    }

    SUBCASE("Leading dot quirk and 0. decimal disambiguation")
    {
        // Leading '.' is skipped; ".07" becomes hex marker "0" followed by '7' -> 7
        CHECK_PARSE_AND_REMAINDER(".07]", 7, "]");

        // "0." is decimal; dots inside decimal are ignored as separators
        CHECK_PARSE_AND_REMAINDER("0.", 0, "");               // bare 0. -> 0
        CHECK_PARSE_AND_REMAINDER("0.123", 123, "");          // 0.123 -> 123
        CHECK_PARSE_AND_REMAINDER(".0.1]", 1, "]");           // leading '.' skipped -> "0.1]" -> decimal 1
        CHECK_PARSE_AND_REMAINDER("1.234.567end", 1234567, "end");
    }

    SUBCASE("Hex width by significant digits (leading zeros ignored after marker)")
    {
        CHECK_PARSE_AND_REMAINDER("00000F", 15, "");          // marker '0' then digits "0000F" -> 15
        CHECK_PARSE_AND_REMAINDER("07FFFFFFF", INT32_MAX, ""); // 8 significant digits -> 32-bit
        CHECK_PARSE_AND_REMAINDER("080000000", (llong)INT32_MIN, ""); // 8 significant digits -> 32-bit negative
        CHECK_PARSE_AND_REMAINDER("0FFFFFFFF", -1, "");       // 8 significant -> 32-bit -1
        CHECK_PARSE_AND_REMAINDER("0FFFFFFFFFFFFFFFF", -1, "");// 16 significant -> 64-bit -1
        CHECK_PARSE_AND_REMAINDER("0FFFFFFFF00", 0xFFFFFFFF00LL, ""); // 64-bit positive
    }

    /*
    SUBCASE("Negative decimal vs negative hex")
    {
        // Negative decimals are supported
        EXPECT_PARSE_AND_REMAINDER("-1", -1, "");
        EXPECT_PARSE_AND_REMAINDER("-0",  0, "");
        EXPECT_PARSE_AND_REMAINDER("-1.234", -1234, "");

        // "-0." is decimal (supported), should parse as 0 with decimal semantics
        EXPECT_PARSE_AND_REMAINDER("-0.", 0, "");

        // Negative hex is unsupported, but parser should:
        //  - log a warning,
        //  - consume the '-', ignore the sign,
        //  - parse and consume the entire hex token.
        EXPECT_PARSE_AND_REMAINDER("-00", 0, "");              // hex zero after sign
        EXPECT_PARSE_AND_REMAINDER("-0F", 15, "");             // hex positive (sign ignored)
        EXPECT_PARSE_AND_REMAINDER("-0FFFFFFFF", -1, "");      // hex, 32-bit width -> -1 (sign ignored)
        EXPECT_PARSE_AND_REMAINDER("  -0F]", 15, "]");         // with whitespace and trailing junk
    }
    */

    SUBCASE("Overflow behaviors")
    {
        // Decimal overflow: > INT64_MAX
        CHECK_PARSE_AND_REMAINDER("9223372036854775808X", -1, "X");

        // Hex overflow: >16 significant hex digits (leading zeros ignored before first non-zero)
        CHECK_PARSE_AND_REMAINDER("0FFFFFFFFFFFFFFFFFFFFZ", -1, "Z"); // 20 significant digits
        CHECK_PARSE_AND_REMAINDER("00000000000000000FFFFFFFFFFFFFFFFFFFF!", -1, "!"); // still overflow
    }

    SUBCASE("Unsupported 0x prefix")
    {
        // Legacy: "0x" is not a hex prefix; hex marker is just '0'.
        // Behavior: marker '0' is consumed, 'x' is non-hex -> stop; resulting value is 0; remainder begins with 'x'.
        CHECK_PARSE_AND_REMAINDER("0xFF", 0, "xFF");
        CHECK_PARSE_AND_REMAINDER("   0x7B", 0, "x7B");
    }
}
