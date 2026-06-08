#include "vMapExport.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>

#include "DBCStorage.h"
#include "MPQ.h"
#include "wmo.h"

DBCStorage<MapEntry> sMapStore;
std::unordered_map<std::string, WMODoodadData> WmoDoodads;
std::map<std::pair<uint32, uint16>, uint32> uniqueObjectIds;

void Abort(const std::string& message)
{
    std::cerr << message << std::endl;
    CloseMPQFiles();
    exit(1);
}

void CreateDir(const fs::path& dir)
{
    if (fs::is_directory(dir))
        return;
    if (fs::create_directories(dir))
        return;
    Abort(std::format("Fatal Error: Could not create directory '{}'. Check your permissions.", dir.string()));
}

std::string GetPlainName(const std::string& filepath)
{
    std::string _filepath(filepath);
    std::ranges::replace(_filepath, '\\', '/');

    const fs::path p(_filepath);
    std::string name = p.filename();
    std::ranges::replace(name, ' ', '_');
    std::ranges::transform(name, name.begin(), [](const unsigned char c) { return std::tolower(c); });
    return name;
}

uint32 GenerateUniqueObjectId(uint32 clientId, uint16 clientDoodadId)
{
    return uniqueObjectIds.emplace(std::make_pair(clientId, clientDoodadId), static_cast<uint32>(uniqueObjectIds.size() + 1)).first->second;
}

bool ExtractSingleWmo(const std::string& filename, std::string& outName)
{
    outName = GetPlainName(filename);
    const fs::path outPath = szWorkDirWmo / outName;
    if (fs::exists(outPath))
        return true;

    std::cout << "Extracting " << filename << std::endl;
    WMORoot froot(filename);
    if (!froot.open())
    {
        std::cerr << "Couldn't open WMORoot!" << std::endl;
        return false;
    }

    FILE* output = fopen(outPath.c_str(), "wb");
    if (!output)
    {
        std::cerr << std::format("Couldn't open file '{}' for writing!", outPath.string()) << std::endl;
        return false;
    }

    froot.ConvertToVMAPRootWmo(output);
    WMODoodadData& doodads = WmoDoodads[outName];
    std::swap(doodads, froot.DoodadData);
    int nVertices = 0;
    uint32 groupCount = 0;
    bool fileOK = true;
    if (froot.nGroups != 0)
    {
        for (uint32 i = 0; i < froot.nGroups; ++i)
        {
            std::string wmoGroupName = std::format("{}_{:03}.wmo", outPath.stem().string(), i);
            fs::path groupFilePath = outPath;
            groupFilePath.replace_filename(wmoGroupName);
            WMOGroup fgroup(groupFilePath);
            if (!fgroup.open(&froot))
            {
                std::cerr << "Could not open group WMO file: " << groupFilePath.string() << std::endl;
                fileOK = false;
                break;
            }

            if (fgroup.ShouldSkip(&froot))
                continue;

            nVertices += fgroup.ConvertToVMAPGroupWmo(output);
            ++groupCount;

            for (uint16 groupReference : fgroup.DoodadReferences)
            {
                if (groupReference >= doodads.Spawns.size())
                    continue;
                if (!froot.ValidDoodadNames.contains(doodads.Spawns[groupReference].NameIndex))
                    continue;
                doodads.References.insert(groupReference);
            }
        }
    }

    fseek(output, 8, SEEK_SET); // Store the correct number of vertices
    fwrite(&nVertices, sizeof(int), 1, output);
    fwrite(&groupCount, sizeof(uint32), 1, output);  // Store the correct number of groups
    fclose(output);

    // Delete the extracted file in the case of an error
    if (!fileOK)
        remove(outPath);
    return true;
}
