/**
* @brief Array of objects (not thread safe).
*
* The point of this type is that the array now OWNS the element.
* It will get deleted when the array is deleted.
*/

#ifndef _INC_CSOBJARRAY_H
#define _INC_CSOBJARRAY_H

#include "CSPtrTypeArray.h"

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
    inline bool DeleteObj( TYPE pData );
    ///@}
};


/* Template methods (inlined or not) are defined here */

// CSObjArray:: Modifiers.

template<class TYPE>
bool CSObjArray<TYPE>::DeleteObj( TYPE pData )
{
    return this->RemovePtr(pData);
}

#endif //_INC_CSOBJARRAY_H
