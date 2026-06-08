#ifndef DBC_STORAGE_ITERATOR_H
#define DBC_STORAGE_ITERATOR_H

#include <map>
#include "Define.h"

template <class T>
class DBCStorageIterator
{
public:
    explicit DBCStorageIterator(std::map<uint32, std::unique_ptr<T>>::iterator start_it) : it(start_it) {}

    const T* operator->() { return it->second.get(); }
    const T* operator*() { return it->second.get(); }

    DBCStorageIterator& operator++() { ++it; return *this; }
    DBCStorageIterator operator++(int) { DBCStorageIterator tmp = *this; ++it; return tmp; }

    bool operator==(const DBCStorageIterator& other) const { return it == other.it; }
    bool operator!=(const DBCStorageIterator& other) const { return it != other.it; }
private:
    std::map<uint32, std::unique_ptr<T>>::iterator it;
};

#endif
