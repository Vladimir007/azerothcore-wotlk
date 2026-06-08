#ifndef LOG_APPENDER_CONSOLE_H
#define LOG_APPENDER_CONSOLE_H

#include "Appender.h"

enum ANSIFgTextAttr
{
    FG_RED = 31,
    FG_GREEN,
    FG_BROWN,
    FG_BLUE,
    FG_MAGENTA,
    FG_CYAN,
    FG_WHITE,
    FG_YELLOW
};

class AppenderConsole : public Appender
{
public:
    static constexpr AppenderType type = APPENDER_CONSOLE;

    AppenderConsole(uint8 _id, std::string const& name, LogLevel level, AppenderFlags flags);
    AppenderType getType() const override { return type; }

private:
    void _write(LogMessage const* message) override;
};

#endif
