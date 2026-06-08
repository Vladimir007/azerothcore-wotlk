#ifndef LOG_COMMON_H
#define LOG_COMMON_H

#include "Define.h"

enum LogLevel : uint8
{
    LOG_LEVEL_DISABLED = 0,
    LOG_LEVEL_FATAL = 1,
    LOG_LEVEL_ERROR = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_INFO = 4,
    LOG_LEVEL_DEBUG = 5,
    LOG_LEVEL_TRACE = 6,

    NUM_ENABLED_LOG_LEVELS = LOG_LEVEL_TRACE, // SKIP
    LOG_LEVEL_INVALID = 0xFF // SKIP
};

enum AppenderType : uint8
{
    APPENDER_NONE,
    APPENDER_CONSOLE,
    APPENDER_DB,
};

enum AppenderFlags : uint8
{
    APPENDER_FLAGS_NONE                   = 0x0,
    APPENDER_FLAGS_PREFIX_TIMESTAMP       = 0x1,
    APPENDER_FLAGS_PREFIX_LOGLEVEL        = 0x2,
    APPENDER_FLAGS_PREFIX_LOG_FILTER_TYPE = 0x4,
};

#endif
