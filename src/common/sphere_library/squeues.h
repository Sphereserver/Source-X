/**
* @file CSArray.h
* @brief Queue custom implementations.
*/

#ifndef _INC_CQUEUE_H
#define _INC_CQUEUE_H

#include <shared_mutex>

#include "../common.h"
#include "../CException.h"

#define _SPHERE_QUEUE_DEFAULT_SIZE 10

template <typename T>
class fixedqueue {
public:
    fixedqueue(uint size=_SPHERE_QUEUE_DEFAULT_SIZE);
    fixedqueue(const fixedqueue<T> & o);
    ~fixedqueue();

    fixedqueue & operator=(const fixedqueue & o);

    void push(T t);
    void pop();
    T front();

    bool empty();
    void clear();
	uint size();
private:
    T * _queue;
	uint _front, _end, _size;
    bool _empty;
};

template <typename T>
class fixedgrowingqueue {
public:
    fixedgrowingqueue(uint size=_SPHERE_QUEUE_DEFAULT_SIZE);
    fixedgrowingqueue(const fixedgrowingqueue<T> & o);
    ~fixedgrowingqueue();

    fixedgrowingqueue & operator=(const fixedgrowingqueue<T> & o);

    void push(T t);
    void pop();
    T front();

    bool empty();
    void clear();
	uint size();

private:
    T * _queue;
	uint _front, _end, _size;
    bool _empty;
};

template <typename T>
class dynamicqueue {
public:
    dynamicqueue();
    // To allow thread secure wrapper constructor.
    dynamicqueue(uint _);
    dynamicqueue(const dynamicqueue<T> & o);
    ~dynamicqueue();

    dynamicqueue & operator=(const dynamicqueue<T> & o);

    void push(T t);
    void pop();
    T front();

    void clear();
    bool empty();
	uint size();
private:
    struct _dynamicqueueitem {
    public:
        _dynamicqueueitem(T item);
        T _item;
        _dynamicqueueitem * _next;
    };
    _dynamicqueueitem * _front, *_end;
	uint _size;
};

template <typename T, class Q>
class tsqueue {
public:
    tsqueue(uint size=_SPHERE_QUEUE_DEFAULT_SIZE);
    tsqueue(const tsqueue<T, Q> & o);

    tsqueue & operator=(const tsqueue<T, Q> & o);

    void push(T t);
    void pop();
    T front();
    T front_and_pop();

    void clear();
    bool empty();
	uint size();
private:
    std::shared_mutex _mutex;
    Q _q;
};

template<typename T>
using tsfixedqueue = tsqueue<T, fixedqueue<T>>;

template<typename T>
using tsfixedgrowingqueue = tsqueue<T, fixedgrowingqueue<T>>;

template<typename T>
using tsdynamicqueue = tsqueue<T, dynamicqueue<T>>;

/****
* Implementations.
*/

template <typename T>
fixedqueue<T>::fixedqueue(uint size) {
    _queue = new T[size];
    _size = size;
    _front = 0;
    _end = 0;
    _empty = true;
}

template <typename T>
fixedqueue<T>::fixedqueue(const fixedqueue<T> & o) {
    _size = o._size;
    _front = 0;
    _end = (o._end - o._front + o._size) % o._size;
    _empty = o._empty;
    _queue = new T[_size];
    if (!_empty) {
        _queue[0] = o._queue[o._front];
        for (uint i = 1; i % _size != _end; ++i) _queue[i] = o._queue[(i + o._front) % _size];
    }
}

template <typename T>
fixedqueue<T>::~fixedqueue() {
    delete[] _queue;
}

template <typename T>
fixedqueue<T> & fixedqueue<T>::operator=(const fixedqueue<T> & o) {
    delete[] _queue;
    _size = o._size;
    _front = 0;
    _end = (o._end - o._front + o._size) % o._size;
    _empty = o._empty;
    _queue = new T[_size];
    if (!_empty) {
        _queue[0] = o._queue[o._front];
        for(uint i = 1; i % _size != _end; ++i) _queue[i] = o._queue[(i + o._front) % _size];
    }
    return *this;
}

template <typename T>
void fixedqueue<T>::push(T t) {
    if (_end == _front && !_empty) {
        throw CSError(LOGL_FATAL, 0, "queue is full.");
    }
    _queue[_end] = t;
    _end++;
    _end %= _size;
    _empty = false;
}

template <typename T>
void fixedqueue<T>::pop() {
    if (_empty) {
        throw CSError(LOGL_FATAL, 0, "queue is empty.");
    }
    _front++;
    _front %= _size;
    if (_front == _end) {
        _empty = true;
    }
}

template <typename T>
T fixedqueue<T>::front() {
    if (_empty) {
        throw CSError(LOGL_FATAL, 0, "queue is empty.");
    }
    return _queue[_front];
}

template <typename T>
bool fixedqueue<T>::empty() {
    return _empty;
}

template <typename T>
void fixedqueue<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
uint fixedqueue<T>::size() {
    return _size;
}

template <typename T>
fixedgrowingqueue<T>::fixedgrowingqueue(uint size) {
	_queue = new T[size];
	_size = size;
	_front = 0;
	_end = 0;
	_empty = true;
}

template <typename T>
fixedgrowingqueue<T>::fixedgrowingqueue(const fixedgrowingqueue<T> & o) {
    _size = o._size;
    _front = 0;
    _end = (o._end - o._front + o._size) % o._size;
    _empty = o._empty;
    _queue = new T[_size];
    if (!_empty) {
        _queue[0] = o._queue[o._front];
        for(uint i = 1; i % _size != _end; ++i) _queue[i] = o._queue[(i + o._front) % _size];
    }
}

template <typename T>
fixedgrowingqueue<T>::~fixedgrowingqueue() {
    delete[] _queue;
}

template <typename T>
fixedgrowingqueue<T> & fixedgrowingqueue<T>::operator=(const fixedgrowingqueue<T> & o) {
    delete[] _queue;
    _size = o._size;
    _front = 0;
    _end = (o._end - o._front + o._size) % o._size;
    _empty = o._empty;
    _queue = new T[_size];
    if (!_empty) {
        _queue[0] = o._queue[o._front];
        for(uint i = 1; i % _size != _end; ++i) _queue[i] = o._queue[(i + o._front) % _size];
    }
    return *this;
}

template <typename T>
void fixedgrowingqueue<T>::push(T t) {
    if (_end == _front && !_empty) {
        T * nqueue = new T[_size + _SPHERE_QUEUE_DEFAULT_SIZE];
        nqueue[0] = _queue[_front];
        for(uint i = 1; i % _size != _end; ++i) nqueue[i] = _queue[(i + _front) % _size];
        _end = _size - 1;
        _front = 0;
        delete[] _queue;
        _queue = nqueue;
        _size += _SPHERE_QUEUE_DEFAULT_SIZE;
    }
    _queue[_end] = t;
    _end++;
    _end %= _size;
    _empty = false;
}

template <typename T>
void fixedgrowingqueue<T>::pop() {
    if (_empty) {
        throw CSError(LOGL_FATAL, 0, "queue is empty.");
    }
    _front++;
    _front %= _size;
    if (_front == _end) {
        _empty = true;
    }
}

template <typename T>
T fixedgrowingqueue<T>::front() {
    if (_empty) {
        throw CSError(LOGL_FATAL, 0, "queue is empty.");
    }
    return _queue[_front];
}

template <typename T>
bool fixedgrowingqueue<T>::empty() {
    return _empty;
}

template <typename T>
void fixedgrowingqueue<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
uint fixedgrowingqueue<T>::size() {
    return _size;
}

template <typename T>
dynamicqueue<T>::dynamicqueue() {
    _size = 0;
    _front = NULL;
    _end = NULL;
}

template <typename T>
dynamicqueue<T>::dynamicqueue(uint _) : dynamicqueue<T>() { UNREFERENCED_PARAMETER(_); }

template <typename T>
dynamicqueue<T>::dynamicqueue(const dynamicqueue<T> & o) {
    _size = o._size;
    _end = _front = NULL;
    _dynamicqueueitem * item = o._front;
    if (_size) {
        _front = new _dynamicqueueitem(item->_item);
        _end = _front;
        while (item->_next != NULL) {
            item = item->_next;
            _end->_next = new _dynamicqueueitem(item->_item);
            _end = _end->_next;
        }
    }
}

template <typename T>
dynamicqueue<T>::~dynamicqueue() {
    clear();
}

template <typename T>
dynamicqueue<T> & dynamicqueue<T>::operator=(const dynamicqueue<T> & o) {
    clear();
    _size = o._size;
    _end = _front = NULL;
    _dynamicqueueitem * item = o._front;
    if (_size) {
        _front = new _dynamicqueueitem(item->_item);
        _end = _front;
        while (item->_next != NULL) {
            item = item->_next;
            _end->_next = new _dynamicqueueitem(item->_item);
            _end = _end->_next;
        }
    }
}

template <typename T>
void dynamicqueue<T>::push(T t) {
    _dynamicqueueitem * item = new _dynamicqueueitem(t);
    if (_size) {
        _end->_next = item;
        _end = item;
    } else {
        _front = _end = item;
    }
    _size++;
}

template <typename T>
void dynamicqueue<T>::pop() {
    if (!_size) throw CSError(LOGL_FATAL, 0, "Queue is empty.");
    _dynamicqueueitem * i = _front;
    _front = i->_next;
    delete i;
    _size--;
}

template <typename T>
T dynamicqueue<T>::front() {
    if (!_size) throw CSError(LOGL_FATAL, 0, "Queue is empty");
    return _front->_item;
}

template <typename T>
void dynamicqueue<T>::clear() {
    while(!empty()) {
        pop();
    }
}

template <typename T>
bool dynamicqueue<T>::empty() {
    return _size == 0;
}

template <typename T>
uint dynamicqueue<T>::size() {
    return _size;
}

template <typename T>
dynamicqueue<T>::_dynamicqueueitem::_dynamicqueueitem(T item) {
	_item = item;
	_next = NULL;
}

template <typename T, class Q>
tsqueue<T, Q>::tsqueue(uint size) : _q(size) {}

template <typename T, class Q>
tsqueue<T, Q>::tsqueue(const tsqueue<T, Q> & o) {
    o._mutex.lock_shared();
    _q = o._q;
    o._mutex.unlock_shared();
}

template <typename T, class Q>
tsqueue<T, Q> & tsqueue<T, Q>::operator=(const tsqueue<T, Q> & o) {
    _mutex.lock();
    o._mutex.lock_shared();
    _q = o._q;
    o._mutex.unlock_shared();
    _mutex.unlock();
    return *this;
}

template <typename T, class Q>
void tsqueue<T, Q>::push(T t) {
    _mutex.lock();
    _q.push(t);
    _mutex.unlock();
}

template <typename T, class Q>
void tsqueue<T, Q>::pop() {
    _mutex.lock();
    _q.pop();
    _mutex.unlock();
}

template <typename T, class Q>
T tsqueue<T, Q>::front() {
    _mutex.lock_shared();
    T x = _q.front();
    _mutex.unlock_shared();
    return x;
}

template <typename T, class Q>
T tsqueue<T, Q>::front_and_pop() {
    _mutex.lock();
    T x = _q.front();
    _q.pop();
    _mutex.unlock();
    return x;
}

template <typename T, class Q>
void tsqueue<T, Q>::clear() {
    _mutex.lock();
    _q.clear();
    _mutex.unlock();
}

template <typename T, class Q>
bool tsqueue<T, Q>::empty() {
    _mutex.lock_shared();
    bool b = _q.empty();
    _mutex.unlock_shared();
    return b;
}

template <typename T, class Q>
uint tsqueue<T, Q>::size() {
    _mutex.lock_shared();
    uint s = _q.size();
    _mutex.unlock_shared();
    return s;
}

#endif //_INC_CQUEUE_H
