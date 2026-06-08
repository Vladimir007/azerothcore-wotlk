#ifndef DATA_MAP_H
#define DATA_MAP_H

#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>

class DataMap
{
public:
    /// Base class that you should inherit in your script. Inheriting classes can be stored to DataMap.
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    /// Returns a pointer to object of requested type stored with given key or nullptr
    template<class T> T* Get(std::string const& k) const
    {
        static_assert(std::is_base_of_v<Base, T>, "T must derive from Base");
        if (Container.empty())
            return nullptr;

        if (const auto it = Container.find(k); it != Container.end())
            return dynamic_cast<T*>(it->second.get());
        return nullptr;
    }

    /// Returns a pointer to object of requested type stored with given key or default constructs one and returns that one
    template<class T, std::enable_if_t<std::is_default_constructible_v<T>, int> = 0>
    T * GetDefault(std::string const& k)
    {
        static_assert(std::is_base_of_v<Base, T>, "T must derive from Base");
        if (T* v = Get<T>(k))
            return v;
        T* v = new T();
        Container.emplace(k, std::unique_ptr<T>(v));
        return v;
    }

    /// Stores a new object that inherits the Base class with the given key
    void Set(std::string const& k, Base* v) { Container[k] = std::unique_ptr<Base>(v); }

    /// Removes objects with given key and returns true if one was removed, false otherwise
    bool Erase(std::string const& k) { return Container.erase(k) != 0; }

private:
    std::unordered_map<std::string, std::unique_ptr<Base>> Container;
};

#endif
