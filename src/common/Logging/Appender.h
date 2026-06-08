#ifndef LOG_APPENDER_H
#define LOG_APPENDER_H

#include <string>
#include "Define.h"
#include "LogCommon.h"

struct LogMessage;

class Appender
{
public:
    Appender(uint8 _id, std::string const& _name, LogLevel _level = LOG_LEVEL_DISABLED, AppenderFlags _flags = APPENDER_FLAGS_NONE);
    virtual ~Appender();

    uint8 getId() const;
    const std::string& getName() const;
    virtual AppenderType getType() const = 0;
    LogLevel getLogLevel() const;
    AppenderFlags getFlags() const;

    void setLogLevel(LogLevel);
    void write(LogMessage* message);
    static char const* getLogLevelString(LogLevel level);

private:
    virtual void _write(const LogMessage* /*message*/) = 0;

    uint8 id;
    std::string name;
    LogLevel level;
    AppenderFlags flags;
};
#endif
