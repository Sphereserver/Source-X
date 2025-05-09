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
protected:
    bool _fBaseDestructorShouldDeleteElements;
    virtual void DestroyElements() noexcept;

public:
    static const char *m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
public:
    CSObjArray() noexcept :
        _fBaseDestructorShouldDeleteElements(true)
    {}
    virtual ~CSObjArray() noexcept {
        ClearFree();
    }

    /**
    * @brief No copy on construction allowed.
    */
    CSObjArray(const CSObjArray<TYPE> & copy) = delete;
    /**
    * @brief No copy allowed.
    */
    CSObjArray<TYPE> & operator=(const CSObjArray<TYPE> & other) = delete;
    ///@}

    /** @name Modifiers:
    */
    ///@{
public:
    /**
   * @brief Check if an index is between 0 and element count.
   * @param i index to check.
   * @return true if index is valid, false otherwise.
   */
    bool IsValidIndex(size_t i) const;

    void clear() = delete;
    virtual void ClearFree();

    ///@}
};


/* Template methods (inlined or not) are defined here */

// CSObjArray:: Modifiers.

//#include "../../sphere/threads.h"

template<class TYPE>
void CSObjArray<TYPE>::DestroyElements() noexcept
{
    if (!this->_fBaseDestructorShouldDeleteElements)
        return;
    for (TYPE& elem : *this) {
        delete elem;
        elem = nullptr;
    }
}

template<class TYPE>
bool CSObjArray<TYPE>::IsValidIndex(size_t i) const
{
    return ((i < this->size()) && ((*this)[i] != nullptr));
}

template<class TYPE>
void CSObjArray<TYPE>::ClearFree()
{
    DestroyElements();
    this->erase(this->begin(), this->end());
}

#endif //_INC_CSOBJARRAY_H
