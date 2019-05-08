/**
* @file  CSTypedArray.h
* @brief Dynamic array (not thread safe).
*/

#ifndef _INC_CSTYPEDARRAY_H
#define _INC_CSTYPEDARRAY_H

#include <cstring>
#include <cstdint>

#include <vector>

#include "../assertion.h"

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
    CSTypedArray() {}
    /**
    * @brief Copy constructor.
    */
    CSTypedArray<TYPE>(const CSTypedArray<TYPE> & copy);
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
    void erase(size_t nIndex);
    /**
    * @brief Insert a new element to the end of the array.
    * @param newElement element to insert.
    * @return the element count of the array.
    */
    size_t push_back(TYPE newElement);
    /**
    * @brief Update element nth to a new value.
    * @param nIndex index of element to update.
    * @param newElement new value.
    */
    void assign(size_t nIndex, TYPE newElement);
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
    inline constexpr static size_t BadIndex();
    /**
    * @brief Check if index is valid for this array.
    * @param i index to check.
    * @return true if index is valid, false otherwise.
    */
    bool IsValidIndex( size_t i ) const;
    ///@}
};



/* Template methods (inlined or not) are defined here */

// CSTypedArray:: Constructors, Destructor, Asign operator.

template<class TYPE>
CSTypedArray<TYPE>::CSTypedArray(const CSTypedArray<TYPE> & copy) : std::vector<TYPE>(static_cast<const std::vector<TYPE>&>(copy)) {}

template<class TYPE>
CSTypedArray<TYPE> & CSTypedArray<TYPE>::operator=( const CSTypedArray<TYPE> & array )
{
    static_cast<std::vector<TYPE> >(*this) = static_cast<const std::vector<TYPE> &>(array);
    return *this;
}

// CSTypedArray:: Element access.

// CSTypedArray:: Modifiers.

template<class TYPE>
size_t CSTypedArray<TYPE>::push_back(TYPE newElement)
{
    std::vector<TYPE>::push_back(newElement);
    return std::vector<TYPE>::size() - 1;
}

template<class TYPE>
void CSTypedArray<TYPE>::insert(size_t nIndex, TYPE newElement)
{	// Bump the existing entry here forward.
    ASSERT(nIndex != this->BadIndex());
    std::vector<TYPE>::emplace(std::vector<TYPE>::begin() + nIndex, newElement);
}


template<class TYPE>
void CSTypedArray<TYPE>::erase(size_t nIndex)
{
    if ( !IsValidIndex(nIndex) )
        return;
    std::vector<TYPE>::erase(std::vector<TYPE>::begin() + nIndex);
}

template<class TYPE>
void CSTypedArray<TYPE>::assign(size_t nIndex, TYPE newElement)
{
    ASSERT(IsValidIndex(nIndex));

    this->at(nIndex) = newElement;

}

template<class TYPE>
void CSTypedArray<TYPE>::assign_at_grow(size_t nIndex, TYPE newElement)
{
    ASSERT(nIndex != this->BadIndex());

    if ( ! IsValidIndex(nIndex))
        std::vector<TYPE>::resize(nIndex + 1);
    assign(nIndex, newElement);
}


// CSTypedArray:: Operations.

template<class TYPE>
constexpr size_t CSTypedArray<TYPE>::BadIndex() // static
{
    return INTPTR_MAX;
}


template<class TYPE>
bool CSTypedArray<TYPE>::IsValidIndex( size_t i ) const
{
    return i < std::vector<TYPE>::size();
}


#endif //_INC_CSTYPEDARRAY_H
