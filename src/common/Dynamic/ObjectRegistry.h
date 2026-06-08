#ifndef NCORE_OBJECT_REGISTRY_H
#define NCORE_OBJECT_REGISTRY_H

#include <map>
#include <memory>
#include <string>

/// ObjectRegistry holds all registry item of the same type
template<class T, class Key = std::string>
class ObjectRegistry final
{
public:
    typedef std::map<Key, std::unique_ptr<T>> RegistryMapType;

    /// Returns a registry item
    T const* GetRegistryItem(Key const& key) const
    {
        auto itr = _registeredObjects.find(key);
        if (itr == _registeredObjects.end())
            return nullptr;
        return itr->second.get();
    }

    static ObjectRegistry* instance()
    {
        static ObjectRegistry* instance = new ObjectRegistry();
        return instance;
    }

    /// Inserts a registry item
    bool InsertItem(T* obj, Key const& key, const bool force = false)
    {
        auto itr = _registeredObjects.find(key);
        if (itr != _registeredObjects.end())
        {
            if (!force)
                return false;
            _registeredObjects.erase(itr);
        }

        _registeredObjects.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(obj));
        return true;
    }

    /// Returns true if registry contains an item
    bool HasItem(Key const& key) const
    {
        return _registeredObjects.contains(key);
    }

    /// Return the map of registered items
    RegistryMapType const& GetRegisteredItems() const
    {
        return _registeredObjects;
    }

private:
    RegistryMapType _registeredObjects;

    ObjectRegistry() {}
    ~ObjectRegistry() {}
};

#endif
