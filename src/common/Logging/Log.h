#ifndef LOG_H
#define LOG_H

#include <memory>
#include <unordered_map>

#include "Define.h"
#include "LogCommon.h"
#include "StringFormat.h"

class Appender;
class Logger;
struct LogMessage;

#define LOGGER_ROOT "root"

typedef Appender*(*AppenderCreatorFn)(uint8 id, const std::string& name, LogLevel level, AppenderFlags flags);

template <class AppenderImpl>
Appender* CreateAppender(uint8 id, const std::string& name, LogLevel level, AppenderFlags flags)
{
    return new AppenderImpl(id, name, level, flags);
}

class Log
{
    typedef std::unordered_map<std::string, Logger> LoggerMap;

    Log();
    ~Log();

public:
    Log(const Log&) = delete;
    Log(Log&&) = delete;
    Log& operator=(const Log&) = delete;
    Log& operator=(Log&&) = delete;

    static Log* instance();

    void Initialize();
    void Close();
    [[nodiscard]] bool ShouldLog(const std::string& type, LogLevel level) const;

    template<typename... Args>
    void outMessage(const std::string& filter, const LogLevel level, std::string_view fmt, Args&&... args)
    {
        _outMessage(filter, level, Acore::StringFormat(fmt, std::forward<Args>(args)...));
    }

    void outMessage(const std::string& filter, const LogLevel level, const std::string& message)
    {
        _outMessage(filter, level, message);
    }

    template<typename... Args>
    void outCommand(const std::string_view fmt, Args&&... args)
    {
        if (!ShouldLog("commands.gm", LOG_LEVEL_INFO))
            return;

        _outCommand(Acore::StringFormat(fmt, std::forward<Args>(args)...));
    }

    template<class AppenderImpl>
    void RegisterAppender()
    {
        RegisterAppender(AppenderImpl::type, &CreateAppender<AppenderImpl>);
    }

    [[nodiscard]] const std::string& GetLogsTimestamp() const { return m_logsTimestamp; }

private:
    static std::string GetTimestampStr();
    void write(std::unique_ptr<LogMessage>&& msg) const;

    [[nodiscard]] const Logger* GetLoggerByType(const std::string& type) const;
    Appender* GetAppenderByName(std::string_view name);
    uint8 NextAppenderId();
    void CreateAppenderFromConfig(const std::string& appenderName);
    void CreateLoggerFromConfig(const std::string& appenderName);
    void RegisterAppender(uint8 index, AppenderCreatorFn appenderCreateFn);
    void _outMessage(const std::string& filter, LogLevel level, std::string_view message);
    void _outCommand(std::string_view message);

    std::unordered_map<uint8, AppenderCreatorFn> appenderFactory;
    std::unordered_map<uint8, std::unique_ptr<Appender>> appenders;
    std::unordered_map<std::string, std::unique_ptr<Logger>> loggers;
    uint8 AppenderId;
    LogLevel highestLogLevel;

    std::string m_logsTimestamp;
};

#define sLog Log::instance()

#define LOG_MESSAGE_BODY(filterType__, level__, ...)                        \
        do                                                              \
        {                                                               \
            if (sLog->ShouldLog(filterType__, level__))                 \
                sLog->outMessage(filterType__, level__, __VA_ARGS__); \
        } while (0)


#define LOG_FATAL(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_FATAL, __VA_ARGS__)
#define LOG_ERROR(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_ERROR, __VA_ARGS__)
#define LOG_WARN(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_INFO(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_DEBUG(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_TRACE(filterType__, ...) LOG_MESSAGE_BODY(filterType__, LogLevel::LOG_LEVEL_TRACE, __VA_ARGS__)
#define LOG_GM(...) sLog->outCommand(__VA_ARGS__)

#endif
