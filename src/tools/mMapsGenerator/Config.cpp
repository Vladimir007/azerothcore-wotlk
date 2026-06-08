#include "Config.h"
#include <filesystem>
#include <boost/filesystem.hpp>
#include <fkYAML/node.hpp>
#include "PathCommon.h"
#include "TerrainBuilder.h"

namespace MMAP
{
    float ComputeBaseUnitDim(const int vertexPerMapEdge)
    {
        return GRID_SIZE / static_cast<float>(vertexPerMapEdge);
    }

    std::pair<uint32, uint32> MakeTileKey(uint32 x, uint32 y)
    {
        return {x, y};
    }

    bool isCurrentDirectory(const std::string& pathStr) {
        try {
            const std::filesystem::path givenPath = std::filesystem::canonical(std::filesystem::absolute(pathStr));
            const std::filesystem::path currentPath = std::filesystem::canonical(std::filesystem::current_path());
            return givenPath == currentPath;
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "Filesystem error: " << e.what() << "\n";
            return false;
        }
    }

    MmapTileRecastConfig ResolvedMeshConfig::toMMAPTileRecastConfig() const {
        MmapTileRecastConfig config;
        config.walkableSlopeAngle = walkableSlopeAngle;
        config.walkableHeight = walkableHeight;
        config.walkableClimb = walkableClimb;
        config.walkableRadius = walkableRadius;
        config.maxSimplificationError = maxSimplificationError;
        config.cellSizeHorizontal = cellSizeHorizontal;
        config.cellSizeVertical = cellSizeVertical;
        config.baseUnitDim = baseUnitDim;
        config.vertexPerMapEdge = vertexPerMapEdge;
        config.vertexPerTileEdge = vertexPerTileEdge;
        config.tilesPerMapEdge = tilesPerMapEdge;
        return config;
    }

    std::optional<Config> Config::FromFile(const std::string_view configFile, const std::string_view dataDir) {
        Config config(dataDir);
        if (!config.LoadConfig(configFile))
            return std::nullopt;
        return config;
    }

    bool Config::CheckDirectories()
    {
        std::vector<std::string> dirFiles;

        if (getDirContents(dirFiles, (_dataDir / "Maps").string()) == LISTFILE_DIRECTORY_NOT_FOUND || dirFiles.empty())
        {
            std::cerr << "'Maps' directory is empty or does not exist" << std::endl;
            return false;
        }

        dirFiles.clear();
        if (getDirContents(dirFiles, (_dataDir / "vMaps").string(), "*.vmtree") == LISTFILE_DIRECTORY_NOT_FOUND || dirFiles.empty())
        {
            std::cerr << "'vMaps' directory is empty or does not exist" << std::endl;
            return false;
        }

        dirFiles.clear();
        if (getDirContents(dirFiles, (_dataDir / "mMaps").string()) == LISTFILE_DIRECTORY_NOT_FOUND)
            return boost::filesystem::create_directory((_dataDir / "mMaps").string());

        dirFiles.clear();
        return true;
    }

    Config::Config(const std::string_view dataDir)
    {
        _dataDir = dataDir;

        // Resolve data dir path. Maybe we need to use an executable path instead of the current dir.
        if (isCurrentDirectory(_dataDir.string()) && !std::filesystem::exists(MapsPath()))
            if (const auto execPath = std::filesystem::path(executableDirectoryPath()); std::filesystem::exists(execPath/ "Maps"))
                _dataDir = execPath;
    }

    ResolvedMeshConfig Config::GetConfigForTile(const uint32 mapID, const uint32 tileX, const uint32 tileY) const
    {
        const MapOverride* mapOverride = nullptr;
        const TileOverride* tileOverride = nullptr;

        // Lookup map and tile overrides
        if (const auto mapIt = _maps.find(mapID); mapIt != _maps.end())
        {
            mapOverride = &mapIt->second;

            const auto tileIt = mapOverride->tileOverrides.find(MakeTileKey(tileY, tileX));
            if (tileIt != mapOverride->tileOverrides.end())
                tileOverride = &tileIt->second;
        }

        // Helper lambdas to resolve values in order: tile -> map -> global
        auto resolveFloat = [&](auto TileField, auto MapField, const float GlobalValue) -> float {
            if (tileOverride && TileField(tileOverride)) return *TileField(tileOverride);
            if (mapOverride && MapField(mapOverride)) return *MapField(mapOverride);
            return GlobalValue;
        };

        auto resolveInt = [&](auto TileField, auto MapField, const int GlobalValue) -> int {
            if (tileOverride && TileField(tileOverride)) return *TileField(tileOverride);
            if (mapOverride && MapField(mapOverride)) return *MapField(mapOverride);
            return GlobalValue;
        };

        // Resolve vertex settings
        const int vertexPerMap = resolveInt(
            [](const TileOverride*) { return std::optional<int>{}; },
            [](const MapOverride* m) { return m->vertexPerMapEdge; },
            _global.vertexPerMapEdge
        );

        const int vertexPerTile = resolveInt(
            [](const TileOverride*) { return std::optional<int>{}; },
            [](const MapOverride* m) { return m->vertexPerTileEdge; },
            _global.vertexPerTileEdge
        );

        ResolvedMeshConfig config;
        config.walkableSlopeAngle = resolveFloat(
            [](const TileOverride* t) { return t->walkableSlopeAngle; },
            [](const MapOverride* m) { return m->walkableSlopeAngle; },
            _global.walkableSlopeAngle
        );

        config.walkableRadius = resolveInt(
            [](const TileOverride* t) { return t->walkableRadius; },
            [](const MapOverride* m) { return m->walkableRadius; },
            _global.walkableRadius
        );

        config.walkableHeight = resolveInt(
            [](const TileOverride* t) { return t->walkableHeight; },
            [](const MapOverride* m) { return m->walkableHeight; },
            _global.walkableHeight
        );

        config.walkableClimb = resolveInt(
            [](const TileOverride* t) { return t->walkableClimb; },
            [](const MapOverride* m) { return m->walkableClimb; },
            _global.walkableClimb
        );

        config.vertexPerMapEdge = vertexPerMap;
        config.vertexPerTileEdge = vertexPerTile;
        config.baseUnitDim = ComputeBaseUnitDim(vertexPerMap);
        config.tilesPerMapEdge = vertexPerMap / vertexPerTile;
        config.maxSimplificationError = _global.maxSimplificationError;
        config.cellSizeHorizontal = config.baseUnitDim;
        config.cellSizeVertical = config.baseUnitDim;

        if (mapOverride && mapOverride->cellSizeHorizontal.has_value())
            config.cellSizeHorizontal = *mapOverride->cellSizeHorizontal;

        if (mapOverride && mapOverride->cellSizeVertical.has_value())
            config.cellSizeVertical = *mapOverride->cellSizeVertical;

        return config;
    }

    bool Config::LoadConfig(std::string_view configFile) {
        FILE* f = std::fopen(configFile.data(), "r");
        if (!f)
            return false;

        fkyaml::node root = fkyaml::node::deserialize(f);
        std::fclose(f);

        if (!root.contains("mMapsConfig"))
            return false;

        fkyaml::node mMapsNode = root["mMapsConfig"];

        auto tryFloat = [](const fkyaml::node& n, const char* key, float& out)
        {
            if (n.contains(key)) out = n[key].get_value<float>();
        };
        auto tryInt = [](const fkyaml::node& n, const char* key, int& out)
        {
            if (n.contains(key)) out = n[key].get_value<int>();
        };

        // Global config
        tryFloat(mMapsNode, "walkableSlopeAngle", _global.walkableSlopeAngle);
        tryInt(mMapsNode, "walkableHeight", _global.walkableHeight);
        tryInt(mMapsNode, "walkableClimb", _global.walkableClimb);
        tryInt(mMapsNode, "walkableRadius", _global.walkableRadius);
        tryInt(mMapsNode, "vertexPerMapEdge", _global.vertexPerMapEdge);
        tryInt(mMapsNode, "vertexPerTileEdge", _global.vertexPerTileEdge);
        tryFloat(mMapsNode, "maxSimplificationError", _global.maxSimplificationError);

        // Map overrides
        if (mMapsNode.contains("mapsOverrides"))
        {
            for (fkyaml::node maps = mMapsNode["mapsOverrides"]; const auto& [map, node] : maps.as_map())
            {
                uint32 mapId = std::stoi(map.as_str());
                fkyaml::node mapNode = node;

                MapOverride override;

                if (mapNode.contains("walkableSlopeAngle"))
                    override.walkableSlopeAngle = mapNode["walkableSlopeAngle"].get_value<float>();
                if (mapNode.contains("walkableRadius"))
                    override.walkableRadius = mapNode["walkableRadius"].get_value<int>();
                if (mapNode.contains("walkableHeight"))
                    override.walkableHeight = mapNode["walkableHeight"].get_value<int>();
                if (mapNode.contains("walkableClimb"))
                    override.walkableClimb = mapNode["walkableClimb"].get_value<int>();
                if (mapNode.contains("vertexPerMapEdge"))
                    override.vertexPerMapEdge = mapNode["vertexPerMapEdge"].get_value<int>();
                if (mapNode.contains("cellSizeHorizontal"))
                    override.cellSizeHorizontal = mapNode["cellSizeHorizontal"].get_value<float>();
                if (mapNode.contains("cellSizeVertical"))
                    override.cellSizeVertical = mapNode["cellSizeVertical"].get_value<float>();

                // Tile overrides
                if (mapNode.contains("tilesOverrides"))
                {
                    for (fkyaml::node tiles = mapNode["tilesOverrides"]; const auto& [tile, node] : tiles.as_map())
                    {
                        std::string key = tile.as_str();
                        fkyaml::node tileNode = node;

                        size_t comma = key.find(',');
                        if (comma == std::string::npos)
                            continue;

                        uint32 tileX = static_cast<uint32>(std::stoi(key.substr(0, comma)));
                        uint32 tileY = static_cast<uint32>(std::stoi(key.substr(comma + 1)));

                        TileOverride tileOverride;
                        if (tileNode.contains("walkableSlopeAngle"))
                            tileOverride.walkableSlopeAngle = tileNode["walkableSlopeAngle"].get_value<float>();
                        if (tileNode.contains("walkableRadius"))
                            tileOverride.walkableRadius = tileNode["walkableRadius"].get_value<int>();
                        if (tileNode.contains("walkableHeight"))
                            tileOverride.walkableHeight = tileNode["walkableHeight"].get_value<int>();
                        if (tileNode.contains("walkableClimb"))
                            tileOverride.walkableClimb = tileNode["walkableClimb"].get_value<int>();

                        override.tileOverrides[{tileX, tileY}] = std::move(tileOverride);
                    }
                }

                _maps[mapId] = std::move(override);
            }
        }

        return true;
    }
}
