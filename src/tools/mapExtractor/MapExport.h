#ifndef NORDCORE_MAP_EXPORT_H
#define NORDCORE_MAP_EXPORT_H

#include <filesystem>
#include "DBCStorage.h"
#include "Define.h"
#include "MPQ.h"

constexpr float EXTRACT_MIN_HEIGHT              = -500.0f;
constexpr float EXTRACT_FLOAT_TO_INT8_LIMIT     = 2.0f;     // Max accuracy = val/256
constexpr float EXTRACT_FLOAT_TO_INT16_LIMIT    = 2048.0f;  // Max accuracy = val/65536
constexpr float EXTRACT_FLAT_HEIGHT_DELTA_LIMIT = 0.005f;   // If max - min less this value - surface is flat
constexpr float EXTRACT_FLAT_LIQUID_DELTA_LIMIT = 0.001f;   // If max - min less this value - liquid surface is flat

namespace fs = std::filesystem;

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

struct LiquidEntry
{
    explicit LiquidEntry(const QueryResult& result)
    {
        const Field* fields = result->Fetch();
        ID = fields[0].Get<uint32>();
        Type = fields[1].Get<uint8>();
    }

    uint32 ID;
    uint8 Type;
};

struct CinematicCameraEntry
{
    explicit CinematicCameraEntry(const QueryResult& result)
    {
        const Field* fields = result->Fetch();
        ID = fields[0].Get<uint32>();
        File = fields[1].Get<std::string>();
    }

    uint32 ID;
    std::string File;
};

void Abort(const std::string& message);
void CreateDir(const fs::path& dir);
uint32 ReadBuild(const std::string& locale);
bool ConvertADT(const std::string& inputPath, const std::string& outputPath, uint32 build, const MapEntry* mapEntry);

extern ArchiveSet gOpenArchives;
extern DBCStorage<MapEntry> sMapStore;
extern DBCStorage<LiquidEntry> sLiquidStore;

#endif
