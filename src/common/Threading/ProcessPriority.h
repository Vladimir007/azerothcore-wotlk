#ifndef PROCESS_PRIORITY_H
#define PROCESS_PRIORITY_H

#include <string>

#define CONFIG_HIGH_PRIORITY "ProcessPriority"

void SetProcessPriority(std::string const& logChannel, bool highPriority);

#endif
