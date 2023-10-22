#ifndef JSS_OBJECT_PTR_HPP
#define JSS_OBJECT_PTR_HPP
#include <cstddef>
#include <utility>
#include <functional>
#include <memory>
#include <type_traits>

namespace jss {
    namespace detail {
        /// Detect the presence of operator*, operator-> and .get() for the
        /// given type
        template <typename Ptr, bool= std::is_class<Ptr>::value>
        struct has_smart_pointer_ops {
            using false_test= char;
            template <typename T> struct true_test { false_test dummy[2]; };

            template <typename T> static false_test test_op_star(...);
            template <typename T>
            static true_test<decltype(*std::declval<T const &>())>
            test_op_star(T *);

            template <typename T> static false_test test_op_arrow(...);
            template <typename T>
            static true_test<decltype(std::declval<T const &>().operator->())>
            test_op_arrow(T *);

            template <typename T> static false_test test_get(...);
            template <typename T>
            static true_test<decltype(std::declval<T const &>().get())>
            test_get(T *);

            static constexpr bool value=
                !std::is_same<decltype(test_get<Ptr>(0)), false_test>::value &&
                !std::is_same<
                    decltype(test_op_arrow<Ptr>(0)), false_test>::value &&
                !std::is_same<
                    decltype(test_op_star<Ptr>(0)), false_test>::value;
        };

        /// Non-class types can't be smart pointers
        template <typename Ptr>
        struct has_smart_pointer_ops<Ptr, false> : std::false_type {};

        /// Ensure that the smart pointer operations give consistent return
        /// types
        template <typename Ptr>
        struct smart_pointer_ops_consistent
            : std::integral_constant<
                  bool,
                  std::is_pointer<decltype(
                      std::declval<Ptr const &>().get())>::value &&
                      std::is_reference<decltype(
                          *std::declval<Ptr const &>())>::value &&
                      std::is_pointer<decltype(
                          std::declval<Ptr const &>().operator->())>::value &&
                      std::is_same<
                          decltype(std::declval<Ptr const &>().get()),
                          decltype(std::declval<Ptr const &>().
                                   operator->())>::value &&
                      std::is_same<
                          decltype(*std::declval<Ptr const &>().get()),
                          decltype(*std::declval<Ptr const &>())>::value> {};

        /// Assume Ptr is a smart pointer if it has the relevant ops and they
        /// are consistent
        template <typename Ptr, bool= has_smart_pointer_ops<Ptr>::value>
        struct is_smart_pointer
            : std::integral_constant<
                  bool, smart_pointer_ops_consistent<Ptr>::value> {};

        /// If Ptr doesn't have the relevant ops then it can't be a smart
        /// pointer
        template <typename Ptr>
        struct is_smart_pointer<Ptr, false> : std::false_type {};

        /// Check if Ptr is a smart pointer that holds a pointer convertible to
        /// T*
        template <typename Ptr, typename T, bool= is_smart_pointer<Ptr>::value>
        struct is_convertible_smart_pointer
            : std::integral_constant<
                  bool, std::is_convertible<
                            decltype(std::declval<Ptr const &>().get()),
                            T *>::value> {};

        /// If Ptr isn't a smart pointer then we don't want it
        template <typename Ptr, typename T>
        struct is_convertible_smart_pointer<Ptr, T, false> : std::false_type {};

    }

    /// A basic "smart" pointer that points to an individual object it does not
    /// own. Unlike a raw pointer, it does not support pointer arithmetic or
    /// array operations, and the pointee cannot be accidentally deleted. It
    /// supports implicit conversion from any smart pointer that holds a pointer
    /// convertible to T*
    template <typename T> class object_ptr {
    public:
        /// Construct a null pointer
        constexpr object_ptr() noexcept : ptr(nullptr) {}
        /// Construct a null pointer
        constexpr object_ptr(std::nullptr_t) noexcept : ptr(nullptr) {}
        /// Construct an object_ptr from a raw pointer
        constexpr object_ptr(T *ptr_) noexcept : ptr(ptr_) {}
        /// Construct an object_ptr from a raw pointer convertible to T*, such
        /// as BaseOfT*
        template <
            typename U,
            typename= std::enable_if_t<std::is_convertible<U *, T *>::value>>
        constexpr object_ptr(U *ptr_) noexcept : ptr(ptr_) {}
        /// Construct an object_ptr from a smart pointer that holds a pointer
        /// convertible to T*,
        /// such as shared_ptr<T> or unique_ptr<BaseOfT>
        template <
            typename Ptr,
            typename= std::enable_if_t<
                detail::is_convertible_smart_pointer<Ptr, T>::value>>
        constexpr object_ptr(Ptr const &other) noexcept : ptr(other.get()) {}

        /// Get the raw pointer value
        constexpr T *get() const noexcept {
            return ptr;
        }

        /// Dereference the pointer
        constexpr T &operator*() const noexcept {
            return *ptr;
        }

        /// Dereference the pointer for ptr->m usage
        constexpr T *operator->() const noexcept {
            return ptr;
        }

        /// Allow if(ptr) to test for null
        constexpr explicit operator bool() const noexcept {
            return ptr != nullptr;
        }

        /// Convert to a raw pointer where necessary
        constexpr explicit operator T *() const noexcept {
            return ptr;
        }

        /// !ptr is true iff ptr is null
        constexpr bool operator!() const noexcept {
            return !ptr;
        }

        /// Change the value
        void reset(T *ptr_= nullptr) noexcept {
            ptr= ptr_;
        }

        /// Check for equality
        friend constexpr bool
        operator==(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return lhs.ptr == rhs.ptr;
        }

        /// Check for inequality
        friend constexpr bool
        operator!=(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return !(lhs == rhs);
        }

        /// a<b provides a total order
        friend constexpr bool
        operator<(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return std::less<void>()(lhs.ptr, rhs.ptr);
        }
        /// a>b is b<a
        friend constexpr bool
        operator>(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return rhs < lhs;
        }
        /// a<=b is !(b<a)
        friend constexpr bool
        operator<=(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return !(rhs < lhs);
        }
        /// a<=b is b<=a
        friend constexpr bool
        operator>=(object_ptr const &lhs, object_ptr const &rhs) noexcept {
            return rhs <= lhs;
        }

    protected:
        /// The stored pointer
        T *ptr;
    };

}

namespace std {
    /// Allow hashing object_ptrs so they can be used as keys in unordered_map
    template <typename T> struct hash<jss::object_ptr<T>> {
        constexpr size_t operator()(jss::object_ptr<T> const &p) const
            noexcept {
            return hash<T *>()(p.get());
        }
    };

    /// Do a static_cast with object_ptr
    template <typename To, typename From>
    typename std::enable_if<
        sizeof(decltype(static_cast<To *>(std::declval<From *>()))) != 0,
        jss::object_ptr<To>>::type
    static_pointer_cast(jss::object_ptr<From> p) {
        return static_cast<To *>(p.get());
    }

    /// Do a dynamic_cast with object_ptr
    template <typename To, typename From>
    typename std::enable_if<
        sizeof(decltype(dynamic_cast<To *>(std::declval<From *>()))) != 0,
        jss::object_ptr<To>>::type
    dynamic_pointer_cast(jss::object_ptr<From> p) {
        return dynamic_cast<To *>(p.get());
    }

}

#endif
