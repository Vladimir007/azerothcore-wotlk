#include "MapBuilder.h"

#include <DetourCommon.h>
#include <DetourNavMesh.h>
#include <filesystem>
#include <format>

#include "IntermediateValues.h"
#include "MapTree.h"
#include "MMapMgr.h"
#include "ModelInstance.h"
#include "PathCommon.h"
#include "StringFormat.h"
#include "VMapMgr.h"

namespace fs = std::filesystem;

namespace MMAP
{
    MapBuilder::MapBuilder(Config* config, const int mapID, const uint32 threads) :
        _threads            (threads),
        _mapID              (mapID),
        _config             (config),
        _totalTiles         (0u),
        _totalTilesProcessed(0u),
        _cancellationToken  (false)
    {
        _terrainBuilder = new TerrainBuilder(config);
        _rcContext = new rcContext(false);
        _threads = std::max(1u, _threads);  // At least 1 thread is needed
        discoverTiles();
    }

    MapBuilder::~MapBuilder()
    {
        for (const auto & mapTiles : _tiles)
        {
            mapTiles.tiles->clear();
            delete mapTiles.tiles;
        }

        delete _terrainBuilder;
        delete _rcContext;
    }

    void MapBuilder::discoverTiles()
    {
        std::vector<std::string> files;
        uint32 mapID, tileX, tileY, tileID, count = 0, fsize = 0;
        char filter[12];

        std::cout << "Discovering maps... ";
        getDirContents(files, _config->MapsPath());
        for (auto & file : files)
        {
            mapID = static_cast<uint32>(atoi(file.substr(0, file.size() - 8).c_str()));
            if (std::find(_tiles.begin(), _tiles.end(), mapID) == _tiles.end())
            {
                _tiles.emplace_back(mapID, new std::set<uint32>);
                count++;
            }
        }

        files.clear();
        getDirContents(files, _config->VMapsPath(), "*.vmtree");
        for (auto & file : files)
        {
            mapID = static_cast<uint32>(atoi(file.substr(0, file.size() - 7).c_str()));
            if (std::find(_tiles.begin(), _tiles.end(), mapID) == _tiles.end())
            {
                _tiles.emplace_back(mapID, new std::set<uint32>);
                count++;
            }
        }
        std::cout << "found " << count << "." << std::endl;

        count = 0;
        std::cout << "Discovering tiles... ";
        for (const auto & mapTiles : _tiles)
        {
            std::set<uint32>* tiles = mapTiles.tiles;
            mapID = mapTiles.mapID;

            sprintf(filter, "%03u*.vmtile", mapID);
            files.clear();
            getDirContents(files, _config->VMapsPath(), filter);
            for (auto & file : files)
            {
                fsize = file.size();

                tileY = static_cast<uint32>(atoi(file.substr(fsize - 12, 2).c_str()));
                tileX = static_cast<uint32>(atoi(file.substr(fsize - 9, 2).c_str()));
                tileID = StaticMapTree::packTileID(tileY, tileX);

                tiles->insert(tileID);
                count++;
            }

            sprintf(filter, "%03u*", mapID);
            files.clear();
            getDirContents(files, _config->MapsPath(), filter);
            for (auto & file : files)
            {
                fsize = file.size();

                tileY = static_cast<uint32>(atoi(file.substr(fsize - 8, 2).c_str()));
                tileX = static_cast<uint32>(atoi(file.substr(fsize - 6, 2).c_str()));
                tileID = StaticMapTree::packTileID(tileX, tileY);

                if (tiles->insert(tileID).second)
                    count++;
            }

            // Make sure we process maps which don't have tiles
            if (tiles->empty())
            {
                // Convert coord bounds to grid bounds
                uint32 minX, minY, maxX, maxY;
                getGridBounds(mapID, minX, minY, maxX, maxY);

                // Add all tiles within bounds to tile list.
                for (uint32 i = minX; i <= maxX; ++i)
                    for (uint32 j = minY; j <= maxY; ++j)
                        if (tiles->insert(StaticMapTree::packTileID(i, j)).second)
                            count++;
            }
        }
        std::cout << "found " << count << "." << std::endl << std::endl;

        // Calculate tiles to process in total
        for (const auto & mapTiles : _tiles)
        {
            if (!shouldSkipMap(mapTiles.mapID))
                _totalTiles += mapTiles.tiles->size();
        }
    }

    std::set<uint32>* MapBuilder::getTileList(uint32 mapID)
    {
        if (const auto itr = std::find(_tiles.begin(), _tiles.end(), mapID); itr != _tiles.end())
            return itr->tiles;

        auto tiles = new std::set<uint32>();
        _tiles.emplace_back(mapID, tiles);
        return tiles;
    }

    void MapBuilder::BuildMaps(const Optional<uint32> mapID)
    {
        printf("Using %u threads to generate mMaps\n", _threads);

        for (uint32 i = 0; i < _threads; ++i)
            _tileBuilders.push_back(new TileBuilder(this));

        if (mapID)
            buildMap(*mapID);
        else
        {
            // Build all maps if no map id has been specified
            for (auto it = _tiles.begin(); it != _tiles.end(); ++it)
                if (!shouldSkipMap(it->mapID))
                    buildMap(it->mapID);
        }

        while (!_queue.Empty())
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        _cancellationToken = true;

        _queue.Cancel();

        for (const auto& builder : _tileBuilders)
            delete builder;

        _tileBuilders.clear();
    }

    void MapBuilder::getGridBounds(const uint32 mapID, uint32& minX, uint32& minY, uint32& maxX, uint32& maxY) const
    {
        // min and max are initialized to invalid values so the caller iterating the [min, max] range
        // will never enter the loop unless valid min/max values are found
        maxX = 0;
        maxY = 0;
        minX = std::numeric_limits<uint32>::max();
        minY = std::numeric_limits<uint32>::max();

        float bMin[3] = { 0, 0, 0 };
        float bMax[3] = { 0, 0, 0 };
        float lMin[3] = { 0, 0, 0 };
        float lMax[3] = { 0, 0, 0 };
        MeshData meshData;

        // Make sure we process maps which don't have tiles.
        // Initialize the static tree, which loads WDT models.
        if (!_terrainBuilder->loadVMap(mapID, 64, 64, meshData))
            return;

        if (meshData.solidVertices.size() + meshData.liquidVertices.size() == 0)
            return;

        // Get the coord bounds of the model data
        if (meshData.solidVertices.size() && meshData.liquidVertices.size())
        {
            rcCalcBounds(meshData.solidVertices.getCArray(), meshData.solidVertices.size() / 3, bMin, bMax);
            rcCalcBounds(meshData.liquidVertices.getCArray(), meshData.liquidVertices.size() / 3, lMin, lMax);
            rcVmin(bMin, lMin);
            rcVmax(bMax, lMax);
        }
        else if (meshData.solidVertices.size())
            rcCalcBounds(meshData.solidVertices.getCArray(), meshData.solidVertices.size() / 3, bMin, bMax);
        else
            rcCalcBounds(meshData.liquidVertices.getCArray(), meshData.liquidVertices.size() / 3, lMin, lMax);

        // Convert coord bounds to grid bounds
        maxX = 32 - bMin[0] / GRID_SIZE;
        maxY = 32 - bMin[2] / GRID_SIZE;
        minX = 32 - bMax[0] / GRID_SIZE;
        minY = 32 - bMax[2] / GRID_SIZE;
    }

    void MapBuilder::buildMap(const uint32 mapID)
    {
        if (const std::set<uint32>* tiles = getTileList(mapID); !tiles->empty())
        {
            // Build navMesh
            dtNavMesh* navMesh = nullptr;
            buildNavMesh(mapID, navMesh);
            if (!navMesh)
            {
                printf("[Map %03i] Failed creating navmesh!\n", mapID);
                _totalTilesProcessed += tiles->size();
                return;
            }

            printf("[Map %03i] We have %u tiles.\n", mapID, static_cast<unsigned int>(tiles->size()));
            for (const unsigned int tile : *tiles)
            {
                uint32 tileX, tileY;
                StaticMapTree::unpackTileID(tile, tileX, tileY);

                TileInfo tileInfo;
                tileInfo.mapID = mapID;
                tileInfo.tileX = tileX;
                tileInfo.tileY = tileY;
                memcpy(&tileInfo.navMeshParams, navMesh->getParams(), sizeof(dtNavMeshParams));
                _queue.Push(tileInfo);
            }

            dtFreeNavMesh(navMesh);
        }
    }

    void MapBuilder::buildNavMesh(uint32 mapID, dtNavMesh*& navMesh)
    {
        const std::set<uint32>* tiles = getTileList(mapID);

        constexpr int polyBits = DT_POLY_BITS;

        const int maxTiles = tiles->size();
        constexpr int maxPolysPerTile = 1 << polyBits;

        // Calculate bounds of map
        uint32 tileXMin = 64, tileYMin = 64, tileXMax = 0, tileYMax = 0, tileX, tileY;
        for (const uint32 tile : *tiles)
        {
            StaticMapTree::unpackTileID(tile, tileX, tileY);

            if (tileX > tileXMax)
                tileXMax = tileX;
            else if (tileX < tileXMin)
                tileXMin = tileX;

            if (tileY > tileYMax)
                tileYMax = tileY;
            else if (tileY < tileYMin)
                tileYMin = tileY;
        }

        // Use Max because '32 - tileX' is negative for values over 32
        float bMin[3], bMax[3];
        getTileBounds(tileXMax, tileYMax, nullptr, 0, bMin, bMax);

        // Now create the navmesh
        dtNavMeshParams navMeshParams = {};
        navMeshParams.tileWidth = GRID_SIZE;
        navMeshParams.tileHeight = GRID_SIZE;
        rcVcopy(navMeshParams.orig, bMin);
        navMeshParams.maxTiles = maxTiles;
        navMeshParams.maxPolys = maxPolysPerTile;

        navMesh = dtAllocNavMesh();
        printf("[Map %03i] Creating navMesh...\n", mapID);
        if (!navMesh->init(&navMeshParams))
        {
            printf("[Map %03i] Failed creating navmesh!\n", mapID);
            return;
        }

        const fs::path fileName = _config->MMapsPath() / std::format(MMAP_FILE_NAME_FORMAT, mapID);

        FILE* file = fopen(fileName.c_str(), "wb");
        if (!file)
        {
            dtFreeNavMesh(navMesh);
            char message[1024];
            sprintf(message, "[Map %03i] Failed to open %s for writing!\n", mapID, fileName.c_str());
            perror(message);
            return;
        }

        // Now that we know navMesh params are valid, we can write them to file
        fwrite(&navMeshParams, sizeof(dtNavMeshParams), 1, file);
        fclose(file);
    }

    void MapBuilder::getTileBounds(const uint32 tileX, const uint32 tileY, const float* vertices, const int vertCount, float* bMin, float* bMax)
    {
        // This is for elevation
        if (vertices && vertCount)
            rcCalcBounds(vertices, vertCount, bMin, bMax);
        else
        {
            bMin[1] = FLT_MIN;
            bMax[1] = FLT_MAX;
        }

        // This is for width and depth
        bMax[0] = (32 - static_cast<int>(tileX)) * GRID_SIZE;
        bMax[2] = (32 - static_cast<int>(tileY)) * GRID_SIZE;
        bMin[0] = bMax[0] - GRID_SIZE;
        bMin[2] = bMax[2] - GRID_SIZE;
    }

    bool MapBuilder::shouldSkipMap(const uint32 mapID) const
    {
        if (_mapID >= 0)
            return static_cast<uint32>(_mapID) != mapID;

        switch (mapID)
        {
        // Junk maps
        case 13:    // test.wdt
        case 25:    // ScottTest.wdt
        case 29:    // Test.wdt
        case 42:    // Colin.wdt
        case 169:   // EmeraldDream.wdt (unused, and very large)
        case 451:   // development.wdt
        case 573:   // ExteriorTest.wdt
        case 597:   // CraigTest.wdt
        case 605:   // development_nonweighted.wdt
        case 606:   // QA_DVD.wdt

        // Transport maps
        case 582:
        case 584:
        case 586:
        case 587:
        case 588:
        case 589:
        case 590:
        case 591:
        case 592:
        case 593:
        case 594:
        case 596:
        case 610:
        case 612:
        case 613:
        case 614:
        case 620:
        case 621:
        case 622:
        case 623:
        case 641:
        case 642:
        case 647:
        case 672:
        case 673:
        case 712:
        case 713:
        case 718:
            return true;
        default:
            break;
        }
        return false;
    }

    rcConfig MapBuilder::getRecastConfig(const ResolvedMeshConfig &cfg, float bMin[3], float bMax[3])
    {
        rcConfig config = {};

        rcVcopy(config.bmin, bMin);
        rcVcopy(config.bmax, bMax);

        config.maxVertsPerPoly = DT_VERTS_PER_POLYGON;
        config.cs = cfg.cellSizeHorizontal;
        config.ch = cfg.cellSizeVertical;
        config.walkableSlopeAngle = cfg.walkableSlopeAngle;
        config.tileSize = cfg.vertexPerTileEdge;
        config.walkableRadius = cfg.walkableRadius;
        config.borderSize = cfg.walkableRadius + 3;
        config.maxEdgeLen = cfg.vertexPerTileEdge + 1;  // Anything bigger than tileSize
        config.walkableHeight = cfg.walkableHeight;
        config.walkableClimb = cfg.walkableClimb;
        config.minRegionArea = rcSqr(60);
        config.mergeRegionArea = rcSqr(50);
        config.maxSimplificationError = cfg.maxSimplificationError;  // Eliminates most jagged edges (tiny polygons)
        config.detailSampleDist = config.cs * 16;
        config.detailSampleMaxError = config.ch * 1;
        return config;
    }

    uint32 MapBuilder::percentageDone(const uint32 totalTiles, const uint32 totalTilesDone)
    {
        if (totalTiles)
            return totalTilesDone * 100 / totalTiles;
        return 0;
    }

    uint32 MapBuilder::currentPercentageDone() const
    {
        return percentageDone(_totalTiles, _totalTilesProcessed);
    }
}
