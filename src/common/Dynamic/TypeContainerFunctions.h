#ifndef TYPE_CONTAINER_FUNCTIONS_H
#define TYPE_CONTAINER_FUNCTIONS_H

#include "Dynamic/TypeList.h"

/*
 * Here you'll find a list of helper functions to make the TypeContainer useful.
 * Without it, it's hard to access or mutate the container.
*/

namespace Acore
{
    // Insert helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Insert(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* obj)
    {
        auto i = elements._element.find(handle);
        if (i == elements._element.end())
        {
            elements._element[handle] = obj;
            return true;
        }
        ASSERT(i->second == obj, "Object with certain key already in but objects are different!");
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Insert(ContainerUnorderedMap<TypeNull, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Insert(ContainerUnorderedMap<T, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Insert(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* obj)
    {
        bool ret = Insert(elements._elements, handle, obj);
        return ret ? ret : Insert(elements._TailElements, handle, obj);
    }

    // Find helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE> const& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        auto i = elements._element.find(handle);
        if (i == elements._element.end())
        {
            return nullptr;
        }
        return i->second;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<TypeNull, KEY_TYPE> const& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<T, KEY_TYPE> const& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    SPECIFIC_TYPE* Find(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE> const& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        SPECIFIC_TYPE* ret = Find(elements._elements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
        return ret ? ret : Find(elements._TailElements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    // Erase helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Remove(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        elements._element.erase(handle);
        return true;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Remove(ContainerUnorderedMap<TypeNull, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Remove(ContainerUnorderedMap<T, KEY_TYPE>& /*elements*/, KEY_TYPE const& /*handle*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Remove(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE>& elements, KEY_TYPE const& handle, SPECIFIC_TYPE* /*obj*/)
    {
        bool ret = Remove(elements._elements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
        return ret ? ret : Remove(elements._TailElements, handle, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    // Count helpers
    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Size(ContainerUnorderedMap<SPECIFIC_TYPE, KEY_TYPE> const& elements, std::size_t* size, SPECIFIC_TYPE* /*obj*/)
    {
        *size = elements._element.size();
        return true;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE>
    bool Size(ContainerUnorderedMap<TypeNull, KEY_TYPE> const& /*elements*/, std::size_t* /*size*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class T>
    bool Size(ContainerUnorderedMap<T, KEY_TYPE> const& /*elements*/, std::size_t* /*size*/, SPECIFIC_TYPE* /*obj*/)
    {
        return false;
    }

    template<class SPECIFIC_TYPE, class KEY_TYPE, class H, class T>
    bool Size(ContainerUnorderedMap<TypeList<H, T>, KEY_TYPE> const& elements, std::size_t* size, SPECIFIC_TYPE* /*obj*/)
    {
        bool ret = Size(elements._elements, size, static_cast<SPECIFIC_TYPE*>(nullptr));
        return ret ? ret : Size(elements._TailElements, size, static_cast<SPECIFIC_TYPE*>(nullptr));
    }

    // Count functions
    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerMapList<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* /*fake*/)
    {
        return elements._element.getSize();
    }

    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerMapList<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerMapList<T>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerMapList<TypeList<SPECIFIC_TYPE, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._elements, fake);
    }

    template<class SPECIFIC_TYPE, class H, class T>
    std::size_t Count(const ContainerMapList<TypeList<H, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._TailElements, fake);
    }

    // Non-const insert functions
    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerMapList<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* obj)
    {
        obj->AddToGrid(elements._element);
        return obj;
    }

    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerMapList<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    // This is a missed
    template<class SPECIFIC_TYPE, class T>
    SPECIFIC_TYPE* Insert(ContainerMapList<T>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr; // A missed
    }

    // Recursion
    template<class SPECIFIC_TYPE, class H, class T>
    SPECIFIC_TYPE* Insert(ContainerMapList<TypeList<H, T>>& elements, SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Insert(elements._elements, obj);
        return t != nullptr ? t : Insert(elements._TailElements, obj);
    }

    // Count functions
    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* /*fake*/)
    {
        return elements._element.getSize();
    }

    template<class SPECIFIC_TYPE>
    std::size_t Count(const ContainerVector<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerVector<T>& /*elements*/, SPECIFIC_TYPE* /*fake*/)
    {
        return 0;
    }

    template<class SPECIFIC_TYPE, class T>
    std::size_t Count(const ContainerVector<TypeList<SPECIFIC_TYPE, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._elements, fake);
    }

    template<class SPECIFIC_TYPE, class H, class T>
    std::size_t Count(const ContainerVector<TypeList<H, T>>& elements, SPECIFIC_TYPE* fake)
    {
        return Count(elements._TailElements, fake);
    }

    // Non-const insert functions
    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE* obj)
    {
        elements._element.push_back(obj);
        return obj;
    }

    template<class SPECIFIC_TYPE>
    SPECIFIC_TYPE* Insert(ContainerVector<TypeNull>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;
    }

    // This is a missed
    template<class SPECIFIC_TYPE, class T>
    SPECIFIC_TYPE* Insert(ContainerVector<T>& /*elements*/, SPECIFIC_TYPE* /*obj*/)
    {
        return nullptr;  // A missed
    }

    // Recursion
    template<class SPECIFIC_TYPE, class H, class T>
    SPECIFIC_TYPE* Insert(ContainerVector<TypeList<H, T>>& elements, SPECIFIC_TYPE* obj)
    {
        SPECIFIC_TYPE* t = Insert(elements._elements, obj);
        return t != nullptr ? t : Insert(elements._TailElements, obj);
    }

    // Non-const remove method
    template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerVector<SPECIFIC_TYPE>& elements, SPECIFIC_TYPE *obj)
    {
        // Simple vector find/swap/pop, this container should be very lightly used,
        // so I don't suspect the linear search complexity to be an issue
        auto itr = std::find(elements._element.begin(), elements._element.end(), obj);
        if (itr != elements._element.end())
        {
            // Swap the element to be removed with the last element
            std::swap(*itr, elements._element.back());

            // Remove the last element (which is now the element we wanted to remove)
            elements._element.pop_back();
        }
        return obj;
    }

    template<class SPECIFIC_TYPE> SPECIFIC_TYPE* Remove(ContainerVector<TypeNull> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    {
        return nullptr;
    }

    // This is a missed
    template<class SPECIFIC_TYPE, class T> SPECIFIC_TYPE* Remove(ContainerVector<T> &/*elements*/, SPECIFIC_TYPE * /*obj*/)
    {
        return nullptr;  // A missed
    }

    template<class SPECIFIC_TYPE, class T, class H> SPECIFIC_TYPE* Remove(ContainerVector<TypeList<H, T> > &elements, SPECIFIC_TYPE *obj)
    {
        // The head element is bad
        SPECIFIC_TYPE* t = Remove(elements._elements, obj);
        return t != nullptr ? t : Remove(elements._TailElements, obj);
    }
}
#endif
