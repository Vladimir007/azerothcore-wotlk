#ifndef NCORE_GRID_REF_MANAGER_H
#define NCORE_GRID_REF_MANAGER_H

#include "RefMgr.h"

template<class OBJECT>
class GridReference;

template<class OBJECT>
class GridRefMgr : public RefMgr<GridRefMgr<OBJECT>, OBJECT>
{
public:
    typedef LinkedListHead::Iterator< GridReference<OBJECT> > iterator;

    GridReference<OBJECT>* getFirst() override
    {
        return static_cast<GridReference<OBJECT>*>(RefMgr<GridRefMgr, OBJECT>::getFirst());
    }

    GridReference<OBJECT>* getLast() override
    {
        return static_cast<GridReference<OBJECT>*>(RefMgr<GridRefMgr, OBJECT>::getLast());
    }

    iterator begin() override { return iterator(getFirst()); }
    iterator end() override { return iterator(nullptr); }
    iterator rbegin() override { return iterator(getLast()); }
    iterator rend() override { return iterator(nullptr); }
};

#endif
