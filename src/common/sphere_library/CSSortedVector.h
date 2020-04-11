#ifndef _INC_CSSORTEDVECTOR_H
#define _INC_CSSORTEDVECTOR_H

#include <functional>   // for std::less
#include <vector>

#define SCONT_BADINDEX size_t(-1)


template <class _Type, class _Comp = std::less<_Type>>
class CSSortedVector : public std::vector<_Type>
{
public:
    using iterator          = typename std::vector<_Type>::iterator;
    using const_iterator    = typename std::vector<_Type>::const_iterator;
    using size_type         = typename std::vector<_Type>::size_type;

private:
    const _Comp _comparatorObj;


    inline size_t lower_element(size_t mySize, _Type const& value) const noexcept;

    template <class _ValType, class _Pred>
    inline size_t binary_search_predicate(size_t mySize, _ValType const& value, _Pred const& predicate) const noexcept;

public:
    CSSortedVector() : _comparatorObj() {}
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
    iterator emplace(_ValType&&... value)
    {
        //_Type * _obj = new _Type(std::forward<_ValType>(value)...);
        _Type _obj(std::forward<_ValType>(value)...);
        const size_t _mySize = this->size();
        const size_t _pos = (_mySize == 0) ? 0 : lower_element(_mySize, _obj);
        return std::vector<_Type>::emplace(this->begin() + _pos, std::move(_obj));
    }

    inline iterator insert(_Type const& value) {
        return emplace(value);
    }
    inline iterator insert( _Type&& value) {
        return emplace(std::move(value));
    }
    //iterator insert(const size_Typepe _Count, const _Typepe& value) = delete;


    size_t find(_Type const& value) const noexcept;

    // predicates should return an int and accept two arguments: the CSSortedVecotor template type and a generic value
    template <class _ValType, class _Pred>
    size_t find_predicate(_ValType const& value, _Pred const& predicate) const noexcept;
};


/* Template methods (inlined or not) are defined here */

template <class _Type, class _Comp>
size_t CSSortedVector<_Type, _Comp>::lower_element(size_t _hi, _Type const& value) const noexcept
{
    // Ensure that we don't call this private method with _hi == 0! Check it before! (Not doing it here to avoiding redundancy)
    /*
    if (_hi == 0)
        return 0;
    */
    size_t _lo = 0;
    while (_lo < _hi)
    {
        const size_t _mid = _lo + ((_hi - _lo) >> 1);
        if (this->_comparatorObj(this->operator[](_mid), value))
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

template <class _Type, class _Comp>
template <class _ValType, class _Pred>
size_t CSSortedVector<_Type, _Comp>::binary_search_predicate(size_t _hi, _ValType const& value, _Pred const& predicate) const noexcept
{
    // Ensure that we don't call this private method with _hi == 0! Check it before! (Not doing it here to avoiding redundancy)
    /*
    if (_hi == 0)
        return 0;
    */
    size_t _lo = 0;
    while (_lo < _hi)
    {
        const size_t _mid = _lo + ((_hi - _lo) >> 1);
        const int _ret = predicate(this->operator[](_mid), value);
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
            return _mid;
        }
    }
    return SCONT_BADINDEX;
}

template <class _Type, class _Comp>
size_t CSSortedVector<_Type, _Comp>::find(_Type const& value) const noexcept
{
    const size_t _sz = this->size();
    if (_sz == 0) {
        return SCONT_BADINDEX;
    }
    const size_t _idx = this->lower_element(_sz, value);
    if (_idx >= _sz) {
        return SCONT_BADINDEX;
    }
    return (!this->_comparatorObj(value, this->operator[](_idx))) ? _idx : SCONT_BADINDEX;
}

template <class _Type, class _Comp>
template <class _ValType, class _Pred>
size_t CSSortedVector<_Type, _Comp>::find_predicate(_ValType const& value, _Pred const& predicate) const noexcept
{
    const size_t _sz = this->size();
    if (_sz == 0) {
        return SCONT_BADINDEX;
    }
    return this->binary_search_predicate(_sz, value, predicate);
}


#endif // _INC_CSSORTEDVECTOR_H
