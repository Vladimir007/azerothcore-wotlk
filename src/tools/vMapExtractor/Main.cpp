#include <iostream>
#include <filesystem>
#include <vector>

#include "adtfile.h"
#include "ArchCollection.h"
#include "MPQ.h"
#include "vMapExport.h"
#include "wdtfile.h"

namespace fs = std::filesystem;

fs::path inputPath(".");
fs::path szWorkDirWmo("Buildings");

void Usage(const std::string& progName)
{

    std::cout << "Usage:" << std::endl;
    std::cout << progName << " -[var] [value]" << std::endl;
    std::cout << "-i set input path (default current dir)" << std::endl;
    std::cout << "-o set output path (default current dir)" << std::endl;
    std::cout << "Example: " << progName << " -i \"/home/username/games/WorldOfWarcraft\"" << std::endl;
    exit(1);
}

void HandleArgs(const int argc, char* arg[])
{
    for (int c = 1; c < argc; ++c)
    {
        if (arg[c][0] != '-')
            Usage(arg[0]);

        switch (arg[c][1])
        {
        case 'i':
            if (c + 1 < argc)
            {
                inputPath = arg[++c];
                continue;
            }
            Usage(arg[0]);
            break;
        case 'o':
            if (c + 1 < argc)
            {
                fs::path outputPath = arg[++c];
                szWorkDirWmo = outputPath / "Buildings";
                continue;
            }
            Usage(arg[0]);
            break;
        default:
            Usage(arg[0]);
            break;
        }
    }
}

int main(const int argc, char* arg[])
{
    HandleArgs(argc, arg);

    // Simple check if working dir is dirty
    if (fs::exists(szWorkDirWmo / DIR_BIN_FILE))
        Abort("Your output directory seems to be polluted, please use an empty directory!");

    CreateDir(szWorkDirWmo);

    // Prepare archive name list
    std::vector<std::string> archives;
    std::string locale;
    fillArchivePathVector(inputPath, archives, locale);

    OpenMPQFiles(archives);

    if (gOpenArchives.empty())
        Abort(std::format("FATAL ERROR: None MPQ archive found by path '{}'.", inputPath.string()));

    std::cout << "Read Map.dbc entries..." << std::endl;
    DBCStorage<MapEntry> sMapStore;
    if (!sMapStore.Load("dbc_map", "id, directory, name", "id"))
        Abort("Can't load Map.dbc entries!");
    std::cout << std::format("Done! ({} maps loaded)", sMapStore.GetNumRows()) << std::endl;

    for (const auto map : sMapStore)
    {
        std::string mpqMapName = std::format(R"(World\Maps\{0}\{0}.wdt)", map->Directory);
        WDTFile WDT(mpqMapName.c_str(), map->Directory);

        if (!WDT.init(map->ID))
            continue;

        std::cout << "Processing Map " << map->Name << std::endl;
        for (int x = 0; x < 64; ++x)
        {
            for (int y = 0; y < 64; ++y)
            {
                if (ADTFile* ADT = WDT.GetMap(x, y))
                {
                    ADT->init(map->ID, x, y);
                    delete ADT;
                }
            }
        }
    }

    // Extract models, listed in GameObjectDisplayInfo.dbc
    ExtractGameObjectModels();

    std::cout << "Work complete. No errors." << std::endl;
    return 0;
}
