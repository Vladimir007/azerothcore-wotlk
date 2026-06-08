#include "Appender.h"
#include <sstream>
#include "LogMessage.h"
#include "StringFormat.h"

Appender::Appender(const uint8 _id, std::string const& _name, const LogLevel _level /* = LOG_LEVEL_DISABLED */, const AppenderFlags _flags /* = APPENDER_FLAGS_NONE */):
    id(_id), name(_name), level(_level), flags(_flags) { }

Appender::~Appender() { }

uint8 Appender::getId() const
{
    return id;
}

const std::string& Appender::getName() const
{
    return name;
}

LogLevel Appender::getLogLevel() const
{
    return level;
}

AppenderFlags Appender::getFlags() const
{
    return flags;
}

void Appender::setLogLevel(const LogLevel _level)
{
    level = _level;
}

void Appender::write(LogMessage* message)
{
    if (!level || level < message->level)
        return;

    std::ostringstream ss;

    if (flags & APPENDER_FLAGS_PREFIX_TIMESTAMP)
        ss << message->getTimeStr() << ' ';

    if (flags & APPENDER_FLAGS_PREFIX_LOGLEVEL)
        ss << Acore::StringFormat("{} ", getLogLevelString(message->level));

    if (flags & APPENDER_FLAGS_PREFIX_LOG_FILTER_TYPE)
        ss << '[' << message->type << "] ";

    message->prefix = ss.str();
    _write(message);
}

char const* Appender::getLogLevelString(const LogLevel level)
{
    switch (level)
    {
        case LOG_LEVEL_FATAL:
            return "FATAL";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_TRACE:
            return "TRACE";
        default:
            return "DISABLED";
    }
}
