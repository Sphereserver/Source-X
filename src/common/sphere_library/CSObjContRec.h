/**
* @file  CSObjContRec.h
* @brief Element of a CSObjCont
*/

#ifndef _INC_CSOBJCONTREC_H
#define _INC_CSOBJCONTREC_H

#include <cstddef>  // for nullptr
class CSObjCont;


/**
* @brief Generic list record.
*
* Each CSObjContRec belongs to just one CSObjCont.
*/
class CSObjContRec
{
public:
    friend class CSObjCont;
    static const char * m_sClassName;
    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief set references for parent, next and previous to nullptr.
    */
    CSObjContRec();
    virtual ~CSObjContRec();
private:
    /**
    * @brief No copies allowed.
    */
    CSObjContRec(const CSObjContRec& copy);
    /**
    * @brief No copies allowed.
    */
    CSObjContRec& operator=(const CSObjContRec& other);
    ///@}
    /** @name Iterators:
    */
    ///@{
public:
    /**
    * @brief get the CSObjCont propietary of this record.
    * @return CSObjCont propietary of this record.
    */
    inline CSObjCont * GetParent() const noexcept;
    ///@}
    /** @name Capacity:
    */
    ///@{
    /**
    * @brief Removes from the parent CSObjCont.
    */
    void RemoveSelf();
    ///@}

private:
    CSObjCont * m_pParent;	// Parent list.
};


/* Inline Methods Definitions */

inline CSObjCont * CSObjContRec::GetParent() const noexcept
{
    return m_pParent;
}

#endif //_INC_CSOBJCONTREC_H
