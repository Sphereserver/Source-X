/**
* @file CSArray.h
* @brief C++ collections custom implementation.
*/

#pragma once
#ifndef _INC_CSARRAY_H
#define _INC_CSARRAY_H

#include <cstring>
#include "../common.h"

#ifdef _MSC_VER
	#define STANDARD_CPLUSPLUS_THIS(_x_) _x_
	#pragma warning(disable:4505)
#else
	#define STANDARD_CPLUSPLUS_THIS(_x_) this->_x_
	#include <cstddef>
#endif
#ifdef __linux
	#define STANDARD_CPLUSPLUS_THIS(_x_) this->_x_
#endif


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
    * @brief set references for parent, next and previous to NULL.
    */
	CSObjListRec();
	virtual ~CSObjListRec();
private:
	/**
    * @brief No copies allowed.
    */
	CSObjListRec(const CSObjListRec& copy);
	/**
    * @brief No copies allowed.
    */
	CSObjListRec& operator=(const CSObjListRec& other);
	///@}
	/** @name Iterators:
	 */
	///@{
public:
	/**
    * @brief get the CSObjList propietary of this record.
    * @return CSObjList propietary of this record.
    */
	inline CSObjList * GetParent() const;
	/**
    * @brief get the next record of the parent list.
    * @return the next record of the parent list.
    */
	inline CSObjListRec * GetNext() const;
	/**
    * @brief get the previous record of the parent list.
    * @return the previous record of the parent list.
    */
	inline CSObjListRec * GetPrev() const;
	///@}
	/** @name Capacity:
	 */
	///@{
	/**
    * @brief Removes from the parent CSObjList.
    */
	inline void RemoveSelf();
	///@}

private:
	CSObjList * m_pParent;	///< Parent list.
	CSObjListRec * m_pNext;  ///< Next record.
	CSObjListRec * m_pPrev;  ///< Prev record.
};

/**
* @brief Generic list of objects.
*/
class CSObjList
{
public:
	friend class CSObjListRec;
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	/**
    * @brief Sets head, tail and count.
    */
	CSObjList();
	virtual ~CSObjList();
private:
	/**
    * @brief No copies allowed.
    */
	CSObjList(const CSObjList& copy);
	/**
    * @brief No copies allowed.
    */
	CSObjList& operator=(const CSObjList& other);
	///@}
	/** @name Capacity:
	 */
	///@{
public:
	/**
    * @brief Get the record count of the list.
    * @return The record count of the list.
    */
	size_t GetCount() const;
	/**
    * @brief Check if CSObjList if empty.
    * @return true if CSObjList is empty, false otherwise.
    */
	bool IsEmpty() const;
	///@}
	/** @name Element Access:
	 */
	///@{
	/**
    * @brief Get the nth element of the list.
    * @param index of the element to get.
    * @return nth element if lenght is greater or equal to index, NULL otherwise.
    */
	CSObjListRec * GetAt( size_t index ) const;
	/**
    * @brief Get the first record of the CSObjList.
    * @return The first record of the CSObjList if list is not empty, NULL otherwise.
    */
	CSObjListRec * GetHead() const;
	/**
    * @brief Get the last record of the CSObjList.
    * @return The last record of the CSObjList if list is not empty, NULL otherwise.
    */
	CSObjListRec * GetTail() const;
	///@}
	/** @name Modifiers:
	 */
	///@{
	/**
    * @brief Remove all records of the CSObjList.
    */
	void DeleteAll();
	/**
    * @brief Remove all records of the CSObjList.
    *
    * TODO: Really needed?
    * @see DeleteAll()
    */
	void Empty();
	/**
    * @brief Insert a record after the referenced record.
    *
    * If the position referenced is NULL, the record is inserted at head.
    * @param pNewRec record to insert.
    * @param pPrev position to insert after.
    */
	virtual void InsertAfter( CSObjListRec * pNewRec, CSObjListRec * pPrev = NULL );
	/**
    * @brief Insert a record at head.
    * @param pNewRec record to insert.
    */
	void InsertHead( CSObjListRec * pNewRec );
	/**
    * @brief Insert a record at tail.
    * @param pNewRec record to insert.
    */
	void InsertTail( CSObjListRec * pNewRec );
protected:
	/**
    * @brief Trigger that fires when a record if removed.
    *
    * Override this to get called when an item is removed from this list.
    * Never called directly. Called CSObjListRec::RemoveSelf()
    * @see CSObjListRec::RemoveSelf()
    * @param pObRec removed record.
    */
	virtual void OnRemoveObj( CSObjListRec* pObRec );
private:
	/**
    * @brief Call the trigger OnRemoveObj.
    *
    * Only called by CSObjListRec::RemoveSelf()
    * @see OnRemoveObj()
    * @param pObRec record to remove.
    */
	void RemoveAtSpecial( CSObjListRec * pObRec );
	///@}

	CSObjListRec * m_pHead;  ///< Head of the list.
	CSObjListRec * m_pTail;  ///< Tail of the list. Do we really care about tail ? (as it applies to lists anyhow)
	size_t m_iCount;		///< Count of elements of the CSObjList.
};

/**
* @brief Typed Array.
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
    * @brief TODOC
    * @param pElements TODOC
    * @param nCount TODOC
    */
	virtual void DestructElements(TYPE* pElements, size_t nCount );
	/**
    * @brief Remove all elements from the array and free mem.
    *
    * TODO: Really needed?
    * @see RemoveAll()
    */
	void Empty();
	/**
    * @brief Insert a element in nth position.
    * @param nIndex position to insert the element.
    * @param newElement element to insert.
    */
	void InsertAt( size_t nIndex, ARG_TYPE newElement );
	/**
    * @brief Remove all elements from the array and free mem.
    */
	void RemoveAll();
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
	TYPE* m_pData;	///< Pointer to allocated mem.
	size_t m_nCount;	///< count of elements stored.
	size_t m_nRealCount;	///< Size of allocated mem.
};

/**
* @brief An Array of pointers.
*/
template<class TYPE>
class CSPtrTypeArray : public CSTypedArray<TYPE, TYPE>
{
public:
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	CSPtrTypeArray();
	virtual ~CSPtrTypeArray();
private:
	/**
    * @brief No copy on construction allowed.
    */
	CSPtrTypeArray<TYPE>(const CSPtrTypeArray<TYPE> & copy);
	/**
    * @brief No copy allowed.
    */
	CSPtrTypeArray<TYPE>& operator=(const CSPtrTypeArray<TYPE> & other);
	///@}
	/** @name Modifiers:
	 */
	///@{
protected:
	/**
    * @brief TODOC
    * @param pElements TODOC
    * @param nCount TODOC
    */
	virtual void DestructElements( TYPE* pElements, size_t nCount );
public:
	/**
    * @brief if data is in array, rmove it.
    * @param pData data to remove from the array.
    */
	bool RemovePtr( TYPE pData );
	///@}
	/** @name Operations:
	 */
	///@{
	/**
    * @brief check if data is in this array.
    * @param pData data to find in the array.
    * @return true if pData is in the array, BadIndex() otherwise.
    */
	bool ContainsPtr( TYPE pData ) const;
	/**
    * @brief get the position of a data in the array.
    * @param pData data to look for.
    * @return position of the data if data is in the array, BadIndex otherwise.
    */
	size_t FindPtr( TYPE pData ) const;
	/**
    * @brief Check if an index is between 0 and element count.
    * @param i index to check.
    * @return true if index is valid, false otherwise.
    */
	bool IsValidIndex( size_t i ) const;
	///@}
};

/**
* @brief Array of objects.
*
* The point of this type is that the array now OWNS the element.
* It will get deleted when the array is deleted.
*/
template<class TYPE>
class CSObjArray : public CSPtrTypeArray<TYPE>
{
public:
	static const char *m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
public:
	CSObjArray();
	virtual ~CSObjArray();
private:
	/**
    * @brief No copy on construction allowed.
    */
	CSObjArray<TYPE>(const CSObjArray<TYPE> & copy);
	/**
    * @brief No copy allowed.
    */
	CSObjArray<TYPE> & operator=(const CSObjArray<TYPE> & other);
	///@}
	/** @name Modifiers:
	 */
	///@{
public:
	/**
    * @brief remove all elements.
    * @param bElements if true, destroy elements too.
    */
	void Clean(bool bElements = false);
	/**
    * @brief Remove an element if exists in the array.
    * @param pData data to remove.
    * @return true if data is removed, false otherwise.
    */
	bool DeleteOb( TYPE pData );
	/**
    * @brief Remove the nth element.
    * @param nIndex position of the element to remove.
    * @return true if index is valid and object is removed, false otherwise.
    */
	void DeleteAt( size_t nIndex );
protected:
	/**
    * @brief Destroy elements.
    * @param pElements pointer to data to destroy.
    * @param nCount Number of elements to destroy.
    */
	virtual void DestructElements( TYPE* pElements, size_t nCount );
	///@}
};

/**
* @brief Array of objects (sorted).
*
* The point of this type is that the array now OWNS the element.
* It will get deleted when the array is deleted.
*/
template<class TYPE,class KEY_TYPE>
class CSObjSortArray : public CSObjArray<TYPE>
{
	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
public:
	CSObjSortArray();
	virtual ~CSObjSortArray();
private:
	/**
    * @brief No copy on construction allowed.
    */
	CSObjSortArray<TYPE, KEY_TYPE>(const CSObjSortArray<TYPE, KEY_TYPE> & copy);
	/**
    * @brief No copy allowed.
    */
	CSObjSortArray<TYPE, KEY_TYPE> & operator=(const CSObjSortArray<TYPE, KEY_TYPE> & other);
	///@}
	/** @name Modifiers:
	 */
	///@{
public:
	/**
    * @brief Adds a value into a position.
    * @see FindKeyNear()
    * @param index position to insert the value.
    * @param iCompareRes modifier to index. See FindKeyNear().
    * @param pNew value to insert.
    * @return position where the value is inserted.
    */
	size_t AddPresorted( size_t index, int iCompareRes, TYPE pNew );
	/**
    * @brief Add a pair key-value and mantain the array sorted.
    *
    * If key exists in the array, will destroy current value and sets the
    * new.
    * @param pNew value to insert.
    * @param key key of the value to insert.
    * @return position where the value is inserted.
    */
	size_t AddSortKey( TYPE pNew, KEY_TYPE key );
	/**
    * @brief TODOC.
    * @param fNoSpaces TODOC.
    * @return TODOC.
    */
	virtual int CompareKey( KEY_TYPE, TYPE, bool fNoSpaces ) const = 0;
	/**
    * @brief Removes the pair key - value from the array.
    * @param key to remove.
    */
	void DeleteKey( KEY_TYPE key );
	///@}
	/** @name Operations:
	 */
	///@{
	/**
    * @brief Check if a key is in the index.
    * @param key key we are looking for.
    * @return true if key is in the array, false otherwise.
    */
	bool ContainsKey( KEY_TYPE key ) const;
	/**
    * @brief Finds the position of the key closest to a provided key.
    *
    * Also sets iCompareRes to a value: 0 if key match with index, -1 if
    * key should be less than index and +1 if key should be greater than
    * index.
    * @param key key to find.
    * @param iCompareRes comparison result.
    * @param fNoSpaces TODOC
    * @return index closest to key.
    */
	size_t FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces = false ) const;
	/**
    * @brief Find a key in the array.
    * @param key key we are looking for.
    * @return a valid index if the key is in the array, BadIndex otherwise.
    */
	size_t FindKey( KEY_TYPE key ) const;
	///@}
};



/* Template methods (inlined or not) are defined here to avoid linker errors */

// CSObjListRec:: Capacity.

inline void CSObjListRec::RemoveSelf()
{
	if (GetParent())
		m_pParent->RemoveAtSpecial(this);
}

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
	SetCount(0);
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

	Empty();
	SetCount(pArray->GetCount());
	memcpy(GetBasePtr(), pArray->GetBasePtr(), GetCount() * sizeof(TYPE));
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::DestructElements(TYPE* pElements, size_t nCount )
{
	UNREFERENCED_PARAMETER(pElements);
	UNREFERENCED_PARAMETER(nCount);
	//memset(static_cast<void *>(pElements), 0, nCount * sizeof(*pElements));
}

template<class TYPE, class ARG_TYPE>
inline void CSTypedArray<TYPE,ARG_TYPE>::Empty()
{
	RemoveAll();
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::InsertAt( size_t nIndex, ARG_TYPE newElement )
{	// Bump the existing entry here forward.
	ASSERT(nIndex != STANDARD_CPLUSPLUS_THIS(BadIndex()));

	SetCount( (nIndex >= m_nCount) ? (nIndex + 1) : (m_nCount + 1) );
	memmove( &m_pData[nIndex + 1], &m_pData[nIndex], sizeof(TYPE) * (m_nCount - nIndex - 1));
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
inline void CSTypedArray<TYPE,ARG_TYPE>::RemoveAll()
{
	SetCount(0);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::RemoveAt( size_t nIndex )
{
	if ( !IsValidIndex(nIndex) )
		return;

	DestructElements(&m_pData[nIndex], 1);
	memmove(&m_pData[nIndex], &m_pData[nIndex + 1], sizeof(TYPE) * (m_nCount - nIndex - 1));
	SetCount(m_nCount - 1);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAt( size_t nIndex, ARG_TYPE newElement )
{
	ASSERT(IsValidIndex(nIndex));

	DestructElements(&m_pData[nIndex], 1);
	m_pData[nIndex] = newElement;
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE,ARG_TYPE>::SetAtGrow( size_t nIndex, ARG_TYPE newElement)
{
	ASSERT(nIndex != STANDARD_CPLUSPLUS_THIS(BadIndex()));

	if ( nIndex >= m_nCount )
		SetCount(nIndex + 1);
	SetAt(nIndex, newElement);
}

template<class TYPE, class ARG_TYPE>
void CSTypedArray<TYPE, ARG_TYPE>::SetCount( size_t nNewCount )
{
	ASSERT(nNewCount != STANDARD_CPLUSPLUS_THIS(BadIndex())); // to hopefully catch integer underflows (-1)
	if (nNewCount == 0)
	{
		// shrink to nothing
		if (m_nCount > 0)
		{
			DestructElements( m_pData, m_nCount );
			delete[] reinterpret_cast<byte *>(m_pData);
			m_nCount = m_nRealCount = 0;	// that's probably wrong.. but SetCount(0) should be never called
			m_pData = NULL;					// before Clean(true) in CSObjArray
		}
		return;
	}

	if ( nNewCount > m_nCount )
	{
		TYPE * pNewData = reinterpret_cast<TYPE *>(new byte[ nNewCount * sizeof( TYPE ) ]);
		if ( m_nCount )
		{
			// copy the old stuff to the new array.
			memcpy( pNewData, m_pData, sizeof(TYPE)*m_nCount );
			delete[] reinterpret_cast<byte *>(m_pData);	// don't call any destructors.
		}

		// Just construct or init the new stuff.
		ConstructElements( pNewData + m_nCount, nNewCount - m_nCount );
		m_pData = pNewData;

		m_nRealCount = nNewCount;
	}

	m_nCount = nNewCount;

/*	if ( nNewCount < 0 )
		return;

	// shrink to nothing?
	if ( !nNewCount )
	{
		if ( m_nRealCount )
		{
			delete[] (byte*) m_pData;
			m_nCount = m_nRealCount = 0;
			m_pData = NULL;
		}
		return;
	}

	//	do we need to resize the array?
	if (( nNewCount > m_nRealCount ) || ( nNewCount < m_nRealCount-10 ))
	{
		m_nRealCount = nNewCount + 5;	// auto-allocate space for 5 extra elements
		TYPE * pNewData = (TYPE*) new byte[m_nRealCount * sizeof(TYPE)];

		// i have already data inside, so move to the new place
		if ( m_nCount )
			memcpy(pNewData, m_pData, sizeof(TYPE) * m_nCount);
		delete[] (byte*) m_pData;
		m_pData = pNewData;
	}

	//	construct new element
	if ( nNewCount > m_nCount )
	{
		ConstructElements(m_pData + m_nCount, nNewCount - m_nCount);
	}

	m_nCount = nNewCount;
*/
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


// CSPtrTypeArray:: Constructors, Destructor, Asign operator.

template<class TYPE>
CSPtrTypeArray<TYPE>::CSPtrTypeArray() {

}

template<class TYPE>
CSPtrTypeArray<TYPE>::~CSPtrTypeArray() {

}

// CSPtrTypeArray:: Modifiers.

template<class TYPE>
inline void CSPtrTypeArray<TYPE>::DestructElements( TYPE* pElements, size_t nCount )
{
	memset(pElements, 0, nCount * sizeof(*pElements));
}

template<class TYPE>
bool CSPtrTypeArray<TYPE>::RemovePtr( TYPE pData )
{
	size_t nIndex = FindPtr( pData );
	if ( nIndex == STANDARD_CPLUSPLUS_THIS(BadIndex()) )
		return false;

	ASSERT( IsValidIndex(nIndex) );
	STANDARD_CPLUSPLUS_THIS(RemoveAt(nIndex));
	return true;
}

template<class TYPE>
bool CSPtrTypeArray<TYPE>::ContainsPtr( TYPE pData ) const
{
	size_t nIndex = FindPtr(pData);
	ASSERT(nIndex == STANDARD_CPLUSPLUS_THIS(BadIndex()) || IsValidIndex(nIndex));
	return nIndex != STANDARD_CPLUSPLUS_THIS(BadIndex());
}

template<class TYPE>
size_t CSPtrTypeArray<TYPE>::FindPtr( TYPE pData ) const
{
	if ( !pData )
		return STANDARD_CPLUSPLUS_THIS(BadIndex());

	for ( size_t nIndex = 0; nIndex < STANDARD_CPLUSPLUS_THIS(GetCount()); nIndex++ )
	{
		if ( STANDARD_CPLUSPLUS_THIS(GetAt(nIndex)) == pData )
			return nIndex;
	}

	return STANDARD_CPLUSPLUS_THIS(BadIndex());
}

template<class TYPE>
bool CSPtrTypeArray<TYPE>::IsValidIndex( size_t i ) const
{
	if ( i >= STANDARD_CPLUSPLUS_THIS(GetCount()) )
		return false;
	return ( STANDARD_CPLUSPLUS_THIS(GetAt(i)) != NULL );
}

// CSObjArray:: Constructors, Destructor, Asign operator.

template<class TYPE>
CSObjArray<TYPE>::CSObjArray()
{
}

template<class TYPE>
inline CSObjArray<TYPE>::~CSObjArray()
{
	// Make sure the virtuals get called.
	STANDARD_CPLUSPLUS_THIS(SetCount(0));
}

// CSObjArray:: Modifiers.

template<class TYPE>
void CSObjArray<TYPE>::Clean(bool bElements)
{
	if ( bElements && STANDARD_CPLUSPLUS_THIS(GetRealCount()) > 0 )
		DestructElements( STANDARD_CPLUSPLUS_THIS(GetBasePtr()), STANDARD_CPLUSPLUS_THIS(GetRealCount()) );

	STANDARD_CPLUSPLUS_THIS(Empty());
}

template<class TYPE>
inline bool CSObjArray<TYPE>::DeleteOb( TYPE pData )
{
	return this->RemovePtr(pData);
}

template<class TYPE>
inline void CSObjArray<TYPE>::DeleteAt( size_t nIndex )
{
	STANDARD_CPLUSPLUS_THIS(RemoveAt(nIndex));
}

template<class TYPE>
void CSObjArray<TYPE>::DestructElements( TYPE* pElements, size_t nCount )
{
	// delete the objects that we own.
	for ( size_t i = 0; i < nCount; i++ )
	{
		if ( pElements[i] != NULL )
			delete pElements[i];
	}
	CSPtrTypeArray<TYPE>::DestructElements(pElements, nCount);
}

// CSObjSortArray:: Constructors, Destructor, Asign operator.

template<class TYPE,class KEY_TYPE>
CSObjSortArray<TYPE,KEY_TYPE>::CSObjSortArray()
{
}

template<class TYPE,class KEY_TYPE>
CSObjSortArray<TYPE,KEY_TYPE>::~CSObjSortArray()
{
}

// CSObjSortArray:: Modifiers.

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::AddPresorted( size_t index, int iCompareRes, TYPE pNew )
{
	if ( iCompareRes > 0 )
		index++;

	this->InsertAt(index, pNew);
	return index;
}

template<class TYPE, class KEY_TYPE>
size_t CSObjSortArray<TYPE, KEY_TYPE>::AddSortKey( TYPE pNew, KEY_TYPE key )
{
	// Insertion sort.
	int iCompareRes;
	size_t index = FindKeyNear(key, iCompareRes);
	if ( iCompareRes == 0 )
	{
		// duplicate should not happen ?!? DestructElements is called automatically for previous.
		this->SetAt(index, pNew);
		return index;
	}
	return AddPresorted(index, iCompareRes, pNew);
}

template<class TYPE,class KEY_TYPE>
inline void CSObjSortArray<TYPE,KEY_TYPE>::DeleteKey( KEY_TYPE key )
{
	DeleteAt(FindKey(key));
}

// CSObjSortArray:: Operations.

template<class TYPE,class KEY_TYPE>
inline bool CSObjSortArray<TYPE,KEY_TYPE>::ContainsKey( KEY_TYPE key ) const
{
	return FindKey(key) != STANDARD_CPLUSPLUS_THIS(BadIndex());
}

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::FindKey( KEY_TYPE key ) const
{
	// Find exact key
	int iCompareRes;
	size_t index = FindKeyNear(key, iCompareRes, false);
	return (iCompareRes != 0 ? STANDARD_CPLUSPLUS_THIS(BadIndex()) : index);
}

template<class TYPE, class KEY_TYPE>
size_t CSObjSortArray<TYPE, KEY_TYPE>::FindKeyNear( KEY_TYPE key, int & iCompareRes, bool fNoSpaces ) const
{

	// Do a binary search for the key.
	// RETURN: index
	//  iCompareRes =
	//		0 = match with index.
	//		-1 = key should be less than index.
	//		+1 = key should be greater than index
	//
	if ( STANDARD_CPLUSPLUS_THIS(GetCount()) <= 0 )
	{
		iCompareRes = -1;
		return 0;
	}

	size_t iHigh = STANDARD_CPLUSPLUS_THIS(GetCount()) - 1;
	size_t iLow = 0;
	size_t i = 0;

	while ( iLow <= iHigh )
	{
		i = (iHigh + iLow) >> 1;
		iCompareRes = CompareKey( key, STANDARD_CPLUSPLUS_THIS(GetAt(i)), fNoSpaces );
		if ( iCompareRes == 0 )
			break;
		if ( iCompareRes > 0 )
			iLow = i + 1;
		else if ( i == 0 )
			break;
		else
			iHigh = i - 1;
	}
	return i;
}


/* Inline Methods Definitions */

// CObjListRec:: Iterators.

CSObjList * CSObjListRec::GetParent() const
{
	return m_pParent;
}

CSObjListRec * CSObjListRec::GetNext() const
{
	return m_pNext;
}

CSObjListRec * CSObjListRec::GetPrev() const
{
	return m_pPrev;
}


#undef STANDARD_CPLUSPLUS_THIS

#endif	// _INC_CSARRAY_H
