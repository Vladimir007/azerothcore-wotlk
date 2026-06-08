#ifndef MMAP_MANAGER_H
#define MMAP_MANAGER_H

#include <memory>
#include "Common.h"
#include "DetourAlloc.h"
#include "DetourExtended.h"
#include "DetourNavMesh.h"

// Memory management
inline void* dtCustomAlloc(const std::size_t size, dtAllocHint /*hint*/)
{
    return new unsigned char[size];
}

inline void dtCustomFree(void* ptr)
{
    delete[] static_cast<unsigned char*>(ptr);
}

namespace MMAP
{
    enum MMAP_LOAD_RESULT
    {
        MMAP_LOAD_RESULT_ERROR,
        MMAP_LOAD_RESULT_OK,
        MMAP_LOAD_RESULT_IGNORED,
    };

    struct NavMeshDeleter
    {
        void operator()(dtNavMesh* navMesh) noexcept { dtFreeNavMesh(navMesh); }
    };

    struct NavMeshQueryDeleter
    {
        void operator()(dtNavMeshQuery* query) noexcept { dtFreeNavMeshQuery(query); }
    };

    using ManagedNavMeshQuery = std::unique_ptr<dtNavMeshQuery, NavMeshQueryDeleter>;

    class MMapMgr
    {
    public:
        MMapMgr() = default;
        ~MMapMgr() = default;

        static std::shared_ptr<dtNavMesh> LoadNavMesh(uint32 mapId);
        static bool LoadTile(dtNavMesh* navMesh, uint32 mapID, int32 x, int32 y);
        static ManagedNavMeshQuery CreateNavMeshQuery(const dtNavMesh* navMesh);

    private:
        static uint32 packTileID(int32 x, int32 y);
    };
}

#endif
