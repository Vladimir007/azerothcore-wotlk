#ifndef NCORE_TYPE_CONTAINER_H
#define NCORE_TYPE_CONTAINER_H

#include <unordered_map>
#include <vector>
#include "GridRefMgr.h"
#include "Dynamic/TypeList.h"

/*
 * @class ContainerMapList is a multi-type container for map elements
 * By itself its meaningless but collaborate along with TypeContainers,
 * it becomes the most powerfully container in the whole system.
 */
template<class OBJECT>
struct ContainerMapList
{
    GridRefMgr<OBJECT> _element;
};

template<>
struct ContainerMapList<TypeNull>  // Nothing is in type null
{
};

template<class H, class T>
struct ContainerMapList<TypeList<H, T>>
{
    ContainerMapList<H> _elements;
    ContainerMapList<T> _TailElements;
};

template<class OBJECT>
struct ContainerVector
{
    std::vector<OBJECT*> _element;
};

template<>
struct ContainerVector<TypeNull>
{
};

template<class H, class T>
struct ContainerVector<TypeList<H, T>>
{
    ContainerVector<H> _elements;
    ContainerVector<T> _TailElements;
};

template<class OBJECT, class KEY_TYPE>
struct ContainerUnorderedMap
{
    std::unordered_map<KEY_TYPE, OBJECT*> _element;
};

template<class KEY_TYPE>
struct ContainerUnorderedMap<TypeNull, KEY_TYPE>
{
};

template<class H, class T, class KEY_TYPE>
struct ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>
{
    ContainerUnorderedMap<H, KEY_TYPE> _elements;
    ContainerUnorderedMap<T, KEY_TYPE> _TailElements;
};

// @class ContainerList is a simple list of different types of elements
template<class OBJECT> struct ContainerList
{
    OBJECT _element;
};

// TypeNull is undefined
template<> struct ContainerList<TypeNull> { };
template<class H, class T> struct ContainerList<TypeList<H, T>>
{
    ContainerList<H> _elements;
    ContainerMapList<T> _TailElements;
};

#include "TypeContainerFunctions.h"
/*
 * @class TypeMapContainer contains a fixed number of types and is
 * determined at compile time.  This is probably the most complicated
 * class and do its simplest thing, that is, holds objects
 * of different types.
 */

template<class OBJECT_TYPES>
class TypeMapContainer
{
public:
    template<class SPECIFIC_TYPE> [[nodiscard]] std::size_t Count() const
    {
        return Acore::Count(i_elements, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    /// Inserts a specific object into the container
    template<class SPECIFIC_TYPE>
    bool insert(SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Acore::Insert(i_elements, obj);
        return t != nullptr;
    }

    ContainerMapList<OBJECT_TYPES>& GetElements() { return i_elements; }
    [[nodiscard]] const ContainerMapList<OBJECT_TYPES>& GetElements() const { return i_elements;}

private:
    ContainerMapList<OBJECT_TYPES> i_elements;
};

template<class OBJECT_TYPES>
class TypeVectorContainer
{
public:
    template<class SPECIFIC_TYPE> [[nodiscard]] std::size_t Count() const { return Acore::Count(i_elements, static_cast<SPECIFIC_TYPE*>(nullptr)); }

    template<class SPECIFIC_TYPE>
    bool Insert(SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Acore::Insert(i_elements, obj);
        return (t != nullptr);
    }

    template<class SPECIFIC_TYPE>
    bool Remove(SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Acore::Remove(i_elements, obj);
        return (t != nullptr);
    }

    ContainerVector<OBJECT_TYPES>& GetElements() { return i_elements; }
    [[nodiscard]] const ContainerVector<OBJECT_TYPES>& GetElements() const { return i_elements; }

private:
    ContainerVector<OBJECT_TYPES> i_elements;
};

template<class OBJECT_TYPES, class KEY_TYPE>
class TypeUnorderedMapContainer
{
public:
    template<class SPECIFIC_TYPE>
    bool Insert(KEY_TYPE const& handle, SPECIFIC_TYPE* obj)
    {
        return Acore::Insert(_elements, handle, obj);
    }

    template<class SPECIFIC_TYPE>
    bool Remove(KEY_TYPE const& handle)
    {
        return Acore::Remove(_elements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Find(KEY_TYPE const& handle)
    {
        return Acore::Find(_elements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    template<class SPECIFIC_TYPE>
    [[nodiscard]] std::size_t Size() const
    {
        std::size_t size = 0;
        Acore::Size(_elements, &size, static_cast<SPECIFIC_TYPE*>(nullptr));
        return size;
    }

    ContainerUnorderedMap<OBJECT_TYPES, KEY_TYPE>& GetElements() { return _elements; }
    [[nodiscard]] ContainerUnorderedMap<OBJECT_TYPES, KEY_TYPE> const& GetElements() const { return _elements; }

private:
    ContainerUnorderedMap<OBJECT_TYPES, KEY_TYPE> _elements;
};

#endif
