#ifndef TYPE_CONTAINER_VISITOR_H
#define TYPE_CONTAINER_VISITOR_H

#include "Dynamic/TypeContainer.h"

/*
 * @class TypeContainerVisitor is implemented as a visitor pattern.  It is
 * a visitor to the TypeContainerList or TypeContainerMapList.  The visitor has
 * to overload its types as a visit method is called.
 */

// Forward declaration
template<class T, class Y> class TypeContainerVisitor;

// Visitor helper
template<class VISITOR, class TYPE_CONTAINER> void VisitorHelper(VISITOR& v, TYPE_CONTAINER& c)
{
    v.Visit(c);
}

// Terminate condition container map list
template<class VISITOR> void VisitorHelper(VISITOR& /*v*/, ContainerMapList<TypeNull>& /*c*/) { }

template<class VISITOR, class T> void VisitorHelper(VISITOR& v, ContainerMapList<T>& c)
{
    v.Visit(c._element);
}

// Recursion container map list
template<class VISITOR, class H, class T> void VisitorHelper(VISITOR& v, ContainerMapList<TypeList<H, T>>& c)
{
    VisitorHelper(v, c._elements);
    VisitorHelper(v, c._TailElements);
}

// For TypeMapContainer
template<class VISITOR, class OBJECT_TYPES> void VisitorHelper(VISITOR& v, TypeMapContainer<OBJECT_TYPES>& c)
{
    VisitorHelper(v, c.GetElements());
}

// VectorContainer
template<class VISITOR> void VisitorHelper(VISITOR& /*v*/, ContainerVector<TypeNull>& /*c*/) {}

template<class VISITOR, class T> void VisitorHelper(VISITOR& v, ContainerVector<T>& c)
{
    v.Visit(c._element);
}

// Recursion container map list
template<class VISITOR, class H, class T> void VisitorHelper(VISITOR& v, ContainerVector<TypeList<H, T>>& c)
{
    VisitorHelper(v, c._elements);
    VisitorHelper(v, c._TailElements);
}

// For TypeMapContainer
template<class VISITOR, class OBJECT_TYPES> void VisitorHelper(VISITOR& v, TypeVectorContainer<OBJECT_TYPES>& c)
{
    VisitorHelper(v, c.GetElements());
}

// TypeUnorderedMapContainer
template<class VISITOR, class KEY_TYPE>
void VisitorHelper(VISITOR& /*v*/, ContainerUnorderedMap<TypeNull, KEY_TYPE>& /*c*/) { }

template<class VISITOR, class KEY_TYPE, class T>
void VisitorHelper(VISITOR& v, ContainerUnorderedMap<T, KEY_TYPE>& c)
{
    v.Visit(c._element);
}

template<class VISITOR, class KEY_TYPE, class H, class T>
void VisitorHelper(VISITOR& v, ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>& c)
{
    VisitorHelper(v, c._elements);
    VisitorHelper(v, c._TailElements);
}

template<class VISITOR, class OBJECT_TYPES, class KEY_TYPE>
void VisitorHelper(VISITOR& v, TypeUnorderedMapContainer<OBJECT_TYPES, KEY_TYPE>& c)
{
    VisitorHelper(v, c.GetElements());
}

template<class VISITOR, class TYPE_CONTAINER>
class TypeContainerVisitor
{
public:
    explicit TypeContainerVisitor(VISITOR& v) : i_visitor(v) { }

    void Visit(TYPE_CONTAINER& c)
    {
        VisitorHelper(i_visitor, c);
    }

    void Visit(const TYPE_CONTAINER& c) const
    {
        VisitorHelper(i_visitor, c);
    }

private:
    VISITOR& i_visitor;
};
#endif
