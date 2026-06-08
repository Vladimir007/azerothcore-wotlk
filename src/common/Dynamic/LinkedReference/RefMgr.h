#ifndef NCORE_REF_MANAGER_H
#define NCORE_REF_MANAGER_H

#include "Dynamic/LinkedList.h"
#include "Dynamic/LinkedReference/Reference.h"

template <class TO, class FROM> class RefMgr : public LinkedListHead
{
public:
    typedef Iterator<Reference<TO, FROM>> iterator;
    RefMgr() = default;
    ~RefMgr() override { clearReferences(); }

    Reference<TO, FROM>* getFirst() override { return static_cast<Reference<TO, FROM>*>(LinkedListHead::getFirst()); }
    [[nodiscard]] Reference<TO, FROM> const* getFirst() const override { return static_cast<Reference<TO, FROM> const*>(LinkedListHead::getFirst()); }
    Reference<TO, FROM>* getLast() override { return static_cast<Reference<TO, FROM>*>(LinkedListHead::getLast()); }
    [[nodiscard]] Reference<TO, FROM> const* getLast() const override { return static_cast<Reference<TO, FROM> const*>(LinkedListHead::getLast()); }

    virtual iterator begin() { return iterator(getFirst()); }
    virtual iterator end() { return iterator(nullptr); }
    virtual iterator rbegin() { return iterator(getLast()); }
    virtual iterator rend() { return iterator(nullptr); }

    void clearReferences()
    {
        LinkedListElement* ref;
        while ((ref = getFirst()) != nullptr)
        {
            static_cast<Reference<TO, FROM>*>(ref)->invalidate();
            ref->delink();  // The 'delink' might be already done by invalidate(), but doing it here again does not hurt and insures an empty list
        }
    }
};

#endif
