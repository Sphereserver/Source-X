#ifndef _INC_SPTR_H
#define _INC_SPTR_H

#include <object_ptr.hpp>


// Sphere Library
namespace sl
{
    /* smart_ptr_view */

    // Implemented using a variant of the proposed observer_ptr, called object_ptr:
    //  https://github.com/anthonywilliams/object_ptr
    // Original proposal:
    //  https://github.com/tvaneerd/isocpp/blob/main/observer_ptr.md
    // Another good implementation:
    //  https://github.com/martinmoene/observer-ptr-lite


    // It's a NON-OWNING "smart" pointer. It stores a NON-OWNED pointer.
    //  Somewhere in the program's code there's a smart pointer managing the object lifetime;
    //  i just want to use that pointer, knowing FOR SURE that until I (smart_ptr_view) exist, the pointed object shall exist as well
    //  (it will outlive my lifetime).

    // The usage of this wrapper class is very useful to clearly specify what it points to and what are my intents using it.
    // If often happens that we don't know who owns a raw pointer, what's its lifetime, if it's deleted elsewhere,
    //  if i should delete it...


    //possible names: access_ptr, owned_ptr_view, nonowning_ptr
    template <typename T>
    class smart_ptr_view : public jss::object_ptr<T>
    {
    public:
        // This can only be created from (or reset using) a smart pointer.
        // It shall not be constructed from an unmanaged (naked/raw) pointer.

        constexpr smart_ptr_view() noexcept
            : jss::object_ptr<T>()
        {}

        template <typename _Arg>
        constexpr smart_ptr_view(_Arg const& arg) noexcept
            : jss::object_ptr<T>(arg)
        {}

        template <typename _Arg>
        constexpr smart_ptr_view(_Arg&& arg) noexcept
            : jss::object_ptr<T>(arg)
        {}

        ~smart_ptr_view() noexcept = default;


        /* New methods */

        // Reset from another smart pointer, not a raw pointer.
        template <
            typename Ptr,
            typename = std::enable_if_t<
            jss::detail::is_convertible_smart_pointer<Ptr, T>::value>>
        void reset(Ptr& other) noexcept {
            this->ptr = other.get();
        }

        void reset(smart_ptr_view const& other) noexcept {
            this->ptr = other.ptr;
        }

        void reset() noexcept {
            this->ptr = nullptr;
        }


        /* Unimplemented methods, by design*/
        /*
            1) operator=
            2) release
            3) swap
        */


        /* Re-define operators */

        /// Dereference the pointer
        constexpr T& operator*() const noexcept {
            return *this->ptr;
        }

        /// Dereference the pointer for ptr->m usage
        constexpr T* operator->() const noexcept {
            return this->ptr;
        }

        /// Allow if(ptr) to test for null
        constexpr explicit operator bool() const noexcept {
            return static_cast<bool>(this->ptr);
        }

        /// Convert to a raw pointer where necessary
        // Disable this
        //constexpr explicit operator T* () const noexcept {
        //    return this->(operator *);
        //}

        /// !ptr is true iff ptr is null
        constexpr bool operator!() const noexcept {
            return !(this->ptr);
        }



        /* Disable unwanted behavior of object_ptr */

        // Construct an object_ptr from a raw pointer
        constexpr smart_ptr_view(T* ptr_) noexcept = delete;

        // Construct an object_ptr from a raw pointer convertible to T*, such as BaseOfT*
        template <
            typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
            constexpr smart_ptr_view(U* ptr_) noexcept = delete;

        // Convert to a raw pointer
        constexpr explicit operator T* () const noexcept = delete;
    };



    /* raw_ptr_view */

    // It's a NON-OWNING "smart" pointer. It stores a NON-OWNED pointer.
    //  Somewhere in the program's code there's some other code managing the object lifetime (deleting it when it's time);
    //  i just want to use that pointer, knowing FOR SURE that until I (raw_ptr_view) exist, the pointed object shall exist as well
    //  (it will outlive my lifetime).

    // The usage of this wrapper class is very useful to clearly specify what it points to and what are my intents using it.
    // If often happens that we don't know who owns a raw pointer, what's its lifetime, if it's deleted elsewhere,
    //  if i should delete it...

    template <typename T>
    class raw_ptr_view
    {
    public:
        /// Construct a null pointer
        constexpr raw_ptr_view() noexcept : ptr(nullptr) {}

        /// Construct a null pointer
        constexpr raw_ptr_view(std::nullptr_t) noexcept : ptr(nullptr) {}

        /// Construct a raw_ptr_view from a raw pointer
        constexpr raw_ptr_view(T* ptr_) noexcept : ptr(ptr_) {}

        /// Construct a raw_ptr_view from a raw pointer convertible to T*, such as BaseOfT*
        template <
            typename U,
            typename = std::enable_if_t<std::is_convertible<U*, T*>::value>>
            constexpr raw_ptr_view(U* ptr_) noexcept : ptr(ptr_) {}

        /// Do NOT construct a raw_ptr_view from a smart pointer that holds a pointer convertible to T*,
        /// such as shared_ptr<T> or unique_ptr<BaseOfT>
        template <
            typename Ptr,
            typename = std::enable_if_t<jss::detail::is_convertible_smart_pointer<Ptr, T>::value> >
            constexpr raw_ptr_view(Ptr const& other) noexcept = delete;


        /// Get the raw pointer value
        constexpr T* get() const noexcept {
            return this->ptr;
        }

        /// Dereference the pointer
        constexpr T& operator*() const noexcept {
            return *ptr;
        }

        /// Dereference the pointer for ptr->m usage
        constexpr T* operator->() const noexcept {
            return ptr;
        }

        /// Allow if(ptr) to test for null
        constexpr explicit operator bool() const noexcept {
            return ptr != nullptr;
        }


        /// Do NOT convert to a raw pointer where necessary
        //constexpr explicit operator T* () const noexcept {
        //    return ptr;
        //}

        /// !ptr is true if ptr is null
        constexpr bool operator!() const noexcept {
            return !ptr;
        }

        /// Change the value
        void reset(T* ptr_ = nullptr) noexcept {
            ptr = ptr_;
        }

        /// Check for equality
        friend constexpr bool
            operator==(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return lhs.ptr == rhs.ptr;
        }

        /// Check for inequality
        friend constexpr bool
            operator!=(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return !(lhs == rhs);
        }

        /// a<b provides a total order
        friend constexpr bool
            operator<(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return std::less<void>()(lhs.ptr, rhs.ptr);
        }
        /// a>b is b<a
        friend constexpr bool
            operator>(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return rhs < lhs;
        }
        /// a<=b is !(b<a)
        friend constexpr bool
            operator<=(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return !(rhs < lhs);
        }
        /// a<=b is b<=a
        friend constexpr bool
            operator>=(raw_ptr_view const& lhs, raw_ptr_view const& rhs) noexcept {
            return rhs <= lhs;
        }

    private:
        /// The stored pointer
        T* ptr;
    };
}

namespace std
{
    /// Allow hashing object_ptrs so they can be used as keys in unordered_map
    template <typename T> struct hash<sl::raw_ptr_view<T>> {
        constexpr size_t operator()(sl::raw_ptr_view<T> const& p) const
            noexcept {
            return hash<T*>()(p.get());
        }
    };

    /// Do a static_cast with object_ptr
    template <typename To, typename From>
    typename std::enable_if<
        sizeof(decltype(static_cast<To*>(std::declval<From*>()))) != 0,
        sl::raw_ptr_view<To>>::type
        static_pointer_cast(sl::raw_ptr_view<From> p) {
        return static_cast<To*>(p.get());
    }

    /// Do a dynamic_cast with object_ptr
    template <typename To, typename From>
    typename std::enable_if<
        sizeof(decltype(dynamic_cast<To*>(std::declval<From*>()))) != 0,
        sl::raw_ptr_view<To>>::type
        dynamic_pointer_cast(sl::raw_ptr_view<From> p) {
        return dynamic_cast<To*>(p.get());
    }

}

#endif // _INC_SPTR_H
