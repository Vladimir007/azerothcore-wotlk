#include "Log.h"

#include <chrono>
#include <memory>
#include <ranges>

#include "AppenderConsole.h"
#include "Config.h"
#include "Errors.h"
#include "Logger.h"
#include "LogMessage.h"
#include "StringConvert.h"
#include "Timer.h"
#include "Tokenize.h"

Log::Log() : AppenderId(0), highestLogLevel(LOG_LEVEL_FATAL)
{
    m_logsTimestamp = "_" + GetTimestampStr();
    RegisterAppender<AppenderConsole>();
}

Log::~Log()
{
    Close();
}

uint8 Log::NextAppenderId()
{
    return AppenderId++;
}

Appender* Log::GetAppenderByName(const std::string_view name)
{
    auto it = appenders.begin();
    while (it != appenders.end() && it->second && it->second->getName() != name)
        ++it;

    return it == appenders.end() ? nullptr : it->second.get();
}

void Log::CreateAppenderFromConfig(const std::string& appenderName)
{
    if (appenderName.empty())
        return;
    std::string name = appenderName.substr(9);

    AppenderType type;
    if (name == "Console")
        type = APPENDER_CONSOLE;
    else if (name == "DB")
        type = APPENDER_DB;
    else
    {
        fmt::print(stderr, "Log::CreateAppenderFromConfig: Unsupported appender type '{}'\n", name);
        return;
    }

    const auto factoryFunction = appenderFactory.find(type);
    if (factoryFunction == appenderFactory.end())
    {
        fmt::print(stderr, "Log::CreateAppenderFromConfig: Appender '{}' is not registered\n", name);
        return;
    }

    // Format = level, flags
    auto options = sConfigMgr->GetOption<std::string>(appenderName, "0,0");

    std::vector<std::string_view> tokens = Acore::Tokenize(options, ',', true);
    if (tokens.size() != 2)
    {
        fmt::print(stderr, "Log::CreateAppenderFromConfig: Wrong configuration for appender {}. Config line: {}\n", name, options);
        return;
    }

    const auto level = static_cast<LogLevel>(Acore::StringTo<uint8>(tokens[0]).value_or(LOG_LEVEL_INVALID));
    if (level > NUM_ENABLED_LOG_LEVELS)
    {
        fmt::print(stderr, "Log::CreateAppenderFromConfig: Wrong Log Level '{}' for appender {}\n", tokens[0], name);
        return;
    }

    AppenderFlags flags;
    if (Optional<uint8> flagsVal = Acore::StringTo<uint8>(tokens[1]))
        flags = static_cast<AppenderFlags>(*flagsVal);
    else
    {
        fmt::print(stderr, "Log::CreateAppenderFromConfig: Unknown flags '{}' for appender {}\n", tokens[1], name);
        return;
    }

    Appender* appender = factoryFunction->second(NextAppenderId(), name, level, flags);
    appenders[appender->getId()].reset(appender);
}

void Log::CreateLoggerFromConfig(const std::string& appenderName)
{
    if (appenderName.empty())
        return;

    std::string name = appenderName.substr(7);

    std::unique_ptr<Logger>& logger = loggers[name];
    if (logger)
    {
        fmt::print(stderr, "Error while configuring Logger {}. Already defined\n", name);
        return;
    }

    const auto logLevelStr = sConfigMgr->GetOption<std::string>(appenderName, "");
    if (logLevelStr.empty())
    {
        fmt::print(stderr, "Log::CreateLoggerFromConfig: Missing config option Logger.{}\n", name);
        return;
    }

    auto level = static_cast<LogLevel>(Acore::StringTo<uint8>(logLevelStr).value_or(LOG_LEVEL_INVALID));
    if (level > NUM_ENABLED_LOG_LEVELS)
    {
        fmt::print(stderr, "Log::CreateLoggerFromConfig: Wrong Log Level '{}' for logger {}\n", logLevelStr, name);
        return;
    }

    if (level > highestLogLevel)
        highestLogLevel = level;

    logger = std::make_unique<Logger>(name, level);

    for (const auto& appender : appenders | std::views::values) {
        if (appender)
            logger->addAppender(appender->getId(), appender.get());
    }
}

void Log::RegisterAppender(const uint8 index, const AppenderCreatorFn appenderCreateFn)
{
    const auto itr = appenderFactory.find(index);
    ASSERT(itr == appenderFactory.end());
    appenderFactory[index] = appenderCreateFn;
}

void Log::_outMessage(const std::string& filter, LogLevel level, std::string_view message)
{
    write(std::make_unique<LogMessage>(level, filter, message));
}

void Log::_outCommand(std::string_view message)
{
    write(std::make_unique<LogMessage>(LOG_LEVEL_INFO, "commands.gm", message));
}

void Log::write(std::unique_ptr<LogMessage>&& msg) const
{
    const Logger* logger = GetLoggerByType(msg->type);
    logger->write(msg.get());
}

const Logger* Log::GetLoggerByType(const std::string& type) const
{
    if (const auto it = loggers.find(type); it != loggers.end())
        return it->second.get();

    if (type == LOGGER_ROOT)
        return nullptr;

    std::string parentLogger = LOGGER_ROOT;
    if (const std::size_t found = type.find_last_of('.'); found != std::string::npos)
        parentLogger = type.substr(0, found);

    return GetLoggerByType(parentLogger);
}

std::string Log::GetTimestampStr()
{
    return Acore::Time::TimeToTimestampStr(GetEpochTime(), "%Y-%m-%d_%H_%M_%S");
}

void Log::Close()
{
    loggers.clear();
    appenders.clear();
}

bool Log::ShouldLog(const std::string& type, const LogLevel level) const
{
    // Don't even look for a logger if the LogLevel is higher than the highest log levels across all loggers
    if (level > highestLogLevel)
        return false;

    const Logger* logger = GetLoggerByType(type);
    if (!logger)
        return false;

    const LogLevel logLevel = logger->getLogLevel();
    return logLevel != LOG_LEVEL_DISABLED && logLevel >= level;
}

Log* Log::instance()
{
    static Log instance;
    return &instance;
}

void Log::Initialize()
{
    highestLogLevel = LOG_LEVEL_FATAL;
    AppenderId = 0;

    for (const std::vector<std::string> keys = sConfigMgr->GetKeysByString("Appender."); const std::string& appenderName : keys)
        CreateAppenderFromConfig(appenderName);

    for (const std::vector<std::string> keys = sConfigMgr->GetKeysByString("Logger."); std::string const& loggerName : keys)
        CreateLoggerFromConfig(loggerName);

    if (loggers.contains(LOGGER_ROOT))
        return;

    // Bad config configuration, creating default config
    fmt::print(stderr, "Wrong Loggers configuration. Review your Logger config section.\n"
                    "Creating default loggers [root (Error), server (Info)] to console\n");

    Close(); // Clean any Logger or Appender created

    const auto appender = new AppenderConsole(NextAppenderId(), "Console", LOG_LEVEL_DEBUG, APPENDER_FLAGS_NONE);
    appenders[appender->getId()].reset(appender);

    const auto rootLogger = new Logger(LOGGER_ROOT, LOG_LEVEL_WARN);
    rootLogger->addAppender(appender->getId(), appender);
    loggers[LOGGER_ROOT].reset(rootLogger);

    const auto serverLogger = new Logger("server", LOG_LEVEL_INFO);
    serverLogger->addAppender(appender->getId(), appender);
    loggers["server"].reset(serverLogger);

    highestLogLevel = LOG_LEVEL_INFO;
}
