#include "ProcessPriority.h"
#include <sys/resource.h>
#include "Log.h"

#define PROCESS_HIGH_PRIORITY (-15) // [-20, 19], default is 0

void SetProcessPriority(std::string const& logChannel, const bool highPriority)
{
    if (highPriority)
    {
        if (setpriority(PRIO_PROCESS, 0, PROCESS_HIGH_PRIORITY))
        {
            LOG_ERROR(logChannel, "Can't set process priority class, error: {}", strerror(errno));
        }
        else
        {
            LOG_INFO(logChannel, "Process priority class set to {}", getpriority(PRIO_PROCESS, 0));
        }
    }
}
