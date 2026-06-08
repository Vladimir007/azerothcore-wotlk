#include "Config.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <locale>
#include <mutex>
#include <ranges>
#include <unordered_map>
#include <unordered_set>

#include "Log.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Util.h"

namespace
{
    std::string _filename;
    std::unordered_map<std::string /*name*/, std::string /*value*/> _configOptions;
    std::unordered_map<std::string /*name*/, std::string /*value*/> _envVarCache;
    std::mutex _configLock;
    ConfigPolicy _policy;

    std::unordered_set<std::string> _criticalConfigOptions = {"DatabaseInfo"};

    bool IsAppConfig(std::string_view fileName)
    {
        const std::size_t foundAuth = fileName.find("authserver.conf");
        const std::size_t foundWorld = fileName.find("worldserver.conf");

        return foundAuth != std::string_view::npos || foundWorld != std::string_view::npos;
    }

    template<typename... Args>
    void PrintError(const std::string_view filename, const std::string_view fmt, Args&& ... args)
    {
        const std::string message = Acore::StringFormat(fmt, std::forward<Args>(args)...);

        if (IsAppConfig(filename))
            std::cerr << message << std::endl;
        else
            LOG_ERROR("server.loading", message);
    }

    template<typename... Args>
    void LogWithSeverity(const ConfigSeverity severity, const std::string_view filename, const std::string_view fmt, Args&&... args)
    {
        std::string message = Acore::StringFormat(fmt, std::forward<Args>(args)...);

        switch (severity)
        {
            case ConfigSeverity::Skip:
                return;
            case ConfigSeverity::Warn:
            {
                if (IsAppConfig(filename))
                    std::cerr << message << std::endl;

                LOG_WARN("server.loading", message);
                return;
            }
            case ConfigSeverity::Error:
            {
                if (IsAppConfig(filename))
                    std::cerr << message << std::endl;

                LOG_ERROR("server.loading", message);
                return;
            }
            case ConfigSeverity::Fatal:
            {
                if (IsAppConfig(filename))
                    std::cerr << message << std::endl;

                LOG_FATAL("server.loading", message);
                ABORT(message);
            }
        }
    }

    void AddKey(std::string const& optionName, std::string const& optionKey)
    {
        // Check exit option
        if (_configOptions.contains(optionName))
            _configOptions.erase(optionName);
        _configOptions.emplace(optionName, optionKey);
    }

    bool ParseFile(std::string const& file)
    {
        std::ifstream in(file);

        if (in.fail())
        {
            ConfigSeverity severity = _policy.missingFileSeverity;
            LogWithSeverity(severity, file, "> Config::LoadFile: Failed open file '{}'", file);
            // Treat SKIP as a successful no-op so the app can proceed
            return severity == ConfigSeverity::Skip;
        }

        uint32 count = 0;
        uint32 lineNumber = 0;
        std::unordered_map<std::string /*name*/, std::string /*value*/> fileConfigs;

        auto IsDuplicateOption = [&](std::string const& confOption)
        {
            if (fileConfigs.contains(confOption))
            {
                PrintError(file, "> Config::LoadFile: Duplicate key name '{}' in config file '{}'", confOption, file);
                return true;
            }

            return false;
        };

        while (in.good())
        {
            lineNumber++;
            std::string line;
            std::getline(in, line);

            // Read line error
            if (!in.good() && !in.eof())
                throw ConfigException(Acore::StringFormat("> Config::LoadFile: Failure to read line number {} in file '{}'", lineNumber, file));

            // Remove whitespace in line
            line = Acore::String::Trim(line, in.getloc());

            if (line.empty())
                continue;

            // Comments and headers
            if (line[0] == '#' || line[0] == '[')
                continue;

            auto const equal_pos = line.find('=');

            if (equal_pos == std::string::npos || equal_pos == line.length())
            {
                PrintError(file, "> Config::LoadFile: Failure to read line number {} in file '{}'. Skip this line", lineNumber, file);
                continue;
            }

            auto entry = Acore::String::Trim(line.substr(0, equal_pos), in.getloc());
            auto value = Acore::String::Trim(line.substr(equal_pos + 1, std::string::npos), in.getloc());

            std::erase(value, '"');

            // Skip if 2+ same options in one config file
            if (IsDuplicateOption(entry))
                continue;

            // Add to temp container
            fileConfigs.emplace(entry, value);
            count++;
        }

        // No lines read
        if (!count)
        {
            ConfigSeverity severity = _policy.missingFileSeverity;
            LogWithSeverity(severity, file, "> Config::LoadFile: Empty file '{}'", file);
            // Treat SKIP as a successful no-op
            return severity == ConfigSeverity::Skip;
        }

        // Add correct keys if file load without errors
        for (auto const& [entry, key] : fileConfigs)
            AddKey(entry, key);

        return true;
    }

    bool LoadFile(std::string const& file)
    {
        try
        {
            return ParseFile(file);
        }
        catch (const std::exception& e)
        {
            PrintError(file, "> {}", e.what());
        }
        return false;
    }

    // Converts ini keys to the environment variable key (upper snake case).
    // Example of conversions:
    //   SomeConfig => SOME_CONFIG
    //   myNestedConfig.opt1 => MY_NESTED_CONFIG_OPT_1
    //   LogDB.Opt.ClearTime => LOG_DB_OPT_CLEAR_TIME
    std::string IniKeyToEnvVarKey(std::string const& key)
    {
        std::string result;

        const char* str = key.c_str();
        const std::size_t n = key.length();

        for (std::size_t i = 0; i < n; ++i)
        {
            const char curr = str[i];
            if (curr == ' ' || curr == '.' || curr == '-')
            {
                result += '_';
                continue;
            }

            if (i < n - 1)
            {
                // Handle "aB" to "A_B"
                if (!isupper(curr) && isupper(str[i + 1]))
                {
                    result += static_cast<char>(std::toupper(curr));
                    result += '_';
                    continue;
                }

                // Handle "a1"/"1a" to "a_1"/"1_a"
                if (isNumeric(curr) != isNumeric(str[i + 1]))
                {
                    result += static_cast<char>(std::toupper(curr));
                    result += '_';
                    continue;
                }
            }

            result += static_cast<char>(std::toupper(curr));
        }
        return result;
    }

    std::string GetEnvVarName(std::string const& configName)
    {
        return "AC_" + IniKeyToEnvVarKey(configName);
    }

    Optional<std::string> EnvVarForIniKey(std::string const& key)
    {
        const std::string envKey = GetEnvVarName(key);
        char* val = std::getenv(envKey.c_str());
        if (!val)
            return std::nullopt;

        return std::string(val);
    }
}

bool ConfigMgr::LoadInitial(std::string const& file)
{
    std::lock_guard lock(_configLock);
    _configOptions.clear();
    return LoadFile(file);
}

ConfigMgr* ConfigMgr::instance()
{
    static ConfigMgr instance;
    return &instance;
}

// Check the _envVarCache if the env var is there
// if not, check the env for the value
Optional<std::string> GetEnvFromCache(std::string const& configName, std::string const& envVarName)
{
    const auto foundInCache = _envVarCache.find(envVarName);
    // If it's not in the cache
    if (foundInCache == _envVarCache.end())
    {
        // Check the env itself
        Optional<std::string> foundInEnv = EnvVarForIniKey(configName);
        if (foundInEnv)
        {
            // If it's found in the env, put it in the cache
            _envVarCache.emplace(envVarName, *foundInEnv);
        }
        // Return the result of checking env
        return foundInEnv;
    }

    return foundInCache->second;
}

std::vector<std::string> ConfigMgr::OverrideWithEnvVariablesIfAny()
{
    std::lock_guard lock(_configLock);
    std::vector<std::string> overriddenKeys;

    for (auto& [name, value] : _configOptions)
    {
        if (name.empty())
            continue;

        Optional<std::string> envVar = EnvVarForIniKey(name);
        if (!envVar)
            continue;

        value = *envVar;
        overriddenKeys.push_back(name);
    }

    return overriddenKeys;
}

template<class T>
T ConfigMgr::GetValueDefault(std::string const& name, T const& def, const bool showLogs /*= true*/) const
{
    std::string strValue;

    auto const& itr = _configOptions.find(name);
    const bool notFound = itr == _configOptions.end();
    auto envVarName = GetEnvVarName(name);
    const Optional<std::string> envVar = GetEnvFromCache(name, envVarName);
    if (envVar)
    {
        // If showLogs and this key/value pair wasn't found in the currently saved config
        if (showLogs && (notFound || itr->second != envVar->c_str()))
        {
            LOG_INFO("server.loading", "> Config: Found config value '{}' from environment variable '{}'.", name, envVarName );
            AddKey(name, envVar->c_str());
        }

        strValue = *envVar;
    }
    else if (notFound)
    {
        if (showLogs)
        {
            const bool isCritical = _criticalConfigOptions.contains(name);
            ConfigSeverity severity = isCritical ? _policy.criticalOptionSeverity : _policy.missingOptionSeverity;

            if (isCritical)
            {
                LogWithSeverity(severity, _filename,
                    "> Config:\n\nFATAL ERROR: Missing property {} in config file {}, add \"{} = {}\" to this file or define '{}' as an environment variable\n\nYour server cannot start without this option!",
                    name, _filename, name, Acore::ToString(def), envVarName);
            }
            else
            {
                std::string configs = _filename;

                LogWithSeverity(severity, _filename,
                    "> Config: Missing property {} in config file {}, add \"{} = {}\" to this file or define '{}' as an environment variable.",
                    name, configs, name, def, envVarName);
            }
        }
        return def;
    }
    else
    {
        strValue = itr->second;
    }

    auto value = Acore::StringTo<T>(strValue);
    if (!value)
    {
        if (showLogs)
        {
            LogWithSeverity(_policy.valueErrorSeverity, _filename,
                "> Config: Bad value defined for name '{}', going to use '{}' instead",
                name, Acore::ToString(def));
        }

        return def;
    }

    return *value;
}

template<>
std::string ConfigMgr::GetValueDefault<std::string>(std::string const& name, std::string const& def, const bool showLogs /*= true*/) const
{
    auto const& itr = _configOptions.find(name);
    const bool notFound = itr == _configOptions.end();
    auto envVarName = GetEnvVarName(name);
    Optional<std::string> envVar = GetEnvFromCache(name, envVarName);
    if (envVar)
    {
        // If showLogs and this key/value pair wasn't found in the currently saved config
        if (showLogs && (notFound || itr->second != envVar->c_str()))
        {
            LOG_INFO("server.loading", "> Config: Found config value '{}' from environment variable '{}'.", name, envVarName);
            AddKey(name, *envVar);
        }

        return *envVar;
    }
    if (notFound)
    {
        if (showLogs)
        {
            const bool isCritical = _criticalConfigOptions.contains(name);
            const ConfigSeverity severity = isCritical ? _policy.criticalOptionSeverity : _policy.missingOptionSeverity;

            if (isCritical)
            {
                LogWithSeverity(severity, _filename,
                                "> Config:\n\nFATAL ERROR: Missing property {} in config file {}, add \"{} = {}\" to this file or define '{}' as an environment variable.\n\nYour server cannot start without this option!",
                                name, _filename, name, def, envVarName);
            }
            else
            {
                std::string configs = _filename;

                LogWithSeverity(severity, _filename,
                                "> Config: Missing property {} in config file {}, add \"{} = {}\" to this file or define '{}' as an environment variable.",
                                name, configs, name, def, envVarName);
            }
        }

        return def;
    }

    return itr->second;
}

template<class T>
T ConfigMgr::GetOption(std::string const& name, T const& def, const bool showLogs /*= true*/) const
{
    return GetValueDefault<T>(name, def, showLogs);
}

template<>
bool ConfigMgr::GetOption<bool>(std::string const& name, bool const& def, const bool showLogs /*= true*/) const
{
    const std::string val = GetValueDefault(name, std::string(def ? "1" : "0"), showLogs);

    const auto boolVal = Acore::StringTo<bool>(val);
    if (!boolVal)
    {
        if (showLogs)
        {
            LogWithSeverity(_policy.valueErrorSeverity, _filename,
                "> Config: Bad value defined for name '{}', going to use '{}' instead",
                name, def ? "true" : "false");
        }

        return def;
    }

    return *boolVal;
}

std::vector<std::string> ConfigMgr::GetKeysByString(std::string const& name)
{
    std::lock_guard lock(_configLock);

    std::vector<std::string> keys;

    for (const auto& optionName : _configOptions | std::views::keys)
    {
        if (!optionName.compare(0, name.length(), name))
        {
            keys.emplace_back(optionName);
        }
    }

    return keys;
}

std::string ConfigMgr::GetFilename()
{
    std::lock_guard lock(_configLock);
    return _filename;
}

std::string ConfigMgr::GetConfigPath()
{
    std::lock_guard lock(_configLock);
    return std::string(_CONF_DIR "/");
}

void ConfigMgr::Configure(std::string const& initFileName)
{
    _filename = initFileName;
}

bool ConfigMgr::LoadAppConfigs()
{
    return LoadInitial(_filename);
}

#define TEMPLATE_CONFIG_OPTION(__typename) \
    template __typename ConfigMgr::GetOption<__typename>(std::string const& name, __typename const& def, bool showLogs /*= true*/) const;

TEMPLATE_CONFIG_OPTION(std::string)
TEMPLATE_CONFIG_OPTION(uint8)
TEMPLATE_CONFIG_OPTION(int8)
TEMPLATE_CONFIG_OPTION(uint16)
TEMPLATE_CONFIG_OPTION(int16)
TEMPLATE_CONFIG_OPTION(uint32)
TEMPLATE_CONFIG_OPTION(int32)
TEMPLATE_CONFIG_OPTION(uint64)
TEMPLATE_CONFIG_OPTION(int64)
TEMPLATE_CONFIG_OPTION(float)

#undef TEMPLATE_CONFIG_OPTION
