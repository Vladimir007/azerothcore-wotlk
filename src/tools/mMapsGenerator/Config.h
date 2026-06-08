#ifndef MMAP_CONFIG_H
#define MMAP_CONFIG_H

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <boost/program_options/options_description.hpp>

#include "Define.h"
#include "MapDefines.h"

namespace fs = std::filesystem;

template <>
struct std::hash<std::pair<uint32_t, uint32_t>>
{
    std::size_t operator()(const std::pair<uint32_t, uint32_t>& p) const noexcept
    {
        return std::hash<uint64_t>()((static_cast<uint64_t>(p.first) << 32) | p.second);
    }
};

namespace MMAP
{
    struct ResolvedMeshConfig {
        float walkableSlopeAngle;
        int walkableRadius;
        int walkableHeight;
        int walkableClimb;
        int vertexPerMapEdge;
        int vertexPerTileEdge;
        int tilesPerMapEdge;
        float baseUnitDim;
        float cellSizeHorizontal;
        float cellSizeVertical;
        float maxSimplificationError;

        MmapTileRecastConfig toMMAPTileRecastConfig() const;
    };

    class Config {
    public:
        static std::optional<Config> FromFile(std::string_view configFile, std::string_view dataDir);

        ~Config() = default;

        ResolvedMeshConfig GetConfigForTile(uint32 mapID, uint32 tileX, uint32 tileY) const;

        fs::path VMapsPath() const { return _dataDir / "vMaps"; }
        fs::path MapsPath() const { return _dataDir / "Maps"; }
        fs::path MMapsPath() const { return _dataDir / "mMaps"; }
        fs::path DataDirPath() const { return _dataDir; }

        bool CheckDirectories();

    private:
        explicit Config(std::string_view dataDir);

        bool LoadConfig(std::string_view configFile);

        struct TileOverride {
            std::optional<float> walkableSlopeAngle;
            std::optional<int> walkableRadius;
            std::optional<int> walkableHeight;
            std::optional<int> walkableClimb;
        };

        struct MapOverride {
            std::optional<float> walkableSlopeAngle;
            std::optional<int> walkableRadius;
            std::optional<int> walkableHeight;
            std::optional<int> walkableClimb;
            std::optional<int> vertexPerMapEdge;
            std::optional<int> vertexPerTileEdge;

            // The width/depth of each cell in the XZ-plane grid used for voxelization. [Units: world units]
            // A smaller value increases navmesh resolution but also memory and CPU usage.
            // Default is equal to calculated baseUnitDim.
            // Recast reference: https://github.com/recastnavigation/recastnavigation/blob/bd98d84c274ee06842bf51a4088ca82ac71f8c2d/Recast/Include/Recast.h#L231
            std::optional<float> cellSizeHorizontal;

            // The height of each cell in the Y-axis used for voxelization. [Units: world units]
            // Controls how vertical features are represented. Lower values improve accuracy for uneven terrain.
            // Default is equal to calculated baseUnitDim.
            // Recast reference: https://github.com/recastnavigation/recastnavigation/blob/bd98d84c274ee06842bf51a4088ca82ac71f8c2d/Recast/Include/Recast.h#L234
            std::optional<float> cellSizeVertical;

            std::unordered_map<std::pair<uint32, uint32>, TileOverride> tileOverrides;
        };

        struct GlobalConfig {
            // Maximum slope angle (in degrees) NPCs can walk on.
            // Surfaces steeper than this will be considered unwalkable.
            float walkableSlopeAngle = 60.0f;

            // Minimum distance (in cell units) around walkable surfaces.
            // Helps prevent NPCs from clipping into walls and narrow gaps.
            int walkableRadius = 2;

            // Minimum ceiling height (in cell units) NPCs need to pass under an obstacle.
            // Controls how much vertical clearance is required.
            int walkableHeight = 6;

            // Maximum height difference (in cell units) NPCs can step up or down.
            // Higher values allow walking over fences, ledges, or steps.
            int walkableClimb = 6;

            // Number of vertices along one edge of the entire map's navmesh grid.
            // Higher values increase mesh resolution but also CPU/memory usage.
            int vertexPerMapEdge = 2000;

            // Number of vertices along one edge of each tile chunk.
            // Must divide (vertexPerMapEdge - 1) evenly for seamless tiles.
            // A higher vertex count per tile means fewer total tiles,
            // reducing runtime work to load, unload, and manage tiles.
            int vertexPerTileEdge = 80;

            // Tolerance for how much a polygon can deviate from the original geometry when simplified.
            // Higher values produce simpler (faster) meshes but can reduce accuracy.
            float maxSimplificationError = 1.8f;
        };

        GlobalConfig _global;
        std::unordered_map<uint32, MapOverride> _maps;
        std::filesystem::path _dataDir;
    };
}

#endif //CONFIG_H
