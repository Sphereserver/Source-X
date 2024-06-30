#ifndef _INC_SPTR_CONTAINERS_H
#define _INC_SPTR_CONTAINERS_H

#include "ssorted_vector.h"
#include "sptr.h"
#include <algorithm> // for std::find_if


// Sphere library
namespace sl
{
    //-- Base types for a standard vector or sorted vector of smart pointers
    template <typename _Type, typename _PtrWrapperType>
    class _ptr_vector_base : public std::vector<_PtrWrapperType>
    {
    public:
        using base_type = typename std::vector<_PtrWrapperType>;
        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;
        using size_type = typename base_type::size_type;

        inline void erase_index(const size_t index) {
            this->erase(this->begin() + index);
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

        const_iterator find_ptr(_Type const * const elem) const {
            return std::find_if(base_type::cbegin(), base_type::cend(),
                [elem](auto const& stored) {
                    return (stored.get() == elem);
                });
        }

        inline bool has_ptr(_Type const* const elem) const {
            return (base_type::cend() != this->find_ptr(elem));
        }

        void remove_ptr(_Type* elem) {
            const size_t uiFoundIndex = this->find_ptr(elem);
            if (uiFoundIndex != sl::scont_bad_index())
                this->remove_index(uiFoundIndex);
        }

        inline bool valid_index(size_t index) const noexcept {
            if constexpr (std::is_same_v<_PtrWrapperType, std::weak_ptr<_Type>>) {
                return (index < this->size()) && !(this->operator[](index).expired());
            }
            else {
                return (index < this->size()) && static_cast<bool>(this->operator[](index));
            }
        }
    };

    template <typename _Type, typename _PtrWrapperType, typename _Comp = std::less<_Type>>
    class _ptr_sorted_vector_base : public sorted_vector<_PtrWrapperType, _Comp>
    {
    public:
        using base_type = typename std::vector<_PtrWrapperType>;
        using iterator = typename base_type::iterator;
        using const_iterator = typename base_type::const_iterator;
        using size_type = typename base_type::size_type;

        const_iterator find_ptr(_Type const* const elem) const {
            return this->cbegin() + this->find_predicate(elem,
                [](auto const& stored, _Type const* const elem) {
                    return (stored.get() == elem);
                });
        }

        void remove_ptr(_Type* elem) {
            const size_t uiFoundIndex = this->find(elem);
            if (uiFoundIndex != sl::scont_bad_index())
                this->remove_index(uiFoundIndex);
        }

        inline bool has_ptr(_Type const* const elem) const {
            return (sl::scont_bad_index() != this->find_ptr(elem));
        }

        inline bool valid_index(size_t index) const noexcept {
            if constexpr (std::is_same_v<_PtrWrapperType, std::weak_ptr<_Type>>) {
                return (index < this->size()) && !(this->operator[](index).expired());
            }
            else {
                return (index < this->size()) && static_cast<bool>(this->operator[](index));
            }
        }
    };


    // -- Vectors of pointers, wrapped in unique pointers

    template <typename _Type>
    class unique_ptr_vector : public _ptr_vector_base<_Type, std::unique_ptr<_Type>>
    {
    public:
        template <typename _PtrType = std::unique_ptr<_Type>>  // Gets both bare pointer and unique_ptr
        void emplace_index_grow(size_t index, _PtrType&& value) {
            if (index >= this->size()) {
                this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
            }
            if (!value || value.get() == nullptr)
                this->operator[](index).reset();
            else
                this->operator[](index) = std::move(value);
        }

        inline void emplace_index_grow(size_t index, std::nullptr_t) {
            this->emplace_index_grow(index, std::unique_ptr<_Type>());
        }

        template <typename... _ArgPackType>
        inline void emplace_front(_ArgPackType&&... args) {
            this->emplace(this->cbegin(), std::forward<_ArgPackType>(args)...);
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
    class unique_ptr_sorted_vector : public _ptr_sorted_vector_base<_Type, std::unique_ptr<_Type>, _Comp>
    {};


    // -- Vectors of pointers, wrapped in shared pointers
    template <typename _Type>
    class shared_ptr_vector : public _ptr_vector_base<_Type, std::shared_ptr<_Type>>
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
            this->emplace(this->cbegin(), std::forward<_ArgPackType>(args)...);
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
    class shared_ptr_sorted_vector : public _ptr_sorted_vector_base<_Type, std::shared_ptr<_Type>, _Comp>
    {};


    // -- Vectors of pointers, wrapped in weak pointers

    template <typename _Type>
    class weak_ptr_vector : public _ptr_vector_base<_Type, std::weak_ptr<_Type>>
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
            this->emplace(this->cbegin(), std::forward<_ArgPackType>(args)...);
        }
    };
    template <typename _Type, typename _Comp = std::less<_Type>>
    class weak_ptr_sorted_vector : public _ptr_sorted_vector_base<_Type, std::weak_ptr<_Type>, _Comp>
    {};


    // -- Vectors of pointers, wrapped in shared pointers
    template <typename _Type>
    class smart_ptr_view_vector : public _ptr_vector_base<_Type, sl::smart_ptr_view<_Type>>
    {
    public:
        template <typename _PtrType = sl::smart_ptr_view<_Type>>  // Gets both unique and shared_ptr
        void emplace_index_grow(size_t index, _PtrType const& value) {
            if (index >= this->size()) {
                this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
            }
            if (!value || value.get() == nullptr)
                this->operator[](index).reset();
            else
                this->operator[](index) = value;
        }

        inline void emplace_index_grow(size_t index, std::nullptr_t) {
            this->emplace_index_grow(index, sl::smart_ptr_view<_Type>());
        }

        template <typename... _ArgPackType>
        inline void emplace_front(_ArgPackType&&... args) {
            this->emplace(this->cbegin(), std::forward<_ArgPackType>(args)...);
        }
    };
    template <typename _Type, typename _Comp = std::less<_Type>>
    class smart_ptr_view_sorted_vector : public _ptr_sorted_vector_base<_Type, sl::smart_ptr_view<_Type>, _Comp>
    {};


    // -- Vectors of pointers, wrapped in shared pointers
    template <typename _Type>
    class raw_ptr_view_vector : public _ptr_vector_base<_Type, sl::raw_ptr_view<_Type>>
    {
    public:
        template <typename _PtrType = sl::raw_ptr_view<_Type>>  // Gets both bare pointer and shared_ptr
        void emplace_index_grow(size_t index, _PtrType value) {
            if (index >= this->size()) {
                this->resize(index + 1); // The capacity will be even greater, since it calls vector::resize
            }
            if (!value)
                this->operator[](index).reset();
            else
                this->operator[](index) = value;
        }

        inline void emplace_index_grow(size_t index, std::nullptr_t) {
            this->emplace_index_grow(index, sl::raw_ptr_view<_Type>());
        }

        template <typename... _ArgPackType>
        inline void emplace_front(_ArgPackType&&... args) {
            this->emplace(this->cbegin(), std::forward<_ArgPackType>(args)...);
        }
    };
    template <typename _Type, typename _Comp = std::less<_Type>>
    class raw_ptr_view_sorted_vector : public _ptr_sorted_vector_base<_Type, sl::raw_ptr_view<_Type>, _Comp>
    {};
}

#endif // !_INC_SPTR_CONTAINERS_H
