#ifndef VMAP_EXPORT_H
#define VMAP_EXPORT_H

#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include "Define.h"
#include "DBCStorage.h"

#define DIR_BIN_FILE "dir_bin"
#define TMP_GAME_OBJECT_MODELS "temp_gameobject_models"

namespace fs = std::filesystem;

namespace VMAP
{
    constexpr char VMAP_MAGIC[] = "VMAP_4.8";
    constexpr char RAW_VMAP_MAGIC[] = "VMAP048";  // Used in extracted vmap files with raw data
}

enum ModelFlags
{
    MOD_M2          = 0x1,
    MOD_WORLD_SPAWN = 0x2,
    MOD_HAS_BOUND   = 0x4,
};

struct MapEntry
{
    explicit MapEntry(const QueryResult& result)
    {
        const Field* fields = result->Fetch();
        ID = fields[0].Get<uint32>();
        Directory = fields[1].Get<std::string>();
        Name = fields[2].Get<std::string>();
    }

    uint32 ID;
    std::string Directory;
    std::string Name;
};

struct GameObjectDisplayInfoEntry
{
    explicit GameObjectDisplayInfoEntry(const QueryResult& result)
    {
        const Field* fields = result->Fetch();
        ID = fields[0].Get<uint32>();
        Name = fields[1].Get<std::string>();
    }

    uint32 ID;
    std::string Name;
};

struct WMODoodadData;

extern fs::path szWorkDirWmo;
extern DBCStorage<MapEntry> sMapStore;
extern std::unordered_map<std::string, WMODoodadData> WmoDoodads;
extern std::map<std::pair<uint32, uint16>, uint32> uniqueObjectIds;

void Abort(const std::string& message);
void CreateDir(const std::string& dir);
std::string GetPlainName(const std::string& filepath);

uint32 GenerateUniqueObjectId(uint32 clientId, uint16 clientDoodadId);
bool ExtractSingleWmo(const std::string& filename, std::string& outName);
bool ExtractSingleModel(const std::string& filename, std::string& outName);
void ExtractGameObjectModels();

#endif
