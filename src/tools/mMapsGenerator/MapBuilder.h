#ifndef MAP_BUILDER_H
#define MAP_BUILDER_H

#include <atomic>
#include <DetourNavMesh.h>
#include <list>
#include <Recast.h>
#include <set>
#include <vector>

#include "Config.h"
#include "Optional.h"
#include "PCQueue.h"
#include "TerrainBuilder.h"
#include "TileBuilder.h"

using namespace VMAP;

namespace MMAP
{
    struct MapTiles
    {
        MapTiles() : mapID(static_cast<uint32>(-1)) {}

        MapTiles(const uint32 id, std::set<uint32>* tiles) : mapID(id), tiles(tiles) {}
        ~MapTiles() = default;

        uint32 mapID;
        std::set<uint32>* tiles{nullptr};

        bool operator==(const uint32 id) const
        {
            return mapID == id;
        }
    };

    typedef std::list<MapTiles> TileList;

    struct Tile
    {
        Tile()  {}
        ~Tile()
        {
            rcFreeCompactHeightfield(chf);
            rcFreeContourSet(cSet);
            rcFreeHeightField(solid);
            rcFreePolyMesh(pMesh);
            rcFreePolyMeshDetail(dMesh);
        }
        rcCompactHeightfield* chf{nullptr};
        rcHeightfield* solid{nullptr};
        rcContourSet* cSet{nullptr};
        rcPolyMesh* pMesh{nullptr};
        rcPolyMeshDetail* dMesh{nullptr};
    };

    struct TileInfo
    {
        TileInfo() : mapID(static_cast<uint32>(-1)), tileX(), tileY(), navMeshParams() {}

        uint32 mapID;
        uint32 tileX;
        uint32 tileY;
        dtNavMeshParams navMeshParams;
    };

    class MapBuilder
    {
        friend class TileBuilder;
    public:
        MapBuilder(Config* config, int mapID, uint32 threads);
        ~MapBuilder();

        void BuildMaps(Optional<uint32> mapID);
        const Config& GetConfig() const { return *_config; }

    private:
        void buildMap(uint32 mapID);  // Builds all mMap tiles for the specified map id (ignores skip settings)
        void discoverTiles();  // Detect maps and tiles
        std::set<uint32>* getTileList(uint32 mapID);

        void buildNavMesh(uint32 mapID, dtNavMesh*& navMesh);

        static void getTileBounds(uint32 tileX, uint32 tileY, const float* vertices, int vertCount, float* bMin, float* bMax);
        void getGridBounds(uint32 mapID, uint32& minX, uint32& minY, uint32& maxX, uint32& maxY) const;

        bool shouldSkipMap(uint32 mapID) const;

        static rcConfig getRecastConfig(const ResolvedMeshConfig &cfg, float bMin[3], float bMax[3]);

        static uint32 percentageDone(uint32 totalTiles, uint32 totalTilesDone);
        uint32 currentPercentageDone() const;

        TerrainBuilder* _terrainBuilder{nullptr};
        TileList _tiles;
        uint32 _threads;
        int32 _mapID;
        Config* _config;

        std::atomic<uint32> _totalTiles;
        std::atomic<uint32> _totalTilesProcessed;

        rcContext* _rcContext{nullptr};  // Build performance - not really used for now

        std::vector<TileBuilder*> _tileBuilders;
        ProducerConsumerQueue<TileInfo> _queue;
        std::atomic<bool> _cancellationToken;
    };
}

#endif
