#ifndef CONFIG_VALUE_CACHE_H
#define CONFIG_VALUE_CACHE_H

#include <variant>
#include "Config.h"
#include "Errors.h"
#include "Log.h"

template<typename ConfigEnum>
class ConfigValueCache
{
    static_assert(std::is_enum_v<ConfigEnum>);

public:
    virtual ~ConfigValueCache() = default;

    enum class Reloadable : bool
    {
        No = false,
        Yes = true
    };

    explicit ConfigValueCache(ConfigEnum const configCount)
    {
        _configs.resize(static_cast<uint32>(configCount));
    }

    void Initialize()
    {
        BuildConfigCache();
        VerifyAllConfigsLoaded();
    }

    template<class T>
    void SetConfigValue(const ConfigEnum config, const std::string& configName, const T& defaultValue, std::function<bool(const T& value)>&& checker = {}, const std::string& validationErrorText = "")
    {
        uint32 const configIndex = static_cast<uint32>(config);
        ASSERT(configIndex < _configs.size(), "Config index out of bounds");
        T const& configValue = sConfigMgr->GetOption<T>(configName, defaultValue);
        ASSERT(_configs[configIndex].index() == 0, "Config overwriting an existing value");

        if (checker && !checker(configValue))
        {
            LOG_ERROR("server.loading", "Server Config (Name: {}) failed validation check '{}'. Default value '{}' will be used instead.", configName, validationErrorText, defaultValue);
            _configs[configIndex] = defaultValue;
        }
        else
            _configs[configIndex] = configValue;
    }

    template<class T>
    void OverwriteConfigValue(const ConfigEnum config, const T& value)
    {
        uint32 const configIndex = static_cast<uint32>(config);
        ASSERT(configIndex < _configs.size(), "Config index out of bounds");
        size_t const oldValueTypeIndex = _configs[configIndex].index();
        ASSERT(oldValueTypeIndex != 0, "Config value must already be set");
        _configs[configIndex] = value;
        ASSERT(oldValueTypeIndex == _configs[configIndex].index(), "Config value type changed");
    }

    template<class T>
    T GetConfigValue(const ConfigEnum config) const
    {
        uint32 const configIndex = static_cast<uint32>(config);
        ASSERT(configIndex < _configs.size(), "Config index out of bounds");
        ASSERT(_configs[configIndex].index() != 0, "Config value must already be set");

        const T* value = std::get_if<T>(&_configs[configIndex]);
        ASSERT(value, "Wrong config variant type");

        return value ? *value : T();
    }

    // Custom handling for string configs to convert from std::string to std::string_view
    std::string_view GetConfigValue(const ConfigEnum config) const
    {
        uint32 const configIndex = static_cast<uint32>(config);
        ASSERT(configIndex < _configs.size(), "Config index out of bounds");
        ASSERT(_configs[configIndex].index() != 0, "Config value must already be set");

        const std::string* stringValue = std::get_if<std::string>(&_configs[configIndex]);
        ASSERT(stringValue, "Wrong config variant type");

        return std::string_view(stringValue ? *stringValue : "");
    }

protected:
    virtual void BuildConfigCache() = 0;

private:
    void VerifyAllConfigsLoaded() const
    {
        uint32 configIndex = 0;
        for (auto const& variant : _configs)
        {
            if (variant.index() == 0)
            {
                LOG_ERROR("server.loading", "Server Config (Index: {}) is defined but not loaded, unable to continue.", configIndex);
                ASSERT(false);
            }

            ++configIndex;
        }
    }

    std::vector<std::variant<std::monostate, float, bool, uint32, std::string>> _configs;
};

#endif
