/**
* @file CSObjSortArray.h
* @brief Array of objects (sorted) (not thread safe).
*
* The point of this type is that the array now OWNS the element.
* It will get deleted when the array is deleted.
*/

#ifndef _INC_CSOBJSORTARRAY_H
#define _INC_CSOBJSORTARRAY_H

#include "CSObjArray.h"

template<class TYPE,class KEY_TYPE>
class CSObjSortArray : public CSObjArray<TYPE>
{
	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
public:
    CSObjSortArray() = default;
    virtual ~CSObjSortArray() override = default;
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

// CSObjSortArray:: Modifiers.

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::AddPresorted( size_t index, int iCompareRes, TYPE pNew )
{
	if ( iCompareRes > 0 )
		++index;

	this->insert(index, pNew);
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
		this->operator[](index) = pNew;
		return index;
	}
	return AddPresorted(index, iCompareRes, pNew);
}

template<class TYPE,class KEY_TYPE>
inline void CSObjSortArray<TYPE,KEY_TYPE>::DeleteKey( KEY_TYPE key )
{
	erase(FindKey(key));
}

// CSObjSortArray:: Operations.

template<class TYPE,class KEY_TYPE>
inline bool CSObjSortArray<TYPE,KEY_TYPE>::ContainsKey( KEY_TYPE key ) const
{
	return FindKey(key) != sl::scont_bad_index();
}

template<class TYPE,class KEY_TYPE>
size_t CSObjSortArray<TYPE,KEY_TYPE>::FindKey( KEY_TYPE key ) const
{
	// Find exact key
	int iCompareRes;
	size_t index = FindKeyNear(key, iCompareRes, false);
	return (iCompareRes != 0 ? sl::scont_bad_index() : index);
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

    size_t iHigh = this->size();
    if (iHigh == 0)
    {
        iCompareRes = -1;
        return 0;
    }

    --iHigh;
    int iLocalCompareRes = -1;
    //const TYPE *pData = this->data();
    size_t iLow = 0;
    size_t i = 0;
    while ( iLow <= iHigh )
    {
        i = (iHigh + iLow) >> 1;
        iLocalCompareRes = CompareKey( key, this->operator[](i), fNoSpaces );
        //iLocalCompareRes = CompareKey( key, pData[i], fNoSpaces );
        if ( iLocalCompareRes == 0 ) {
            iCompareRes = iLocalCompareRes;
            return i;
        }
        if ( iLocalCompareRes > 0 ) {
            iLow = i + 1;
        }
        else if ( i == 0 ) {
            iCompareRes = iLocalCompareRes;
            return 0;
        }
        else {
            iHigh = i - 1;
        }
    }
    iCompareRes = iLocalCompareRes;
    return i;

    // This doesn't work... Requires further investigation. I wonder if it would even be more efficient then the algorithm above
    /*
    size_t iHigh = this->size();
	if (iHigh <= 0 )
	{
		iCompareRes = -1;
		return 0;
	}

    int iLocalCompareRes;
	size_t iLow = 0;
	while ( iLow < iHigh )
	{
		const size_t i = iLow + ((iHigh - iLow) >> 1);
		iLocalCompareRes = CompareKey( key, this->operator[](i), fNoSpaces );
        //iLocalCompareRes = CompareKey( key, pData[i], fNoSpaces );
		if ( iLocalCompareRes == 0 )
        {
            iCompareRes = iLocalCompareRes;
            return i;
        }
		if ( iLocalCompareRes > 0 )
            iHigh = i;
		else
            iLow = i + 1;
	}
    iCompareRes = iLocalCompareRes;
	return iHigh;
    */
}


#endif	// _INC_CSARRAY_H
