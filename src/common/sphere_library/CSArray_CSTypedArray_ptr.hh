#ifndef _INC_CSARRAY_CSTYPEDARRAY_PTR_HH
#define _INC_CSARRAY_CSTYPEDARRAY_PTR_HH

// It doesn't work yet!

#include <memory>
#include <vector>
#include <cinttypes>
#include "../assertion.h"

template <typename T>
struct CSPointerizer
{
    std::shared_ptr<T> _data;
    //static const bool isTypePointer()   { return false; }

    inline CSPointerizer()                          { }
    inline CSPointerizer(T data)                    { _data.reset(new T(data)); }
    //inline CSPointerizer(const T& data)             { _data.reset(new T(data)); }
    inline CSPointerizer(const CSPointerizer& other){ _data = other._data; }
    inline T operator*()                { return *_data; }
    inline T operator*() const          { return *_data; }
    inline T operator->()               { return *_data; }
    inline T operator->() const         { return *_data; }
    inline operator T()                 { return *_data; }
    inline operator const T() const     { return *_data; }
    inline operator T&()                { return *_data; }
    inline operator const T&() const    { return *_data; }
    inline T& operator&()               { return *_data; }
    inline const T& operator&() const   { return *_data; }
    CSPointerizer& operator=(const CSPointerizer& other)
    {
        _data = other._data;
        return *this;
    }
};

template <typename T>
struct CSPointerizer<T*>
{
    T* _data;
    //static const bool isTypePointer()   { return true; }

    inline CSPointerizer()                          { _data = nullptr; }
    //inline CSPointerizer(T* data)                   { _data = data; }
    inline CSPointerizer(const T* data)             { _data = const_cast<T*>(data); }
    inline CSPointerizer(const CSPointerizer& other){ _data = other._data; }
    inline T* operator*()               { return _data; }
    inline T* operator*() const         { return _data; }
    inline T* operator->()              { return _data; }
    inline T* operator->() const        { return _data; }
    inline const T*& operator&() const  { return _data; }
    inline operator T*()                { return _data; }
    inline operator const T*() const    { return _data; }
    inline operator T*&()               { return _data; }
    inline operator const T*&() const   { return _data; }
    CSPointerizer& operator=(const CSPointerizer& other)
    {
        _data = other._data;
        return *this;
    }
};

/**
* @brief Typed Array (not thread safe).
*
* NOTE: This will not call true constructors or destructors !
* TODO: Really needed two types in template?
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
    const TYPE * GetBasePtr() const;
    TYPE * GetBasePtr();
    /**
    * @brief Check if index is valid for this array.
    * @param i index to check.
    * @return true if index is valid, false otherwise.
    */
    bool IsValidIndex( size_t i ) const;
    ///@}

private:
    std::vector<CSPointerizer<TYPE>> _dataVec;
};



/* Template methods (inlined or not) are defined here */

// CSTypedArray:: Constructors, Destructor, Asign operator.

template<class TYPE, class ARG_TYPE>
CSTypedArray<TYPE,ARG_TYPE>::CSTypedArray()
{
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
    return _dataVec.size();
}

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE,ARG_TYPE>::GetRealCount() const
{
    return _dataVec.size();
}

// CSTypedArray:: Element access.

template<class TYPE, class ARG_TYPE>
inline TYPE CSTypedArray<TYPE,ARG_TYPE>::operator[](size_t nIndex) const
{
    ASSERT(IsValidIndex(nIndex));
    return *_dataVec[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline TYPE& CSTypedArray<TYPE,ARG_TYPE>::operator[](size_t nIndex)
{
    ASSERT(IsValidIndex(nIndex));
    return _dataVec[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline TYPE& CSTypedArray<TYPE,ARG_TYPE>::ElementAt( size_t nIndex )
{
    ASSERT(IsValidIndex(nIndex));
    return _dataVec[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline const TYPE& CSTypedArray<TYPE,ARG_TYPE>::ElementAt( size_t nIndex ) const
{
    ASSERT(IsValidIndex(nIndex));
    return _dataVec[nIndex];
}

template<class TYPE, class ARG_TYPE>
inline TYPE CSTypedArray<TYPE,ARG_TYPE>::GetAt( size_t nIndex) const
{
    ASSERT(IsValidIndex(nIndex));
    return *_dataVec[nIndex];
}

// CSTypedArray:: Modifiers.

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE,ARG_TYPE>::Add( ARG_TYPE newElement )
{
    // Add to the end.
    _dataVec.emplace_back(newElement);
    return (_dataVec.size() - 1);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::Copy(const CSTypedArray<TYPE, ARG_TYPE> * pArray)
{
    if ( !pArray || pArray == this )	// it was !=
        return;

    _dataVec = pArray->_dataVec;
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::InsertAt( size_t nIndex, ARG_TYPE newElement )
{	// Bump the existing entry here forward.
    ASSERT(nIndex != this->BadIndex());

    if (nIndex >= _dataVec.size())
        _dataVec.resize(nIndex+1);
    _dataVec.emplace(_dataVec.begin()+nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
inline void CSTypedArray<TYPE,ARG_TYPE>::Clear()
{
    _dataVec.clear();
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::RemoveAt( size_t nIndex )
{
    if ( !IsValidIndex(nIndex) )
        return;

    _dataVec.erase(_dataVec.begin()+nIndex);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAt( size_t nIndex, ARG_TYPE newElement )
{
    ASSERT(IsValidIndex(nIndex));
    _dataVec.emplace(_dataVec.begin()+nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAtGrow( size_t nIndex, ARG_TYPE newElement )
{
    if (nIndex >= _dataVec.size())
        _dataVec.resize(nIndex+1);
    SetAt(nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE, ARG_TYPE>::SetCount( size_t nNewCount )
{
    ASSERT(nNewCount != this->BadIndex()); // to hopefully catch integer underflows (-1)
    _dataVec.resize(nNewCount);
}

// CSTypedArray:: Operations.


template<class TYPE, class ARG_TYPE>
const TYPE * CSTypedArray<TYPE, ARG_TYPE>::GetBasePtr() const
{
    if (_dataVec.empty())
        return nullptr;
    return reinterpret_cast<const TYPE*>(_dataVec.data());
}

template<class TYPE, class ARG_TYPE>
TYPE * CSTypedArray<TYPE, ARG_TYPE>::GetBasePtr()
{
    if (_dataVec.empty())
        return nullptr;
    return const_cast<TYPE*>(reinterpret_cast<const TYPE*>(_dataVec.data()));
}

template<class TYPE, class ARG_TYPE>
inline size_t CSTypedArray<TYPE, ARG_TYPE>::BadIndex() const
{
    return INTPTR_MAX;
}

template<class TYPE, class ARG_TYPE>
inline bool CSTypedArray<TYPE,ARG_TYPE>::IsValidIndex( size_t i ) const
{
    return ( i < _dataVec.size() );
}


#endif //_INC_CSARRAY_CSTYPEDARRAY_PTR_HH

