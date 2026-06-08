#ifndef CONFIG_H
#define CONFIG_H

#include <stdexcept>
#include <vector>
#include "Define.h"

enum class ConfigSeverity : uint8
{
    Skip,
    Warn,
    Error,
    Fatal
};

struct ConfigPolicy
{
    ConfigSeverity defaultSeverity = ConfigSeverity::Warn;
    ConfigSeverity missingFileSeverity = ConfigSeverity::Error;
    ConfigSeverity missingOptionSeverity = ConfigSeverity::Warn;
    ConfigSeverity criticalOptionSeverity = ConfigSeverity::Fatal;
    ConfigSeverity unknownOptionSeverity = ConfigSeverity::Error;
    ConfigSeverity valueErrorSeverity = ConfigSeverity::Error;
};

class ConfigMgr
{
    ConfigMgr() = default;
    ~ConfigMgr() = default;
public:
    static bool LoadAppConfigs();
    static void Configure(std::string const& initFileName);

    static ConfigMgr* instance();

    /// Overrides configuration with environment variables and returns overridden keys
    static std::vector<std::string> OverrideWithEnvVariablesIfAny();

    static std::string GetFilename();
    static std::string GetConfigPath();
    static std::vector<std::string> GetKeysByString(std::string const& name);

    template<class T>
    T GetOption(std::string const& name, T const& def, bool showLogs = true) const;

    [[nodiscard]] bool isDryRun() const { return dryRun; }
    void setDryRun(const bool mode) { dryRun = mode; }

private:
    /// Method used only for loading main configuration files (authserver.conf and worldserver.conf)
    static bool LoadInitial(std::string const& file);

    template<class T>
    T GetValueDefault(std::string const& name, T const& def, bool showLogs = true) const;

    bool dryRun = false;
};

class ConfigException : public std::length_error
{
public:
    explicit ConfigException(std::string const& message) : std::length_error(message) { }
};

#define sConfigMgr ConfigMgr::instance()

#endif
