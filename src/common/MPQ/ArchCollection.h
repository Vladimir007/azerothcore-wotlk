#ifndef MPQ_ARCH_COLLECTION_H
#define MPQ_ARCH_COLLECTION_H

#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#define LANG_COUNT 12
static const std::array<std::string, LANG_COUNT> ArchLocales = {
    "enGB", "enUS", "deDE", "esES", "frFR", "koKR", "zhCN", "zhTW", "enCN", "enTW", "esMX", "ruRU"
};

inline bool fillArchivePathVector(fs::path sourceDir, std::vector<std::string>& pArchiveNames, std::string& localeOut)
{
    // Prepare source directory
    sourceDir = sourceDir / "Data";
    if (!fs::is_directory(sourceDir))
    {
        std::cerr << "Game data directory not found: " << sourceDir << std::endl;
        return false;
    }
    std::cout << "Game data path: " << sourceDir << std::endl;

    // Find locale
    std::optional<std::string> firstLocale = std::nullopt;
    for (auto& locale : ArchLocales)
    {
        if (!fs::is_directory(sourceDir / locale))
            continue;
        std::cout << "Found locale: " << locale << std::endl;
        firstLocale = locale;
        break;
    }
    if (!firstLocale)
    {
        std::cerr << "Locale not found" << std::endl;
        return false;
    }

    localeOut = firstLocale.value();

    // Add common archives
    const std::vector<std::string> RootArchNames =
    {
        "common.MPQ",
        "common-2.MPQ",
        "lichking.MPQ",
        "expansion.MPQ",
        "patch.MPQ",
    };
    for (const auto& name : RootArchNames)
    {
        if (auto archivePath = sourceDir / name; fs::is_regular_file(archivePath))
            pArchiveNames.push_back(archivePath.string());
    }

    for (int i = 2; i <= 99; i++)
    {
        fs::path archivePath = sourceDir / std::format("patch-{}.MPQ", i);
        if (!fs::is_regular_file(archivePath))
            break;
        pArchiveNames.push_back(archivePath.string());
    }

    // Add locale archives
    const std::vector<std::string> LocaleArchNames =
    {
        "locale",
        "expansion-locale",
        "lichking-locale",
        "patch",
    };
    for (const auto& name : LocaleArchNames)
    {
        if (fs::path archivePath = sourceDir / localeOut / std::format("{}-{}.MPQ", name, localeOut); fs::is_regular_file(archivePath))
            pArchiveNames.push_back(archivePath.string());
    }
    for (int i = 2; i <= 99; i++)
    {
        fs::path archivePath = sourceDir / localeOut / std::format("patch-{}-{}.MPQ", localeOut, i);
        if (!fs::is_regular_file(archivePath))
            break;
        pArchiveNames.push_back(archivePath.string());
    }
    return true;
}

#endif
