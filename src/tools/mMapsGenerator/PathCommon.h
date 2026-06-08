#ifndef MMAP_COMMON_H
#define MMAP_COMMON_H

#include <cerrno>
#include <dirent.h>
#include <string>
#include <vector>
#include <boost/dll/runtime_symbol_info.hpp>

namespace MMAP
{
    inline std::string executableDirectoryPath()
    {
        return boost::dll::program_location().parent_path().string();
    }

    inline bool matchWildcardFilter(const char* filter, const char* str)
    {
        if (!filter || !str)
            return false;

        // End on null character
        while (*filter && *str)
        {
            if (*filter == '*')
            {
                if (*++filter == '\0')  // wildcard at end of filter means all remaining chars match
                    return true;

                while (*filter != *str)
                {
                    if (*str == '\0')
                        return false;  // Reached end of string without matching next filter character
                    str++;
                }
            }
            else if (*filter != *str)
                return false;  // Mismatch

            filter++;
            str++;
        }

        return (*filter == '\0' || (*filter == '*' && *++filter == '\0')) && *str == '\0';
    }

    enum ListFilesResult
    {
        LISTFILE_DIRECTORY_NOT_FOUND = 0,
        LISTFILE_OK = 1
    };

    inline ListFilesResult getDirContents(std::vector<std::string>& fileList, const std::string& dirpath = ".", const std::string& filter = "*")
    {
        const char* p = dirpath.c_str();
        DIR* dirp = opendir(p);
        dirent* dp;

        while (dirp)
        {
            errno = 0;
            if ((dp = readdir(dirp)) != nullptr)
            {
                if (matchWildcardFilter(filter.c_str(), dp->d_name))
                    fileList.emplace_back(dp->d_name);
            }
            else
                break;
        }

        if (dirp)
            closedir(dirp);
        else
            return LISTFILE_DIRECTORY_NOT_FOUND;

        return LISTFILE_OK;
    }
}

#endif
