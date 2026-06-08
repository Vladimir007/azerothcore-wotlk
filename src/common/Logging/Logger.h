#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <unordered_map>
#include "Define.h"
#include "LogCommon.h"

class Appender;
struct LogMessage;

class Logger
{
public:
    Logger(std::string const& _name, LogLevel _level);

    void addAppender(uint8 type, Appender* appender);
    void delAppender(uint8 type);

    std::string const& getName() const;
    LogLevel getLogLevel() const;
    void setLogLevel(LogLevel _level);
    void write(LogMessage* message) const;

private:
    std::string name;
    LogLevel level;
    std::unordered_map<uint8, Appender*> appenders;
};

#endif
