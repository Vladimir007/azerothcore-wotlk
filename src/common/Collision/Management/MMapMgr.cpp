#include "MMapMgr.h"
#include <format>
#include <filesystem>

#include "Config.h"
#include "Errors.h"
#include "Log.h"
#include "MapDefines.h"

namespace fs = std::filesystem;

namespace MMAP
{
    std::shared_ptr<dtNavMesh> MMapMgr::LoadNavMesh(uint32 mapId)
    {
        // Load and init dtNavMesh - read parameters from file
        const fs::path dataPath = sConfigMgr->GetOption<std::string>("DataDir", ".");
        std::string fileName = (dataPath / "mMaps" / std::format(MMAP_FILE_NAME_FORMAT, mapId)).string();

        FILE* file = fopen(fileName.c_str(), "rb");
        if (!file)
        {
            LOG_DEBUG("maps", "MMAP:loadMapData: Error: Could not open mMap file '{}'", fileName);
            return nullptr;
        }

        dtNavMeshParams params;
        const uint32 count = static_cast<uint32>(fread(&params, sizeof(dtNavMeshParams), 1, file));
        fclose(file);
        if (count != 1)
        {
            LOG_DEBUG("maps", "MMAP:loadMapData: Error: Could not read params from file '{}'", fileName);
            return nullptr;
        }

        dtNavMesh* mesh = dtAllocNavMesh();
        ASSERT(mesh);
        if (mesh->init(&params) != DT_SUCCESS)
        {
            dtFreeNavMesh(mesh);
            LOG_ERROR("maps", "MMAP:loadMapData: Failed to initialize dtNavMesh for mMap {:03} from file {}", mapId, fileName);
            return nullptr;
        }

        LOG_DEBUG("maps", "MMAP:loadMapData: Loaded {:03}.mmap", mapId);

        return std::shared_ptr<dtNavMesh>(mesh, NavMeshDeleter());
    }

    uint32 MMapMgr::packTileID(const int32 x, const int32 y)
    {
        return static_cast<uint32>(x << 16 | y);
    }

    bool MMapMgr::LoadTile(dtNavMesh* navMesh, uint32 mapID, int32 x, int32 y)
    {
        const auto dataDir = fs::path(sConfigMgr->GetOption<std::string>("DataDir", "."));
        const fs::path fileName = dataDir / "mMaps" / std::format(MMAP_TILE_FILE_NAME_FORMAT, mapID, x, y);
        FILE* file = fopen(fileName.c_str(), "rb");
        if (!file)
        {
            LOG_DEBUG("maps", "MMAP:loadMap: Could not open mmtile file '{}'", fileName.string());
            return false;
        }

        // Read header
        MmapTileHeader fileHeader;
        if (fread(&fileHeader, sizeof(MmapTileHeader), 1, file) != 1 || fileHeader.mmapMagic != MMAP_MAGIC)
        {
            LOG_ERROR("maps", "MMAP:loadMap: Bad header in mMap {:03}{:02}{:02}.mmtile", mapID, x, y);
            fclose(file);
            return false;
        }

        if (fileHeader.mmapVersion != MMAP_VERSION)
        {
            LOG_ERROR("maps", "MMAP:loadMap: {:03}{:02}{:02}.mmtile was built with generator v{}, expected v{}",
                           mapID, x, y, fileHeader.mmapVersion, MMAP_VERSION);
            fclose(file);
            return false;
        }

        const auto data = static_cast<unsigned char*>(dtAlloc(fileHeader.size, DT_ALLOC_PERM));
        ASSERT(data);

        if (!fread(data, fileHeader.size, 1, file))
        {
            LOG_ERROR("maps", "MMAP:loadMap: Bad header or data in mMap {:03}{:02}{:02}.mmtile", mapID, x, y);
            fclose(file);
            return false;
        }
        fclose(file);

        dtTileRef tileRef = 0;

        // Memory allocated for data is now managed by detour, and will be deallocated when the tile is removed
        if (dtStatusSucceed(navMesh->addTile(data, fileHeader.size, DT_TILE_FREE_DATA, 0, &tileRef)))
        {
            const auto header = reinterpret_cast<dtMeshHeader*>(data);
            LOG_DEBUG("maps", "MMAP:loadMap: Loaded mmtile {:03}[{:02},{:02}] into {:03}[{:02},{:02}]", mapID, x, y, mapID, header->x, header->y);
            return true;
        }

        LOG_ERROR("maps", "MMAP:loadMap: Could not load {:03}{:02}{:02}.mmtile into navmesh", mapID, x, y);
        dtFree(data);
        return false;
    }

    ManagedNavMeshQuery MMapMgr::CreateNavMeshQuery(const dtNavMesh* navMesh)
    {
        dtNavMeshQuery* query = dtAllocNavMeshQuery();
        ASSERT(query);

        if (dtStatusFailed(query->init(navMesh, 1024)))
        {
            dtFreeNavMeshQuery(query);
            return nullptr;
        }

        return ManagedNavMeshQuery(query);
    }
}
