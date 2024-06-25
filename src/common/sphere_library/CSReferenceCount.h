/**
* @file CSReferenceCount.h
* @brief Lightweight wrapped pointer reference counter to a master/owner object.
*/

#ifndef _INC_CSREFERENCECOUNT_H
#define _INC_CSREFERENCECOUNT_H

#include <type_traits>
#include <utility>
//#include <variant>

template <typename T>
class CSReferenceCountedOwned;
//template <typename T>
//class CSReferenceCountedVariant;

// This one is a reference to a CSReferenceCountedOwner.
// CSReferenceCountedOwner holds the original object/value and the number of child references.
// The CSReferenceCountedOwner object must not be moved in memory, it will invalidate _owner address.
template <typename T>
class CSReferenceCounted
{
    friend class CSReferenceCountedOwned<T>;
    CSReferenceCountedOwned<T> * _owner;

protected:
    /*
    template <typename ...ConstructorParams>
    CSReferenceCounted(CSReferenceCounter* owner, ConstructorParams&& ...params) noexcept :
        _owner(owner), _heldObj(std::forward<ConstructorParams>(params)...)
    {
        _owner->_counted_references += 1;
    }
    */

    explicit CSReferenceCounted(CSReferenceCountedOwned<T>* owner) noexcept :
        _owner(owner)
    {
        _owner->_counted_references += 1;
    }

public:
    CSReferenceCounted() noexcept = delete;
    explicit CSReferenceCounted(CSReferenceCounted const& other) noexcept :
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

    inline T& get() noexcept
    {
        return _owner->_heldObj;
    }

    T& operator*() noexcept
    {
        return this->get();
    }
    T* operator->() noexcept
    {
        return &(this->get());
    }
};


template <typename T>
class CSReferenceCountedOwned
{
    //friend class CSReferenceCountedVariant<T>;
    //CSReferenceCountedOwned(CSReferenceCountedOwned const& other) noexcept = default;
    //CSReferenceCountedOwned& operator=(CSReferenceCountedOwned const& other) noexcept = default;
public:
    CSReferenceCountedOwned(CSReferenceCountedOwned const& other) noexcept = delete;
    CSReferenceCountedOwned& operator=(CSReferenceCountedOwned const& other) noexcept = delete;


public:
    unsigned int _counted_references;
    T _heldObj;

    CSReferenceCountedOwned() noexcept :
        _counted_references(1), _heldObj()
    {}

    //template <typename ...ConstructorParams>
    //CSReferenceCountedOwned(ConstructorParams&& ...params) noexcept requires (sizeof...(ConstructorParams) > 0) :
    //    _counted_references(1), _heldObj(std::forward<ConstructorParams>(params)...)
    //{
    //}
    ~CSReferenceCountedOwned() noexcept = default;

    CSReferenceCountedOwned(CSReferenceCountedOwned&& other) noexcept = default;
    CSReferenceCountedOwned& operator=(CSReferenceCountedOwned&& other) noexcept = default;

    inline T& get() noexcept
    {
        return _heldObj;
    }

    T& operator*() noexcept
    {
        return this->get();
    }
    T* operator->() noexcept
    {
        return &(this->get());
    }

    auto GetRef() noexcept
    {
        return CSReferenceCounted<T>(this);
    }
};

/*
Not yet working

template <typename T>
class CSReferenceCountedVariant : protected std::variant<CSReferenceCounted<T>, CSReferenceCountedOwned<T>>
{
    using base = typename std::variant<CSReferenceCounted<T>, CSReferenceCountedOwned<T>>;
    using base::base;
    //using base::operator=;

public:
    CSReferenceCountedVariant() : base() {}
    CSReferenceCountedVariant(CSReferenceCounted<T>&& arg) : base(std::in_place_index<0>, std::move(arg)) {}
    CSReferenceCountedVariant(CSReferenceCountedOwned<T>&& arg) : base(std::in_place_index<1>, std::move(arg)) {}

    //template <typename ..._Args>
    //explicit CSReferenceCountedVariant(_Args&&... args) requires (sizeof...(_Args) > 0) :
    //    base(std::forward(args)...) {}

    CSReferenceCountedVariant(CSReferenceCountedVariant && other)
    {
        base::operator=(std::move(other));
    }
    CSReferenceCountedVariant& operator=(CSReferenceCountedVariant && other)
    {
        base::operator=(std::move(other));
        return *this;
    }

    CSReferenceCountedVariant(CSReferenceCountedVariant const& other) = delete;
    CSReferenceCountedVariant& operator=(CSReferenceCountedVariant const& other) = delete;

    T& operator*()
    {
        if (std::holds_alternative<CSReferenceCounted<T>>(*this))
        {
            return std::get_if<CSReferenceCounted<T>>(this)->get();
        }
        else if (std::holds_alternative<CSReferenceCountedOwned<T>>(*this))
        {
            return std::get_if<CSReferenceCountedOwned<T>>(this)->get();
        }
        else
        {
            throw std::bad_variant_access();
        }
    }
    T* operator->()
    {
        if (std::holds_alternative<CSReferenceCounted<T>>(*this))
        {
            return &(std::get_if<CSReferenceCounted<T>>(this)->get());
        }
        else if (std::holds_alternative<CSReferenceCountedOwned<T>>(*this))
        {
            return &(std::get_if<CSReferenceCountedOwned<T>>(this)->get());
        }
        else
        {
            throw std::bad_variant_access();
        }
    }
};

*/

#endif  // _INC_CSREFERENCECOUNT_H
