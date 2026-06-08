#include "Logger.h"

#include <ranges>

#include "Appender.h"
#include "LogMessage.h"

Logger::Logger(std::string const& _name, const LogLevel _level): name(_name), level(_level) { }

std::string const& Logger::getName() const
{
    return name;
}

LogLevel Logger::getLogLevel() const
{
    return level;
}

void Logger::addAppender(const uint8 type, Appender* appender)
{
    appenders[type] = appender;
}

void Logger::delAppender(const uint8 type)
{
    appenders.erase(type);
}

void Logger::setLogLevel(const LogLevel _level)
{
    level = _level;
}

void Logger::write(LogMessage* message) const
{
    if (!level || level < message->level || message->text.empty())
        return;

    for (const auto& appender : appenders | std::views::values)
        if (appender)
            appender->write(message);
}
