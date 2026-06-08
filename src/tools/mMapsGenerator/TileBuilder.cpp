#include "TileBuilder.h"

#include <DetourNavMeshBuilder.h>
#include <format>
#include <G3D/Array.h>

#include "IntermediateValues.h"
#include "MapBuilder.h"
#include "MapDefines.h"

using namespace MMAP;

void CleanVertices(G3D::Array<float>& vertices, G3D::Array<int>& tris)
{
    std::map<int, int> vertMap;

    int* t = tris.getCArray();
    const float* v = vertices.getCArray();

    G3D::Array<float> cleanedVertices;
    int count = 0;
    // collect all the vertex indices from triangle
    for (int i = 0; i < tris.size(); ++i)
    {
        if (vertMap.contains(t[i]))
            continue;
        std::pair<int, int> val;
        val.first = t[i];

        const int index = val.first;
        val.second = count;

        vertMap.insert(val);
        cleanedVertices.append(v[index * 3], v[index * 3 + 1], v[index * 3 + 2]);
        count++;
    }

    vertices.fastClear();
    vertices.append(cleanedVertices);
    cleanedVertices.clear();

    // Update triangles to use new indices
    for (int i = 0; i < tris.size(); ++i)
    {
        std::map<int, int>::iterator it;
        if ((it = vertMap.find(t[i])) == vertMap.end())
            continue;
        t[i] = it->second;
    }

    vertMap.clear();
}

TileBuilder::TileBuilder(MapBuilder* mapBuilder) :
        m_mapBuilder(mapBuilder),
        m_terrainBuilder(nullptr),
        m_workerThread(&TileBuilder::WorkerThread, this),
        m_rcContext(nullptr)
{
    m_terrainBuilder = new TerrainBuilder(&m_mapBuilder->GetConfig());
    m_rcContext = new rcContext(false);
}

TileBuilder::~TileBuilder()
{
    WaitCompletion();

    delete m_terrainBuilder;
    delete m_rcContext;
}

void TileBuilder::WaitCompletion()
{
    if (m_workerThread.joinable())
        m_workerThread.join();
}

void TileBuilder::WorkerThread()
{
    while (true)
    {
        TileInfo tileInfo;

        m_mapBuilder->_queue.WaitAndPop(tileInfo);

        if (m_mapBuilder->_cancellationToken)
            return;

        dtNavMesh* navMesh = dtAllocNavMesh();
        if (!navMesh->init(&tileInfo.navMeshParams))
        {
            printf("[Map %04i] Failed creating navmesh for tile %i,%i !\n", tileInfo.mapID, tileInfo.tileX, tileInfo.tileY);
            dtFreeNavMesh(navMesh);
            return;
        }

        buildTile(tileInfo.mapID, tileInfo.tileX, tileInfo.tileY, navMesh);

        dtFreeNavMesh(navMesh);
    }
}

void TileBuilder::buildTile(const uint32 mapID, const uint32 tileX, const uint32 tileY, dtNavMesh* navMesh)
{
    if (shouldSkipTile(mapID, tileX, tileY))
    {
        ++m_mapBuilder->_totalTilesProcessed;
        return;
    }

    printf("%u%% [Map %04i] Building tile [%02u,%02u]\n", m_mapBuilder->currentPercentageDone(), mapID, tileX, tileY);

    MeshData meshData;

    // Get heightmap data
    m_terrainBuilder->loadMap(mapID, tileX, tileY, meshData);

    // Get model data
    m_terrainBuilder->loadVMap(mapID, tileY, tileX, meshData);

    // If there is no data, give up now
    if (!meshData.solidVertices.size() && !meshData.liquidVertices.size())
    {
        ++m_mapBuilder->_totalTilesProcessed;
        return;
    }

    // Remove unused vertices
    CleanVertices(meshData.solidVertices, meshData.solidTris);
    CleanVertices(meshData.liquidVertices, meshData.liquidTris);

    // gather all mesh data for final data check, and bounds calculation
    G3D::Array<float> allVertices;
    allVertices.append(meshData.liquidVertices);
    allVertices.append(meshData.solidVertices);

    if (!allVertices.size())
    {
        ++m_mapBuilder->_totalTilesProcessed;
        return;
    }

    // get bounds of current tile
    float bMin[3], bMax[3];
    m_mapBuilder->getTileBounds(tileX, tileY, allVertices.getCArray(), allVertices.size() / 3, bMin, bMax);

    // Build navmesh tile
    buildMoveMapTile(mapID, tileX, tileY, meshData, bMin, bMax, navMesh);

    ++m_mapBuilder->_totalTilesProcessed;
}

void TileBuilder::buildMoveMapTile(uint32 mapID, uint32 tileX, uint32 tileY, MeshData& meshData, float bMin[3], float bMax[3], dtNavMesh* navMesh)
{
    // Console output
    std::string tileString = std::format("[Map {:03}] [{:02}, {:02}] ", mapID, tileX, tileY);
    std::cout << tileString << "Building mMap tiles..." << std::endl;

    float* tVertices = meshData.solidVertices.getCArray();
    int tVerticesN = meshData.solidVertices.size() / 3;
    int* tTriangles = meshData.solidTris.getCArray();
    int tTrianglesN = meshData.solidTris.size() / 3;

    float* lVertices = meshData.liquidVertices.getCArray();
    int lVerticesN = meshData.liquidVertices.size() / 3;
    int* lTriangles = meshData.liquidTris.getCArray();
    int lTrianglesN = meshData.liquidTris.size() / 3;
    uint8* lTrianglesFlags = meshData.liquidType.getCArray();

    ResolvedMeshConfig cfg = m_mapBuilder->GetConfig().GetConfigForTile(mapID, tileX, tileY);
    int tilesPerMap = cfg.tilesPerMapEdge;
    float baseUnitDim = cfg.baseUnitDim;

    rcConfig config = m_mapBuilder->getRecastConfig(cfg, bMin, bMax);

    // This sets the dimensions of the heightfield - should maybe happen before border padding
    rcCalcGridSize(config.bmin, config.bmax, config.cs, &config.width, &config.height);

    // Allocate tiles
    auto tiles = new Tile[tilesPerMap * tilesPerMap];

    // Initialize per tile config.
    rcConfig tileCfg = config;
    tileCfg.width = config.tileSize + config.borderSize * 2;
    tileCfg.height = config.tileSize + config.borderSize * 2;

    // Merge per tile poly and detail meshes
    auto pmMerge = new rcPolyMesh*[tilesPerMap * tilesPerMap];
    auto dmMerge = new rcPolyMeshDetail*[tilesPerMap * tilesPerMap];
    int nMerge = 0;

    // Build all tiles
    for (int y = 0; y < tilesPerMap; ++y)
    {
        for (int x = 0; x < tilesPerMap; ++x)
        {
            Tile& tile = tiles[x + y * tilesPerMap];

            // Calculate the per tile bounding box.
            tileCfg.bmin[0] = config.bmin[0] + config.cs * static_cast<float>(config.tileSize * x);
            tileCfg.bmin[2] = config.bmin[2] + config.cs * static_cast<float>(config.tileSize * y);
            tileCfg.bmax[0] = config.bmin[0] + config.cs * static_cast<float>(config.tileSize * (x + 1));
            tileCfg.bmax[2] = config.bmin[2] + config.cs * static_cast<float>(config.tileSize * (y + 1));

            tileCfg.bmin[0] -= static_cast<float>(tileCfg.borderSize) * tileCfg.cs;
            tileCfg.bmin[2] -= static_cast<float>(tileCfg.borderSize) * tileCfg.cs;
            tileCfg.bmax[0] += static_cast<float>(tileCfg.borderSize) * tileCfg.cs;
            tileCfg.bmax[2] += static_cast<float>(tileCfg.borderSize) * tileCfg.cs;

            // Build heightfield
            tile.solid = rcAllocHeightfield();
            if (!tile.solid || !rcCreateHeightfield(m_rcContext, *tile.solid, tileCfg.width, tileCfg.height, tileCfg.bmin, tileCfg.bmax, tileCfg.cs, tileCfg.ch))
            {
                std::cerr << tileString << "Failed building heightfield!" << std::endl;
                continue;
            }

            // Mark all walkable tiles, both liquids and solids

            auto triFlags = new unsigned char[tTrianglesN];
            memset(triFlags, NAV_GROUND, tTrianglesN * sizeof(unsigned char));
            rcClearUnwalkableTriangles(m_rcContext, tileCfg.walkableSlopeAngle, tVertices, tVerticesN, tTriangles, tTrianglesN, triFlags);
            rcRasterizeTriangles(m_rcContext, tVertices, tVerticesN, tTriangles, triFlags, tTrianglesN, *tile.solid, config.walkableClimb);
            delete[] triFlags;

            rcFilterLowHangingWalkableObstacles(m_rcContext, config.walkableClimb, *tile.solid);
            rcFilterLedgeSpans(m_rcContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid);
            rcFilterWalkableLowHeightSpans(m_rcContext, tileCfg.walkableHeight, *tile.solid);

            // Add liquid triangles
            rcRasterizeTriangles(m_rcContext, lVertices, lVerticesN, lTriangles, lTrianglesFlags, lTrianglesN, *tile.solid, config.walkableClimb);

            // Compact heightfield spans
            tile.chf = rcAllocCompactHeightfield();
            if (!tile.chf || !rcBuildCompactHeightfield(m_rcContext, tileCfg.walkableHeight, tileCfg.walkableClimb, *tile.solid, *tile.chf))
            {
                std::cerr << tileString << "Failed compacting heightfield!" << std::endl;
                continue;
            }

            // Build polymesh intermediates
            if (!rcErodeWalkableArea(m_rcContext, config.walkableRadius, *tile.chf))
            {
                std::cerr << tileString << "Failed eroding area!" << std::endl;
                continue;
            }

            if (!rcBuildDistanceField(m_rcContext, *tile.chf))
            {
                std::cerr << tileString << "Failed building distance field!" << std::endl;
                continue;
            }

            if (!rcBuildRegions(m_rcContext, *tile.chf, tileCfg.borderSize, tileCfg.minRegionArea, tileCfg.mergeRegionArea))
            {
                std::cerr << tileString << "Failed building regions!" << std::endl;
                continue;
            }

            tile.cSet = rcAllocContourSet();
            if (!tile.cSet || !rcBuildContours(m_rcContext, *tile.chf, tileCfg.maxSimplificationError, tileCfg.maxEdgeLen, *tile.cSet))
            {
                std::cerr << tileString << "Failed building contours!" << std::endl;
                continue;
            }

            // Build polymesh
            tile.pMesh = rcAllocPolyMesh();
            if (!tile.pMesh || !rcBuildPolyMesh(m_rcContext, *tile.cSet, tileCfg.maxVertsPerPoly, *tile.pMesh))
            {
                std::cerr << tileString << "Failed building polymesh!" << std::endl;
                continue;
            }

            tile.dMesh = rcAllocPolyMeshDetail();
            if (!tile.dMesh || !rcBuildPolyMeshDetail(m_rcContext, *tile.pMesh, *tile.chf, tileCfg.detailSampleDist, tileCfg.detailSampleMaxError, *tile.dMesh))
            {
                std::cerr << tileString << "Failed building polymesh detail!" << std::endl;
                continue;
            }

            // Free those up
            rcFreeHeightField(tile.solid);
            tile.solid = nullptr;
            rcFreeCompactHeightfield(tile.chf);
            tile.chf = nullptr;
            rcFreeContourSet(tile.cSet);
            tile.cSet = nullptr;

            pmMerge[nMerge] = tile.pMesh;
            dmMerge[nMerge] = tile.dMesh;
            nMerge++;
        }
    }

    IntermediateValues iv;
    iv.polyMesh = rcAllocPolyMesh();
    if (!iv.polyMesh)
    {
        std::cerr << tileString << "Failed alloc polyMesh!" << std::endl;
        delete[] pmMerge;
        delete[] dmMerge;
        delete[] tiles;
        return;
    }
    rcMergePolyMeshes(m_rcContext, pmMerge, nMerge, *iv.polyMesh);

    iv.polyMeshDetail = rcAllocPolyMeshDetail();
    if (!iv.polyMeshDetail)
    {
        std::cerr << tileString << "Failed alloc polyMeshDetail!" << std::endl;
        delete[] pmMerge;
        delete[] dmMerge;
        delete[] tiles;
        return;
    }
    rcMergePolyMeshDetails(m_rcContext, dmMerge, nMerge, *iv.polyMeshDetail);

    // Free things up
    delete[] pmMerge;
    delete[] dmMerge;
    delete[] tiles;

    // Set polygons as walkable
    // TODO: special flags for DYNAMIC polygons, i.e. surfaces that can be turned on and off
    for (int i = 0; i < iv.polyMesh->npolys; ++i)
        if (iv.polyMesh->areas[i] & RC_WALKABLE_AREA)
            iv.polyMesh->flags[i] = iv.polyMesh->areas[i];

    // Setup mesh parameters
    dtNavMeshCreateParams params = {};
    params.verts = iv.polyMesh->verts;
    params.vertCount = iv.polyMesh->nverts;
    params.polys = iv.polyMesh->polys;
    params.polyAreas = iv.polyMesh->areas;
    params.polyFlags = iv.polyMesh->flags;
    params.polyCount = iv.polyMesh->npolys;
    params.nvp = iv.polyMesh->nvp;
    params.detailMeshes = iv.polyMeshDetail->meshes;
    params.detailVerts = iv.polyMeshDetail->verts;
    params.detailVertsCount = iv.polyMeshDetail->nverts;
    params.detailTris = iv.polyMeshDetail->tris;
    params.detailTriCount = iv.polyMeshDetail->ntris;
    params.offMeshConCount = 0;
    params.walkableHeight = baseUnitDim * config.walkableHeight;  // Agent height
    params.walkableRadius = baseUnitDim * config.walkableRadius;  // Agent radius
    params.walkableClimb = baseUnitDim * config.walkableClimb;    // Keep less that walkableHeight (aka agent height)!
    params.tileX = ((bMin[0] + bMax[0]) / 2 - navMesh->getParams()->orig[0]) / GRID_SIZE;
    params.tileY = ((bMin[2] + bMax[2]) / 2 - navMesh->getParams()->orig[2]) / GRID_SIZE;
    rcVcopy(params.bmin, bMin);
    rcVcopy(params.bmax, bMax);
    params.cs = config.cs;
    params.ch = config.ch;
    params.tileLayer = 0;
    params.buildBvTree = true;

    // Will hold final navmesh
    unsigned char* navData = nullptr;
    int navDataSize = 0;

    do
    {
        // these values are checked within dtCreateNavMeshData - handle them here
        // so we have a clear error message
        if (params.nvp > DT_VERTS_PER_POLYGON)
        {
            std::cerr << tileString << "Invalid vertices-per-polygon value!" << std::endl;
            break;
        }
        if (params.vertCount >= 0xffff)
        {
            std::cerr << tileString << "Too many vertices!" << std::endl;
            break;
        }
        if (!params.vertCount || !params.verts)
        {
            // Occurs mostly when adjacent tiles have models loaded but those models don't span into this tile
            break;
        }
        if (!params.polyCount || !params.polys)
        {
            // We have flat tiles with no actual geometry - don't build those,
            // its useless keep in mind that we do output those into debug info
            std::cerr << tileString << "No polygons to build on tile!" << std::endl;
            break;
        }
        if (!params.detailMeshes || !params.detailVerts || !params.detailTris)
        {
            std::cerr << tileString << "No detail mesh to build tile!" << std::endl;
            break;
        }

        std::cout << tileString << "Building navmesh tile..." << std::endl;
        if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
        {
            std::cerr << tileString << "Failed building navmesh tile!" << std::endl;
            break;
        }

        dtTileRef tileRef = 0;
        std::cout << tileString << "Adding tile to navmesh..." << std::endl;
        // DT_TILE_FREE_DATA tells detour to deallocate memory when the tile is removed via removeTile()
        dtStatus dtResult = navMesh->addTile(navData, navDataSize, DT_TILE_FREE_DATA, 0, &tileRef);
        if (!tileRef || dtResult != DT_SUCCESS)
        {
            std::cerr << tileString << "Failed adding tile to navmesh!" << std::endl;
            break;
        }

        // File output
        const fs::path fileName = m_mapBuilder->GetConfig().MMapsPath() / std::format(MMAP_TILE_FILE_NAME_FORMAT, mapID, tileY, tileX);

        FILE* file = fopen(fileName.c_str(), "wb");
        if (!file)
        {
            std::cerr << tileString << std::format("Failed to open file '{}' for writing!", fileName.string()) << std::endl;
            navMesh->removeTile(tileRef, nullptr, nullptr);
            break;
        }

        std::cout << tileString << "Writing to file..." << std::endl;

        // Write header
        MmapTileHeader header;
        header.size = static_cast<uint32>(navDataSize);
        header.recastConfig = cfg.toMMAPTileRecastConfig();
        fwrite(&header, sizeof(MmapTileHeader), 1, file);

        // Write data
        fwrite(navData, sizeof(unsigned char), navDataSize, file);
        fclose(file);

        // Now that tile is written to disk, we can unload it
        navMesh->removeTile(tileRef, nullptr, nullptr);
    } while (false);
}

bool TileBuilder::shouldSkipTile(uint32 mapID, uint32 tileX, uint32 tileY) const
{
    const fs::path fileName = m_mapBuilder->GetConfig().MMapsPath() / std::format(MMAP_TILE_FILE_NAME_FORMAT, mapID, tileY, tileX);

    FILE* file = fopen(fileName.c_str(), "rb");
    if (!file)
        return false;

    MmapTileHeader header;
    const int count = fread(&header, sizeof(MmapTileHeader), 1, file);
    fclose(file);
    if (count != 1)
        return false;

    if (header.mmapMagic != MMAP_MAGIC || header.dtVersion != static_cast<uint32>(DT_NAVMESH_VERSION))
        return false;

    if (header.mmapVersion != MMAP_VERSION)
        return false;

    const auto desiredRecastConfig = m_mapBuilder->GetConfig().GetConfigForTile(mapID, tileX, tileY).toMMAPTileRecastConfig();
    return header.recastConfig == desiredRecastConfig;
}
