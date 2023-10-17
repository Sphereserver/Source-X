#ifndef _SPTR_CONTAINERS_H
#define _SPTR_CONTAINERS_H

#include "CSSortedVector.h"
#include <algorithm>
#include <memory>


//-- Base types for a standard vector or sorted vector of smart pointers
template <typename _Type, typename _PtrWrapperType>
class _CSPtrVectorBase : public std::vector<_PtrWrapperType>
{
public:
    using _base_type = std::vector<_PtrWrapperType>;
    using iterator = typename _base_type::iterator;

    inline void erase_index(const size_t index) {
        _base_type::erase(_base_type::begin() + index);
    }

    void erase_element(_Type* elem) {
        size_t idx = 0;
        for (_PtrWrapperType const& ptr : *this) {
            if (ptr.get() == elem) {
                this->erase_index(idx);
                return;
            }
            ++idx;
        }
    }

    inline bool valid_index(size_t index) const noexcept {
        if constexpr (std::is_same_v<_PtrWrapperType, std::weak_ptr<_Type>>) {
            return (index < _base_type::size()) && !(_base_type::operator[](index).expired());
        }
        else {
            return (index < _base_type::size()) && static_cast<bool>(_base_type::operator[](index));
        }
    }
};

template <typename _Type, typename _PtrWrapperType, typename _Comp = std::less<_Type>>
class _CSPtrSortedVectorBase : public CSSortedVector<_PtrWrapperType, _Comp>
{
public:
    using _base_type = CSSortedVector<_PtrWrapperType>;
    using iterator = typename _base_type::iterator;

    inline bool valid_index(size_t index) const noexcept {
        if constexpr (std::is_same_v<_PtrWrapperType, std::weak_ptr<_Type>>) {
            return (index < _base_type::size()) && !(_base_type::operator[](index).expired());
        }
        else {
            return (index < _base_type::size()) && static_cast<bool>(_base_type::operator[](index));
        }
    }
};


// -- Vectors of pointers, wrapped in unique pointers

template <typename _Type>
class CSUniquePtrVector : public _CSPtrVectorBase<_Type, std::unique_ptr<_Type>>
{
public:
    template <typename _PtrType = std::unique_ptr<_Type>>  // Gets both bare pointer and unique_ptr
    void emplace_index_grow(size_t index, _PtrType value) {
        if (index >= this->size()) {
            this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
        }
        if (!value || value.get() == nullptr)
            this->operator[](index).reset();
        else
            this->operator[](index) = value;
    }

    inline void emplace_index_grow(size_t index, std::nullptr_t) {
        this->emplace_index_grow(index, std::unique_ptr<_Type>());
    }

    template <typename... _ArgPackType>
    inline void emplace_front(_ArgPackType&&... args) {
        _base_type::emplace(_base_type::cbegin(), std::forward<_ArgPackType>(args)...);
    }

    // Explicitly create a unique_ptr, then add to this container.
    //void emplace_index_grow(size_t, _Type*) = delete;

    /*
    template <typename... _ArgType>
    void emplace_index_grow(size_t index, _ArgType&&... value) {
        if (index >= this->size()) {
            this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
        }
        this->operator[](index) = std::move(std::make_unique<std::remove_reference_t<_Type>, _ArgType&&...>
            (std::forward<_ArgType>(value)...));
    }
    */

    /*
    // An old attempt with good ideas
    template <typename... _ArgType>
    void emplace_index_grow(size_t index, _ArgType&&... value) {
        if (index >= this->size()) {
            this->resize(index); // The capacity will be even greater, since it calls vector::resize
        }
        if constexpr (sizeof...(value) == 1) {
            using _FirstPackedType = std::tuple_element_t<0, std::tuple<_ArgType...>>;
            if constexpr (std::is_pointer_v<std::remove_reference_t<_FirstPackedType>>) {
                auto _FirstPackedValue = std::get<0>(std::tie(value...));
                this->operator[](index).reset(std::forward<_Type*>(_FirstPackedValue));
            }
        }
        else {
        this->operator[](index) = std::move(std::make_unique<_Type>(std::forward<_ArgType>(value)...));
        }
    }
    */
};
template <typename _Type, typename _Comp = std::less<_Type>>
class CSUniquePtrSortedVector : public _CSPtrSortedVectorBase<_Type, std::unique_ptr<_Type>, _Comp>
{};


// -- Vectors of pointers, wrapped in shared pointers
template <typename _Type>
class CSSharedPtrVector : public _CSPtrVectorBase<_Type, std::shared_ptr<_Type>>
{
public:
    template <typename _PtrType = std::shared_ptr<_Type>>  // Gets both bare pointer and shared_ptr
    void emplace_index_grow(size_t index, _PtrType value) {
        if (index >= this->size()) {
            this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
        }
        if (!value || value.get() == nullptr)
            this->operator[](index).reset();
        else
            this->operator[](index) = value;
    }

    inline void emplace_index_grow(size_t index, std::nullptr_t) {
        this->emplace_index_grow(index, std::shared_ptr<_Type>());
    }

    template <typename... _ArgPackType>
    inline void emplace_front(_ArgPackType&&... args) {
        _base_type::emplace(_base_type::cbegin(), std::forward<_ArgPackType>(args)...);
    }

    // Explicitly create a unique_ptr, then add to this container.
    //void emplace_index_grow(size_t, _Type*) = delete;

    /*
    template <typename... _ArgType>
    void emplace_index_grow(size_t index, _ArgType&&... value) {
        if (index >= this->size()) {
            this->resize(index+1); // The capacity will be even greater, since it calls vector::resize
        }
        this->operator[](index) = std::move(std::make_shared<std::remove_reference_t<_Type>, _ArgType&&...>
            (std::forward<_ArgType>(value)...));
    }*/
};
template <typename _Type, typename _Comp = std::less<_Type>>
class CSSharedPtrSortedVector : public _CSPtrSortedVectorBase<_Type, std::shared_ptr<_Type>, _Comp>
{};


// -- Vectors of pointers, wrapped in weak pointers

template <typename _Type>
class CSWeakPtrVector : public _CSPtrVectorBase<_Type, std::weak_ptr<_Type>>
{
public:
    template <typename _ArgType>
    void emplace_index_grow(size_t index, _ArgType&& value) {
        if (index >= this->size()) {
            this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
        }
        this->operator[](index) = std::forward<_ArgType>(value);
    }

    void emplace_index_grow(size_t index, std::nullptr_t) {
        if (index >= this->size()) {
            this->resize(index); // The capacity will be even greater, since it calls vector::resize
        }
        else {
            this->operator[](index).reset();
        }
    }

    template <typename... _ArgPackType>
    inline void emplace_front(_ArgPackType&&... args) {
        _base_type::emplace(_base_type::cbegin(), std::forward<_ArgPackType>(args)...);
    }
};
template <typename _Type, typename _Comp = std::less<_Type>>
class CSWeakPtrSortedVector : public _CSPtrSortedVectorBase<_Type, std::weak_ptr<_Type>, _Comp>
{};


#endif // !_SPTR_CONTAINERS_H
