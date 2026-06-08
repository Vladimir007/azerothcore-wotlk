#ifndef LOG_APPENDER_DB_H
#define LOG_APPENDER_DB_H

#include "Appender.h"

class AppenderDB : public Appender
{
public:
    static constexpr AppenderType type = APPENDER_DB;

    AppenderDB(uint8 id, const std::string& name, LogLevel level, AppenderFlags flags);
    ~AppenderDB() override;

    AppenderType getType() const override { return type; }

private:
    void _write(LogMessage const* message) override;
};

#endif
