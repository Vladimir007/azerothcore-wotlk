#include "Errors.h"
#include <cstdio>
#include <thread>
#include "Duration.h"

/**
    @file Errors.cpp

    @brief This file contains definitions of functions used for reporting critical application errors

    It is very important that (std::)abort is NEVER called in place of *((volatile int*)nullptr) = 0;
    exit(1) calls here are for static analysis tools to indicate that calling functions defined in this file
    terminates the application.
 */

// Should be easily accessible in gdb
extern "C" { char const* AcoreAssertionFailedMessage = nullptr; }
#define Crash(message) \
    AcoreAssertionFailedMessage = strdup(message); \
    *((volatile int*)nullptr) = 0; \
    exit(1);

namespace
{
    /**
    * @name MakeMessage
    * @brief Make message for display errors
    * @param messageType Message type (ASSERTION FAILED, FATAL ERROR, ERROR) end etc
    * @param file Path to file
    * @param line Line number in file
    * @param function Function name
    * @param message Condition to string format
    * @param fmtMessage [optional] Display format message after condition
    * @param debugInfo [optional] Display debug info
    */
    std::string MakeMessage(std::string_view messageType, std::string_view file, uint32 line, std::string_view function,
        std::string_view message, std::string_view fmtMessage = {}, std::string_view debugInfo = {})
    {
        std::string msg = Acore::StringFormat("\n>> {}\n\n# Location: {}:{}\n# Function: {}\n# Condition: {}\n", messageType, file, line, function, message);

        if (!fmtMessage.empty())
            msg.append(Acore::StringFormat("# Message: {}\n", fmtMessage));

        if (!debugInfo.empty())
            msg.append(Acore::StringFormat("\n# Debug info: {}\n", debugInfo));

        return Acore::StringFormat("#{0:-^{2}}#\n {1: ^{2}} \n#{0:-^{2}}#\n", "", msg, 70);
    }

    /**
    * @name MakeAbortMessage
    * @brief Make message for display errors
    * @param file Path to file
    * @param line Line number in file
    * @param function Function name
    * @param fmtMessage [optional] Display format message after condition
    */
    std::string MakeAbortMessage(std::string_view file, uint32 line, std::string_view function, std::string_view fmtMessage = {})
    {
        std::string msg = Acore::StringFormat("\n>> ABORTED\n\n# Location '{}:{}'\n# Function '{}'\n", file, line, function);

        if (!fmtMessage.empty())
        {
            msg.append(Acore::StringFormat("# Message '{}'\n", fmtMessage));
        }

        return Acore::StringFormat(
            "\n#{0:-^{2}}#\n"
            " {1: ^{2}} \n"
            "#{0:-^{2}}#\n", "", msg, 70);
    }
}

void Acore::Assert(const std::string_view file, const uint32 line, const std::string_view function,
    const std::string_view debugInfo, const std::string_view message, const std::string_view fmtMessage /*= {}*/)
{
    std::string formattedMessage = MakeMessage("ASSERTION FAILED", file, line, function, message, fmtMessage, debugInfo);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    Crash(formattedMessage.c_str());
}

void Acore::Fatal(const std::string_view file, const uint32 line, const std::string_view function,
    const std::string_view message, const std::string_view fmtMessage /*= {}*/)
{
    std::string formattedMessage = MakeMessage("FATAL ERROR", file, line, function, message, fmtMessage);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    std::this_thread::sleep_for(10s);
    Crash(formattedMessage.c_str());
}

void Acore::Error(const std::string_view file, const uint32 line, const std::string_view function, const std::string_view message)
{
    std::string formattedMessage = MakeMessage("ERROR", file, line, function, message);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    std::this_thread::sleep_for(10s);
    Crash(formattedMessage.c_str());
}

void Acore::Warning(const std::string_view file, const uint32 line, const std::string_view function, const std::string_view message)
{
    std::string formattedMessage = MakeMessage("WARNING", file, line, function, message);
    fmt::print(stderr, "{}", formattedMessage);
}

void Acore::Abort(const std::string_view file, const uint32 line, const std::string_view function, const std::string_view fmtMessage /*= {}*/)
{
    std::string formattedMessage = MakeAbortMessage(file, line, function, fmtMessage);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    std::this_thread::sleep_for(10s);
    Crash(formattedMessage.c_str());
}

void Acore::AbortHandler(int sigval)
{
    // Nothing useful to log here, no way to pass args
    std::string formattedMessage = StringFormat("Caught signal {}\n", sigval);
    fmt::print(stderr, "{}", formattedMessage);
    fflush(stderr);
    Crash(formattedMessage.c_str());
}

std::string GetDebugInfo()
{
    return "";
}
