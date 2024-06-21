#include "../../sphere/threads.h"
#include "../CException.h"
#include "../CExpression.h"
#include "CValueDefs.h"


//*******************************************
// -CValueRangeDef

int CValueRangeDef::GetLinear( int iPercent ) const
{
    // ARGS: iPercent = 0-1000
    return (int)m_iLo + IMulDiv( GetRange(), iPercent, 1000 );
}

int CValueRangeDef::GetRandom() const
{
    return ( (int)m_iLo + g_Rand.GetVal(GetRange()) );
}

int CValueRangeDef::GetRandomLinear( int iPercent ) const
{
    ADDTOCALLSTACK("CValueRangeDef::GetRandomLinear");
    return ( ( GetRandom() + GetLinear(iPercent) ) / 2 );
}

bool CValueRangeDef::Load( tchar * pszDef )
{
    ADDTOCALLSTACK("CValueRangeDef::Load");

    // it can be a range (with format {lo# hi#}) or a single value, even without brackets,
    //	so we don't need a warning if GetRangeVals doesn't find the brackets
    int64 piVal[2];
    int iQty = g_Exp.GetRangeVals( pszDef, piVal, ARRAY_COUNT(piVal), true);
    if ( iQty <= 0 )
        return false;

    m_iLo = piVal[0];
    if ( iQty > 1 )
        m_iHi = piVal[1];
    else
        m_iHi = m_iLo;
    return true;
}

const tchar * CValueRangeDef::Write() const
{
    //ADDTOCALLSTACK("CValueRangeDef::Write");
    return nullptr;
}

//*******************************************
// -CValueCurveDef

const tchar * CValueCurveDef::Write() const
{
    ADDTOCALLSTACK("CValueCurveDef::Write");
    tchar * pszOut = Str_GetTemp();
    size_t j = 0;
    size_t iQty = m_aiValues.size();
    for ( size_t i = 0; i < iQty; i++ )
    {
        if ( i > 0 )
            pszOut[j++] = ',';

        j += sprintf( pszOut + j, "%d", m_aiValues[i] );
    }
    pszOut[j] = '\0';
    return pszOut;
}

bool CValueCurveDef::Load( tchar * pszDef )
{
    ADDTOCALLSTACK("CValueCurveDef::Load");
    // ADV_RATE = Chance at 0, to 100.0
    int64 Arg_piCmd[101];
    size_t iQty = Str_ParseCmds( pszDef, Arg_piCmd, ARRAY_COUNT(Arg_piCmd));
    m_aiValues.resize(iQty);
    if ( iQty == 0 )
    {
        return false;
    }
    for ( size_t i = 0; i < iQty; i++ )
    {
        m_aiValues[i] = (int)(Arg_piCmd[i]);
    }
    return true;
}

int CValueCurveDef::GetLinear( int iSkillPercent ) const
{
    ADDTOCALLSTACK("CValueCurveDef::GetLinear");
    //
    // ARGS:
    //  iSkillPercent = 0 - 1000 = 0 to 100.0 percent
    //  m_Rate[3] = the 3 advance rate control numbers, 100,50,0 skill levels
    //		Acts as line segments.
    // RETURN:
    //  raw chance value.

    int iSegSize;
    int iLoIdx;

    int iQty = (int) m_aiValues.size();
    switch (iQty)
    {
        case 0:
            return 0;	// no values defined !
        case 1:
            return m_aiValues[0];
        case 2:
            iLoIdx = 0;
            iSegSize = 1000;
            break;
        case 3:
            // Do this the fastest.
            if ( iSkillPercent >= 500 )
            {
                iLoIdx = 1;
                iSkillPercent -= 500;
            }
            else
            {
                iLoIdx = 0;
            }
            iSegSize = 500;
            break;
        default:
            // More
            iLoIdx = IMulDiv( iSkillPercent, iQty, 1000 );
            --iQty;
            if ( iLoIdx >= iQty )
                iLoIdx = iQty - 1;
            iSegSize = 1000 / iQty;
            iSkillPercent -= (int)( iLoIdx * iSegSize );
            break;
    }

    int iLoVal = m_aiValues[iLoIdx];
    int iHiVal = m_aiValues[iLoIdx + 1];
    int iChance = iLoVal + (int)IMulDivLL( iHiVal - iLoVal, iSkillPercent, iSegSize );

    if ( iChance <= 0 )
        return 0; // less than no chance ?

    return iChance;
}

int CValueCurveDef::GetRandom( ) const
{
    ADDTOCALLSTACK("CValueCurveDef::GetRandom");
    return GetLinear( g_Rand.GetVal( 1000 ) );
}


int CValueCurveDef::GetRandomLinear( int iSkillPercent  ) const
{
    ADDTOCALLSTACK("CValueCurveDef::GetRandomLinear");
    return ( GetLinear( iSkillPercent ) + GetRandom() ) / 2;
}

int CValueCurveDef::GetChancePercent( int iSkillPercent ) const
{
    ADDTOCALLSTACK("CValueCurveDef::GetChancePercent");
    // ARGS:
    //  iSkillPercent = 0 - 1000 = 0 to 100.0 percent
    //
    //  m_Rate[3] = the 3 advance rate control numbers, 0,50,100 skill levels
    //   (How many uses for a gain of .1 (div by 100))
    // RETURN:
    //  percent chance of success * 10 = 0 - 1000.

    // How many uses for a gain of .1 (div by 100)
    int iChance = GetLinear( iSkillPercent );
    if ( iChance <= 0 )
        return 0; // less than no chance ?
                  // Express uses as a percentage * 10.
    return( 100000 / iChance );
}
