#ifndef TILE_BUILDER_H
#define TILE_BUILDER_H

#include <DetourNavMesh.h>
#include <Recast.h>
#include <thread>

#include "Define.h"
#include "TerrainBuilder.h"

namespace MMAP
{
    class MapBuilder;

    class TileBuilder
    {
    public:
        explicit TileBuilder(MapBuilder* mapBuilder);

        TileBuilder(TileBuilder&&) = default;
        ~TileBuilder();

        void WorkerThread();
        void WaitCompletion();
    private:
        void buildTile(uint32 mapID, uint32 tileX, uint32 tileY, dtNavMesh* navMesh);
        bool shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY) const;
        void buildMoveMapTile(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, float bMin[3], float bMax[3], dtNavMesh* navMesh);

        MapBuilder* m_mapBuilder;
        TerrainBuilder* m_terrainBuilder;
        std::thread m_workerThread;
        rcContext* m_rcContext;  // Build performance - not really used for now
    };
}

#endif
