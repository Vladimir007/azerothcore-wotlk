#ifndef NCORE_ERRORS_H
#define NCORE_ERRORS_H

#include "StringFormat.h"

namespace Acore
{
    // Default function
    [[noreturn]] void Assert(std::string_view file, uint32 line, std::string_view function, std::string_view debugInfo, std::string_view message, std::string_view fmtMessage = {});
    [[noreturn]] void Fatal(std::string_view file, uint32 line, std::string_view function, std::string_view message, std::string_view fmtMessage = {});
    [[noreturn]] void Error(std::string_view file, uint32 line, std::string_view function, std::string_view message);
    [[noreturn]] void Abort(std::string_view file, uint32 line, std::string_view function, std::string_view fmtMessage = {});

    template<typename... Args>
    void Assert(const std::string_view file, const uint32 line, const std::string_view function, const std::string_view debugInfo,
        const std::string_view message, std::string_view fmt, Args&&... args)
    {
        Assert(file, line, function, debugInfo, message, StringFormat(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Fatal(const std::string_view file, const uint32 line, const std::string_view function,
        const std::string_view message, std::string_view fmt, Args&&... args)
    {
        Fatal(file, line, function, message, StringFormat(fmt, std::forward<Args>(args)...));
    }

    template<typename... Args>
    void Abort(const std::string_view file, const uint32 line, const std::string_view function,
        std::string_view fmt, Args&&... args)
    {
        Abort(file, line, function, StringFormat(fmt, std::forward<Args>(args)...));
    }

    void Warning(std::string_view file, uint32 line, std::string_view function, std::string_view message);

    [[noreturn]] void AbortHandler(int sigval);

}

std::string GetDebugInfo();

#define WPFatal(cond, ...) do { if (!(cond)) Acore::Fatal(__FILE__, __LINE__, __FUNCTION__, #cond, ##__VA_ARGS__); } while(0)
#define WPError(cond, msg) do { if (!(cond)) Acore::Error(__FILE__, __LINE__, __FUNCTION__, (msg)); } while(0)
#define WPWarning(cond, msg) do { if (!(cond)) Acore::Warning(__FILE__, __LINE__, __FUNCTION__, (msg)); } while(0)
#define WPAbort(...) do { Acore::Abort(__FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); } while(0)

#define ASSERT(cond, ...) do { if (!(cond)) Acore::Assert(__FILE__, __LINE__, __FUNCTION__, GetDebugInfo(), #cond, ##__VA_ARGS__); } while(0)
#define ASSERT_NO_DEBUG_INFO(cond) do { if (!(cond)) Acore::Assert(__FILE__, __LINE__, __FUNCTION__, "", #cond); } while(0)

#define ABORT WPAbort

template <typename T>
T* ASSERT_NOTNULL_IMPL(T* pointer, std::string_view expr)
{
    ASSERT(pointer, "{}", expr);
    return pointer;
}

#define ASSERT_NOTNULL(pointer) ASSERT_NOTNULL_IMPL(pointer, #pointer)

#endif
