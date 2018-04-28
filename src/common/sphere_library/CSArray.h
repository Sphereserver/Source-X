/**
* @file CSArray.h
* @brief C++ collections custom implementation.
*/

#ifndef _INC_CSARRAY_H
#define _INC_CSARRAY_H

//#include "../common.h"
//#include "CSArray_CSTypedArray_ptr.hh" // It doesn't work yet!
#include "CSArray_CSTypedArray.hh"

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
	virtual inline ~CSObjListRec() {
		RemoveSelf();
	}
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
	CSObjList * m_pParent;	// Parent list.
	CSObjListRec * m_pNext;  // Next record.
	CSObjListRec * m_pPrev;  // Prev record.
};

/**
* @brief Generic list of objects (not thread safe).
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
	virtual ~CSObjList() {
		Clear();
	}

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
	inline size_t GetCount() const {
		return m_iCount;
	}
	/**
    * @brief Check if CSObjList if empty.
    * @return true if CSObjList is empty, false otherwise.
    */
	inline bool IsEmpty() const {
		return !GetCount();
	}
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
	inline CSObjListRec * GetHead() const {
		return m_pHead;
	}
	/**
    * @brief Get the last record of the CSObjList.
    * @return The last record of the CSObjList if list is not empty, NULL otherwise.
    */
	inline CSObjListRec * GetTail() const {
		return m_pTail;
	}
	///@}
	/** @name Modifiers:
	 */
	///@{
	/**
    * @brief Remove all records of the CSObjList.
    */
	void Clear();
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
	inline void InsertHead( CSObjListRec * pNewRec ) {
		InsertAfter(pNewRec, NULL);
	}
	/**
    * @brief Insert a record at tail.
    * @param pNewRec record to insert.
    */
	inline void InsertTail( CSObjListRec * pNewRec ) {
		InsertAfter(pNewRec, GetTail());
	}
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
	///@}
private:

	CSObjListRec * m_pHead;  // Head of the list.
	CSObjListRec * m_pTail;  // Tail of the list. Do we really care about tail ? (as it applies to lists anyhow)
	size_t m_iCount;		// Count of elements of the CSObjList.
};


/**
* @brief An Array of pointers (not thread safe).
*/
template<class TYPE>
class CSPtrTypeArray : public CSTypedArray<TYPE, TYPE>
{
public:
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	CSPtrTypeArray() {}
	virtual ~CSPtrTypeArray() {}
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
* @brief Array of objects (not thread safe).
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
	CSObjArray() {}
	virtual ~CSObjArray() {}
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
    * @brief Remove an element if exists in the array.
    * @param pData data to remove.
    * @return true if data is removed, false otherwise.
    */
	bool DeleteObj( TYPE pData );
	///@}
};

/**
* @brief Array of objects (sorted) (not thread safe).
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
	CSObjSortArray() {}
	virtual ~CSObjSortArray() {}
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



/* Template methods (inlined or not) are defined here */


// CSObjListRec:: Capacity.

inline void CSObjListRec::RemoveSelf()
{
	if (GetParent())
		m_pParent->OnRemoveObj(this);	// call any approriate virtuals.
}


// CSPtrTypeArray:: Modifiers.

template<class TYPE>
bool CSPtrTypeArray<TYPE>::RemovePtr( TYPE pData )
{
	size_t nIndex = FindPtr( pData );
	if ( nIndex == this->BadIndex() )
		return false;

	ASSERT( IsValidIndex(nIndex) );
    this->RemoveAt(nIndex);
	return true;
}

template<class TYPE>
bool CSPtrTypeArray<TYPE>::ContainsPtr( TYPE pData ) const
{
	size_t nIndex = FindPtr(pData);
	ASSERT((nIndex == this->BadIndex()) || IsValidIndex(nIndex));
	return nIndex != this->BadIndex();
}

template<class TYPE>
size_t CSPtrTypeArray<TYPE>::FindPtr( TYPE pData ) const
{
	if ( !pData )
		return this->BadIndex();

	for ( size_t nIndex = 0; nIndex < this->GetCount(); ++nIndex )
	{
		if ( this->GetAt(nIndex) == pData )
			return nIndex;
	}

	return this->BadIndex();
}

template<class TYPE>
bool CSPtrTypeArray<TYPE>::IsValidIndex( size_t i ) const
{
	if ( i >= this->GetCount() )
		return false;
	return ( this->GetAt(i) != NULL );
}


// CSObjArray:: Modifiers.

template<class TYPE>
inline bool CSObjArray<TYPE>::DeleteObj( TYPE pData )
{
	return this->RemovePtr(pData);
}


// CSObjSortArray:: Modifiers.

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::AddPresorted( size_t index, int iCompareRes, TYPE pNew )
{
	if ( iCompareRes > 0 )
		++index;

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
		// duplicate should not happen ?!?
        this->SetAt(index, pNew);
		return index;
	}
	return AddPresorted(index, iCompareRes, pNew);
}

template<class TYPE,class KEY_TYPE>
inline void CSObjSortArray<TYPE,KEY_TYPE>::DeleteKey( KEY_TYPE key )
{
	RemoveAt(FindKey(key));
}

// CSObjSortArray:: Operations.

template<class TYPE,class KEY_TYPE>
inline bool CSObjSortArray<TYPE,KEY_TYPE>::ContainsKey( KEY_TYPE key ) const
{
	return FindKey(key) != this->BadIndex();
}

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::FindKey( KEY_TYPE key ) const
{
	// Find exact key
	int iCompareRes;
	size_t index = FindKeyNear(key, iCompareRes, false);
	return (iCompareRes != 0 ? this->BadIndex() : index);
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
	if ( this->GetCount() <= 0 )
	{
		iCompareRes = -1;
		return 0;
	}

	size_t iHigh = this->GetCount() - 1;
	size_t iLow = 0;
	size_t i = 0;

	while ( iLow <= iHigh )
	{
		i = (iHigh + iLow) >> 1;
		iCompareRes = CompareKey( key, this->GetAt(i), fNoSpaces );
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


#endif	// _INC_CSARRAY_H
