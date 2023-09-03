/**
* @file smap.h
* @brief Map custom implementations.
*/

#ifndef _INC_SMAP_H
#define _INC_SMAP_H

#include <shared_mutex>
#include <cstdint>
#include "../CException.h"

#define _SPHERE_MAP_DEFAULT_SIZE 10

template<typename K, typename V>
class staticmap {
public:
    class CellPosition {
    public:
        V & operator=(V & x);
        operator V ();
        friend class staticmap;
    private:
        CellPosition(staticmap * m, bool e, K k, size_t p);
        staticmap * _map;
        bool _exists;
        K _key;
        size_t _position;
    };

    staticmap(size_t size=_SPHERE_MAP_DEFAULT_SIZE);
    staticmap(const staticmap & o);

    ~staticmap();

    staticmap & operator=(const staticmap & o);

    bool empty() const;
    size_t size() const;
    size_t max_size() const;

    CellPosition operator[](K key);
    void erase(K key);
    void swap(staticmap & o);
    void clear();
    size_t count(K key) const;
protected:
    class Cell {
    public:
        K key;
        V value;
    };
    void _reserve(size_t n);
    void _resize(size_t s);

    Cell * _map;
    size_t _capacity;
    size_t _size;
};

template<typename K, typename V>
class dynamicmap {
protected:
    class Node {
    public:
        K key;
        V value;
        Node * l, * r, *p;
        Node();
        Node(Node & o);
        ~Node();
        size_t child_count() const;
        void replace_by_child();
        void remove_child(Node * n);
    };
    Node * _root;
    size_t _size;
public:
    class CellPosition {
    public:
        CellPosition(dynamicmap * m, Node * n, bool e, K k);
        V & operator=(V & x);
        operator V ();
        friend class dynamicmap;
    private:
        dynamicmap * _map;
        Node * _n;
        bool _exists;
        K _key;
    };

    dynamicmap();
    dynamicmap(size_t size);
    dynamicmap(const dynamicmap & o);
    ~dynamicmap();

    dynamicmap & operator=(const dynamicmap & o);

    bool empty() const;
    size_t size() const;
    size_t count(K key) const;

    CellPosition operator[](K key);

    void erase(K key);

    void swap(dynamicmap & o);
    void clear();
};


template<typename K, typename V, typename M>
class tsmap {
public:
    class CellPosition {
    public:
        CellPosition(tsmap * m, typename M::CellPosition c);
        ~CellPosition();
        V & operator=(V & x);
        operator V ();
    private:
        tsmap * _map;
        typename M::CellPosition _cell;
        bool _unlocked;
    };

    tsmap(size_t size=_SPHERE_MAP_DEFAULT_SIZE);
    tsmap(const tsmap & o);

    tsmap & operator=(const tsmap & o);

    bool empty() const;
    size_t size() const;
    size_t count(K key) const;

    CellPosition operator[](K key);

    void erase(K key);

    void swap(tsmap & o);
    void clear();
private:
    mutable std::shared_mutex _mutex;
    M _map;
};

template <typename K, typename V>
using tsstaticmap = tsmap<K, V, staticmap<K, V> >;

template <typename K, typename V>
using tsdynamicmap = tsmap<K, V, dynamicmap<K, V> >;

/****
* Implementations.
*/

template<typename K, typename V>
staticmap<K,V>::CellPosition::CellPosition(staticmap * m, bool e, K k, size_t p) {
    _map = m;
    _exists = e;
    _key = k;
    _position = p;
}

template<typename K, typename V>
V & staticmap<K,V>::CellPosition::operator=(V & x) {
    if (!_exists) {
        _map->_reserve(_map->_size + 1);
        memmove(&_map->_map[_position + 1], &_map->_map[_position], sizeof(staticmap<K,V>::Cell) * (_map->_size - _position));
        _map->_size++;
        _map->_map[_position].key = _key;
    }
    _map->_map[_position].value = x;
    return x;
}

template<typename K, typename V>
staticmap<K,V>::CellPosition::operator V () {
    if (!_exists) {
        throw CSError(LOGL_FATAL, 0, "Key not found");
    }
    return _map->_map[_position].value;
}

template<typename K, typename V>
staticmap<K,V>::staticmap(size_t size) {
    _capacity = size;
    _size = 0;
    _map = new Cell[_capacity];
}

template<typename K, typename V>
staticmap<K,V>::staticmap(const staticmap<K,V> & o) {
    _capacity = o._capacity;
    _size = o._size;
    _map = new Cell[_capacity];
    memcpy(_map, o._map, sizeof(Cell) * _capacity);
}

template<typename K, typename V>
staticmap<K,V>::~staticmap() {
    delete[] _map;
}

template<typename K, typename V>
staticmap<K,V> & staticmap<K,V>::operator=(const staticmap<K,V> & o) {
    delete[] _map;
    _capacity = o._capacity;
    _size = o._size;
    _map = new Cell[_capacity];
    memcpy(_map, o._map, sizeof(Cell) * _capacity);
    return *this;
}

template<typename K, typename V>
bool staticmap<K,V>::empty() const {
    return _size == 0;
}

template<typename K, typename V>
size_t staticmap<K,V>::size() const {
    return _size;
}

template<typename K, typename V>
size_t staticmap<K,V>::max_size() const {
    return UINTPTR_MAX;
}

template<typename K, typename V>
typename staticmap<K,V>::CellPosition staticmap<K,V>::operator[](K key) {
    size_t init = 0, end = _size - 1, pivot = 0;
    K pivot_key;
    bool stop = false;
    while ( init > end && !stop) {
        pivot = (end - init) / 2 + init;
        pivot_key = _map[pivot].key;
        if (pivot_key == key) {
            init = pivot;
            stop = true;
        } else if (key < pivot_key) {
            end = pivot - 1;
        } else {
            init = pivot + 1;
        }
    }
    return CellPosition(this, pivot_key == key, key, pivot);
}

template<typename K, typename V>
void staticmap<K,V>::erase(K key) {
    CellPosition p = (*this)[key];
    if (p._exists) {
        memmove(&_map[p._position], &_map[p._position + 1], sizeof(Cell) * (_size - p._position));
        _size--;
    }
}


template<typename K, typename V>
void staticmap<K,V>::swap(staticmap & o) {
    Cell * map = _map;
    size_t size = _size;
    size_t capacity = _capacity;
    _map = o._map;
    _size = o._size;
    _capacity = o._capacity;
    o._map = map;
    o._size = size;
    o._capacity = capacity;
}


template<typename K, typename V>
void staticmap<K,V>::clear() {
    _size = 0;
}

template<typename K, typename V>
size_t staticmap<K,V>::count(K key) const {
    CellPosition p = (*this)[key];
    if (p._exists) { return 1; }
    return 0;
}

template<typename K, typename V>
void staticmap<K,V>::_reserve(size_t n) {
    if (_capacity < n) {
        _resize(n + _SPHERE_MAP_DEFAULT_SIZE);
    }
}

template<typename K, typename V>
void staticmap<K,V>::_resize(size_t s) {
    Cell * n = new Cell[s];
    size_t nsize = _size;
    if (s < _size) nsize = s;
    memcpy(n, _map, sizeof(Cell) * nsize);
    _size = nsize;
    _capacity = s;
    delete[] _map;
    _map = n;
}

template<typename K, typename V>
dynamicmap<K,V>::Node::Node() : l(nullptr), r(nullptr), p(nullptr) {}

template<typename K, typename V>
dynamicmap<K,V>::Node::Node(Node & o) {
    key = o.key;
    value = o.value;
    l = nullptr;
    r = nullptr;
    p = o.p;
    if (o.l) { l = new Node(*(o.l)); }
    if (o.r) { r = new Node(*(o.r)); }
}

template<typename K, typename V>
dynamicmap<K,V>::Node::~Node() {
    if (l) { delete l; }
    if (r) { delete r; }
}

template<typename K, typename V>
size_t dynamicmap<K,V>::Node::child_count() const {
    size_t count = 0;
    if (l != nullptr) count++;
    if (r != nullptr) count++;
    return count;
}

template<typename K, typename V>
void dynamicmap<K,V>::Node::replace_by_child() {
    if (r != nullptr && l != nullptr) {
        throw CSError(LOGL_FATAL, 0, "Tree inconsistency.");
    }
    Node * replacement;
    if (l != nullptr) {
        replacement = l;
        l = nullptr;
    } else {
        replacement = r;
        r = nullptr;
    }
    replacement->p = p;
    if (p != nullptr) {
        if (p->l == this) {
            p->l = replacement;
        } else if (p->r == this) {
            p->r = replacement;
        } else {
            throw CSError(LOGL_FATAL, 0, "Tree inconsistency.");
        }
    }
}

template<typename K, typename V>
void dynamicmap<K,V>::Node::remove_child(Node * n) {
    if (l == n) {
        l = nullptr;
    } else if (r == n) {
        r = nullptr;
    } else {
        throw CSError(LOGL_FATAL, 0, "Tree inconsistency.");
    }
    n->p = nullptr;
}

template<typename K, typename V>
V & dynamicmap<K,V>::CellPosition::operator=(V & x) {
    if (!_exists) {
        Node * n2 = new Node();
        n2->key = _key;
        n2->value = x;
        if (_key < _n->key) { _n->l = n2; }
        if (_key > _n->key) { _n->r = n2; }
        n2->p = _n;
        _n = n2;
        _map->_size++;
    }
    _n->value = x;
    return x;
}

template<typename K, typename V>
dynamicmap<K,V>::CellPosition::operator V () {
    if (!_exists) {
        throw CSError(LOGL_FATAL, 0, "Key not found");
    }
    return _n->value;
}

template<typename K, typename V>
dynamicmap<K,V>::dynamicmap() {
    _root = nullptr;
    _size = 0;
}

template<typename K, typename V>
dynamicmap<K,V>::dynamicmap(size_t size) : dynamicmap<K,V>() { UnreferencedParameter(size); }

template<typename K, typename V>
dynamicmap<K,V>::dynamicmap(const dynamicmap & o) {
    _root = new Node(*(o._root));
    _size = o._size;
}

template<typename K, typename V>
dynamicmap<K,V>::~dynamicmap() {
    delete _root;
}

template<typename K, typename V>
dynamicmap<K,V> & dynamicmap<K,V>::operator=(const dynamicmap<K,V> & o) {
    delete _root;
    _root = new Node(*(o._root));
    _size = o._size;
}

template<typename K, typename V>
bool dynamicmap<K,V>::empty() const {
    return _size == 0;
}

template<typename K, typename V>
size_t dynamicmap<K,V>::size() const {
    return _size;
}

template<typename K, typename V>
dynamicmap<K,V>::CellPosition::CellPosition(dynamicmap * m, Node * n, bool e, K k) {
    _map = m;
    _n = n;
    _exists = e;
    _key = k;
}

template<typename K, typename V>
typename dynamicmap<K,V>::CellPosition dynamicmap<K,V>::operator[](K key) {
    Node * n = _root, * n_ant = nullptr;
    while(n != nullptr && n->key != key) {
        n_ant = n;
        if (n->key < key) { n = n->r; }
        else { n = n->l; }
    }
    return CellPosition(this, (n != nullptr) ? n : n_ant, n != nullptr, key);
}

template<typename K, typename V>
void dynamicmap<K,V>::erase(K key) {
    CellPosition p = (*this)[key];
    if (!p._exists) {
        throw CSError(LOGL_FATAL, 0, "Key not found.");
    }
    Node * n = p._n;
    // Case leaf:
    size_t childs = n->child_count();
    if (childs == 0) {
        n->p->remove_child(n);
        delete n;
        // Case one child.
    } else if (childs == 1) {
        n->replace_by_child();
        delete n;
        // Case two childs.
    } else {
        // Search for a minimum.
        Node * min = n->r;
        while (min->l != nullptr) min = min->l;
        if (min->child_count() == 1) {
            min->replace_by_child();
        } else {
            min->p->remove_child(min);
        }
        n->key = min->key;
        n->value = min->value;
        delete min;
    }
    _size--;
}

template<typename K, typename V>
void dynamicmap<K,V>::swap(dynamicmap & o) {
    Node * tmproot = _root;
    size_t tmpsize = _size;
    _root = o._root;
    _size = o._size;
    o._root = tmproot;
    o._size = tmpsize;
}

template<typename K, typename V>
void dynamicmap<K,V>::clear() {
    delete _root;
    _size = 0;
}

template<typename K, typename V>
size_t dynamicmap<K,V>::count(K key) const {
    CellPosition p = (*this)[key];
    if (p._exists) { return 1; }
    return 0;
}

template<typename K, typename V, typename M>
tsmap<K,V,M>::CellPosition::CellPosition(tsmap<K,V,M> * m, typename M::CellPosition c) : _map(m), _cell(c), _unlocked(false) {}

template<typename K, typename V, typename M>
tsmap<K,V,M>::CellPosition::~CellPosition() {
    if (!_unlocked) {
        _map->_mutex.unlock();
    }
}

template<typename K, typename V, typename M>
V & tsmap<K,V,M>::CellPosition::operator=(V & x) {
    _cell = x;
    _map->_mutex.unlock();
    _unlocked = true;
    return x;
}

template<typename K, typename V, typename M>
tsmap<K,V,M>::CellPosition::operator V () {
    V x = _cell;
    _map->_mutex.unlock();
    _unlocked = true;
    return x;
}


template<typename K, typename V, typename M>
tsmap<K,V,M>::tsmap(size_t size) : _map(size) {}


template<typename K, typename V, typename M>
tsmap<K,V,M>::tsmap(const tsmap & o) {
    _mutex.lock();
    o._mutex.lock_shared();
    _map = o._map;
    o._mutex.unlock_shared();
    _mutex.unlock();
}

template<typename K, typename V, typename M>
tsmap<K,V,M> & tsmap<K,V,M>::operator=(const tsmap<K,V,M> & o) {
    _mutex.lock();
    o._mutex.lock_shared();
    _map = o._map;
    o._mutex.unlock_shared();
    _mutex.unlock();
    return *this;
}

template<typename K, typename V, typename M>
bool tsmap<K,V,M>::empty() const {
    bool e;
    _mutex.lock_shared();
    e = _map.empty();
    _mutex.unlock_shared();
    return e;
}

template<typename K, typename V, typename M>
size_t tsmap<K,V,M>::size() const {
    size_t s;
    _mutex.lock_shared();
    s = _map.size();
    _mutex.unlock_shared();
    return s;
}

template<typename K, typename V, typename M>
typename tsmap<K,V,M>::CellPosition tsmap<K,V,M>::operator[](K key) {
    _mutex.lock();
    return CellPosition(this, _map[key]);
}

template<typename K, typename V, typename M>
void tsmap<K,V,M>::erase(K key) {
    _mutex.lock();
    _map.erase(key);
    _mutex.unlock();
}

template<typename K, typename V, typename M>
void tsmap<K,V,M>::swap(tsmap<K,V,M> & o) {
    _mutex.lock();
    o._mutex.lock();
    _map.swap(o._map);
    o._mutex.unlock();
    _mutex.unlock();
}

template<typename K, typename V, typename M>
void tsmap<K,V,M>::clear() {
    _mutex.lock();
    _map.clear();
    _mutex.unlock();
}

template<typename K, typename V, typename M>
size_t tsmap<K,V,M>::count(K key) const {
    size_t c;
    _mutex.lock_shared();
    c = _map.count(key);
    _mutex.unlock_shared();
    return c;
}

#endif //_INC_SMAP_H