#include "AppenderConsole.h"
#include "LogMessage.h"
#include "Util.h"

AppenderConsole::AppenderConsole(const uint8 _id, std::string const& name, const LogLevel level, const AppenderFlags flags) :
    Appender(_id, name, level, flags) {}

void AppenderConsole::_write(LogMessage const* message)
{
    const bool stdout_stream = !(message->level == LOG_LEVEL_ERROR || message->level == LOG_LEVEL_FATAL);

    ANSIFgTextAttr color;
    switch (message->level)
    {
        case LOG_LEVEL_TRACE:
            color = FG_YELLOW;
            break;
        case LOG_LEVEL_DEBUG:
            color = FG_MAGENTA;
            break;
        case LOG_LEVEL_INFO:
            color = FG_CYAN;
            break;
        case LOG_LEVEL_WARN:
            color = FG_BROWN;
            break;
        case LOG_LEVEL_FATAL:
            [[fallthrough]];
        case LOG_LEVEL_ERROR:
            [[fallthrough]];
        default:
            color = FG_RED;
            break;
    }


    fprintf(stdout_stream ? stdout : stderr, "\x1b[%d%sm", color, color == FG_YELLOW ? ";1" : "");
    utf8printf(stdout_stream ? stdout : stderr, "%s%s\n", message->prefix.c_str(), message->text.c_str());
    fprintf(stdout_stream ? stdout : stderr, "\x1b[0m");
}
