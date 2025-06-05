#include <doctest/doctest.h>
#include "../../src/common/CExpression.h"
#include <cctype> // toupper

#ifndef MSVC_COMPILER
#define ATTR_NO_UBSAN __attribute__((no_sanitize("undefined")))
#else
#define ATTR_NO_UBSAN __declspec(no_sanitize_address)
#endif


namespace { namespace ref_impl
{

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


    // Part of CExpression::GetSingle that parses numerical strings.
    int64 GetSingle_num(lpctstr &pszArgs) ATTR_NO_UBSAN
    {
        // Parse just a single expression without any operators or ranges.
        GETNONWHITESPACE(pszArgs);

        if ( pszArgs[0] == '.' )
            ++pszArgs;

        if ( pszArgs[0] == '0' )	// leading '0' = hex value
        {
            // Hex value
            if ( pszArgs[1] == '.' )	// leading '0.' = decimal value
            {
                pszArgs += 2;
                goto try_dec;
            }

            lpctstr pStart = pszArgs;
            int64 iVal = 0;
            for (;;)
            {
                tchar ch = *pszArgs;
                if ( IsDigit(ch) )
                    ch -= '0';
                else
                {
                    ch = static_cast<tchar>(tolower(ch));
                    if ( (ch > 'f') || (ch < 'a') )
                    {
                        if ( (ch == '.') && (pStart[0] != '0') )	// ok I'm confused, it must be decimal
                        {
                            pszArgs = pStart;
                            goto try_dec;
                        }
                        break;
                    }
                    ch -= 'a' - 10;
                }
                iVal *= 0x10;
                iVal += ch;
                ++pszArgs;
            }
            return iVal;
        }
        else if ( (pszArgs[0] == '.') || IsDigit(pszArgs[0]) )
        {
            // Decimal value
        try_dec:
            int64 iVal = 0;
            for ( ; ; ++pszArgs )
            {
                if ( *pszArgs == '.' )
                    continue;	// just skip this
                if ( !IsDigit(*pszArgs) )
                    break;
                iVal *= 10;
                iVal += static_cast<int64>(*pszArgs) - '0';
            }
            return iVal;
        }
        else if ( ! _ISCSYMF(pszArgs[0]) )
        {
            //#pragma region maths  // MSVC specific
            // some sort of math op ?

            switch ( pszArgs[0] )
            {
                /*
                case '{':
                    ++pszArgs;
                    return GetRangeNumber( pszArgs );
                case '[':
                case '(': // Parse out a sub expression.
                    ++pszArgs;
                    return GetVal( pszArgs );
                */
                case '+':
                    ++pszArgs;
                    break;
                case '-':
                    ++pszArgs;
                    return -GetSingle_num( pszArgs );
                case '~':	// Bitwise not.
                    ++pszArgs;
                    return ~GetSingle_num( pszArgs );
                case '!':	// boolean not.
                    ++pszArgs;
                    if ( pszArgs[0] == '=' )  // odd condition such as (!=x) which is always true of course.
                    {
                        ++pszArgs;		// so just skip it. and compare it to 0
                        return GetSingle_num( pszArgs );
                    }
                    return !GetSingle_num( pszArgs );
                case ';':	// seperate field.
                case ',':	// seperate field.
                case '\0':
                    return 0;
            }
        }
        throw std::runtime_error("reached an unexpected point?");
    }
}}


TEST_CASE("Numerical string parsing")\
// clazy:exclude=non-pod-global-static
{
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
            in_str_consumable = p.first, out_cur_impl = Exp_GetLLVal(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);

            in_str_consumable = p.first, out_ref_impl = ref_impl::GetSingle_num(in_str_consumable);
            CHECK_EQ(out_cur_impl, out_ref_impl);
        }

        // With this data the old reference implementation failed
        constexpr std::pair<char const * const, int64> test_data_ref_fail[]
            {
                {"9223372036854775808",  /* INT64_MAX + 1 */ -1}     // Overflow
            };

        for (auto const& p : test_data_ref_fail)
        {
            in_str_consumable = p.first, out_cur_impl = Exp_GetLLVal(in_str_consumable);
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
            in_str_consumable = p.first, out_cur_impl = Exp_GetLLVal(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);

            in_str_consumable = p.first, out_ref_impl = ref_impl::GetSingle_num(in_str_consumable);
            CHECK_EQ(out_cur_impl, out_ref_impl);
        }

        // With this data the old reference implementation failed
        constexpr std::pair<char const * const, int64> test_data_ref_fail[]
        {
            {"0FFFFFFFF",           -1},     // This is a SphereServer quirk for historical reasons.
            {"0FFFFFFFFFFFFFFFF1",  -1}     // Overflow
        };

        for (auto const& p : test_data_ref_fail)
        {
            in_str_consumable = p.first, out_cur_impl = Exp_GetLLVal(in_str_consumable);
            CHECK_EQ(out_cur_impl, p.second);
        }
    }
}
