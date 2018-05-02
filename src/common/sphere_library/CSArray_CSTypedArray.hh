#ifndef _INC_CSARRAY_CSTYPEDARRAY_HH
#define _INC_CSARRAY_CSTYPEDARRAY_HH

#include <cstring>
#include <cstdint>
#include "../assertion.h"

/**
* @brief Typed Array (not thread safe).
*
* NOTE: This will not call true constructors or destructors !
* TODOC: Really needed two types in template? Answer: yes. If we are storing instances instead of pointers,
*  we can set ARG_TYPE to be a reference of TYPE.
*/
template<class TYPE, class ARG_TYPE>
class CSTypedArray
{
public:
    static const char *m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief Initializes array.
    *
    * Sets m_pData to NULL and counters to zero.
    */
    CSTypedArray();
    virtual ~CSTypedArray();
    const CSTypedArray<TYPE, ARG_TYPE> & operator=(const CSTypedArray<TYPE, ARG_TYPE> &array);
private:
    /**
    * @brief No copy on construction allowed.
    */
    CSTypedArray<TYPE, ARG_TYPE>(const CSTypedArray<TYPE, ARG_TYPE> & copy);
    ///@}
    /** @name Capacity:
    */
    ///@{
public:
    /**
    * @brief Get the element count in array.
    * @return get the element count in array.
    */
    size_t GetCount() const;
    /**
    * @brief Get the total element that fits in allocated mem.
    * @return get the total element that fits in allocated mem.
    */
    size_t GetRealCount() const;
    ///@}
    /** @name Element access:
    */
    ///@{
    /**
    * @brief get the nth element.
    *
    * Also checks if index is valid.
    * @see GetAt()
    * @param nIndex position of the element.
    * @return Element in nIndex position.
    */
    TYPE operator[](size_t nIndex) const;
    /**
    * @brief get a reference to the nth element.
    *
    * Also checks if index is valid.
    * @see ElementAt()
    * @param nIndex position of the element.
    * @return Element in nIndex position.
    */
    TYPE& operator[](size_t nIndex);
    /**
    * @brief get a reference to the nth element.
    *
    * Also checks if index is valid.
    * @param nIndex position of the element.
    * @return Element in nIndex position.
    */
    TYPE& ElementAt( size_t nIndex );
    /**
    * @brief get a reference to the nth element.
    *
    * Also checks if index is valid.
    * @param nIndex position of the element.
    * @return Element in nIndex position.
    */
    const TYPE& ElementAt( size_t nIndex ) const;
    /**
    * @brief get the nth element.
    *
    * Also checks if index is valid.
    * @param nIndex position of the element.
    * @return Element in nIndex position.
    */
    TYPE GetAt( size_t nIndex) const;
    ///@}
    /** @name Modifiers:
    */
    ///@{
    /**
    * @brief Insert a new element to the end of the array.
    * @param newElement element to insert.
    * @return the element count of the array.
    */
    size_t Add( ARG_TYPE newElement );
    /**
    * @brief TODOC
    * @param pElements TODOC
    * @param nCount TODOC
    */
    virtual void ConstructElements(TYPE* pElements, size_t nCount );
    /**
    * @brief Copy an CSTypedArray into this.
    * @param pArray array to copy.
    */
    void Copy( const CSTypedArray<TYPE, ARG_TYPE> * pArray );
    /**
    * @brief Remove all elements from the array and free mem.
    */
    void Clear();
    /**
    * @brief Insert a element in nth position.
    * @param nIndex position to insert the element.
    * @param newElement element to insert.
    */
    void InsertAt( size_t nIndex, ARG_TYPE newElement );

    /**
    * @brief Removes the nth element and move the next elements one position left.
    * @param nIndex position of the element to remove.
    */
    void RemoveAt( size_t nIndex );
    /**
    * @brief Update element nth to a new value.
    * @param nIndex index of element to update.
    * @param newElement new value.
    */
    void SetAt( size_t nIndex, ARG_TYPE newElement );
    /**
    * @brief Update element nth to a new value.
    *
    * If size of array is lesser to nIndex, increment array size.
    * @param nIndex index of element to update.
    * @param newElement new value.
    */
    void SetAtGrow( size_t nIndex, ARG_TYPE newElement);
    /**
    * @brief Realloc the internal data into a new size.
    * @param nNewCount new size of the mem.
    */
    void SetCount( size_t nNewCount );
    ///@}
    /** @name Operations:
    */
    ///@{
    inline size_t BadIndex() const;
    /**
    * @brief Get the internal data pointer.
    *
    * This is dangerous to use of course.
    * @return the internal data pointer.
    */
    TYPE * GetBasePtr() const;
    /**
    * @brief Check if index is valid for this array.
    * @param i index to check.
    * @return true if index is valid, false otherwise.
    */
    bool IsValidIndex( size_t i ) const;
    ///@}

private:
    TYPE* m_pData;			// Pointer to allocated mem.
    size_t m_nCount;		// count of elements stored.
    size_t m_nRealCount;	// Size of allocated mem.
};



/* Template methods (inlined or not) are defined here */

// CSTypedArray:: Constructors, Destructor, Asign operator.

template<class TYPE, class ARG_TYPE>
CSTypedArray<TYPE,ARG_TYPE>::CSTypedArray()
{
    m_pData = NULL;
    m_nCount = 0;
    m_nRealCount = 0;
}

template<class TYPE, class ARG_TYPE>
inline CSTypedArray<TYPE,ARG_TYPE>::~CSTypedArray()
{
    Clear();
}

template<class TYPE, class ARG_TYPE>
const CSTypedArray<TYPE, ARG_TYPE> & CSTypedArray<TYPE, ARG_TYPE>::operator=( const CSTypedArray<TYPE, ARG_TYPE> & array )
{
    Copy(&array);
    return *this;
}

// CSTypedArray:: Capacity.

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE,ARG_TYPE>::GetCount() const
{
    return m_nCount;
}

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE,ARG_TYPE>::GetRealCount() const
{
    return m_nRealCount;
}

// CSTypedArray:: Element access.

template<class TYPE, class ARG_TYPE>
inline TYPE CSTypedArray<TYPE,ARG_TYPE>::operator[](size_t nIndex) const
{
    return GetAt(nIndex);
}

template<class TYPE, class ARG_TYPE>
inline TYPE& CSTypedArray<TYPE,ARG_TYPE>::operator[](size_t nIndex)
{
    return ElementAt(nIndex);
}

template<class TYPE, class ARG_TYPE>
inline TYPE& CSTypedArray<TYPE,ARG_TYPE>::ElementAt( size_t nIndex )
{
    ASSERT(IsValidIndex(nIndex));
    return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline const TYPE& CSTypedArray<TYPE,ARG_TYPE>::ElementAt( size_t nIndex ) const
{
    ASSERT(IsValidIndex(nIndex));
    return m_pData[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline TYPE CSTypedArray<TYPE,ARG_TYPE>::GetAt( size_t nIndex) const
{
    ASSERT(IsValidIndex(nIndex));
    return m_pData[nIndex];
}

// CSTypedArray:: Modifiers.

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE,ARG_TYPE>::Add( ARG_TYPE newElement )
{
    // Add to the end.
    SetAtGrow(GetCount(), newElement);
    return (m_nCount - 1);
}

template<class TYPE, class ARG_TYPE>
inline void CSTypedArray<TYPE,ARG_TYPE>::ConstructElements(TYPE* pElements, size_t nCount )
{
    // first do bit-wise zero initialization
    memset(static_cast<void *>(pElements), 0, nCount * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::Copy(const CSTypedArray<TYPE, ARG_TYPE> * pArray)
{
    if ( !pArray || pArray == this )	// it was !=
        return;

    Clear();
    if (!pArray->GetCount())
        return;
    
    SetCount(pArray->GetCount());
    memcpy(GetBasePtr(), pArray->GetBasePtr(), GetCount() * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::InsertAt( size_t nIndex, ARG_TYPE newElement )
{	// Bump the existing entry here forward.
    ASSERT(nIndex != this->BadIndex());

    SetCount( (nIndex >= m_nCount) ? (nIndex + 1) : (m_nCount + 1) );
    if (nIndex != m_nCount-1)
        memmove(&m_pData[nIndex + 1], &m_pData[nIndex], sizeof(TYPE) * (m_nCount - nIndex - 1));
    memcpy(&m_pData[nIndex], &newElement, sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::Clear()
{
    delete[] reinterpret_cast<uint8_t *>(m_pData);
    m_pData = NULL;
    m_nCount = m_nRealCount = 0;
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::RemoveAt( size_t nIndex )
{
    if ( !IsValidIndex(nIndex) )
        return;

    if (nIndex < m_nCount-1)
        memmove(&m_pData[nIndex], &m_pData[nIndex + 1], sizeof(TYPE) * (m_nCount - nIndex - 1));
    SetCount(m_nCount - 1);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAt( size_t nIndex, ARG_TYPE newElement )
{
    ASSERT(IsValidIndex(nIndex));


#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdynamic-class-memaccess"
#endif
    memcpy(&m_pData[nIndex], &newElement, sizeof(TYPE));
#ifdef __clang__
    #pragma clang diagnostic pop
#endif
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAtGrow( size_t nIndex, ARG_TYPE newElement)
{
    ASSERT(nIndex != this->BadIndex());

    if ( nIndex >= m_nCount )
        SetCount(nIndex + 1);
    SetAt(nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE, ARG_TYPE>::SetCount( size_t nNewCount )
{
    ASSERT(nNewCount != this->BadIndex()); // to hopefully catch integer underflows (-1)
    if (nNewCount == 0)
    {
        // shrink to nothing
        if (m_nCount > 0)
            Clear();
        return;
    }

    if ( nNewCount > m_nCount )
    {
#if __GNUC__ >= 7
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Walloc-size-larger-than="
#endif
        uint8_t* pNewData = new uint8_t[nNewCount * sizeof(TYPE)];
        if ( m_nCount )
        {
            // copy the old stuff to the new array.
            memcpy( pNewData, reinterpret_cast<uint8_t*>(m_pData), sizeof(TYPE)*m_nCount );
            delete[] reinterpret_cast<uint8_t *>(m_pData);	// don't call any destructors.
        }
#if __GNUC__ >= 7
    #pragma GCC diagnostic pop
#endif

        // Just construct or init the new stuff.
        ConstructElements( reinterpret_cast<TYPE*>(pNewData + sizeof(TYPE)*m_nCount), nNewCount - m_nCount );
        m_pData = reinterpret_cast<TYPE*>(pNewData);

        m_nRealCount = nNewCount;
    }

    m_nCount = nNewCount;
}

// CSTypedArray:: Operations.

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE, ARG_TYPE>::BadIndex() const
{
    return INTPTR_MAX;
}

template<class TYPE, class ARG_TYPE>
inline TYPE * CSTypedArray<TYPE,ARG_TYPE>::GetBasePtr() const
{
    return m_pData;
}


template<class TYPE, class ARG_TYPE>
inline bool CSTypedArray<TYPE,ARG_TYPE>::IsValidIndex( size_t i ) const
{
    return ( i < m_nCount );
}


#endif //_INC_CSARRAY_CSTYPEDARRAY_HH