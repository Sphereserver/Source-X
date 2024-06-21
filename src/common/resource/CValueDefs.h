/**
* @file CValueDefs.h
* @brief Outward packets.
*/

#ifndef _INC_CVALUEDEFS_H
#define _INC_CVALUEDEFS_H

#include "../sphere_library/CSTypedArray.h"


/**
* @struct  CValueRangeDef
*
* @brief   A value range definition.<
*/
struct CValueRangeDef
{
public:
    int64 m_iLo; // Lower Value
    int64 m_iHi; // Higher Value
public:

    /**
    * @fn  void Init()
    *
    * @brief Initializes this object.
    */
    void Init()
    {
        m_iLo = INT64_MIN;
        m_iHi = INT64_MIN;
    }

    /**
    * @fn  int GetRange() const
    *
    * @brief   Gets the range.
    *
    * @return  The result range.
    */
    int GetRange() const
    {
        return (int)(m_iHi - m_iLo);
    }

    /**
    * @fn  int GetLinear( int iPercent ) const
    *
    * @brief   Gets a linear value.
    *
    * @param   iPercent    Zero-based index of the percent.
    *
    * @return  The linear.
    */
    int GetLinear( int iPercent ) const;

    /**
    * @fn  int GetRandom() const
    *
    * @brief   Gets a random value.
    *
    * @return  The random.
    */
    int GetRandom() const;

    /**
    * @fn  int GetRandomLinear( int iPercent ) const;
    *
    * @brief   Gets a random linear value.
    *
    * @param   iPercent    Zero-based index of the percent.
    *
    * @return  The random linear.
    */
    int GetRandomLinear( int iPercent ) const;

    /**
    * @fn  bool Load( tchar * pszDef );
    *
    * @brief   Loads the given psz definition from script.
    *
    * @param [in,out]  pszDef  If non-null, the definition to load.
    *
    * @return  true if it succeeds, false if it fails.
    */
    bool Load( tchar * pszDef );

    /**
    * @fn  const tchar * Write() const;
    *
    * @brief   Outputs the value.
    *
    * @return  null if it fails, else a pointer to a const tchar.
    */
    const tchar * Write() const;

public:
    CValueRangeDef()
    {
        Init();
    }
};

/**
* @struct  CValueCurveDef
*
* @brief   Describe an arbitrary curve for a range from 0.0 to 100.0 (1000).
*          May be a list of probabilties from 0 skill to 100.0% skill.
*/
struct CValueCurveDef
{
public:
    CSTypedArray<int> m_aiValues;		// 0 to 100.0 skill levels

public:
    void Init()
    {
        m_aiValues.clear();
    }

    /**
    * @fn  bool Load( tchar * pszDef );
    *
    * @brief   Loads the value from script.
    *
    * @param [in,out]  pszDef  If non-null, the definition to load.
    *
    * @return  true if it succeeds, false if it fails.
    */
    bool Load( tchar * pszDef );

    /**
    * @fn  const tchar * Write() const;
    *
    * @brief   Outputs the value.
    *
    * @return  null if it fails, else a pointer to a const tchar.
    */
    const tchar * Write() const;

    /**
    * @fn  int GetLinear( int iSkillPercent ) const;
    *
    * @brief   Gets a linear value.
    *
    * @param   iSkillPercent   Zero-based index of the skill percent.
    *
    * @return  The linear.
    */
    int GetLinear( int iSkillPercent ) const;

    /**
    * @fn  int GetChancePercent( int iSkillPercent ) const;
    *
    * @brief   Gets the chance percent.
    *
    * @param   iSkillPercent   Zero-based index of the skill percent.
    *
    * @return  The chance percent.
    */
    int GetChancePercent( int iSkillPercent ) const;

    /**
    * @fn  int GetRandom() const;
    *
    * @brief   Gets a random value.
    *
    * @return  The random.
    */
    int GetRandom() const;

    /**
    * @fn  int GetRandomLinear( int iPercent ) const;
    *
    * @brief   Gets a random linear value.
    *
    * @param   iPercent    Zero-based index of the percent.
    *
    * @return  The random linear.
    */
    int GetRandomLinear( int iPercent ) const;

public:
    CValueCurveDef() noexcept = default;
    ~CValueCurveDef() noexcept = default;

    CValueCurveDef(const CValueCurveDef& copy) = delete;
    CValueCurveDef& operator=(const CValueCurveDef& other) = delete;
};


#endif // _INC_CVALUEDEFS_H
