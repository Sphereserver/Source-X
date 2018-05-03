/**
* @file  CSPtrTypeArray.h
* @brief An Array of pointers (not thread safe).
*/

#ifndef _INC_CSPTRTYPEARRAY_H
#define _INC_CSPTRTYPEARRAY_H

#include "CSTypedArray.h"

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


#endif // _INC_CSPTRTYPEARRAY_H

