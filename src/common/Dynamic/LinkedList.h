#ifndef NCORE_LINKED_LIST
#define NCORE_LINKED_LIST

#include <iterator>
#include "Define.h"

class LinkedListHead;

class LinkedListElement
{
    friend class LinkedListHead;

    LinkedListElement* iNext{nullptr};
    LinkedListElement* iPrev{nullptr};

public:
    LinkedListElement() = default;
    virtual ~LinkedListElement() { delink(); }

    [[nodiscard]] bool hasNext() const { return iNext && iNext->iNext != nullptr; }
    [[nodiscard]] bool hasPrev() const { return iPrev && iPrev->iPrev != nullptr; }
    [[nodiscard]] bool isInList() const { return iNext != nullptr && iPrev != nullptr; }

    virtual LinkedListElement* next() { return hasNext() ? iNext : nullptr; }
    [[nodiscard]] virtual LinkedListElement const* next() const { return hasNext() ? iNext : nullptr; }
    virtual LinkedListElement* prev() { return hasPrev() ? iPrev : nullptr; }
    [[nodiscard]] virtual LinkedListElement const* prev() const { return hasPrev() ? iPrev : nullptr; }

    virtual LinkedListElement* nocheck_next() { return iNext; }
    [[nodiscard]] virtual LinkedListElement const* nocheck_next() const { return iNext; }
    virtual LinkedListElement* nocheck_prev() { return iPrev; }
    [[nodiscard]] virtual LinkedListElement const* nocheck_prev() const { return iPrev; }

    void delink()
    {
        if (isInList())
        {
            iNext->iPrev = iPrev;
            iPrev->iNext = iNext;
            iNext = nullptr;
            iPrev = nullptr;
        }
    }

    void insertBefore(LinkedListElement* pElem)
    {
        pElem->iNext = this;
        pElem->iPrev = iPrev;
        iPrev->iNext = pElem;
        iPrev = pElem;
    }

    void insertAfter(LinkedListElement* pElem)
    {
        pElem->iPrev = this;
        pElem->iNext = iNext;
        iNext->iPrev = pElem;
        iNext = pElem;
    }
};

class LinkedListHead
{
    LinkedListElement iFirst;
    LinkedListElement iLast;
    uint32 iSize{0};

public:
    virtual ~LinkedListHead() = default;

    LinkedListHead()
    {
        // Create empty list
        iFirst.iNext = &iLast;
        iLast.iPrev = &iFirst;
    }

    [[nodiscard]] bool IsEmpty() const { return !iFirst.iNext->isInList(); }

    virtual LinkedListElement* getFirst() { return IsEmpty() ? nullptr : iFirst.iNext; }
    [[nodiscard]] virtual LinkedListElement const* getFirst() const { return IsEmpty() ? nullptr : iFirst.iNext; }

    virtual LinkedListElement* getLast() { return IsEmpty() ? nullptr : iLast.iPrev; }
    [[nodiscard]] virtual LinkedListElement const* getLast() const  { return IsEmpty() ? nullptr : iLast.iPrev; }

    void insertFirst(LinkedListElement* pElem)
    {
        iFirst.insertAfter(pElem);
    }

    void insertLast(LinkedListElement* pElem)
    {
        iLast.insertBefore(pElem);
    }

    [[nodiscard]] uint32 getSize() const
    {
        if (!iSize)
        {
            uint32 result = 0;
            LinkedListElement const* e = getFirst();
            while (e)
            {
                ++result;
                e = e->next();
            }
            return result;
        }
        return iSize;
    }

    void incSize() { ++iSize; }
    void decSize() { --iSize; }

    template<class _Ty>
    class Iterator
    {
    public:
        typedef std::bidirectional_iterator_tag     iterator_category;
        typedef _Ty                                 value_type;
        typedef ptrdiff_t                           difference_type;
        typedef ptrdiff_t                           distance_type;
        typedef _Ty*                                pointer;
        typedef _Ty const*                          const_pointer;
        typedef _Ty&                                reference;
        typedef _Ty const&                          const_reference;

        Iterator() : _Ptr(nullptr)
        {
            // construct with null node pointer
        }

        explicit Iterator(const pointer _pNode) : _Ptr(_pNode)
        {
            // Construct with node pointer _pNode
        }

        Iterator& operator=(Iterator const& _Right)
        {
            _Ptr = _Right._Ptr;
            return *this;
        }

        Iterator& operator=(const_pointer const& _Right)
        {
            _Ptr = pointer(_Right);
            return *this;
        }

        reference operator*()
        {
            // Return designated value
            return *_Ptr;
        }

        pointer operator->()
        {
            // Return pointer to class object
            return _Ptr;
        }

        Iterator& operator++()
        {
            // Preincrement
            _Ptr = _Ptr->next();
            return *this;
        }

        Iterator operator++(int)
        {
            // Postincrement
            iterator _Tmp = *this;
            ++*this;
            return _Tmp;
        }

        Iterator& operator--()
        {
            // predecrement
            _Ptr = _Ptr->prev();
            return *this;
        }

        Iterator operator--(int)
        {
            // postdecrement
            iterator _Tmp = *this;
            --*this;
            return _Tmp;
        }

        bool operator==(Iterator const& _Right) const
        {
            // Test for iterator equality
            return _Ptr == _Right._Ptr;
        }

        bool operator!=(Iterator const& _Right) const
        {
            // Rest for iterator inequality
            return !(*this == _Right);
        }

        bool operator==(pointer const& _Right) const
        {
            // Test for pointer equality
            return _Ptr != _Right;
        }

        bool operator!=(pointer const& _Right) const
        {
            // Test for pointer equality
            return !(*this == _Right);
        }

        bool operator==(const_reference _Right) const
        {
            // Test for reference equality
            return _Ptr == &_Right;
        }

        bool operator!=(const_reference _Right) const
        {
            // Test for reference equality
            return _Ptr != &_Right;
        }

        pointer _Mynode()
        {
            // Return node pointer
            return _Ptr;
        }

    protected:
        pointer _Ptr; // Pointer to node
    };

    typedef Iterator<LinkedListElement> iterator;
};

#endif
