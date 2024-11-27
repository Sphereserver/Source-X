/**
* @file  CSObjListRec.h
* @brief Element of a CSObjList
*/

#ifndef _INC_CSOBJLISTREC_H
#define _INC_CSOBJLISTREC_H

class CSObjList;


/**
* @brief Generic list record.
*
* Each CSObjListRec belongs to just one CSObjList.
*/
class CSObjListRec
{
public:
    friend class CSObjList;
    static const char * m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief set references for parent, next and previous to nullptr.
    */
    CSObjListRec() noexcept;

    virtual ~CSObjListRec();

    /**
    * @brief No copies allowed.
    */
    CSObjListRec(const CSObjListRec& copy) = delete;
    /**
    * @brief No copies allowed.
    */
    CSObjListRec& operator=(const CSObjListRec& other) = delete;
    ///@}

    /** @name Iterators:
    */
    ///@{
public:
    /**
    * @brief get the CSObjList propietary of this record.
    * @return CSObjList propietary of this record.
    */
    inline CSObjList * GetParent() const noexcept;
    /**
    * @brief get the next record of the parent list.
    * @return the next record of the parent list.
    */
    inline CSObjListRec * GetNext() const noexcept;
    /**
    * @brief get the previous record of the parent list.
    * @return the previous record of the parent list.
    */
    inline CSObjListRec * GetPrev() const noexcept;
    ///@}
    /** @name Capacity:
    */
    ///@{
    /**
    * @brief Removes from the parent CSObjList.
    */
    void RemoveSelf();
    ///@}

private:
    CSObjList * m_pParent;	// Parent list.
    CSObjListRec * m_pNext;  // Next record.
    CSObjListRec * m_pPrev;  // Prev record.
};


/* Inline Methods Definitions */

// CSObjListRec:: Capacity.

//void CSObjListRec::RemoveSelf()   // defined in CSObjList.cpp


// CObjListRec:: Iterators.

inline CSObjList * CSObjListRec::GetParent() const noexcept
{
    return m_pParent;
}

inline CSObjListRec * CSObjListRec::GetNext() const noexcept
{
    return m_pNext;
}

inline CSObjListRec * CSObjListRec::GetPrev() const noexcept
{
    return m_pPrev;
}

#endif //_INC_CSOBJLISTREC_H
