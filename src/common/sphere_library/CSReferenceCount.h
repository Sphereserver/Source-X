#ifndef _INC_CSREFERENCECOUNT_H
#define _INC_CSREFERENCECOUNT_H

#include <type_traits>

template <typename T, typename ...ConstructorParams>
class CSReferenceCountedOwned;

template <typename T>
class CSReferenceCounted
{
    friend class CSReferenceCountedOwned<T>;
    CSReferenceCountedOwned<T> * _owner;

protected:
    /*
    CSReferenceCounted(CSReferenceCounter* owner, ConstructorParams&& ...params) noexcept :
        _owner(owner), _heldObj(std::forward<ConstructorParams>(params)...)
    {
        _owner->_counted_references += 1;
    }
    */
    CSReferenceCounted(CSReferenceCountedOwned<T>* owner) noexcept :
        _owner(owner)
    {
        _owner->_counted_references += 1;
    }

public:
    CSReferenceCounted(CSReferenceCounted const& other) noexcept :
        _owner(other._owner)
    {
        _owner->_counted_references += 1;
    }

    ~CSReferenceCounted() noexcept
    {
        _owner->_counted_references -= 1;
    }

    CSReferenceCounted& operator=(CSReferenceCounted const& other) noexcept
    {
        _owner = other._owner;
        _owner->_counted_references += 1;
        return *this;
    }

    T& operator*() noexcept
    {
        return _owner->_heldObj;
    }
    T* operator->() noexcept
    {
        return &_owner->_heldObj;
    }
};


template <typename T, typename ...ConstructorParams>
class CSReferenceCountedOwned
{
public:
    friend class CSReferenceCountedOwned;
    T _heldObj;
    unsigned int _counted_references;

    CSReferenceCountedOwned(ConstructorParams&& ...params) noexcept :
        _counted_references(1), _heldObj(std::forward<ConstructorParams>(params)...)
    {
    }
    CSReferenceCountedOwned(CSReferenceCountedOwned&& other) noexcept = default;
    ~CSReferenceCountedOwned() noexcept = default;

    CSReferenceCountedOwned(CSReferenceCountedOwned const& other) noexcept = delete;
    CSReferenceCountedOwned& operator=(CSReferenceCountedOwned const& other) noexcept = delete;

    T& operator*() noexcept
    {
        return _heldObj;
    }
    T* operator->() noexcept
    {
        return &_heldObj;
    }

    auto GetRef() noexcept
    {
        return CSReferenceCounted<T>(this);
    }
};


#endif  // _INC_CSREFERENCECOUNT_H
