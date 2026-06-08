#include <filesystem>
#include <format>
#include <iostream>
#include <set>
#include <vector>

#include "ArchCollection.h"
#include "DBCStorage.h"
#include "MapDefines.h"
#include "MapExport.h"
#include "MPQ.h"
#include "wdt.h"

namespace fs = std::filesystem;

fs::path outputPath(".");
fs::path inputPath(".");

// Extractor options
bool ExtractModeDBC = false;

void Usage(const std::string& progName)
{
    std::cout << "Usage:" << std::endl;
    std::cout << progName << " -[var] [value]" << std::endl;
    std::cout << "-i set input path (default current dir)" << std::endl;
    std::cout << "-o set output path (default current dir)" << std::endl;
    std::cout << "-d extract DBC files only (default 0)" << std::endl;
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
                outputPath = arg[++c];
                continue;
            }
            Usage(arg[0]);
            break;

        case 'm':
            if (c + 1 < argc)
            {
                ExtractModeDBC = atoi(arg[++c]) != 0;
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

void ExtractDBCFiles()
{
    std::cout << "Extracting DBC files..." << std::endl;

    std::set<std::string> dbcFiles;

    // Get DBC file list
    for (const auto& gOpenArchive : gOpenArchives)
    {
        std::vector<string> files;
        gOpenArchive->GetFileListTo(files);
        for (auto & file : files)
            if (file.ends_with(".dbc"))
                dbcFiles.insert(file);
    }

    const fs::path dbcPath = outputPath / "DBC";
    CreateDir(dbcPath);

    uint32 count = 0;
    for (const auto & dbcFile : dbcFiles)
    {
        fs::path filename = dbcPath / dbcFile.substr(strlen("DBFilesClient\\"));
        if (fs::exists(filename))
            continue;

        if (!ExtractMPQFile(dbcFile.c_str(), filename.string()))
            Abort(std::format("Can't extract camera file: {}", filename.string()));
        ++count;
    }
    std::cout << std::format("Extracted {} DBC files", count) << std::endl;
}

void ExtractCameraFiles()
{
    std::cout << "Extracting camera files..." << std::endl;
    DBCStorage<CinematicCameraEntry> sCamStore;
    if (!sCamStore.Load("dbc_cinematic_camera", "id, file", "id"))
        Abort("Failed to collect cinematic camera entries.");

    const fs::path camerasPath = outputPath / "Cameras";
    CreateDir(camerasPath);

    uint32 count = 0;
    for (const auto entry : sCamStore)
    {
        std::string camFile(entry->File);
        if (camFile.ends_with(".mdx"))
            camFile.replace(camFile.length() - 4, 4, ".m2");

        fs::path filename = camerasPath / camFile.substr(strlen("Cameras\\"));
        if (fs::exists(filename))
            continue;
        if (!ExtractMPQFile(camFile.c_str(), filename.string()))
            Abort(std::format("Can't extract camera file: {}", filename.string()));
        ++count;
    }
    std::cout << std::format("Extracted {} camera files", count) << std::endl;
}

void ExtractMapsFromMpq(const uint32 build)
{
    std::cout << "Extracting maps..." << std::endl;

    std::cout << "Read Map.dbc entries..." << std::endl;
    if (!sMapStore.Load("dbc_map", "id, directory, name", "id"))
        Abort("Can't load Map.dbc entries!");
    std::cout << std::format("Done! ({} maps loaded)", sMapStore.GetNumRows()) << std::endl;

    std::cout << "Read LiquidType.dbc entries..." << std::endl;
    if (!sLiquidStore.Load("dbc_liquid", "id, type", "id"))
        Abort("Can't load LiquidType.dbc entries!");
    std::cout << std::format("Done! ({} liquid entries loaded)", sLiquidStore.GetNumRows()) << std::endl;

    const fs::path mapsPath = outputPath / "Maps";
    CreateDir(mapsPath);

    std::cout << "Convert map files" << std::endl;
    int cnt = 0;
    int mapCount = sMapStore.GetNumRows();
    for (const auto map : sMapStore)
    {
        std::cout << std::format("\rExtract {} ({}/{})", map->Name, ++cnt, mapCount) << std::endl;
        std::string mpqMapName = std::format(R"(World\Maps\{}\{}.wdt)", map->Directory, map->Directory);
        FileWDT wdt;
        if (!wdt.loadFile(mpqMapName, false))
            continue;

        for (uint32 y = 0; y < WDT_MAP_SIZE; ++y)
        {
            for (uint32 x = 0; x < WDT_MAP_SIZE; ++x)
            {
                if (!wdt.main->adt_list[y][x].exist)
                    continue;
                std::string mpqFileName = std::format(R"(World\Maps\{}\{}_{}_{}.adt)", map->Directory, map->Directory, x, y);
                fs::path outputFile = mapsPath / std::format("{:03}{:02}{:02}.map", map->ID, y, x);
                ConvertADT(mpqFileName, outputFile.string(), build, map);
            }

            // Draw progress bar
            std::cout << "\rProcessing..." << 100 * (y + 1) / WDT_MAP_SIZE << "%";
        }
    }
    std::cout << std::endl;
}

int main(const int argc, char* arg[])
{
    std::cout << "Map Extractor" << std::endl;
    std::cout << "===================" << std::endl << std::endl;

    HandleArgs(argc, arg);

    std::vector<std::string> archivePaths;
    std::string locale;
    if (!fillArchivePathVector(inputPath, archivePaths, locale))
    {
        std::cerr << "Fatal error: can't collect MPQ archives to extract" << std::endl;
        exit(1);
    }

    OpenMPQFiles(archivePaths);

    if (ExtractModeDBC)
        ExtractDBCFiles();
    else
    {
        const uint32 build = ReadBuild(locale);
        std::cout << "Detected client build: " << build << std::endl;
        ExtractCameraFiles();
        ExtractMapsFromMpq(build);
    }

    CloseMPQFiles();

    return 0;
}
