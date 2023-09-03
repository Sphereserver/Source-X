/**
* @file sstacks.h
* @brief Stack custom implementations.
*/

#ifndef _INC_CSTACK_H
#define _INC_CSTACK_H

#include <shared_mutex>
#include "../CException.h"

#define _SPHERE_STACK_DEFAULT_SIZE 10

template<typename T>
class fixedstack {
public:
    fixedstack(size_t size=_SPHERE_STACK_DEFAULT_SIZE);
    fixedstack(const fixedstack<T> & o);
    ~fixedstack();

    fixedstack & operator=(const fixedstack<T> & o);

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
class fixedgrowingstack {
public:
    fixedgrowingstack(size_t size=_SPHERE_STACK_DEFAULT_SIZE);
    fixedgrowingstack(const fixedgrowingstack<T> & o);
    ~fixedgrowingstack();

    fixedgrowingstack & operator=(const fixedgrowingstack<T> & o);

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
class dynamicstack {
public:
    dynamicstack();
    // To allow thread secure wrapper constructor.
    dynamicstack(size_t _);
    dynamicstack(const dynamicstack<T> & o);
    ~dynamicstack();

    dynamicstack & operator=(const dynamicstack<T> & o);

    void push(T t);
    void pop();
    T top() const;

    void clear();
    bool empty() const;
    size_t size() const;

private:
    struct _dynamicstackitem {
    public:
        _dynamicstackitem(T item, _dynamicstackitem * next);
        T _item;
        _dynamicstackitem * _next;
    };
    _dynamicstackitem * _top;
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

template<typename T>
using tsfixedstack = tsstack<T, fixedstack<T>>;

template<typename T>
using tsfixedgrowingstack = tsstack<T, fixedgrowingstack<T>>;

template<typename T>
using tsdynamicstack = tsstack<T, dynamicstack<T>>;

/****
* Implementations.
*/

template <typename T>
fixedstack<T>::fixedstack(size_t size) {
    _stack = new T[size];
    _top = 0;
    _size = size;
}

template <typename T>
fixedstack<T>::fixedstack(const fixedstack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
}

template <typename T>
fixedstack<T>::~fixedstack() {
    delete[] _stack;
}

template <typename T>
fixedstack<T> & fixedstack<T>::operator=(const fixedstack<T> & o) {
    delete[] _stack;
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
    return *this;
}

template <typename T>
void fixedstack<T>::push(T t) {
    if (_top == _size) {
        throw CSError(LOGL_FATAL, 0, "stack is full.");
    }
    _stack[_top++] = t;
}

template <typename T>
void fixedstack<T>::pop() {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _top--;
}

template <typename T>
T fixedstack<T>::top() const {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _stack[_top - 1];
}

template <typename T>
void fixedstack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool fixedstack<T>::empty() const {
    return _top == 0;
}

template <typename T>
size_t fixedstack<T>::size() const {
    return _top;
}

template <typename T>
fixedgrowingstack<T>::fixedgrowingstack(size_t size) {
_stack = new T[size];
_top = 0;
_size = size;
}

template <typename T>
fixedgrowingstack<T>::fixedgrowingstack(const fixedgrowingstack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
}

template <typename T>
fixedgrowingstack<T>::~fixedgrowingstack() {
    delete[] _stack;
}

template <typename T>
fixedgrowingstack<T> & fixedgrowingstack<T>::operator=(const fixedgrowingstack<T> & o) {
    _stack = new T[o._size];
    _top = o._top;
    _size = o._size;
    for(size_t i = 0; i < _top; ++i) _stack[i] = o._stack[i];
    return *this;
}

template <typename T>
void fixedgrowingstack<T>::push(T t) {
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
void fixedgrowingstack<T>::pop() {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _top--;
}

template <typename T>
T fixedgrowingstack<T>::top() const {
    if (!_top) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _stack[_top - 1];
}

template <typename T>
void fixedgrowingstack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool fixedgrowingstack<T>::empty() const {
    return _top == 0;
}

template <typename T>
size_t fixedgrowingstack<T>::size() const {
    return _top;
}

template <typename T>
dynamicstack<T>::dynamicstack() {
    _top = nullptr;
    _size = 0;
}

template <typename T>
dynamicstack<T>::dynamicstack(size_t _) : dynamicstack<T>() { UnreferencedParameter(_); }

template <typename T>
dynamicstack<T>::dynamicstack(const dynamicstack<T> & o) {
    _size = o._size;
    _top = nullptr;
    _dynamicstackitem * next = o._top, * prev;
    if (next) {
        _top = new _dynamicstackitem(next->_item, nullptr);
        prev = _top;
        next = next->_next;
        while (next) {
            prev->_next = new _dynamicstackitem(next->_item, nullptr);
            prev = prev->_next;
            next = next->_next;
        }
    }
}

template <typename T>
dynamicstack<T>::~dynamicstack() {
    clear();
};

template <typename T>
dynamicstack<T> & dynamicstack<T>::operator=(const dynamicstack<T> & o) {
    _size = o._size;
    _top = nullptr;
    _dynamicstackitem * next = o._top, * prev;
    if (next) {
        _top = new _dynamicstackitem(next->_item, nullptr);
        prev = _top;
        next = next->_next;
        while (next) {
            prev->_next = new _dynamicstackitem(next->_item, nullptr);
            prev = prev->_next;
            next = next->_next;
        }
    }
    return *this;
}

template <typename T>
void dynamicstack<T>::push(T t) {
    _dynamicstackitem * ntop = new _dynamicstackitem(t, _top);
    _top = ntop;
    _size++;
}

template <typename T>
void dynamicstack<T>::pop() {
    if (_top == nullptr) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    _dynamicstackitem * oldtop = _top;
    _top = oldtop->_next;
    delete oldtop;
    _size--;
}

template <typename T>
T dynamicstack<T>::top() const {
    if (_top == nullptr) {
        throw CSError(LOGL_FATAL, 0, "stack is empty.");
    }
    return _top->_item;
}

template <typename T>
void dynamicstack<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool dynamicstack<T>::empty() const {
    return _top == nullptr;
}

template <typename T>
size_t dynamicstack<T>::size() const {
    return _size;
}

template <typename T>
dynamicstack<T>::_dynamicstackitem::_dynamicstackitem(T item, _dynamicstackitem * next) {
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
    o._mutex.shared_lock();
    _s = o._s;
    o._mutex.shared_unlock();
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
    _mutex.shared_lock();
    T x = _s.top();
    _mutex.shared_unlock();
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
    _mutex.shared_lock();
    bool b = _s.empty();
    _mutex.shared_unlock();
    return b;
}

template<typename T, class S>
size_t tsstack<T, S>::size() const {
    _mutex.shared_lock();
    size_t s = _s.size();
    _mutex.shared_unlock();
    return s;
}

#endif //_INC_CSTACK_H
