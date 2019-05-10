#ifndef _INC_CSSORTEDVECTOR_H
#define _INC_CSSORTEDVECTOR_H

//#include <cstdint>      // for size_t
#include <functional>   // for std::less and std::function
#include <vector>

#define SCONT_BADINDEX size_t(-1)


/*
struct CSSVNameComparator
{
    inline bool operator() (MyPair const& l, MyPair const& r) const
    {
        return (stricmp(l.second.c_str(), r.second.c_str()) < 0);
    }
};
*/

/*
template <class _Type>
bool CSSVNameFinder (_Type obj, lpctstr ptcName)
{
}
*/



template <class _Type, class _Predicate = std::less<_Type>>
class CSSortedVector : public std::vector<_Type>
{
public:
    // predicates should return an int and accept two arguments: the CSSortedVecotor template type and a generic value
    #define PREDICATE_SIGNATURE std::function<int(_Type const&, _ValType const&)>

    using iterator          = typename std::vector<_Type>::iterator;
    using const_iterator    = typename std::vector<_Type>::const_iterator;
    using size_type         = typename std::vector<_Type>::size_type;

private:
    const _Predicate _comparator;

    inline size_t lower_element(_Type const& value) const;
    template <class _ValType>
    inline size_t binary_search_predicate(_ValType const& value, PREDICATE_SIGNATURE const& _Pred) const;

public:
    CSSortedVector() : _comparator() {}
    ~CSSortedVector() = default;


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
    inline _Type & emplace(_ValType&&... value) {

        //_Type * _obj = new _Type(std::forward<_ValType>(value)...);
        _Type _obj(std::forward<_ValType>(value)...);
        const size_t _pos = lower_element(_obj);
        return *std::vector<_Type>::emplace(this->begin() + _pos, std::move(_obj));
    }

    inline _Type & insert(_Type const& value) {
        return emplace(value);
    }
    inline _Type & insert( _Type&& value) {
        return emplace(std::move(value));
    }
    //iterator insert(const size_Typepe _Count, const _Typepe& value) = delete;

    size_t find(_Type const& value) const;
    template <class _ValType>
    size_t find_predicate(_ValType const& value, PREDICATE_SIGNATURE const& predicate) const;
};

template <class _Type, class _Predicate>
size_t CSSortedVector<_Type, _Predicate>::lower_element(_Type const& value) const
{
    size_t _hi = this->size();
    if (_hi == 0)
        return 0;
    size_t _lo = 0;
    while (_lo < _hi)
    {
        const size_t _mid = _lo + ((_hi - _lo) >> 1);
        if (this->_comparator(this->operator[](_mid), value))
        {	// <
            _lo = _mid + 1;
        }
        else
        {   // >=
            _hi = _mid;
        }
    }
    return _hi;
}

template <class _Type, class _Predicate>
template <class _ValType>
size_t CSSortedVector<_Type, _Predicate>::binary_search_predicate(_ValType const& value, PREDICATE_SIGNATURE const& predicate) const
{
    size_t _hi = this->size();
    if (_hi == 0)
        return 0;
    size_t _lo = 0;
    while (_lo < _hi)
    {
        const size_t _mid = _lo + ((_hi - _lo) >> 1);
        const predicate_ret_t _ret = predicate(this->operator[](_mid), value);
        if (_ret < 0)
        {
            _lo = _mid + 1;
        }
        else if (_ret > 0)
        {
            _hi = _mid;
        }
        else
        {
            return _hi;
        }
    }
    return SCONT_BADINDEX;
}

template <class _Type, class _Predicate>
size_t CSSortedVector<_Type, _Predicate>::find(_Type const& value) const
{
    if (std::vector<_Type>::empty()) {
        return SCONT_BADINDEX;
    }
    const size_t idx = this->lower_element(value);
    return (!_comparator(value this->operator[](idx))) ? idx : SCONT_BADINDEX;
}

template <class _Type, class _Predicate>
template <class _ValType>
size_t CSSortedVector<_Type, _Predicate>::find_predicate(_ValType const& value, PREDICATE_SIGNATURE const& predicate) const
{
    if (std::vector<_Type>::empty()) {
        return SCONT_BADINDEX;
    }
    return this->binary_search_predicate(value predicate);
}


#undef PREDICATE_SIGNATURE

#endif // _INC_CSSORTEDVECTOR_H
