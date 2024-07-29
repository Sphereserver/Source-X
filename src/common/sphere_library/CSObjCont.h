/**
* @file  CSObjCont.h
* @brief Generic container, meant to efficiently hold objects (not thread safe).
*/

#ifndef _INC_CSOBJCONT_H
#define _INC_CSOBJCONT_H

#include "CSObjContRec.h"
#include <vector>
#include <utility> // for std::move


/* Reverse container wrapper */

template <typename T>
class cont_reversed
{
    T _iterable;

public:
    explicit cont_reversed(T&& iterable) noexcept : _iterable{ std::move(iterable) } {}

    inline auto begin() const noexcept  { return std::rbegin(_iterable); }
    inline auto end() const noexcept    { return std::rend(_iterable);   }
};


template <typename T>
class cont_reverse_iterate
{
    T & _iterable;

public:
    explicit cont_reverse_iterate(T &iterable) noexcept : _iterable{ iterable } {}

    inline auto begin() const noexcept  { return std::rbegin(_iterable); }
    inline auto end() const noexcept    { return std::rend(_iterable); }
};



/* CSObjCont */

#define BASECONT std::vector<CSObjContRec*>
class CSObjCont
{
protected:
    BASECONT _Contents;
    bool _fClearingContainer;

public:
    friend class CSObjContRec;
    static const char * m_sClassName;

    /** @name Constructors, Destructor, Asign operator:
    */
    ///@{
    /**
    * @brief Sets head, tail and count.
    */
    CSObjCont();
    virtual ~CSObjCont();

    /**
    * @brief No copies allowed.
    */
    CSObjCont(const CSObjCont& copy) = delete;
    /**
    * @brief No copies allowed.
    */
    CSObjCont& operator=(const CSObjCont& other) = delete;
    ///@}

    /** @name Iterators:
    */
    ///@{

public:
    using iterator               = BASECONT::iterator;
    using const_iterator         = BASECONT::const_iterator;
    using reverse_iterator       = BASECONT::reverse_iterator;
    using const_reverse_iterator = BASECONT::const_reverse_iterator;

    inline iterator begin() noexcept                        { return _Contents.begin();  }
    inline iterator end()   noexcept                        { return _Contents.end();    }
    inline const_iterator begin()  const noexcept           { return _Contents.begin();  }
    inline const_iterator end()    const noexcept           { return _Contents.end();    }
    inline const_iterator cbegin() const noexcept           { return _Contents.cbegin(); }
    inline const_iterator cend()   const noexcept           { return _Contents.cend();   }

    inline reverse_iterator rbegin() noexcept               { return _Contents.rbegin(); }
    inline reverse_iterator rend()   noexcept               { return _Contents.rend();   }
    inline const_reverse_iterator rbegin()  const noexcept  { return _Contents.rbegin(); }
    inline const_reverse_iterator rend()    const noexcept  { return _Contents.rend();   }

    inline size_t size() const noexcept                     { return _Contents.size();   }
    inline const CSObjContRec *const * data() const noexcept{ return _Contents.data();   }

    inline BASECONT::iterator erase(BASECONT::iterator it)  { return _Contents.erase(it); }

    /**
    * @brief Returns a copy of the CSObjCont base container, which is safe to iterate on even if one of its elements is ::Delete'd.
    *   When an element is deleted from the CSObjCont with CObjBase::Delete or its virtuals, the element is removed from the CSObjCont, thus invalidating
    *   its existing iterators, but the removed element is only scheduled for deletion on the next tick, so the object does still exist by the time we are
    *   iterating on this container.
    *   So, if we iterate (on the same tick and without using "delete" on the elements) on another copy container, which holds the same pointers to CSObjListRec
    *   but without being modified during the iteration, we are safe and prevent the invalidation of the existing iterators, loop indexes and such.
    * @return A copy of the base container.
    */
    inline BASECONT GetIterationSafeCont() const noexcept;

    /**
    * @brief Returns a reverse-iterated copy of the CSObjCont base container, which is safe to iterate on even if one of its elements is ::Delete'd.
    *   See GetIterationSafeContReverse() for more info.
    *   As a general rule of thumb, whenever possible it's better to iterate through the elements in reverse order, in the case of the raw CSObjCont and of the
    *   BASECONT copy returned by GetIterationSafeCont. This happens because if an element gets deleted, it will be erased in any case from the CSObjCont, and
    *   removing the last element is more efficient than removing the first element (since we are using std::vector as BASECONT).
    * @return A copy of the base container, which will be iterated in reverse order.
    */
    inline cont_reversed<BASECONT> GetIterationSafeContReverse() const noexcept;

    ///@}

    /** @name Capacity:
    */
    ///@{

    /**
    * @brief Check if CSObjCont if empty.
    * @return true if CSObjCont is empty, false otherwise.
    */
    inline bool IsContainerEmpty() const noexcept;

    /**
    * @brief Get the record count of the list.
    * @return The record count of the list.
    */
    inline size_t GetContentCount() const noexcept;
    ///@}

    /** @name Element Access:
    */
    ///@{

    /**
    * @brief Get the nth element of the list.
    * @param index of the element to get.
    * @return nth element if length is greater or equal to index, nullptr otherwise.
    */
    inline CSObjContRec* GetContentIndex(size_t index) const noexcept;

    /**
    * @brief Get the first record of the CSObjCont.
    * @return The first record of the CSObjCont if list is not empty, nullptr otherwise.
    */
    inline CSObjContRec* GetContainerHead() const noexcept;

    /**
    * @brief Get the last record of the CSObjCont.
    * @return The last record of the CSObjCont if list is not empty, nullptr otherwise.
    */
    inline CSObjContRec* GetContainerTail() const noexcept;


    /** @name Modifiers:
    */
    ///@{

    /**
    * @brief Remove all records of the CSObjCont.
    * @param fClosingWorld Am i closing world UIDs or just destroyng everything before shutdown?
    */
    void ClearContainer(bool fClosingWorld);

    /**
    * @brief Insert a record at head.
    * @param pNewRec record to insert.
    */
    //void InsertContentHead(CSObjContRec* pNewRec);

    /**
    * @brief Insert a record at tail.
    * @param pNewRec record to insert.
    */
    void InsertContentTail(CSObjContRec* pNewRec);

protected:
    /**
    * @brief Trigger that fires when a record if removed.
    *
    * Override this to get called when an item is removed from this list.
    * Never called directly. Called CSObjContRec::RemoveSelf()
    * @see CSObjContRec::RemoveSelf().
    *
    * @param pObRec removed record.
    */
    virtual void OnRemoveObj( CSObjContRec* pObjRec );
    ///@}
};


/* Inlined methods are defined here */

BASECONT CSObjCont::GetIterationSafeCont() const noexcept
{
    return _Contents;   // Return a copy of the base container
}

cont_reversed<BASECONT> CSObjCont::GetIterationSafeContReverse() const noexcept
{
    return cont_reversed<BASECONT>(BASECONT(_Contents)); // Return a reverse-iterable copy of the base container
}

bool CSObjCont::IsContainerEmpty() const noexcept
{
    return _Contents.empty();
}

size_t CSObjCont::GetContentCount() const noexcept
{
    return _Contents.size();
}

CSObjContRec* CSObjCont::GetContentIndex(size_t i) const noexcept
{
    return _Contents[i];
}

CSObjContRec* CSObjCont::GetContainerHead() const noexcept
{
    return _Contents.front();
}

CSObjContRec* CSObjCont::GetContainerTail() const noexcept
{
    return _Contents.back();
}

#undef BASECONT

#endif //_INC_CSOBJCONT_H
