/**
* @file  CSTypedArray.h
* @brief Dynamic array (not thread safe).
*/

#ifndef _INC_CSTYPEDARRAY_H
#define _INC_CSTYPEDARRAY_H

#include "../assertion.h"
#include "ssorted_vector.h"

/**
* @brief Typed Array:
*
* std::vector with more methods.
*/
template<class TYPE>
class CSTypedArray : public std::vector<TYPE>
{
public:
    static const char *m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief Initializes array.
    *
    * Sets m_pData to nullptr and counters to zero.
    */
    CSTypedArray() noexcept = default;
    /**
    * @brief Copy constructor.
    */

    CSTypedArray(const CSTypedArray<TYPE> & copy);
    /**
    * @brief Assign operator
    */
    CSTypedArray<TYPE> & operator=(const CSTypedArray<TYPE> &array);
public:
    ///@}

    /** @name Modifiers:
    */
    ///@{
    /**
    * @brief Insert a element in nth position.
    * @param nIndex position to insert the element.
    * @param newElement element to insert.
    */
    void insert(size_t nIndex, TYPE newElement);
    /**
    * @brief Removes the nth element and move the next elements one position left.
    * @param nIndex position of the element to remove.
    */
    void erase_at(size_t nIndex);
    /**
    * @brief Update element nth to a new value.
    * @param nIndex index of element to update.
    * @param newElement new value.
    */
    void assign_at(size_t nIndex, TYPE newElement);
    /**
    * @brief Update element nth to a new value.
    * @param nIndex index of element to update.
    * @param newElement new value.
    */
    void assign_at_grow(size_t nIndex, TYPE newElement);
    ///@}
    /** @name Index Validation:
    */
    ///@{
    /**
    * @brief Check if index is valid for this array.
    * @param i index to check.
    * @return true if index is valid, false otherwise.
    */
    inline bool IsValidIndex( size_t i ) const;
    ///@}
};



/* Template methods (inlined or not) are defined here */

// CSTypedArray:: Constructors, Destructor, Assign operator.

template<class TYPE>
CSTypedArray<TYPE>::CSTypedArray(const CSTypedArray<TYPE> & copy) : std::vector<TYPE>(static_cast<const std::vector<TYPE>&>(copy)) {}

template<class TYPE>
CSTypedArray<TYPE> & CSTypedArray<TYPE>::operator=( const CSTypedArray<TYPE> & array )
{
    static_cast<std::vector<TYPE> >(*this) = static_cast<const std::vector<TYPE> &>(array);
    return *this;
}

// CSTypedArray:: Modifiers.

template<class TYPE>
void CSTypedArray<TYPE>::insert(size_t nIndex, TYPE newElement)
{	// Bump the existing entry here forward.
    ASSERT(nIndex != sl::scont_bad_index());
    std::vector<TYPE>::emplace(std::vector<TYPE>::begin() + nIndex, newElement);
}


template<class TYPE>
void CSTypedArray<TYPE>::erase_at(size_t nIndex)
{
    if ( !IsValidIndex(nIndex) )
        return;
    std::vector<TYPE>::erase(std::vector<TYPE>::begin() + nIndex);
}

template<class TYPE>
void CSTypedArray<TYPE>::assign_at(size_t nIndex, TYPE newElement)
{
    ASSERT(IsValidIndex(nIndex));

    this->at(nIndex) = newElement;

}

template<class TYPE>
void CSTypedArray<TYPE>::assign_at_grow(size_t nIndex, TYPE newElement)
{
    ASSERT(nIndex != sl::scont_bad_index());

    if ( ! IsValidIndex(nIndex))
        std::vector<TYPE>::resize(nIndex + 1);
    assign_at(nIndex, newElement);
}


// CSTypedArray:: Operations.

template<class TYPE>
bool CSTypedArray<TYPE>::IsValidIndex( size_t i ) const
{
    return i < std::vector<TYPE>::size();
}


#endif //_INC_CSTYPEDARRAY_H
