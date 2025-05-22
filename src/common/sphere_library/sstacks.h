/**
* @file sstacks.h
* @brief Stack custom implementations.
*/

#ifndef _INC_CSTACK_H
#define _INC_CSTACK_H

#include <shared_mutex>
//#include "../CException.h" // included in the precompiled header

namespace sl
{

static constexpr size_t _SPHERE_STACK_DEFAULT_SIZE = 10;

template<typename T, std::size_t N>
class fixed_comptime_stack
{
public:
    constexpr fixed_comptime_stack();

    constexpr bool empty() const;
    constexpr bool full() const;
    constexpr size_t size() const;
    constexpr size_t capacity() const;

    void push(const T& value);
    void pop();
    T& top();

    const T& top() const;
    const T* data() const;
    //T* data();

private:
    T m_data[N];
    size_t m_top;
};

template<typename T>
class fixed_runtime_stack {
public:
    fixed_runtime_stack(size_t size=_SPHERE_STACK_DEFAULT_SIZE);
    fixed_runtime_stack(const fixed_runtime_stack<T> & o);
    ~fixed_runtime_stack();

    fixed_runtime_stack & operator=(const fixed_runtime_stack<T> & o);

    void push(T t);
    void pop();
    T top() const;

    void clear();
    bool empty() const;
    size_t size() const;

private:
    T * _stack;
    size_t _top, _size;
};

template <typename T>
class fixed_growing_stack {
public:
    fixed_growing_stack(size_t size=_SPHERE_STACK_DEFAULT_SIZE);
    fixed_growing_stack(const fixed_growing_stack<T> & o);
    ~fixed_growing_stack();

    fixed_growing_stack & operator=(const fixed_growing_stack<T> & o);

    void push(T t);
    void pop();
    T top() const;

    void clear();
    bool empty() const;
    size_t size() const;
private:
    T * _stack;
    size_t _top, _size;
};

template<typename T>
class dynamic_list_stack {
public:
    dynamic_list_stack();
    // To allow thread secure wrapper constructor.
    dynamic_list_stack(size_t _);
    dynamic_list_stack(const dynamic_list_stack<T> & o);
    ~dynamic_list_stack();

    dynamic_list_stack & operator=(const dynamic_list_stack<T> & o);

    void push(T t);
    void pop();
    T top() const;

    void clear();
    bool empty() const;
    size_t size() const;

private:
    struct _dynamicliststackitem {
    public:
        _dynamicliststackitem(T item, _dynamicliststackitem * next);
        T _item;
        _dynamicliststackitem * _next;
    };
    _dynamicliststackitem * _top;
    size_t _size;
};

template <typename T, class S>
class tsstack {
public:
    tsstack(size_t size=_SPHERE_STACK_DEFAULT_SIZE);
    tsstack(const tsstack<T, S> & o);

    tsstack & operator=(const tsstack<T, S> & o);

    void push(T t);
    void pop();
    T top() const;

    void clear();
    bool empty() const;
    size_t size() const;
private:
    mutable std::shared_mutex _mutex;
    S _s;
};

template<typename T, size_t N>
using ts_fixed_comptime_stack = tsstack<T, fixed_comptime_stack<T, N>>;

template<typename T>
using ts_fixed_runtime_stack = tsstack<T, fixed_runtime_stack<T>>;

template<typename T>
using ts_fixed_growing_stack = tsstack<T, fixed_growing_stack<T>>;

template<typename T>
using ts_dynamic_list_stack = tsstack<T, dynamic_list_stack<T>>;

/****
* Implementations.
*/

template <typename T, size_t N>
constexpr fixed_comptime_stack<T, N>::fixed_comptime_stack() : m_top(0) {}

template <typename T, size_t N>
constexpr bool fixed_comptime_stack<T, N>::empty() const { return m_top == 0; }

template <typename T, size_t N>

constexpr bool fixed_comptime_stack<T, N>::full() const { return m_top == N; }

template <typename T, size_t N>
constexpr size_t fixed_comptime_stack<T, N>::size() const { return m_top; }

template <typename T, size_t N>
constexpr size_t fixed_comptime_stack<T, N>::capacity() const { return N; }

template <typename T, size_t N>
void fixed_comptime_stack<T, N>::push(const T& value) {
    if (full()) [[unlikely]]
        throw std::overflow_error("Stack full");
    m_data[m_top++] = value;
}

template <typename T, size_t N>
void fixed_comptime_stack<T, N>::pop() {
    if (empty()) [[unlikely]]
        throw std::underflow_error("Stack empty");
    --m_top;
}

template <typename T, size_t N>
T& fixed_comptime_stack<T, N>::top() {
    if (empty()) [[unlikely]]
        throw std::underflow_error("Stack empty");
    return m_data[m_top - 1];
}

template <typename T, size_t N>
const T& fixed_comptime_stack<T, N>::top() const {
    if (empty()) [[unlikely]]
        throw std::underflow_error("Stack empty");
    return m_data[m_top - 1];
}

template <typename T, size_t N>
const T* fixed_comptime_stack<T, N>::data() const {
    return m_data;
}
/*
template <typename T, size_t N>
T* fixed_comptime_stack<T, N>::data() {
    return m_data;
}
*/
template <typename T>
fixed_runtime_stack<T>::fixed_runtime_stack(size_t size) {
    _stack = new T[size];
    _top = 0;
    _size = size;
}

template <typename T>
fixed_runtime_stack<T>::fixed_runtime_stack(const fixed_runtime_stack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
}

template <typename T>
fixed_runtime_stack<T>::~fixed_runtime_stack() {
    delete[] _stack;
}

template <typename T>
fixed_runtime_stack<T> & fixed_runtime_stack<T>::operator=(const fixed_runtime_stack<T> & o) {
    delete[] _stack;
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
    return *this;
}

template <typename T>
void fixed_runtime_stack<T>::push(T t) {
    if (_top == _size) {
        throw CSError(LOGL_FATAL, 0, "stack is full.");
    }
    _stack[_top++] = t;
}

template <typename T>
void fixed_runtime_stack<T>::pop() {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _top--;
}

template <typename T>
T fixed_runtime_stack<T>::top() const {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _stack[_top - 1];
}

template <typename T>
void fixed_runtime_stack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool fixed_runtime_stack<T>::empty() const {
    return _top == 0;
}

template <typename T>
size_t fixed_runtime_stack<T>::size() const {
    return _top;
}

template <typename T>
fixed_growing_stack<T>::fixed_growing_stack(size_t size) {
    _stack = new T[size];
    _top = 0;
    _size = size;
}

template <typename T>
fixed_growing_stack<T>::fixed_growing_stack(const fixed_growing_stack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
}

template <typename T>
fixed_growing_stack<T>::~fixed_growing_stack() {
    delete[] _stack;
}

template <typename T>
fixed_growing_stack<T> & fixed_growing_stack<T>::operator=(const fixed_growing_stack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
    return *this;
}

template <typename T>
void fixed_growing_stack<T>::push(T t) {
    if (_top == _size) {
        T * nstack = new T[_size + _SPHERE_STACK_DEFAULT_SIZE];
        for(size_t i = 0; i < _top; ++i) nstack[i] = _stack[i];
        delete[] _stack;
        _stack = nstack;
        _size += _SPHERE_STACK_DEFAULT_SIZE;
    }
    _stack[_top++] = t;
}

template <typename T>
void fixed_growing_stack<T>::pop() {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _top--;
}

template <typename T>
T fixed_growing_stack<T>::top() const {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _stack[_top - 1];
}

template <typename T>
void fixed_growing_stack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool fixed_growing_stack<T>::empty() const {
    return _top == 0;
}

template <typename T>
size_t fixed_growing_stack<T>::size() const {
    return _top;
}

template <typename T>
dynamic_list_stack<T>::dynamic_list_stack() {
    _top = nullptr;
    _size = 0;
}

template <typename T>
dynamic_list_stack<T>::dynamic_list_stack(size_t _) : dynamic_list_stack<T>() { UnreferencedParameter(_); }

template <typename T>
dynamic_list_stack<T>::dynamic_list_stack(const dynamic_list_stack<T> & o) {
    _size = o._size;
    _top = nullptr;
    _dynamicliststackitem * next = o._top, * prev;
    if (next) {
        _top = new _dynamicliststackitem(next->_item, nullptr);
        prev = _top;
        next = next->_next;
        while (next) {
            prev->_next = new _dynamicliststackitem(next->_item, nullptr);
            prev = prev->_next;
            next = next->_next;
        }
    }
}

template <typename T>
dynamic_list_stack<T>::~dynamic_list_stack() {
    clear();
};

template <typename T>
dynamic_list_stack<T> & dynamic_list_stack<T>::operator=(const dynamic_list_stack<T> & o) {
    _size = o._size;
    _top = nullptr;
    _dynamicliststackitem * next = o._top, * prev;
    if (next) {
        _top = new _dynamicliststackitem(next->_item, nullptr);
        prev = _top;
        next = next->_next;
        while (next) {
            prev->_next = new _dynamicliststackitem(next->_item, nullptr);
            prev = prev->_next;
            next = next->_next;
        }
    }
    return *this;
}

template <typename T>
void dynamic_list_stack<T>::push(T t) {
    _dynamicliststackitem * ntop = new _dynamicliststackitem(t, _top);
    _top = ntop;
    _size++;
}

template <typename T>
void dynamic_list_stack<T>::pop() {
    if (_top == nullptr) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _dynamicliststackitem * oldtop = _top;
    _top = oldtop->_next;
    delete oldtop;
    _size--;
}

template <typename T>
T dynamic_list_stack<T>::top() const {
    if (_top == nullptr) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _top->_item;
}

template <typename T>
void dynamic_list_stack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool dynamic_list_stack<T>::empty() const {
    return _top == nullptr;
}

template <typename T>
size_t dynamic_list_stack<T>::size() const {
    return _size;
}

template <typename T>
dynamic_list_stack<T>::_dynamicliststackitem::_dynamicliststackitem(T item, _dynamicliststackitem * next) {
    _item = item;
    _next = next;
}

template<typename T, class S>
tsstack<T, S>::tsstack(size_t size) : _s(size) {}

template<typename T, class S>
tsstack<T, S>::tsstack(const tsstack<T, S> & o) {
    o._mutex.lock_shared();
    _s = o._s;
    o._mutex.unlock_shared();
}

template<typename T, class S>
tsstack<T, S> & tsstack<T, S>::operator=(const tsstack<T, S> & o) {
    _mutex.lock();
    o._mutex.lock_shared();
    _s = o._s;
    o._mutex.unlock_shared();
    _mutex.unlock();
}

template<typename T, class S>
void tsstack<T, S>::push(T t) {
    _mutex.lock();
    _s.push(t);
    _mutex.unlock();
}

template<typename T, class S>
void tsstack<T, S>::pop() {
    _mutex.lock();
    _s.pop();
    _mutex.unlock();
}

template<typename T, class S>
T tsstack<T, S>::top() const {
    _mutex.lock_shared();
    T x = _s.top();
    _mutex.unlock_shared();
    return x;
}

template<typename T, class S>
void tsstack<T, S>::clear() {
    _mutex.lock();
    _s.clear();
    _mutex.unlock();
}

template<typename T, class S>
bool tsstack<T, S>::empty() const {
    _mutex.lock_shared();
    bool b = _s.empty();
    _mutex.unlock_shared();
    return b;
}

template<typename T, class S>
size_t tsstack<T, S>::size() const {
    _mutex.lock_shared();
    size_t s = _s.size();
    _mutex.unlock_shared();
    return s;
}

}

#endif //_INC_CSTACK_H
