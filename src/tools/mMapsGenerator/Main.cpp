#include <boost/filesystem.hpp>

#include "Config.h"
#include "MapBuilder.h"
#include "PathCommon.h"
#include "Timer.h"
#include "Util.h"

using namespace MMAP;

bool handleArgs(const int argc, char** argv, int& mapID, std::string& configFilePath, uint32& threads, std::filesystem::path& dataDirPath)
{
    bool hasCustomConfigPath = false;
    const char* param = nullptr;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string paramName = argv[i]; paramName == "--config")
        {
            param = argv[++i];
            if (!param)
                return false;

            hasCustomConfigPath = true;
            configFilePath = param;
        }
        else if (paramName == "--threads")
        {
            param = argv[++i];
            if (!param)
                return false;
            threads = static_cast<unsigned int>(std::max(0, atoi(param)));
        }
        else if (paramName == "--map")
        {
            param = argv[++i];
            if (!param)
                return false;
            std::string mapStr = param;
            if (mapStr == "0")
                mapID = 0;
            else if (const int map = atoi(mapStr.c_str()); map > 0)
                mapID = map;
            else
            {
                std::cout << "Invalid map id: " << mapStr << std::endl;
                return false;
            }
        }
        else if (!paramName.starts_with("--"))
            dataDirPath = paramName;
    }

    if (!hasCustomConfigPath)
    {
        FILE* f = fopen(configFilePath.c_str(), "r");
        if (!f)
        {
            const auto execRelPath = std::filesystem::path(executableDirectoryPath()) / configFilePath;
            f = fopen(execRelPath.c_str(), "r");
            if (!f)
                return false;
            configFilePath = execRelPath.string();
        }
        fclose(f);
    }

    if (!std::filesystem::is_directory(dataDirPath))
    {
        std::cerr << "Data directory not found: " << dataDirPath.string() << ". Specify its path." << std::endl;
        return false;
    }

    return true;
}

void Abort(const std::string& message)
{
    std::cerr << message << std::endl;
    exit(1);
}

int main(const int argc, char** argv)
{
    int mapID = -1;
    uint32 threads = std::thread::hardware_concurrency();
    std::string configFilePath = "mMapsConfig.yaml";
    std::filesystem::path dataDirPath(".");

    if (!handleArgs(argc, argv, mapID, configFilePath, threads, dataDirPath))
        Abort("You have specified invalid parameters");

    auto config = Config::FromFile(configFilePath, dataDirPath.string());
    if (!config)
        Abort("Failed to load configuration.");
    if (!config->CheckDirectories())
        Abort("Ensure to specify valid data directory path.");

    MapBuilder builder(&config.value(), mapID, threads);

    const uint32 start = getMSTime();
    if (mapID >= 0)
        builder.BuildMaps(static_cast<uint32>(mapID));
    else
        builder.BuildMaps({});
    std::cout << "Finished. mMaps were built in " << secsToTimeString(GetMSTimeDiffToNow(start) / 1000) << std::endl;
    return 0;
}
