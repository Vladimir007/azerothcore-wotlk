#ifndef LOG_MESSAGE_H
#define LOG_MESSAGE_H

#include <string>
#include "Define.h"
#include "Duration.h"
#include "LogCommon.h"
#include "Timer.h"

struct LogMessage
{
    LogMessage(const LogLevel _level, std::string const& _type, const std::string_view _text) :
        level(_level), type(_type), text(std::string(_text)), mtime(GetEpochTime()) { }

    LogMessage(LogMessage const& /*other*/) = delete;
    LogMessage& operator=(LogMessage const& /*other*/) = delete;

    static std::string getTimeStr(const Seconds time) { return Acore::Time::TimeToTimestampStr(time, "%Y-%m-%d %X"); }
    std::string getTimeStr() const { return getTimeStr(mtime); }

    LogLevel const level;
    std::string const type;
    std::string const text;
    std::string prefix;
    Seconds mtime;

    ///@ Returns size of the log message content in bytes
    uint32 Size() const
    {
        return static_cast<uint32>(prefix.size() + text.size());
    }
};

#endif
