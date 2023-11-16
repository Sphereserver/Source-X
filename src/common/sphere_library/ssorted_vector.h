#ifndef _INC_SORTED_VECTOR_H
#define _INC_SORTED_VECTOR_H

#include <functional>   // for std::less
#include <vector>


// Sphere library
namespace sl
{
#if __cplusplus >= 202002L
        // C++20 (and later) code
        consteval
#else
        constexpr
#endif
        size_t scont_bad_index() noexcept {
            return size_t(-1);
        }
        
        template <class _Type, class _Comp = std::less<_Type>>
        class sorted_vector : public std::vector<_Type>
        {
        public:
            using base_type = typename std::vector<_Type>;
            using iterator = typename base_type::iterator;
            using const_iterator = typename base_type::const_iterator;
            using size_type = typename base_type::size_type;

        private:
            const _Comp _comparatorObj;


            inline size_t lower_element(size_t mySize, const _Type* const dataptr, _Type const& value) const noexcept;

            template <class _ValType, class _Pred>
            inline size_t binary_search_predicate(size_t mySize, _ValType const& value, _Pred&& predicate) const noexcept;

        public:
            sorted_vector() : _comparatorObj() {}
            ~sorted_vector() = default;


            /* Does it work?
            push_back(...) = delete;
            emplace_back(...) = delete;
            */

            template<class... _ValType>
            auto emplace_back(_ValType&&... value) = delete;

            void push_back(_Type const& value) = delete;
            void push_back(_Type&& value) = delete;
            void push_back(const bool& value) = delete;

            template<class... _ValType>
            iterator emplace(const_iterator itWhere, _ValType&&... value) = delete;

            iterator insert(const_iterator itWhere, _Type const& value) = delete;
            iterator insert(const_iterator itWhere, _Type&& value) = delete;
            iterator insert(const_iterator itWhere, const size_t _Count, _Type const& value) = delete;

            template<class... _ValType>
            iterator emplace(_ValType&&... value)
            {
                //_Type * _obj = new _Type(std::forward<_ValType>(value)...);
                _Type _obj(std::forward<_ValType>(value)...);
                const size_t _mySize = base_type::size();
                const size_t _pos = (_mySize == 0) ? 0 : this->lower_element(_mySize, base_type::data(), _obj);
                return base_type::emplace(base_type::begin() + _pos, std::move(_obj));
            }

            inline iterator insert(_Type const& value) {
                return this->emplace(value);
            }
            inline iterator insert(_Type&& value) {
                return this->emplace(std::move(value));
            }
            //iterator insert(const size_Typepe _Count, const _Typepe& value) = delete;

            inline void remove_index(const size_t index) {
                base_type::erase(base_type::begin() + index);
            }


            size_t find(_Type const& value) const noexcept;

            // predicates should return an int and accept two arguments: the CSSortedVecotor template type and a generic value
            template <class _ValType, class _Pred>
            size_t find_predicate(_ValType const& value, _Pred&& predicate) const noexcept;
        };


        /* Template methods (inlined or not) are defined here */

        template <class _Type, class _Comp>
        size_t sorted_vector<_Type, _Comp>::lower_element(size_t _size, const _Type* const dataptr, _Type const& value) const noexcept
        {
            // Ensure that we don't call this private method with _hi == 0! Check it before! (Not doing it here to avoiding redundancy)
            // It can return an index equal to the size! (Last element + 1, useful to add items in this ordered container)
            /*
            if (_size == 0)
                return 0;
            */

            size_t _lo = 0;
            while (_lo < _size)
            {
                const size_t _mid = _lo + ((_size - _lo) >> 1);
                if (this->_comparatorObj(dataptr[_mid], value))
                {	// <
                    _lo = _mid + 1;
                }
                else
                {   // >=
                    _size = _mid;
                }
            }
            return _size;

            /*
            // Alternative implementation, to be completed. Logarithmic time, but better use of CPU instruction pipelining and branch prediction, at the cost of more total comparations (?)
            // It's worth running some benchmarks before switching to this.

            const _Type* const  raw  = this->data();
            const _Type*        base = raw;
            while (_hi > 1)
            {
                const size_t half = _hi >> 1;
                base = this->_comparatorObj(base[half], value) ? base : &base[half];
                _hi -= half;
            }
            return (base - raw + (this->_comparatorObj(*base, value)));
            */
        }

        template <class _Type, class _Comp>
        template <class _ValType, class _Pred>
        size_t sorted_vector<_Type, _Comp>::binary_search_predicate(size_t _size, _ValType const& value, _Pred && predicate) const noexcept
        {
            // Ensure that we don't call this private method with _hi == 0! Check it before! (Not doing it here to avoiding redundancy)
            /*
            if (_hi == 0)
                return 0;
            */

            const _Type* const _dataptr = base_type::data();
            size_t _lo = 0;
            while (_lo < _size)
            {
                const size_t _mid = _lo + ((_size - _lo) >> 1);
                const int _ret = predicate(_dataptr[_mid], value);
                if (_ret < 0)
                {
                    _lo = _mid + 1;
                }
                else if (_ret > 0)
                {
                    _size = _mid;
                }
                else
                {
                    return _mid;
                }
            }
            return sl::scont_bad_index();
        }

        template <class _Type, class _Comp>
        size_t sorted_vector<_Type, _Comp>::find(_Type const& value) const noexcept
        {
            const size_t _mySize = base_type::size();
            if (_mySize == 0) {
                return sl::scont_bad_index();
            }
            const _Type* const _dataptr = base_type::data();
            size_t _idx = this->lower_element(_mySize, _dataptr, value);
            if (_idx == _mySize)
                return sl::scont_bad_index();
            return (!this->_comparatorObj(value, _dataptr[_idx])) ? _idx : sl::scont_bad_index();
        }

        template <class _Type, class _Comp>
        template <class _ValType, class _Pred>
        size_t sorted_vector<_Type, _Comp>::find_predicate(_ValType const& value, _Pred && predicate) const noexcept
        {
            const size_t _mySize = base_type::size();
            if (_mySize == 0) {
                return sl::scont_bad_index();
            }
            return this->binary_search_predicate(_mySize, value, predicate);
        }

    }



#endif // _INC_SORTED_VECTOR_H